/* 
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom Corporation
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom Corporation and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

#include "cms_util.h"
#include "cms_core.h"
#include "ssk.h"
#include "../../libs/cms_core/linux/rut_gpon_voice.h"
#include "dal_voice.h"

#ifdef VOXXXLOAD


   /*
    * The vodsl start algorithm will depend on
    * a configuration variable, which can be in one of 3 states:
    * 1. start vodsl on any WAN conn up, this is the 3.x behavior.
    * 2. start vodsl on specific WAN conn up, this will probably be used
    *    if the modem is configured with multiple PVC's.
    * 3. start vodsl on br0 up, this is used for debug on LAN side.
    */

UBOOL8 isVodslOnLanSide=FALSE;
UBOOL8 isVodslOnAnyWan=FALSE;
char * vodslWanIfName=NULL;  /* if this is not null, then vodsl is bound to this wanIfName */

/* Voice is not OMCI Managed by default so no need to wait for that first configuration */
static UBOOL8 isVodslInitialized=TRUE;

#if DMP_X_BROADCOM_COM_EPON_1
/* Initially we dont want to start voice is EPON has not finished booting */
static UBOOL8 isEponMacInitialized=FALSE;
static char pendingBoundIpAddr[CMS_IPADDR_LENGTH] = {'\0'};
#endif /* DMP_X_BROADCOM_COM_EPON_1 */

#ifdef SIPLOAD
static CmsRet sendCallMgrRestartMsgToVodsl( void );
#endif /* SIPLOAD */

static UBOOL8 isEthObjectOnBr0(const InstanceIdStack *iidStack, char *br0ipAddr);
#if DMP_EPON_VOICE_OAM 
UBOOL8 isEponObjectOnBr0(const InstanceIdStack *iidStack, char *br0ipAddr);
#endif /* DMP_EPON_VOICE_OAM */
static CmsRet sendStartVodslMsgToSmd(const char *param);
static CmsRet sendStopVodslMsgToSmd( void );
static CmsRet setVodslBoundIp ( const char * ipAddr );
static UBOOL8 isVodslRunning(void);
static UBOOL8 isDataPathAvailable(void);

void processVodslShutdown(void)
{
   CmsRet ret;

   cmsLog_debug("vodsl stop requested");

   /* Tell SMD to stop voice */
   if(isVodslRunning())
   {
      if( (ret = sendStopVodslMsgToSmd()) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to shutdown VODSL , ret=%d", ret);
      }
   }
}

void processVodslStart(void)
{     
   CmsRet ret;

   if ( !isVodslInitialized )
   {
      return;
   }
   
   /* Tell SMD to start voice */
   if(!isVodslRunning())
   {
      if( (ret = sendStartVodslMsgToSmd("")) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to start VODSL , ret=%d", ret);
      }
   }
   else
   {
      cmsLog_error("Vodsl already RUNNING\n");
   }
}

void processVodslRestart(void)
{
   cmsLog_debug("vodsl restart requested");

#if MGCPLOAD
   /* Stop Voice */
   processVodslShutdown();
   
   /* While loop waiting for voice to shutdown */
   
   /* Delay of 1 sec */
   /* Start Voice */   
   processVodslStart();
#endif
#if SIPLOAD
  /* Restart callmanager to pickup new ipaddress */
  sendCallMgrRestartMsgToVodsl();
#endif   
}

void processVodslReboot(void)
{
   cmsLog_debug("vodsl reboot requested");

   /* De-initialize Voice */
   deInitializeVoice();
   /* Delay of 1 sec */
   sleep(1);
   /* Re-Initialize Voice */   
   initializeVoice();
}

void processConfigUploadComplete(void)
{
   CmsRet ret;

#if DMP_X_BROADCOM_COM_EPON_1
   if ( !isEponMacInitialized )
   {
      cmsLog_error("Unable to process CMS_MSG_CONFIG_UPLOAD_COMPLETE since EPON MAC has not Initialized yet");      
      return;
   }
#endif /* DMP_X_BROADCOM_COM_EPON_1 */
   
   if (isVodslInitialized)
   {
      return;
   }

   /* Assume that (initial) Voice Configuration is done */
   isVodslInitialized = TRUE;

   if ( !isDataPathAvailable() )
   {
      /* Rely on RCL to handle IpHost Address Assignment */
      return;
   }

   /* Datapath is open, go ahead and start Voice */
   if ( !isVodslRunning() )
   {
      if( (ret = sendStartVodslMsgToSmd("")) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to start VODSL , ret=%d", ret);
      }
   }
}

void processMibReset(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   VoiceObject *objVoice;
   CmsRet ret = CMSRET_SUCCESS;
   char mgtProt[] = MDMVS_TR69;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      return;
   }
   if ((ret = dalVoice_GetManagementProtocol(NULL, mgtProt, sizeof(mgtProt))) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to determine mgtProt, ret=%d", ret);
      cmsLck_releaseLock();
      return;
   }
   cmsLck_releaseLock();

   if (strncmp(mgtProt, MDMVS_OMCI, sizeof(mgtProt)) == 0)
   {
      defaultVoice();
      processVodslShutdown();   
      isVodslInitialized = FALSE;
      isVodslOnLanSide = FALSE;
      isVodslOnAnyWan = FALSE;
      
      /* Set OMCI bound interface name */
      /* Acquire CMS lock */
      if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get lock, ret=%d", ret);
         return;
      }

      /* Set the boundifname for OMCI */
      if ( (ret = cmsObj_getNext(MDMOID_VOICE, &iidStack, (void**)&objVoice)) != CMSRET_SUCCESS )
      {
         cmsLog_error("failed to get voice object, ret=%d", ret);
         cmsLck_releaseLock();
         return;	
      }

      /* Setup bound interface name pointer */
      CMSMEM_REPLACE_STRING_FLAGS(objVoice->X_BROADCOM_COM_BoundIfName, MDMVS_OMCI, ALLOC_SHARED_MEM);
      
      if ( (ret = cmsObj_set(objVoice, &iidStack)) != CMSRET_SUCCESS )
      {
         cmsLog_error("failed to set voice object, ret=%d", ret);
      }

      /* Free voice object */
      cmsObj_free((void**)&objVoice);

      /* Release CMS lock */
      cmsLck_releaseLock();
   }
}

/** initializeVoice()
 * This functions does the following:
 * -Figure out which interface voice is bound to
 * -Set the isVodslOnLanSide isVodslOnAnyWan flags
 * -Call init function if voice is on LAN
 */
void initializeVoice(void)
{
   VoiceObject *voiceObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("vodsl being initialized");

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      return;
   }

   if ((ret = cmsObj_getNext(MDMOID_VOICE, &iidStack, (void **) &voiceObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get voice object, ret=%d", ret);
      cmsLck_releaseLock();
      return;
   }

   if (cmsUtl_strcmp(voiceObj->X_BROADCOM_COM_BoundIfName, MDMVS_LAN) == 0)
   {
      /*
       * this function may be called at runtime when the BoundIfName param
       * changes, so reset all my internal variables.
       */
      isVodslOnLanSide = TRUE;
      isVodslOnAnyWan = FALSE;
      CMSMEM_FREE_BUF_AND_NULL_PTR(vodslWanIfName);

      /*
       * LAN side may already have link up, so ssk will not get a link status change event.
       * So check if LAN side link is already up and start vodsl.
       */
      initVodslBoundIpIfLanSideUpLocked();
   }
   else if (cmsUtl_strcmp(voiceObj->X_BROADCOM_COM_BoundIfName, MDMVS_ANY_WAN) == 0)
   {
      /*
       * this function may be called at runtime when the BoundIfName param
       * changes, so reset all my internal variables.
       */
      isVodslOnLanSide = FALSE;
      isVodslOnAnyWan = TRUE;
      CMSMEM_FREE_BUF_AND_NULL_PTR(vodslWanIfName);

      /*
       * Initialize voice with the bound IP address of the correct WAN connection
       */
      initVodslBoundIpIfWanSideUpLocked(voiceObj->X_BROADCOM_COM_BoundIfName);
   }
   else
   {
      /*
       * this function may be called at runtime when the BoundIfName param
       * changes, so reset all my internal variables.
       */
      isVodslOnLanSide = FALSE;
      isVodslOnAnyWan = FALSE;
      REPLACE_STRING_IF_NOT_EQUAL(vodslWanIfName, voiceObj->X_BROADCOM_COM_BoundIfName);

      /*
       * Initialize voice with the bound IP address of the correct WAN connection
       */
      initVodslBoundIpIfWanSideUpLocked(voiceObj->X_BROADCOM_COM_BoundIfName);
   }

#if DMP_X_ITU_ORG_GPON_1 || DMP_EPON_VOICE_OAM
   /* OMCI is only optional for RG-Full */
#if DMP_X_BROADCOM_COM_GPONRG_OMCI_LIGHT_1
   /* Nothing to do: OMCI not supported */
#else

#if DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
   if (cmsUtl_strcmp(voiceObj->X_BROADCOM_COM_ManagementProtocol, MDMVS_OMCI) == 0)
#endif
   {
      /* Restore OMCI given default of TR69 */
      dalVoice_SetManagementProtocol(NULL, MDMVS_OMCI);

      /* Assume that Configuration Cycle hasn't ended yet */
      isVodslInitialized = FALSE;
   }
#if DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1   
   else
   {
      /* Initialization complete */
      isVodslInitialized = TRUE;
   }  
#endif

#endif /* DMP_X_BROADCOM_COM_GPONRG_OMCI_LIGHT_1 */
#endif /* DMP_X_ITU_ORG_GPON_1 */


   cmsObj_free((void **) &voiceObj);

   cmsLck_releaseLock();

   cmsLog_debug("isVodslOnLanSide=%d isVodslOnAnyWan=%d vodslWanIfName=%s",
                isVodslOnLanSide, isVodslOnAnyWan, vodslWanIfName);
                
   return;
}


void deInitializeVoice(void)
{
   CmsMsgHeader *msgHdr;
   CmsRet ret;

   if( isVodslRunning() )
   {
      cmsLog_error("vodsl being deinitialized");
   
      msgHdr = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
      if (msgHdr == NULL)
      {
         cmsLog_error("message header allocation failed");
         return;
      }
   
      msgHdr->dst = EID_VODSL; 
      msgHdr->src = EID_SSK;
      msgHdr->type = CMS_MSG_VODSL_IS_READY_FOR_DEINIT;
      msgHdr->flags_request = 1;
      msgHdr->flags_bounceIfNotRunning = 1;
   
      /* If the voice application is ready to deinit, go ahead with it. If not, ignore voice stop */
      if(cmsMsg_sendAndGetReply(msgHandle, msgHdr) == TRUE)
      {
         ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to get lock, ret=%d", ret);
         }
         else
         {
            /* clear boundIP, RCL will take care of rest */
            setVodslBoundIp("0.0.0.0");
   
            isVodslOnLanSide = FALSE;
            isVodslOnAnyWan = FALSE;
            CMSMEM_FREE_BUF_AND_NULL_PTR(vodslWanIfName);
   
            cmsLck_releaseLock();
         }
      }
      else
      {
         cmsLog_error("vodsl is not ready for deinit, ignore command to deinit\n");
      }
   
      cmsMem_free(msgHdr);
   }
   else
   {
      cmsLog_error("vodsl is not initialized");      
   }
   return;
}


void defaultVoice(void)
{
   char currMgtProt[] = MDMVS_TR69;

   // Aquire MDM lock & test for success.
   if (cmsLck_acquireLock() == CMSRET_SUCCESS)
   {
      // Preserve Management Protocol
      dalVoice_GetManagementProtocol(NULL, currMgtProt, sizeof(currMgtProt));

      // Set remainder to Default
      dalVoice_SetDefaults(NULL, NULL);

      dalVoice_SetManagementProtocol(NULL, currMgtProt);

      // Release Lock
      cmsLck_releaseLock();
   }
}

#if DMP_X_BROADCOM_COM_EPON_1
void processEponMacBootInd()
{  
   cmsLog_notice("EPON MAC has Initialized");         
       
   /* If we have received the EPON_MAC_BOOT_COMPLETE message, this means the EPON MAC has finished booting */
   isEponMacInitialized = TRUE;
   
   /* Check if we already have a pending bound ipaddress */
   if( strlen(pendingBoundIpAddr) )
   {
      if (cmsLck_acquireLock() == CMSRET_SUCCESS)
      {
         setVodslBoundIp( pendingBoundIpAddr );
         
         /* Clear the pending address */
         pendingBoundIpAddr[0] = '\0';
      }         
      // Release Lock
      cmsLck_releaseLock();
   }
   else
   {
      cmsLog_notice("No pending boundIpAddr");         
   }
}
#endif /* DMP_X_BROADCOM_COM_EPON_1    */

/** initVodslOnLanSideLocked()
 * This function is called when isVodslOnLanSide is TRUE 
 * and when LAN link goes UP. it does the following:
 * -Check if lan object is on br0
 * -Get ip address of LAN and store as boundIP for Vodsl
 */
void initVodslOnLanSideLocked(const char *ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   LanEthIntfObject *ethObj=NULL;
#if DMP_EPON_VOICE_OAM  
   LanEponIntfObject *eponObj=NULL;
#endif /* DMP_EPON_VOICE_OAM */   
   char br0ipAddr[CMS_IPADDR_LENGTH] = {0};

   UBOOL8 found = FALSE;
   CmsRet ret;

   cmsLog_debug("entered, ifName=%s", ifName);

   while (!found &&
         (ret = cmsObj_getNext(MDMOID_LAN_ETH_INTF, &iidStack,
                               (void **) &ethObj)) == CMSRET_SUCCESS)
   {
      found = (cmsUtl_strcmp(ethObj->X_BROADCOM_COM_IfName, ifName) == 0);
      cmsObj_free((void **) &ethObj);
   }

   if (!found)
   {
      cmsLog_error("invalid ifName %s", ifName);
      return;
   }

   if (isEthObjectOnBr0(&iidStack, br0ipAddr))
   {
      setVodslBoundIp(br0ipAddr);
      return;
   }
   
#if DMP_EPON_VOICE_OAM 
   INIT_INSTANCE_ID_STACK(&iidStack); 
   found = FALSE;
   
   while (!found &&
         (ret = cmsObj_getNext(MDMOID_LAN_EPON_INTF, &iidStack,
                               (void **) &eponObj)) == CMSRET_SUCCESS)
   {
      found = (cmsUtl_strcmp(eponObj->ifName, ifName) == 0);
      cmsObj_free((void **) &eponObj);
   }

   if (!found)
   {
      cmsLog_error("invalid ifName %s", ifName);
      return;
   }

   if (isEponObjectOnBr0(&iidStack, br0ipAddr))
   {
      setVodslBoundIp(br0ipAddr);
   }
#endif /* DMP_EPON_VOICE_OAM */ 

   return;
}

/** initVodslBoundIpIfLanSideUpLocked()
 * This function is called when isVodslOnLanSide is TRUE and it does the following:
 * -Figure out whether link is up
 * -Check if linkup lan object is on br0
 * -Get ip address of LAN and store as boundIP for Vodsl
 */
void initVodslBoundIpIfLanSideUpLocked(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   LanEthIntfObject *ethObj=NULL;
#if DMP_EPON_VOICE_OAM  
   LanEponIntfObject *eponObj=NULL;
#endif /* DMP_EPON_VOICE_OAM */   
   UBOOL8 found = FALSE;
   char br0ipAddr[CMS_IPADDR_LENGTH] = {0};
   CmsRet ret;

   cmsLog_debug("initVodslBoundIpIfLanSideUpLocked");
   
   /* iterate through all eth objects */
   while ( !found &&
           ( ret = cmsObj_getNext(MDMOID_LAN_ETH_INTF, &iidStack, (void **) &ethObj) == CMSRET_SUCCESS ) )
   {
      if (cmsUtl_strcmp(ethObj->status, MDMVS_UP) == 0)  /* link is up */
      {
         if (isEthObjectOnBr0(&iidStack, br0ipAddr))  /* eth is on br0 */
         {
            if ( setVodslBoundIp(br0ipAddr) == CMSRET_SUCCESS )
            {
               cmsLog_debug("found %s (ip=%s) is up, start vodsl", ethObj->X_BROADCOM_COM_IfName, br0ipAddr);
               found = TRUE;
            }
            else
            {
               cmsLog_error("Error during setVodslBoundIp");
            }
         }
      }

      cmsObj_free((void **) &ethObj);
   }

#if DMP_EPON_VOICE_OAM 
   if ( found == TRUE )
   {
      return;
   }

   INIT_INSTANCE_ID_STACK(&iidStack); 

   /* iterate through all epon objects */
   while ( !found &&
           ( ret = cmsObj_getNext(MDMOID_LAN_EPON_INTF, &iidStack, (void **) &eponObj) == CMSRET_SUCCESS ) )
   {
      if (cmsUtl_strcmp(eponObj->status, MDMVS_UP) == 0)  /* link is up */
      {
         if (isEponObjectOnBr0(&iidStack, br0ipAddr))  /* epon is on br0 */
         {
            if ( setVodslBoundIp(br0ipAddr) == CMSRET_SUCCESS )
            {
               cmsLog_debug("found %s (ip=%s) is up, start vodsl", eponObj->ifName, br0ipAddr);
               found = TRUE;
            }
            else
            {
               cmsLog_error("Error during setVodslBoundIp");
            }
         }
      }

      cmsObj_free((void **) &eponObj);
   }
#endif /* DMP_EPON_VOICE_OAM */
   return;
}

/** initVodslBoundIpIfWanSideUpLocked()
 * This function is called when isVodslOnWanSide is TRUE and it does the following:
 * -Figure out whether link is up
 * -Get ip address of WAN and store as boundIP for Vodsl
 */
void initVodslBoundIpIfWanSideUpLocked(char *ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject  *ipConn = NULL;
   WanPppConnObject *pppConn = NULL;
   UBOOL8 found = FALSE;
   char ipAddr[CMS_IPADDR_LENGTH] = {0};
   CmsRet ret;
#if VOICE_IPV6_SUPPORT
   int isIpv6 = 0;
#endif   

   cmsLog_debug("initVodslBoundIpIfWanSideUpLocked");

#if VOICE_IPV6_SUPPORT
   char ipAddrFamily[BUFLEN_16];
   unsigned int length = 16;
   DAL_VOICE_PARMS voiceParms;

   ret = dalVoice_GetIpFamily( &voiceParms, ipAddrFamily, length );
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get the IP address family for voice, ret=%d", ret);
      return;
   }
   
   isIpv6 = !(cmsUtl_strcmp( ipAddrFamily, MDMVS_IPV6 ));
#endif 

   /* Iterate through all IP connection objects */
   while ( (ret = cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack, (void**)&ipConn )) == CMSRET_SUCCESS )
   {
      if(ipConn->X_BROADCOM_COM_IfName != NULL && !(cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_ROUTED)) &&
         (!cmsUtl_strcmp(ipConn->X_BROADCOM_COM_IfName, ifName) || !cmsUtl_strcmp(ifName, MDMVS_ANY_WAN)))
      {
#if VOICE_IPV6_SUPPORT
         if ( isIpv6 )
         {
            if ( !cmsUtl_strcmp(ipConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) )
            {
               if ( ipConn->X_BROADCOM_COM_ExternalIPv6Address )
               {
                  strncpy(ipAddr, ipConn->X_BROADCOM_COM_ExternalIPv6Address, CMS_IPADDR_LENGTH);
                  cmsObj_free((void**)&ipConn);
                  found = TRUE;
                  break;
               }
            }		    
         }
         else
#endif
         {
            if ( !cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED) )
            {
               strncpy(ipAddr, ipConn->externalIPAddress, CMS_IPADDR_LENGTH);
               cmsObj_free((void**)&ipConn);
               found = TRUE;
               break; 
            } 
         } 
      } 

      cmsObj_free((void**)&ipConn);
   }

   if ( found == FALSE )
   {
      /* Iterate through all PPP connection objects */
      INIT_INSTANCE_ID_STACK(&iidStack); 
      while ( (ret = cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void**)&pppConn )) == CMSRET_SUCCESS )
      {
         if(pppConn->X_BROADCOM_COM_IfName != NULL && !(cmsUtl_strcmp(pppConn->connectionType, MDMVS_IP_ROUTED)) &&
            (!cmsUtl_strcmp(pppConn->X_BROADCOM_COM_IfName, ifName) || !cmsUtl_strcmp(ifName, MDMVS_ANY_WAN)))
         {  
#if VOICE_IPV6_SUPPORT
            if ( isIpv6 )
            {
               if ( !cmsUtl_strcmp(pppConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) )
               {  
                  if ( pppConn->X_BROADCOM_COM_ExternalIPv6Address )
                  {
                     strncpy(ipAddr, pppConn->X_BROADCOM_COM_ExternalIPv6Address, CMS_IPADDR_LENGTH);
                     cmsObj_free((void**)&ipConn);
                     found = TRUE;
                     break;
                  }
               }		    
            }
            else
#endif
            {
               if ( !cmsUtl_strcmp(pppConn->connectionStatus, MDMVS_CONNECTED) )
               {
                  strncpy(ipAddr, pppConn->externalIPAddress, CMS_IPADDR_LENGTH);
                  cmsObj_free((void**)&pppConn);
                  found = TRUE;
                  break;
               }
            } 
         }

         cmsObj_free((void**)&pppConn);
      }
   }

#ifdef DMP_X_ITU_ORG_GPON_1
   if ( found == FALSE )
   {
      UBOOL8 foundBcmIpHost = FALSE;
      BCM_IpHostConfigDataObject *bcmIpHost = NULL;
      IpHostConfigDataObject *ipHost = NULL;
      InstanceIdStack iidBcmIpHost = EMPTY_INSTANCE_ID_STACK;
      struct in_addr inAddr;

      /* Iterate through all BCM Ip Host objects */
      while ( foundBcmIpHost == FALSE &&
              (ret = cmsObj_getNext(MDMOID_BC_IP_HOST_CONFIG_DATA, &iidBcmIpHost, (void**)&bcmIpHost )) == CMSRET_SUCCESS )
      {
         if (bcmIpHost->interfaceName != NULL &&
             cmsUtl_strcmp(bcmIpHost->interfaceName, ifName) == 0)
         {  
            /* Iterate through all Ip Host objects */
            INIT_INSTANCE_ID_STACK(&iidStack); 
            while ( found == FALSE &&
                    (ret = cmsObj_getNext(MDMOID_IP_HOST_CONFIG_DATA, &iidStack, (void**)&ipHost )) == CMSRET_SUCCESS )
            {  
               if ( ipHost->managedEntityId == bcmIpHost->managedEntityId &&
                   ipHost->currentAddress != 0 )
               {  
                  inAddr.s_addr = ipHost->currentAddress;
                  strcpy(ipAddr, inet_ntoa(inAddr));
                  found = TRUE;
               }
               cmsObj_free((void**)&ipHost);
            }
            foundBcmIpHost = TRUE;
         }
         cmsObj_free((void**)&bcmIpHost);
      }
   }
#endif    // DMP_X_ITU_ORG_GPON_1

   if( found == TRUE )
   {
      /* Set IP address for VODSL */
      setVodslBoundIp(ipAddr);
      return;    
   }
   else
   {
      cmsLog_error("Failed to get WAN Connection object");
      return;
   }
}


UBOOL8 isEthObjectOnBr0(const InstanceIdStack *iidStack, char *br0ipAddr)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   LanHostCfgObject *lanHostCfgObj=NULL;
   LanIpIntfObject *lanIpIntfObj=NULL;
   UBOOL8 isOnBr0=FALSE;
#if VOICE_IPV6_SUPPORT
   IPv6LanIntfAddrObject *ulaAddrObj = NULL;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   char ipAddrFamily[BUFLEN_16];
   unsigned int length = 16;
   DAL_VOICE_PARMS voiceParms;
#endif
   CmsRet ret;
  
#if VOICE_IPV6_SUPPORT
   /* No need to acquire lock here because we are in the voice init function 
    * and the lock is already acquired */
   ret = dalVoice_GetIpFamily( &voiceParms, ipAddrFamily, length );
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get the IP address family for voice, ret=%d", ret);
      return FALSE;
   }
#endif 

   /*
    * Now get the ancestor LanHostConfigManagement object, and then
    * get the first child IPInterface object to get the IP address of the
    * lan subnet.
    */
   ret = cmsObj_getAncestor(MDMOID_LAN_HOST_CFG, MDMOID_LAN_ETH_INTF, &ancestorIidStack, (void **) &lanHostCfgObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor lanHostCfg object, ret=%d", ret);
      return FALSE;
   }

   cmsObj_free((void **) &lanHostCfgObj);

   ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF, &ancestorIidStack, &brIidStack, (void **) &lanIpIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ip interface");
      return FALSE;
   }

   /* vodsl will only work on LAN side br0 */
   if (cmsUtl_strcmp(lanIpIntfObj->X_BROADCOM_COM_IfName, "br0") == 0)
   {
      isOnBr0 = TRUE;
#if VOICE_IPV6_SUPPORT
      if (cmsUtl_strcmp(ipAddrFamily, MDMVS_IPV6) == 0)
      {
         ret = cmsObj_getNext(MDMOID_I_PV6_LAN_INTF_ADDR, &iidStack2, (void **)&ulaAddrObj);
         if ((ret != CMSRET_SUCCESS)
          || (ulaAddrObj->uniqueLocalAddress == NULL) 
          || (cmsUtl_strcmp(ulaAddrObj->uniqueLocalAddress, "") == 0) ) 
         {
            cmsLog_error("could not find ipv6 address object");
            return FALSE;
         }
         else
         {
            cmsLog_debug("Found IPv6 object, IP address %s\n", ulaAddrObj->uniqueLocalAddress);
         }
         sprintf(br0ipAddr, "%s", ulaAddrObj->uniqueLocalAddress);
         cmsObj_free((void **) &ulaAddrObj);
      }
      else
#endif
      {
         /* default to IPv4 */
         sprintf(br0ipAddr, "%s", lanIpIntfObj->IPInterfaceIPAddress);
      }
   }

   cmsObj_free((void **) &lanIpIntfObj);

   return isOnBr0;
}

#if DMP_EPON_VOICE_OAM 
UBOOL8 isEponObjectOnBr0(const InstanceIdStack *iidStack, char *br0ipAddr)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   LanHostCfgObject *lanHostCfgObj=NULL;
   LanIpIntfObject *lanIpIntfObj=NULL;
   UBOOL8 isOnBr0=FALSE;
#if VOICE_IPV6_SUPPORT
   IPv6LanIntfAddrObject *ulaAddrObj = NULL;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   char ipAddrFamily[BUFLEN_16];
   unsigned int length = 16;
   DAL_VOICE_PARMS voiceParms;
#endif
   CmsRet ret;

#if VOICE_IPV6_SUPPORT
   /* No need to acquire lock here because we are in the voice init function 
    * and the lock is already acquired */
   ret = dalVoice_GetIpFamily( &voiceParms, ipAddrFamily, length );
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get the IP address family for voice, ret=%d", ret);
      return FALSE;
   }
#endif 

   /*
    * Now get the ancestor LanHostConfigManagement object, and then
    * get the first child IPInterface object to get the IP address of the
    * lan subnet.
    */
   ret = cmsObj_getAncestor(MDMOID_LAN_HOST_CFG, MDMOID_LAN_EPON_INTF, &ancestorIidStack, (void **) &lanHostCfgObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor lanHostCfg object, ret=%d", ret);
      return FALSE;
   }

   cmsObj_free((void **) &lanHostCfgObj);

   ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF, &ancestorIidStack, &brIidStack, (void **) &lanIpIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ip interface");
      return FALSE;
   }

   /* vodsl will only work on LAN side br0 */
   if (cmsUtl_strcmp(lanIpIntfObj->X_BROADCOM_COM_IfName, "br0") == 0)
   {
      isOnBr0 = TRUE;
#if VOICE_IPV6_SUPPORT
      if (cmsUtl_strcmp(ipAddrFamily, MDMVS_IPV6) == 0)
      {
         ret = cmsObj_getNext(MDMOID_I_PV6_LAN_INTF_ADDR, &iidStack2, (void **)&ulaAddrObj);
         if ((ret != CMSRET_SUCCESS)
          || (ulaAddrObj->uniqueLocalAddress == NULL) 
          || (cmsUtl_strcmp(ulaAddrObj->uniqueLocalAddress, "") == 0) ) 
         {
            cmsLog_error("could not find ipv6 address object");
            return FALSE;
         }
         else
         {
            cmsLog_debug("Found IPv6 object, IP address %s\n", ulaAddrObj->uniqueLocalAddress);
         }
         sprintf(br0ipAddr, "%s", ulaAddrObj->uniqueLocalAddress);
         cmsObj_free((void **) &ulaAddrObj);
      }
      else
#endif
      {
         /* default to IPv4 */
         sprintf(br0ipAddr, "%s", lanIpIntfObj->IPInterfaceIPAddress);
      }
   }

   cmsObj_free((void **) &lanIpIntfObj);

   return isOnBr0;
}
#endif /* DMP_EPON_VOICE_OAM */

/** initVodslOnWanIntfLocked()
 * This function is called when isVodslOnLanSide is FALSE 
 * and a WAN link goes up/state changes. It does the following:
 * -Set ip address of WAN as boundIP for Vodsl
 */
void initVodslOnWanIntfLocked(const char *ifName, const char *ipAddr)
{
   if (ipAddr == NULL || !cmsUtl_strcmp(ipAddr, "0.0.0.0"))
   {
      /*
       * This function might get called for a bridge PVC coming up.
       * Can't start vodsl on a bridge PVC.
       */
      cmsLog_debug("ignore WAN conn up for bridge PVC");
      return;
   }

   if (isVodslOnAnyWan)
   {
      cmsLog_debug("start vodsl on wan intf %s due to any WAN up", ifName);
      setVodslBoundIp(ipAddr);
   }
   else if (cmsUtl_strcmp(vodslWanIfName, ifName) == 0)
   {
      cmsLog_debug("start vodsl on wan intf %s due to exact match on BoundIfName", ifName);
      setVodslBoundIp(ipAddr);
   }
   else
   {
      cmsLog_debug("Do not start vodsl due to mismatch wan ifname %s, BoundIfName=%s", ifName, vodslWanIfName);
   }

   return;
}


static CmsRet setVodslBoundIp ( const char * ipAddr )
{
   VoiceObject *voiceObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
     
   /* get and set the variable */

   if ((ret = cmsObj_getNext(MDMOID_VOICE, &iidStack, (void **) &voiceObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get voice object, ret=%d", ret);
   }
   else
   {
      if (ipAddr == NULL)
      {
         cmsObj_free((void **) &voiceObj);
         cmsLog_error("NULL or INVALID bound IP for vodsl");
         return ret;
      }
      else
      {
#if DMP_X_BROADCOM_COM_EPON_1
         if( !isEponMacInitialized )
         {
            /* If EPON MAC is not up, we store the ipaddress into our pending variable */
            strncpy( pendingBoundIpAddr, ipAddr, CMS_IPADDR_LENGTH);            
         }
         else
#endif /* DMP_X_BROADCOM_COM_EPON_1 */
         {
            CMSMEM_REPLACE_STRING(voiceObj->X_BROADCOM_COM_BoundIpAddr, ipAddr);
            if ((ret = cmsObj_set(voiceObj, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsObj_free((void **) &voiceObj);
               cmsLog_error("could not set voice obj, ret=%d", ret);
               return ret;
            }
         }  
      }
   }

   cmsObj_free((void **) &voiceObj);

   return ret;   
}

static CmsRet sendStartVodslMsgToSmd(const char *param)
{
   CmsMsgHeader *msgHdr;
   char *dataPtr;
   UINT32 pid;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   cmsLog_debug("starting vodsl with param=%s", param);

   /* allocate a message body big enough to hold the header and args */
   msgHdr = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader) + strlen(param) + 1, ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      cmsLog_error("message header allocation failed");
      return ret;
   }

   msgHdr->src = EID_SSK;
   msgHdr->dst = EID_SMD;
   msgHdr->type = CMS_MSG_START_APP;
   msgHdr->wordData = EID_VODSL;
   msgHdr->dataLength = strlen(param) + 1;
   
   dataPtr = (char *) (msgHdr+1);
   strcpy(dataPtr, param);
   
   if ((pid = cmsMsg_sendAndGetReply(msgHandle, msgHdr)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start vodsl msg");
   }
   else
   {
      ret = CMSRET_SUCCESS;
      cmsLog_debug("vodsl started at pid %d", pid);
   }


   cmsMem_free(msgHdr);
   
   return ret;
}

static CmsRet sendStopVodslMsgToSmd( void )
{
   CmsMsgHeader msgHdr = EMPTY_MSG_HEADER;
   CmsRet ret;
   
   msgHdr.dst = EID_SMD; 
   msgHdr.src = EID_SSK;
   msgHdr.type = CMS_MSG_STOP_APP;
   msgHdr.wordData = EID_VODSL;
   msgHdr.flags_request = 1;

   ret = cmsMsg_sendAndGetReply(msgHandle, &msgHdr);
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("SSK: voice stop message is sent to SMD");
   }
   else
   {
      cmsLog_error("SSK: failed to send voice stop message to SMD ret=%d",ret);
   }

   return ret;   
}

#ifdef SIPLOAD
static CmsRet sendCallMgrRestartMsgToVodsl( void )
{
   CmsMsgHeader msgHdr = EMPTY_MSG_HEADER;
   CmsRet ret;

   cmsLog_debug("sending CM restart msg to vodsl");

   msgHdr.dst = EID_VODSL; 
   msgHdr.src = EID_SSK;
   msgHdr.type = CMS_MSG_RESTART_VODSL_CALLMGR;
   msgHdr.flags_event = 1;

   if ((ret = cmsMsg_send(msgHandle, &msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_RESTART_VODSL_CALLMGR msg to vodsl, ret=%d", ret);
   }

   return ret;
}
#endif /* SIPLOAD */

static UBOOL8 isVodslRunning(void)
{
   CmsMsgHeader msgHdr = EMPTY_MSG_HEADER;
   CmsRet ret;

   cmsLog_debug("%s", __FUNCTION__);

   msgHdr.dst = EID_SMD; 
   msgHdr.src = EID_SSK;
   msgHdr.type = CMS_MSG_IS_APP_RUNNING;
   msgHdr.flags_request = 1;
   msgHdr.wordData = EID_VODSL;
         
   ret = cmsMsg_sendAndGetReply(msgHandle, &msgHdr);
   if( ret == CMSRET_SUCCESS )
   {
      return TRUE;
   }
   else if( ret == CMSRET_OBJECT_NOT_FOUND )
   {
      return FALSE;
   }
   else
   {
      cmsLog_error("could not send CMS_MSG_IS_APP_RUNNING msg to smd, ret=%d", ret);
      return FALSE;
   }
}

static UBOOL8 isDataPathAvailable(void)
{
   VoiceObject *voiceObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 isBoundIpSet = FALSE;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      return isBoundIpSet;
   }

   if ((ret = cmsObj_getNext(MDMOID_VOICE, &iidStack, (void **) &voiceObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get voice object, ret=%d", ret);
      cmsLck_releaseLock();
      return isBoundIpSet;
   }

   if ((cmsUtl_strcmp(voiceObj->X_BROADCOM_COM_BoundIpAddr, "0.0.0.0")) != 0 )
   {
      isBoundIpSet = TRUE;
   }
   
   cmsLck_releaseLock();
   return isBoundIpSet ;
}

#endif /* VOXXXLOAD */
