/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2008:proprietary:standard
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
 *
 ************************************************************************/

/*This file includes the basic WLAN rlated MDM Data Structure operation*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cms.h"
#include "board.h"

#include "cms_util.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "cms_boardcmds.h"
#include "cms_boardioctl.h"

#include "cms_core.h"

#include "cms_dal.h"
#include <bcmnvram.h>

#include "wlapi.h"
#include "wlmngr.h"

#include "wllist.h"

#include "wlsyscall.h"
#include "wldsltr.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>

//  #define WLRDMDMDBG		1
//  #define WLWRTMDMDBG		1
// #define WLWRTMDMDBG_ASSOC 1


/* Const Definition*/
#define WL_ADAPTER_NUM   (wlgetintfNo())
#define MDM_STRCPY(x, y)    if ( (y) != NULL ) \
		    CMSMEM_REPLACE_STRING_FLAGS( (x), (y), mdmLibCtx.allocFlags )

/*MDM Lock Waiting time 10s*/
#define LOCK_WAIT	10000

static int 
wl_parse_country_spec(const char *spec, char *ccode, int *regrev) 
{ 
	char *revstr; 
	char *endptr = NULL; 
	int ccode_len; 
	int rev = 0; 

	revstr = strchr(spec, '/'); 

	if (revstr) { 
		rev = atoi(revstr + 1); 
	} 

	if (revstr) 
		ccode_len = (int)(uintptr)(revstr - spec); 
	else 
		ccode_len = (int)strlen(spec); 

	if (ccode_len > 3) { 
		fprintf(stderr, 
				"Could not parse a 2-3 char country code " 
				"in the country string \"%s\"\n", 
				spec); 
		return -1; 
	} 

	memcpy(ccode, spec, ccode_len); 
	ccode[ccode_len] = '\0'; 
	*regrev = rev; 
	return 0; 
} 

/*Following functions are used to write CFG data into MDM*/
static CmsRet wlReadIntfWepKeyCfg( int objId,  int intfId,   WIRELESS_MSSID_VAR *m_wlMssidVarPtr )
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _WlKey64CfgObject *wepkey64Obj=NULL;
   _WlKey128CfgObject *wepkey128Obj=NULL;
   
   int i=0;
   
   //read WEpKey 64
    for ( i=0; i<4; i++ ) {
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_KEY64_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1); 

	   if ((ret = cmsObj_get(MDMOID_WL_KEY64_CFG, &(pathDesc.iidStack), 0, (void **) &wepkey64Obj)) != CMSRET_SUCCESS)
	   {
		      break;
	   }
	   if ( wepkey64Obj->wlKey64 != NULL ) {
  	  	   strcpy(m_wlMssidVarPtr->wlKeys64[i], wepkey64Obj->wlKey64 );
          }
#ifdef WLRDMDMDBG
          printf("m_wlMssidVarPtr->wlKeys64[%d]=%s\n", i,  m_wlMssidVarPtr->wlKeys64[i] );
#endif
	   cmsObj_free((void **) &wepkey64Obj);
    }

    if ( ret ==CMSRET_SUCCESS ) {
    //read WEPKEY 128
	    for ( i=0; i<4; i++ ) {
			   INIT_PATH_DESCRIPTOR(&pathDesc);
			   pathDesc.oid = MDMOID_WL_KEY128_CFG;
			   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
			   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
			   PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
			   PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1); 

			   if ((ret = cmsObj_get(MDMOID_WL_KEY128_CFG, &(pathDesc.iidStack), 0, (void **) &wepkey128Obj)) != CMSRET_SUCCESS)
			   {
			      break;
			   }
			   if ( wepkey128Obj->wlKey128 != NULL ) {
		  	  	   strcpy(m_wlMssidVarPtr->wlKeys128[i], wepkey128Obj->wlKey128 );
		         }
#ifdef WLRDMDMDBG	
			  printf("m_wlMssidVarPtr->wlKeys128[%d]=%s\n", i,  m_wlMssidVarPtr->wlKeys128[i] );
#endif
			  cmsObj_free((void **) &wepkey128Obj);
	    	}
    	}

	return ret;	
}

#ifdef SUPPORT_WSC
static CmsRet wlReadIntfWpsCfg( int objId,    int intfId, WIRELESS_MSSID_VAR *m_wlMssidVarPtr)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   _WlWpsCfgObject      *wpsObj =NULL;
  
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_WPS_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
	  if ((ret = cmsObj_get(MDMOID_WL_WPS_CFG, &(pathDesc.iidStack), 0, (void **) &wpsObj)) != CMSRET_SUCCESS)
	  {
		return ret;
	  }
	  if (  wpsObj->wsc_mode != NULL )
		strcpy(m_wlMssidVarPtr->wsc_mode, wpsObj->wsc_mode );
		
	  if (  wpsObj->wsc_config_state != NULL )
		strcpy(m_wlMssidVarPtr->wsc_config_state, wpsObj->wsc_config_state );

#ifdef WLRDMDMDBG	
	  printf("m_wlMssidVarPtr->wsc_mode=%s\n", m_wlMssidVarPtr->wsc_mode );
	  printf("m_wlMssidVarPtr->wsc_config_state=%s\n", m_wlMssidVarPtr->wsc_config_state );
#endif
         cmsObj_free((void **) &wpsObj);
         return ret;

}
#endif


static CmsRet wlReadIntfMacFltCfg( int objId,    int intfId, WLAN_ADAPTER_STRUCT *wl_instance)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   
   _WlMacFltObject *macFltObj=NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    WIRELESS_MSSID_VAR *m_wlMssidVarPtr =  &(wl_instance->m_wlMssidVar[intfId]);

   struct wl_flt_mac_entry *entry;
#ifdef WLRDMDMDBG	
   int i=0;
#endif
	
#ifdef WLRDMDMDBG	
   printf("objId=%d, intfId=%d\n", objId, intfId );
#endif

   list_del_all(m_wlMssidVarPtr->m_tblFltMac, struct wl_flt_mac_entry);



   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WL_MAC_FLT;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
	   
   while ( (ret = cmsObj_getNextInSubTree(MDMOID_WL_MAC_FLT, 
	   				&(pathDesc.iidStack), &iidStack, (void **) &macFltObj)) == CMSRET_SUCCESS )  {
#ifdef WLRDMDMDBG	
            printf(" currentDepth=%d\n", iidStack.currentDepth );
	     for ( i=0; i <  iidStack.currentDepth; i++ )
			printf("[%d]=%d\n", i, iidStack.instance[i] );

	     printf("macFltObj->wlMacAddr=%s\n", macFltObj->wlMacAddr);
            printf("macFltObj->wlIfcname=%s\n", macFltObj->wlIfcname);
	     printf("macFltObj->wlSsid=%s\n", macFltObj->wlSsid);
#endif

	     entry = malloc( sizeof(WL_FLT_MAC_ENTRY));

            if ( entry != NULL ) {
                   strcpy(entry->macAddress, macFltObj->wlMacAddr);
                   strcpy(entry->ssid, "");
	            strcpy(entry->ifcName, macFltObj->wlIfcname);
	            list_add( entry, (m_wlMssidVarPtr->m_tblFltMac), struct wl_flt_mac_entry);
             }
             cmsObj_free((void **)&macFltObj);
   	}
   
       return CMSRET_SUCCESS;
}

static CmsRet wlReadIntfCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _WlVirtIntfCfgObject *intfObj=NULL;

   WIRELESS_MSSID_VAR *m_wlMssidVarPtr; 
   
   int j=0;

    for ( j=0; j<WL_NUM_SSID; j++ ) {

#ifdef WLRDMDMDBG	
	   printf("%s@%d IntfIdx=%d\n", __FUNCTION__, __LINE__, j );
#endif
  	   m_wlMssidVarPtr =  &(wl_instance->m_wlMssidVar[j]);
	   
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   
	   pathDesc.oid = MDMOID_WL_VIRT_INTF_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), j+1); 

#ifdef WLRDMDMDBG	
	   printf("objId=%d\n", objId);
#endif

          if ((ret = cmsObj_get(MDMOID_WL_VIRT_INTF_CFG, &(pathDesc.iidStack), 0, (void **) &intfObj)) != CMSRET_SUCCESS)
	  {

		break;;
	  }



         if ( intfObj->wlSsid != NULL )
	   	strcpy(m_wlMssidVarPtr->wlSsid, intfObj->wlSsid);

         if ( intfObj->wlBrName != NULL )
            strcpy(m_wlMssidVarPtr->wlBrName, intfObj->wlBrName);
         if ( intfObj->wlBssMacAddr != NULL )
		 	strcpy( wl_instance->bssMacAddr[j], intfObj->wlBssMacAddr);

	  if ( intfObj->wlWpaPsk != NULL )
		strcpy(m_wlMssidVarPtr->wlWpaPsk,intfObj->wlWpaPsk );
	  if ( intfObj->wlRadiusKey != NULL )
	       strcpy(m_wlMssidVarPtr->wlRadiusKey,  intfObj->wlRadiusKey);
	  if ( intfObj->wlWep != NULL )
	       strcpy(m_wlMssidVarPtr->wlWep, intfObj->wlWep);
	  if ( intfObj->wlWpa != NULL )
	       strcpy(m_wlMssidVarPtr->wlWpa,intfObj->wlWpa );
		   
	  m_wlMssidVarPtr->wlRadiusServerIP.s_addr = inet_addr(intfObj->wlRadiusServerIP);
			
	  if ( intfObj->wlAuthMode != NULL )
	        strcpy(m_wlMssidVarPtr->wlAuthMode, intfObj->wlAuthMode);
	  m_wlMssidVarPtr->wlWpaGTKRekey= intfObj->wlWpaGTKRekey;
	  m_wlMssidVarPtr->wlRadiusPort = intfObj->wlRadiusPort;
	  m_wlMssidVarPtr->wlAuth = intfObj->wlAuth;
	  m_wlMssidVarPtr->wlEnblSsid = intfObj->wlEnblSsid;
	  m_wlMssidVarPtr->wlKeyIndex128 = intfObj->wlKeyIndex128;
	  m_wlMssidVarPtr->wlKeyIndex64 = intfObj->wlKeyIndex64;
	  m_wlMssidVarPtr->wlKeyBit = intfObj->wlKeyBit;
	  m_wlMssidVarPtr->wlPreauth = intfObj->wlPreauth;
	  m_wlMssidVarPtr->wlNetReauth = intfObj->wlNetReauth;
		//     wlNasWillrun; /*runtime*/
	  m_wlMssidVarPtr->wlHide = intfObj->wlHide;
	  m_wlMssidVarPtr->wlAPIsolation = intfObj->wlAPIsolation;
	  m_wlMssidVarPtr->wlMaxAssoc = intfObj->wlMaxAssoc;
	  m_wlMssidVarPtr->wlDisableWme = intfObj->wlDisableWme; /* this is per ssid advertisement in beacon/probe_resp */
#ifdef WMF
	  m_wlMssidVarPtr->wlEnableWmf = intfObj->wlEnableWmf;
#endif
#ifdef HSPOT_SUPPORT 
	  m_wlMssidVarPtr->wlEnableHspot = intfObj->wlEnableHspot;
#endif

#ifdef BCMWAPI_WAI
    if ((intfObj->wlWapiCertificate != NULL) && (intfObj->wlWapiCertificate[0] != '\0') && (strlen(intfObj->wlWapiCertificate) < 2048))
    {
        char var_name[32];
        char var_value[32];
        int fd;

        // AS server [ip:port] is currently hard-coded in WAPID.
        // When external AS is supported, we'll save this in MDM.

        sprintf(var_name, "%s_wai_as_ip", intfObj->wlIfcname);
        nvram_set(var_name, "127.0.0.1");

        sprintf(var_name, "%s_wai_as_port", intfObj->wlIfcname);
        nvram_set(var_name, "3810");

        // This "cert_index" means certificate types.
        // Currently only X509 (1) is valid.

        sprintf(var_name, "%s_wai_cert_index", intfObj->wlIfcname);
        nvram_set(var_name, "1");

        // This "cert_name" means AP certificate file name.
        // Currently it is hard-coded in WAPID.

        sprintf(var_name, "%s_wai_cert_name", intfObj->wlIfcname);
        sprintf(var_value, "/var/%s_apcert.cer", intfObj->wlIfcname);
        nvram_set(var_name, var_value);

        fd = open(var_value, O_WRONLY | O_TRUNC | O_CREAT);

        if (fd > 0)
        {
            write(fd, intfObj->wlWapiCertificate, strlen(intfObj->wlWapiCertificate));

            // The value of (1) means valid.
            sprintf(var_name, "%s_wai_cert_status", intfObj->wlIfcname);
            nvram_set(var_name, "1");

            close(fd);
        }
    }
#endif
	  if ( intfObj->wlFltMacMode != NULL )
	      strcpy(m_wlMssidVarPtr->wlFltMacMode, intfObj->wlFltMacMode );

#ifdef WLRDMDMDBG	
	  printf("intfObj->wlFltMacMode=%s\n", intfObj->wlFltMacMode ); //jhc&z
#endif
         if ( intfObj->wlIfcname != NULL )
		 	strcpy( wl_instance->m_ifcName[j], intfObj->wlIfcname );

#ifdef WLRDMDMDBG	
	  printf("intfObj->wlEnblSsid[%d] intfObj->wlIdx[%d]\n", intfObj->wlEnblSsid, intfObj->wlIdx);
	  printf(" intfObj->wlSsid[%s] intfObj->wlIfcname=[%s] wl_instance->m_ifcName[%d]=[%s]\n", 
	  	intfObj->wlSsid, intfObj->wlIfcname, j, wl_instance->m_ifcName[j]);
	  printf("wl_instance->m_wlMssidVar[0].wlSsid=%s\n", wl_instance->m_wlMssidVar[0].wlSsid);
#endif
	  cmsObj_free((void **) &intfObj);



//read wepkey Cfg
	   if ( (ret=wlReadIntfWepKeyCfg( objId, j, m_wlMssidVarPtr)) != CMSRET_SUCCESS )
	   	break;

#ifdef SUPPORT_WSC
// read WPS  Cfg
	   if ( (ret =wlReadIntfWpsCfg( objId,   j,  m_wlMssidVarPtr)) != CMSRET_SUCCESS )
	   	break;
#endif

// read MacFltCfg
	   if ( (ret =wlReadIntfMacFltCfg( objId,   j,  wl_instance)) != CMSRET_SUCCESS )
	   	break;

    }

    strcpy( wl_instance->m_wlVar.wlWlanIfName, wl_instance->m_ifcName[0] ); // wlWlanIfName should remove


    return ret;
}


static CmsRet wlReadBaseCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   _WlBaseCfgObject *wlBaseCfgObj=NULL;
   WIRELESS_VAR *m_wlVarPtr = &(wl_instance->m_wlVar);
   

//read BaseCfg Object to m_wlVar
   iidStack.instance[0] = 1;
   iidStack.instance[1] = objId;
   iidStack.currentDepth=2;
   if ((ret = cmsObj_get(MDMOID_WL_BASE_CFG, &iidStack, 0, (void **)&wlBaseCfgObj)) != CMSRET_SUCCESS)  {

	      return ret;
    }

   m_wlVarPtr->wlEnbl = wlBaseCfgObj->wlEnbl;
	 
//Runtime   m_wlVarPtr->wlSsidIdx = wlBaseCfgObj->wlSsidIdx;
#ifdef WLRDMDMDBG	
  printf("m_wlVarPtr->wlSsidIdx=%d\n", m_wlVarPtr->wlSsidIdx );
#endif
   
   if ( wlBaseCfgObj->wlMode != NULL )
	   strcpy(m_wlVarPtr->wlMode, wlBaseCfgObj->wlMode);

   if ( wlBaseCfgObj->wlCountry != NULL )
	  wl_parse_country_spec(wlBaseCfgObj->wlCountry,m_wlVarPtr->wlCountry,&m_wlVarPtr->wlRegRev); 
   //strcpy( m_wlVarPtr->wlCountry, wlBaseCfgObj->wlCountry);
   //m_wlVarPtr->wlRegRev = wlBaseCfgObj->wlRegRev;

//   wlFltMacMode[WL_SM_SIZE_MAX];

  if ( wlBaseCfgObj->wlPhyType!= NULL )
	   strcpy(m_wlVarPtr->wlPhyType, wlBaseCfgObj->wlPhyType);
  
  m_wlVarPtr->wlCoreRev = wlBaseCfgObj->wlCoreRev;

  if ( wlBaseCfgObj->wlBasicRate!= NULL )
  	strcpy(m_wlVarPtr->wlBasicRate, wlBaseCfgObj->wlBasicRate);

  if ( wlBaseCfgObj->wlProtection != NULL )
	   strcpy(m_wlVarPtr->wlProtection , wlBaseCfgObj->wlProtection);
  
  if ( wlBaseCfgObj->wlPreambleType != NULL )
	   strcpy(m_wlVarPtr->wlPreambleType, wlBaseCfgObj->wlPreambleType);
  
  if ( wlBaseCfgObj->wlAfterBurnerEn != NULL )
	   strcpy(m_wlVarPtr->wlAfterBurnerEn, wlBaseCfgObj->wlAfterBurnerEn);

  if ( wlBaseCfgObj->wlFrameBurst != NULL )
	   strcpy(m_wlVarPtr->wlFrameBurst, wlBaseCfgObj->wlFrameBurst);

//  strcpy(m_wlVarPtr->wlWlanIfName, "wl0" ); // should not be hardcodded


//read wdsCfg   char wlWds[WL_WDS_NUM][WL_MID_SIZE_MAX];

  
   m_wlVarPtr->wlChannel = wlBaseCfgObj->wlChannel;
   wl_instance->wlCurrentChannel = wlBaseCfgObj->wlCurrentChannel;

  
   m_wlVarPtr->wlFrgThrshld = wlBaseCfgObj->wlFrgThrshld;
   m_wlVarPtr->wlRtsThrshld = wlBaseCfgObj->wlRtsThrshld;
   m_wlVarPtr->wlDtmIntvl = wlBaseCfgObj->wlDtmIntvl;
   m_wlVarPtr->wlBcnIntvl = wlBaseCfgObj->wlBcnIntvl;
   m_wlVarPtr->wlRate = wlBaseCfgObj->wlRate;
   m_wlVarPtr->wlgMode= wlBaseCfgObj->wlgMode;

   m_wlVarPtr->wlBand = wlBaseCfgObj->wlBand;
   m_wlVarPtr->wlMCastRate = wlBaseCfgObj->wlMCastRate;
   m_wlVarPtr->wlInfra = wlBaseCfgObj->wlInfra;
   m_wlVarPtr->wlAntDiv = wlBaseCfgObj->wlAntDiv;
   m_wlVarPtr->wlWme = wlBaseCfgObj->wlWme;
   m_wlVarPtr->wlWmeNoAck = wlBaseCfgObj->wlWmeNoAck;
   m_wlVarPtr->wlWmeApsd = wlBaseCfgObj->wlWmeApsd;
   m_wlVarPtr->wlTxPwrPcnt = wlBaseCfgObj->wlTxPwrPcnt;
   m_wlVarPtr->wlRegMode = wlBaseCfgObj->wlRegMode;
   m_wlVarPtr->wlDfsPreIsm = wlBaseCfgObj->wlDfsPreIsm;
   m_wlVarPtr->wlDfsPostIsm = wlBaseCfgObj->wlDfsPostIsm; 
   m_wlVarPtr->wlTpcDb = wlBaseCfgObj->wlTpcDb;
   m_wlVarPtr->wlCsScanTimer = wlBaseCfgObj->wlCsScanTimer;  
   m_wlVarPtr->wlGlobalMaxAssoc = wlBaseCfgObj->wlGlobalMaxAssoc;

   m_wlVarPtr->wlRifsAdvert   = wlBaseCfgObj->wlRifsAdvert;
   m_wlVarPtr->wlChanImEnab   = wlBaseCfgObj->wlChanImEnab;
   m_wlVarPtr->wlObssCoex     = wlBaseCfgObj->wlObssCoex;
   m_wlVarPtr->wlRxChainPwrSaveEnable       = wlBaseCfgObj->wlRxChainPwrSaveEnable;
   m_wlVarPtr->wlRxChainPwrSaveQuietTime    = wlBaseCfgObj->wlRxChainPwrSaveQuietTime;
   m_wlVarPtr->wlRxChainPwrSavePps          = wlBaseCfgObj->wlRxChainPwrSavePps;
   m_wlVarPtr->wlRadioPwrSaveEnable         = wlBaseCfgObj->wlRadioPwrSaveEnable;
   m_wlVarPtr->wlRadioPwrSaveQuietTime      = wlBaseCfgObj->wlRadioPwrSaveQuietTime;
   m_wlVarPtr->wlRadioPwrSavePps            = wlBaseCfgObj->wlRadioPwrSavePps;
   m_wlVarPtr->wlRadioPwrSaveLevel         = wlBaseCfgObj->wlRadioPwrSaveLevel;
   
   // WEP over WDS
   m_wlVarPtr->wlWdsSec = wlBaseCfgObj->wlWdsSec;
   if (wlBaseCfgObj->wlWdsKey != NULL)
       strcpy(m_wlVarPtr->wlWdsKey, wlBaseCfgObj->wlWdsKey);
   m_wlVarPtr->wlWdsSecEnable = wlBaseCfgObj->wlWdsSecEnable;
   m_wlVarPtr->wlEnableUre = wlBaseCfgObj->wlEnableUre;    
   m_wlVarPtr->wlStaRetryTime = wlBaseCfgObj->wlStaRetryTime;    
   wl_instance->m_bands = wlBaseCfgObj->wlMBands;
   if ( wlBaseCfgObj->wlVer != NULL )
	strcpy(wl_instance->wlVer, wlBaseCfgObj->wlVer);
   m_wlVarPtr->wlLazyWds = wlBaseCfgObj->wlLazyWds;
   /* Comment out this line, don't need to use this item
   wl_instance->maxMbss = wlBaseCfgObj->wlMaxMbss;
   */
   wl_instance->mbssSupported = wlBaseCfgObj->wlMbssSupported;
   wl_instance->numBss = wlBaseCfgObj->wlNumBss;

   wl_instance->aburnSupported = wlBaseCfgObj->wlAburnSupported;
   wl_instance->ampduSupported = wlBaseCfgObj->wlAmpduSupported;
   wl_instance->amsduSupported = wlBaseCfgObj->wlAmsduSupported ;
   wl_instance->wmeSupported = wlBaseCfgObj->wlWmeSupported;    
#ifdef WMF
   wl_instance->wmfSupported = wlBaseCfgObj->wlWmfSupported;
#endif   
#ifdef DUCATI
    wl_instance->wlVecSupported = wlBaseCfgObj->wlVecSupported;
    wl_instance->wlIperf        = wlBaseCfgObj->wlIperf;
    wl_instance->wlVec          = wlBaseCfgObj->wlVec;
#endif

   cmsObj_free((void **) &wlBaseCfgObj);

//Debug purpose Show the data

#ifdef WLRDMDMDBG	
  printf("m_wlVarPtr->wlSsidIdx=%d\n", m_wlVarPtr->wlSsidIdx );

  printf("m_wlVarPtr->wlMode=%s\n", m_wlVarPtr->wlMode );
  
  printf("m_wlVarPtr->wlChannel = %d\n", m_wlVarPtr->wlChannel );
  printf("m_wlVarPtr->wlCountry=%s\n", m_wlVarPtr->wlCountry );
  printf("m_wlVarPtr->wlPhyType=%s\n", m_wlVarPtr->wlPhyType );
  
  printf("m_wlVarPtr->wlBasicRate=%s\n", m_wlVarPtr->wlBasicRate );

  printf("m_wlVarPtr->wlProtection=%s\n", m_wlVarPtr->wlProtection );
  printf("m_wlVarPtr->wlPreambleType=%s\n", m_wlVarPtr->wlPreambleType );

  printf("m_wlVarPtr->wlAfterBurnerEn=%s\n", m_wlVarPtr->wlAfterBurnerEn );

  printf("m_wlVarPtr->wlFrameBurst=%s\n", m_wlVarPtr->wlFrameBurst );
  
  printf("wl_instance->wlCurrentChannel = %d\n", wl_instance->wlCurrentChannel );
   
//  printf("wl_instance->wlWlanIfName=%s\n", wl_instance->wlWlanIfName ); 
   
  printf("m_wlVarPtr->wlEnbl=%d\n",  m_wlVarPtr->wlEnbl);
   
  printf("m_wlVarPtr->wlFrgThrshld=%d\n", m_wlVarPtr->wlFrgThrshld);
  printf("m_wlVarPtr->wlRtsThrshld=%d\n",    m_wlVarPtr->wlRtsThrshld);
  printf(" m_wlVarPtr->wlDtmIntvl=%d\n",    m_wlVarPtr->wlDtmIntvl);
  printf("m_wlVarPtr->wlBcnIntvl=%d\n",   m_wlVarPtr->wlBcnIntvl);
  printf("m_wlVarPtr->wlRate=%ld\n",    m_wlVarPtr->wlRate);
  printf("m_wlVarPtr->wlgMode=%d\n",    m_wlVarPtr->wlgMode);
  printf("m_wlVarPtr->wlBand=%d\n",    m_wlVarPtr->wlBand);
  printf("m_wlVarPtr->wlMCastRate=%d\n",    m_wlVarPtr->wlMCastRate);
  printf("m_wlVarPtr->wlInfra=%d\n",    m_wlVarPtr->wlInfra);
  printf("m_wlVarPtr->wlAntDiv=%d\n",   m_wlVarPtr->wlAntDiv);
  printf("m_wlVarPtr->wlWme=%d\n",    m_wlVarPtr->wlWme);
  printf("m_wlVarPtr->wlWmeNoAck=%d\n",    m_wlVarPtr->wlWmeNoAck);
  printf("m_wlVarPtr->wlWmeApsd=%d\n",    m_wlVarPtr->wlWmeApsd);
  printf("m_wlVarPtr->wlTxPwrPcnt=%d\n",    m_wlVarPtr->wlTxPwrPcnt);
  printf("m_wlVarPtr->wlRegMode=%d\n",    m_wlVarPtr->wlRegMode);
  printf("m_wlVarPtr->wlDfsPreIsm=%d\n",    m_wlVarPtr->wlDfsPreIsm);
  printf("m_wlVarPtr->wlDfsPostIsm=%d\n",    m_wlVarPtr->wlDfsPostIsm); 
  printf("m_wlVarPtr->wlTpcDb=%d\n",    m_wlVarPtr->wlTpcDb);
  printf("m_wlVarPtr->wlCsScanTimer=%d\n",    m_wlVarPtr->wlCsScanTimer);  
  printf("m_wlVarPtr->wlGlobalMaxAssoc=%d\n",    m_wlVarPtr->wlGlobalMaxAssoc);
  printf("wl_instance->m_bands=%d\n", wl_instance->m_bands );
  printf("wl_instance->wlVer=%s\n", wl_instance->wlVer );
  printf("m_wlVarPtr->wlLazyWds = %d\n", m_wlVarPtr->wlLazyWds);
  printf("wl_instance->maxMbss = %d\n", wl_instance->maxMbss);
  printf("wl_instance->mbssSupported = %d\n", wl_instance->mbssSupported);
  printf("wl_instance->numBss = %d\n", wl_instance->numBss );
#endif
   return ret;
}

#ifdef SUPPORT_MIMO

static CmsRet wlReadMimoCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   WIRELESS_VAR *m_wlVarPtr = &(wl_instance->m_wlVar);
   
  _WlMimoCfgObject *wlMimoCfgObj = NULL;

	iidStack.instance[0] = 1;
	iidStack.instance[1] = objId;
	iidStack.currentDepth=2;

   if ((ret = cmsObj_get(MDMOID_WL_MIMO_CFG, &iidStack, 0, (void **)&wlMimoCfgObj)) != CMSRET_SUCCESS) {
	      return ret;
   }  

   m_wlVarPtr->wlNBwCap =  wlMimoCfgObj->wlNBwCap;
   m_wlVarPtr->wlNCtrlsb = wlMimoCfgObj->wlNCtrlsb;
   m_wlVarPtr->wlNBand = wlMimoCfgObj->wlNBand;
   m_wlVarPtr->wlNMcsidx = wlMimoCfgObj->wlNMcsidx;
   if ( wlMimoCfgObj->wlNProtection !=NULL )
   	strcpy( m_wlVarPtr->wlNProtection,wlMimoCfgObj->wlNProtection);
   if ( wlMimoCfgObj->wlRifs != NULL )
   	strcpy(m_wlVarPtr->wlRifs,wlMimoCfgObj->wlRifs);
   if ( wlMimoCfgObj->wlAmpdu != NULL )
   	strcpy(m_wlVarPtr->wlAmpdu,wlMimoCfgObj->wlAmpdu);
   if ( wlMimoCfgObj->wlAmsdu != NULL )
   	strcpy(m_wlVarPtr->wlAmsdu, wlMimoCfgObj->wlAmsdu);
   if ( wlMimoCfgObj->wlNmode != NULL )
   	strcpy(m_wlVarPtr->wlNmode, wlMimoCfgObj->wlNmode);
   m_wlVarPtr->wlNReqd = wlMimoCfgObj->wlNReqd;
   m_wlVarPtr->wlStbcTx = wlMimoCfgObj->wlStbcTx;
   m_wlVarPtr->wlStbcRx = wlMimoCfgObj->wlStbcRx;

#ifdef WLRDMDMDBG	
   printf("wlMimoCfgObj->wlNBand=%d\n", wlMimoCfgObj->wlNBand );
#endif

   cmsObj_free((void **) &wlMimoCfgObj);
   return ret;
}
#endif


#ifdef SUPPORT_SES   
static CmsRet wlReadSesCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   WIRELESS_VAR *m_wlVarPtr = &(wl_instance->m_wlVar);
   _WlSesCfgObject *wlSesCfgObj = NULL;

      iidStack.instance[0] = 1;
	iidStack.instance[1] = objId;
	iidStack.currentDepth=2;

  if ((ret = cmsObj_get(MDMOID_WL_SES_CFG, &iidStack, 0, (void **)&wlSesCfgObj)) != CMSRET_SUCCESS)
	   {
	      
	      return ret;
	   }  

   m_wlVarPtr->wlSesEnable = wlSesCfgObj->wlSesEnable;
   m_wlVarPtr->wlSesEvent = wlSesCfgObj->wlSesEvent;
   if ( wlSesCfgObj->wlSesStates != NULL )
   	strcpy(m_wlVarPtr->wlSesStates,wlSesCfgObj->wlSesStates);
   if ( wlSesCfgObj->wlSesSsid != NULL )
   	strcpy(m_wlVarPtr->wlSesSsid,wlSesCfgObj->wlSesSsid);
   if ( wlSesCfgObj->wlSesWpaPsk != NULL )
   	strcpy(m_wlVarPtr->wlSesWpaPsk, wlSesCfgObj->wlSesWpaPsk);  
   m_wlVarPtr->wlSesHide =wlSesCfgObj->wlSesHide;
   m_wlVarPtr->wlSesAuth =  wlSesCfgObj->wlSesAuth; 
   if ( wlSesCfgObj->wlSesAuthMode != NULL )
   	strcpy(m_wlVarPtr->wlSesAuthMode,wlSesCfgObj->wlSesAuthMode); 
   if ( wlSesCfgObj->wlSesWep != NULL )
   	strcpy(m_wlVarPtr->wlSesWep, wlSesCfgObj->wlSesWep);   
   if ( wlSesCfgObj->wlSesWpa != NULL )
   	strcpy(m_wlVarPtr->wlSesWpa,   wlSesCfgObj->wlSesWpa);      
   if ( wlSesCfgObj->wlSesWpa != NULL )
   	strcpy(m_wlVarPtr->wlSesWpa,  wlSesCfgObj->wlSesWpa);      
   m_wlVarPtr->wlSesClEnable =   wlSesCfgObj->wlSesClEnable;   
   m_wlVarPtr->wlSesClEvent =   wlSesCfgObj->wlSesClEvent;   
   if ( wlSesCfgObj->wlSesWdsWsec != NULL )
   	strcpy(m_wlVarPtr->wlWdsWsec,wlSesCfgObj->wlSesWdsWsec); 

   
#ifdef WLRDMDMDBG	
   printf("wlSesCfgObj->wlSesEnable=%d\n", wlSesCfgObj->wlSesEnable );
#endif

   cmsObj_free((void **) &wlSesCfgObj);

   return ret;

}
#endif

static CmsRet wlReadStaticWdsCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   
   _WlStaticWdsCfgObject *staticWdsCfgObj=NULL;

   int i=0;

#ifdef WLWRTMDMDBG_ASSOC
    printf("%s@%d objId=%d\n", __FUNCTION__, __LINE__, objId );
#endif

   for ( i=0; i<4; i++ ) {

  	   memset( wl_instance->m_wlVar.wlWds[i], 0, 32 );
           INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_STATIC_WDS_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1);

       if ((ret = cmsObj_get(MDMOID_WL_STATIC_WDS_CFG, &(pathDesc.iidStack), 0, (void **) &staticWdsCfgObj)) != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Get Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
	      return ret;
	   }


	   if ( staticWdsCfgObj->wlMacAddr != NULL )
	            strcpy(wl_instance->m_wlVar.wlWds[i], staticWdsCfgObj->wlMacAddr );
	   
#ifdef WLWRTMDMDBG_ASSOC
          printf("wl_instance->m_wlVar.wlWds[%d]=%s\n",  i, wl_instance->m_wlVar.wlWds[i]);
#endif
 	   cmsObj_free((void **)&staticWdsCfgObj);
   }
   return CMSRET_SUCCESS;
}


//read Dynamic Wds Cfg
static CmsRet wlReadWdsCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   
   _WlWdsCfgObject *wdsCfgObj=NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   struct wl_flt_mac_entry *entry;

#ifdef WLRDMDMDBG	
   int i=0;

   printf("%s@%d objId=%d\n", __FUNCTION__, __LINE__, objId );
#endif   
	
/*cleanup the old wds list first! */
   list_del_all(wl_instance->m_tblWdsMac, struct wl_flt_mac_entry);
  
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WL_WDS_CFG;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
	   
   while ( (ret = cmsObj_getNextInSubTree(MDMOID_WL_WDS_CFG, 
	   				&(pathDesc.iidStack), &iidStack, (void **) &wdsCfgObj)) == CMSRET_SUCCESS )  {

#ifdef WLRDMDMDBG	
            printf(" currentDepth=%d\n", iidStack.currentDepth );
	     for ( i=0; i <  iidStack.currentDepth; i++ )
			printf("[%d]=%d\n", i, iidStack.instance[i] );

	     printf("wdsCfgObj->wlMacAddr=%s\n", wdsCfgObj->wlMacAddr);
#endif
	     entry = malloc( sizeof(WL_FLT_MAC_ENTRY));
            memset( entry, 0, sizeof(WL_FLT_MAC_ENTRY) );
			
            if ( entry != NULL ) {
                   strcpy(entry->macAddress, wdsCfgObj->wlMacAddr);
#if 0
/*No need to read. */ 
                  strcpy(entry->ssid, wdsCfgObj->wlSsid);
	            strcpy(entry->ifcName, wdsCfgObj->wlIfcname);
#endif
	            list_add( entry, (wl_instance->m_tblWdsMac), struct wl_flt_mac_entry);
             }
             cmsObj_free((void **)&wdsCfgObj);
   	}


#ifdef WLRDMDMDBG	
	list_for_each(entry, (wl_instance->m_tblWdsMac)) {
		printf("entry->macAddress=%s\n", entry->macAddress );
       }
#endif
   
       return CMSRET_SUCCESS;
}

CmsRet wlReadTr69Cfg( int objId, struct wl_dsl_tr_struct *wl_dsl_trPtr, 
                          struct wl_dsl_tr_wepkey_struct *wl_dsl_tr_wepkey, struct wl_dsl_tr_presharedkey_struct *wl_dsl_tr_presharedkey)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   _LanWlanObject *wllanWlanObj=NULL;

   MdmPathDescriptor pathDesc;
   LanWlanWepKeyObject *wlwepKeyObj = NULL;
   LanWlanPreSharedKeyObject *wlPreSharedKeyObj = NULL;
   int i =0;
   int guestMdmOff;
   struct mdm_object_mbss_struct *mdm_object_mbssPtr;
   struct wl_dsl_tr_mbss_struct *wl_dsl_tr_mbssPtr;
   LanWlanVirtMbssObject *wllanWlanVirtMbssObj=NULL;
   MdmPathDescriptor pathDesc_virtMbss;

//read Base Object
   iidStack.instance[0] = 1;
   iidStack.instance[1] = objId;
   iidStack.currentDepth=2;
   if ((ret = cmsObj_get(MDMOID_LAN_WLAN, &iidStack, 0, (void **)&wllanWlanObj)) != CMSRET_SUCCESS)  {
	      
	      return ret;
    }

    memset( wl_dsl_trPtr, 0, sizeof ( struct wl_dsl_tr_struct) );
	
    wl_dsl_trPtr->enable = wllanWlanObj->enable;

    if ( wllanWlanObj->status != NULL )
        strcpy( wl_dsl_trPtr->status, wllanWlanObj->status);
	
    if ( wllanWlanObj->BSSID != NULL )
        strcpy( wl_dsl_trPtr->BSSID, wllanWlanObj->BSSID );
    if ( wllanWlanObj->maxBitRate != NULL )
        strcpy( wl_dsl_trPtr->maxBitRate, wllanWlanObj->maxBitRate );
    wl_dsl_trPtr->channel = wllanWlanObj->channel;
    if ( wllanWlanObj->SSID != NULL )
        strcpy( wl_dsl_trPtr->SSID, wllanWlanObj->SSID);
	
    if ( wllanWlanObj->beaconType != NULL )
        strcpy( wl_dsl_trPtr->beaconType, wllanWlanObj->beaconType);
    wl_dsl_trPtr->MACAddressControlEnabled = wllanWlanObj->MACAddressControlEnabled;
    if ( wllanWlanObj->standard != NULL )
        strcpy( wl_dsl_trPtr->standard, wllanWlanObj->standard);
    wl_dsl_trPtr->WEPKeyIndex = wllanWlanObj->WEPKeyIndex;
    if ( wllanWlanObj->keyPassphrase != NULL )
        strcpy(wl_dsl_trPtr->keyPassphrase, wllanWlanObj->keyPassphrase);
    if ( wllanWlanObj->WEPEncryptionLevel  != NULL )
        strcpy( wl_dsl_trPtr->WEPEncryptionLevel, wllanWlanObj->WEPEncryptionLevel );
    if ( wllanWlanObj->basicEncryptionModes != NULL )
        strcpy( wl_dsl_trPtr->basicEncryptionModes, wllanWlanObj->basicEncryptionModes);
    if ( wllanWlanObj->basicAuthenticationMode != NULL )
        strcpy( wl_dsl_trPtr->basicAuthenticationMode, wllanWlanObj->basicAuthenticationMode);
    if ( wllanWlanObj->WPAEncryptionModes != NULL )
        strcpy( wl_dsl_trPtr->WPAEncryptionModes, wllanWlanObj->WPAEncryptionModes);
    if ( wllanWlanObj->WPAAuthenticationMode != NULL )
        strcpy( wl_dsl_trPtr->WPAAuthenticationMode, wllanWlanObj->WPAAuthenticationMode);
    if ( wllanWlanObj->IEEE11iEncryptionModes != NULL )
        strcpy( wl_dsl_trPtr->IEEE11iEncryptionModes, wllanWlanObj->IEEE11iEncryptionModes);
    if ( wllanWlanObj->IEEE11iAuthenticationMode != NULL )
        strcpy( wl_dsl_trPtr->IEEE11iAuthenticationMode, wllanWlanObj->IEEE11iAuthenticationMode);
    if ( wllanWlanObj->possibleChannels!= NULL )
        strcpy( wl_dsl_trPtr->possibleChannels, wllanWlanObj->possibleChannels);
    if ( wllanWlanObj->basicDataTransmitRates != NULL )
        strcpy( wl_dsl_trPtr->basicDataTransmitRates, wllanWlanObj->basicDataTransmitRates);
    if ( wllanWlanObj->operationalDataTransmitRates != NULL )
        strcpy( wl_dsl_trPtr->operationalDataTransmitRates, wllanWlanObj->operationalDataTransmitRates);
    if ( wllanWlanObj->possibleDataTransmitRates != NULL )
        strcpy( wl_dsl_trPtr->possibleDataTransmitRates, wllanWlanObj->possibleDataTransmitRates);
    wl_dsl_trPtr->insecureOOBAccessEnabled = wllanWlanObj->insecureOOBAccessEnabled;
    wl_dsl_trPtr->beaconAdvertisementEnabled = wllanWlanObj->beaconAdvertisementEnabled;
    wl_dsl_trPtr->radioEnabled = wllanWlanObj->radioEnabled;
    wl_dsl_trPtr->autoRateFallBackEnabled = wllanWlanObj->autoRateFallBackEnabled;
    if ( wllanWlanObj->locationDescription != NULL )
        strcpy( wl_dsl_trPtr->locationDescription, wllanWlanObj->locationDescription);
    if ( wllanWlanObj->regulatoryDomain != NULL )
        strcpy( wl_dsl_trPtr->regulatoryDomain, wllanWlanObj->regulatoryDomain);
    wl_dsl_trPtr->totalPSKFailures = wllanWlanObj->totalPSKFailures;
    wl_dsl_trPtr->totalIntegrityFailures = wllanWlanObj->totalIntegrityFailures;
    if ( wllanWlanObj->channelsInUse != NULL )
        strcpy( wl_dsl_trPtr->channelsInUse, wllanWlanObj->channelsInUse);
    if ( wllanWlanObj->deviceOperationMode != NULL )
        strcpy( wl_dsl_trPtr->deviceOperationMode, wllanWlanObj->deviceOperationMode);
    wl_dsl_trPtr->distanceFromRoot = wllanWlanObj->distanceFromRoot;
    if ( wllanWlanObj->peerBSSID != NULL )
        strcpy( wl_dsl_trPtr->peerBSSID, wllanWlanObj->peerBSSID);
    if ( wllanWlanObj->authenticationServiceMode != NULL )
        strcpy( wl_dsl_trPtr->authenticationServiceMode, wllanWlanObj->authenticationServiceMode);
    wl_dsl_trPtr->totalBytesSent = wllanWlanObj->totalBytesSent;
    wl_dsl_trPtr->totalBytesReceived = wllanWlanObj->totalBytesReceived;
    wl_dsl_trPtr->totalPacketsSent = wllanWlanObj->totalPacketsSent;
    wl_dsl_trPtr->totalPacketsReceived = wllanWlanObj->totalPacketsReceived;

    wl_dsl_trPtr->X_BROADCOM_COM_RxErrors = wllanWlanObj->X_BROADCOM_COM_RxErrors;
    wl_dsl_trPtr->X_BROADCOM_COM_RxDrops = wllanWlanObj->X_BROADCOM_COM_RxDrops;
    wl_dsl_trPtr->X_BROADCOM_COM_TxErrors = wllanWlanObj->X_BROADCOM_COM_TxErrors;
    wl_dsl_trPtr->X_BROADCOM_COM_TxDrops = wllanWlanObj->X_BROADCOM_COM_TxDrops;
	
	
    wl_dsl_trPtr->totalAssociations = wllanWlanObj->totalAssociations;

/*Private fileds*/
    strcpy( wl_dsl_trPtr->X_BROADCOM_COM_IfName, wllanWlanObj->X_BROADCOM_COM_IfName);
    wl_dsl_trPtr->X_BROADCOM_COM_HideSSID = wllanWlanObj->X_BROADCOM_COM_HideSSID;
    wl_dsl_trPtr->X_BROADCOM_COM_TxPowerPercent = wllanWlanObj->X_BROADCOM_COM_TxPowerPercent;

    guestMdmOff = sizeof(struct mdm_object_mbss_struct);
    for (i=0; i<WL_NUM_SSID-1; i++) {
        wl_dsl_tr_mbssPtr = &wl_dsl_trPtr->GuestMbss[i];        
        mdm_object_mbssPtr = (struct mdm_object_mbss_struct *)((int)&wllanWlanObj->X_BROADCOM_COM_GuestSSID + i * guestMdmOff);

        if (i < GUEST2_BSS_IDX ) {
            strcpy( wl_dsl_tr_mbssPtr->GuestSSID, mdm_object_mbssPtr->X_BROADCOM_COM_GuestSSID);
            strcpy( wl_dsl_tr_mbssPtr->GuestBSSID, mdm_object_mbssPtr->X_BROADCOM_COM_GuestBSSID);
            wl_dsl_tr_mbssPtr->GuestEnable = mdm_object_mbssPtr->X_BROADCOM_COM_GuestEnable;
            wl_dsl_tr_mbssPtr->GuestHiden = mdm_object_mbssPtr->X_BROADCOM_COM_GuestHiden;
            wl_dsl_tr_mbssPtr->GuestIsolateClients = mdm_object_mbssPtr->X_BROADCOM_COM_GuestIsolateClients;
            wl_dsl_tr_mbssPtr->GuestDisableWMMAdvertise = mdm_object_mbssPtr->X_BROADCOM_COM_GuestDisableWMMAdvertise;
            wl_dsl_tr_mbssPtr->GuestMaxClients = mdm_object_mbssPtr->X_BROADCOM_COM_GuestMaxClients;
        } else {
            INIT_PATH_DESCRIPTOR(&pathDesc_virtMbss);
            pathDesc_virtMbss.oid = MDMOID_LAN_WLAN_VIRT_MBSS;
            PUSH_INSTANCE_ID(&(pathDesc_virtMbss.iidStack), 1); 
            PUSH_INSTANCE_ID(&(pathDesc_virtMbss.iidStack), objId); 
            PUSH_INSTANCE_ID(&(pathDesc_virtMbss.iidStack), i-GUEST2_BSS_IDX+1); 

            if ((ret = cmsObj_get(pathDesc_virtMbss.oid, &(pathDesc_virtMbss.iidStack), 0, (void **) &wllanWlanVirtMbssObj)) != CMSRET_SUCCESS)
            {
                break;
            }
            strcpy( wl_dsl_tr_mbssPtr->GuestSSID, wllanWlanVirtMbssObj->mbssGuestSSID);
            strcpy( wl_dsl_tr_mbssPtr->GuestBSSID, wllanWlanVirtMbssObj->mbssGuestBSSID);
            wl_dsl_tr_mbssPtr->GuestEnable = wllanWlanVirtMbssObj->mbssGuestEnable;
            wl_dsl_tr_mbssPtr->GuestHiden = wllanWlanVirtMbssObj->mbssGuestHiden;
            wl_dsl_tr_mbssPtr->GuestIsolateClients = wllanWlanVirtMbssObj->mbssGuestIsolateClients;
            wl_dsl_tr_mbssPtr->GuestDisableWMMAdvertise = wllanWlanVirtMbssObj->mbssGuestDisableWMMAdvertise;
            wl_dsl_tr_mbssPtr->GuestMaxClients = wllanWlanVirtMbssObj->mbssGuestMaxClients;
            cmsObj_free((void **) &wllanWlanVirtMbssObj);
        }
    }

    cmsObj_free((void **) &wllanWlanObj);
    if ( ret != CMSRET_SUCCESS )
		return ret;
	

#ifdef WLRDMDMDBG	
   
    printf("wl_dsl_trPtr->enable=%d\n", wl_dsl_trPtr->enable);

    printf("wl_dsl_trPtr->status=%s\n", wl_dsl_trPtr->status);
	
    printf("wl_dsl_trPtr->BSSID=%s\n", wl_dsl_trPtr->BSSID );

    printf("wl_dsl_trPtr->maxBitRate=%s\n", wl_dsl_trPtr->maxBitRate );
		
    printf("wl_dsl_trPtr->channel = %d\n", wl_dsl_trPtr->channel);

    printf("strcpy( wl_dsl_trPtr->SSID=%s\n", wl_dsl_trPtr->SSID);
	
    printf("wl_dsl_trPtr->beaconType=%s\n", wl_dsl_trPtr->beaconType);
	
    printf("wl_dsl_trPtr->MACAddressControlEnabled = %d\n", wl_dsl_trPtr->MACAddressControlEnabled);
    printf(" wl_dsl_trPtr->standard=%s\n",  wl_dsl_trPtr->standard);
	
    printf("wl_dsl_trPtr->WEPKeyIndex=%d\n",  wl_dsl_trPtr->WEPKeyIndex);
    printf("wl_dsl_trPtr->WEPEncryptionLevel=%s\n", wl_dsl_trPtr->WEPEncryptionLevel );
	
    printf("wl_dsl_trPtr->basicEncryptionModes=%s\n", wl_dsl_trPtr->basicEncryptionModes);
    printf("wl_dsl_trPtr->basicAuthenticationMode=%s\n", wl_dsl_trPtr->basicAuthenticationMode);
    printf("wl_dsl_trPtr->WPAEncryptionModes=%s\n", wl_dsl_trPtr->WPAEncryptionModes);
    printf("wl_dsl_trPtr->WPAAuthenticationMode=%s\n", wl_dsl_trPtr->WPAAuthenticationMode);
    printf("wl_dsl_trPtr->basicDataTransmitRates=%s\n", wl_dsl_trPtr->basicDataTransmitRates);
    printf("wl_dsl_trPtr->radioEnabled = %d\n", wl_dsl_trPtr->radioEnabled);
    printf("wl_dsl_trPtr->regulatoryDomain=%s\n", wl_dsl_trPtr->regulatoryDomain);

#endif

/*Read Wep Key*/
    for ( i=0; i<4; i++ ) {
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_LAN_WLAN_WEP_KEY;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1); 
	   

	   if ((ret = cmsObj_get(MDMOID_LAN_WLAN_WEP_KEY, &(pathDesc.iidStack), 0, (void **) &wlwepKeyObj)) != CMSRET_SUCCESS)
	   {
		      
		      break;
	   }
	   if ( wlwepKeyObj->WEPKey!= NULL ) {
  	  	   strcpy( wl_dsl_tr_wepkey->WEPKey[i], wlwepKeyObj->WEPKey);
          }
#ifdef WLRDMDMDBG
          printf("wl_dsl_tr_wepkey[%d]->WEPKey=%s\n", i,  (wl_dsl_tr_wepkey+i)->WEPKey );
#endif
	   cmsObj_free((void **) &wlwepKeyObj);
    }

/*Read PreSharedKey*/
    for ( i=0; i<1; i++ ) 
    {
       INIT_PATH_DESCRIPTOR(&pathDesc);
       pathDesc.oid = MDMOID_LAN_WLAN_PRE_SHARED_KEY;
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1); 

       if ((ret = cmsObj_get(MDMOID_LAN_WLAN_PRE_SHARED_KEY, &(pathDesc.iidStack), 0, (void **) &wlPreSharedKeyObj)) != CMSRET_SUCCESS)
       {
          break;
       }
#if 0
       if ( wlPreSharedKeyObj->preSharedKey!= NULL ) 
       {
          strcpy( wl_dsl_tr_presharedkey->PreSharedKey[i], wlPreSharedKeyObj->preSharedKey);
       }
#else
       if ( wlPreSharedKeyObj->keyPassphrase!= NULL ) 
       {
          strcpy( wl_dsl_tr_presharedkey->PreSharedKey[i], wlPreSharedKeyObj->keyPassphrase);
       }
#endif
#ifdef WLRDMDMDBG
          printf("wl_dsl_tr_presharedkey[%d]->PreSharedKey=%s\n", i,  (wl_dsl_tr_presharedkey+i)->PreSharedKey );
#endif
       cmsObj_free((void **) &wlPreSharedKeyObj);
    }
   return ret;
}



static CmsRet wlReadMdmOne( int idx )
{
      CmsRet ret = CMSRET_SUCCESS;

#ifdef WLRDMDMDBG
   printf("reading WLAN Object[%d]\n", idx);
#endif

	if ( (ret = wlReadBaseCfg( idx+1, (m_instance_wl +idx) ) )  != CMSRET_SUCCESS)    {
	      printf("%s@%d ReadBaseCfg Error\n", __FUNCTION__, __LINE__ );
	      return ret;
	}
	 
#ifdef SUPPORT_SES   
       if ( (ret = wlReadSesCfg( idx+1, (m_instance_wl +idx) ) )  != CMSRET_SUCCESS) {
	      printf("%s@%d ReadSesCfg Error\n", __FUNCTION__, __LINE__ );
	      return ret;
	}
#endif

#ifdef SUPPORT_MIMO
	if ( (ret = wlReadMimoCfg( idx+1, (m_instance_wl +idx) ) )  != CMSRET_SUCCESS) {
	      printf("%s@%d ReadMimoCfg Error\n", __FUNCTION__, __LINE__ );
	      return ret;
	}
#endif

	if ( (ret = wlReadIntfCfg( idx+1, (m_instance_wl+idx)  ))  != CMSRET_SUCCESS)   {
	      printf("%s@%d ReadIntfCfg Error\n", __FUNCTION__, __LINE__ );
	      return ret;
	}

	if ( (ret = wlReadStaticWdsCfg( idx+1, (m_instance_wl+idx)  ))  != CMSRET_SUCCESS)   {
	      printf("%s@%d ReadWdsCfg Error\n", __FUNCTION__, __LINE__ );
	      return ret;
	}
		

//This is Dynamic WDS
	if ( (ret = wlReadWdsCfg( idx+1, (m_instance_wl+idx)  ))  != CMSRET_SUCCESS)  {
	      printf("%s@%d ReadWdsCfg Error\n", __FUNCTION__, __LINE__ );
	      return ret;
	}

   return ret;
}

static CmsRet wlReadMdm( void )
{
      CmsRet ret = CMSRET_SUCCESS;
      int idx=0;

// read adapters
      for ( idx=0; idx<WL_ADAPTER_NUM; idx++ ) {
           ret = wlReadMdmOne(idx);
	    if ( ret != CMSRET_SUCCESS )
			break;
	}

   return ret;
}

CmsRet wlLockReadMdm( void )
{
      CmsRet ret;

#ifdef WLRDMDMDBG
      printf("Lock reading WLAN Object\n");
#endif

      ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT); //3 //3s delay
      if ( ret != CMSRET_SUCCESS ) {
	      return ret;
      	}

	ret = wlReadMdm();
	cmsLck_releaseLock();

   return ret;
}

CmsRet wlUnlockReadMdm( void )
{
#ifdef WLRDMDMDBG
   printf("Unlock reading WLAN Object\n");
#endif

   return wlReadMdm();
}

CmsRet wlLockReadMdmOne( int idx )
{
      CmsRet ret;

#ifdef WLRDMDMDBG
      printf("Lock reading WLAN Object[%d]\n", idx);
#endif

      ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT ); //3 //3s delay
      if ( ret != CMSRET_SUCCESS ) {
	  	printf("Could not get lock!\n");
	      return ret;
      	}

	ret = wlReadMdmOne(idx);
	cmsLck_releaseLock();

   return ret;
}

CmsRet wlUnlockReadMdmOne( int idx )
{
#ifdef WLRDMDMDBG
   printf("Unlock reading WLAN Object[%d]\n", idx);
#endif

   return wlReadMdmOne(idx);
}

CmsRet wlLockReadMdmTr69Cfg( int idx )
{
    CmsRet ret;
	
#ifdef WLRDMDMDBG
    printf("Lock Reading Wlan Tr69Cfg Object[%d]\n", idx);
#endif

      ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT ); //3 //3s delay
      if ( ret != CMSRET_SUCCESS ) {
	  	printf("Could not get lock!\n");
	      return ret;
      	}
      ret = wlReadTr69Cfg(idx+1, (wl_dsl_tr+idx), wl_dsl_tr_wepkey+idx , wl_dsl_tr_presharedkey+idx);
      cmsLck_releaseLock();
 
   return ret;
}

CmsRet wlUnlockReadMdmTr69Cfg( int idx )
{
    CmsRet ret;
	
#ifdef WLRDMDMDBG
   printf("Unlock Reading Wlan Tr69Cfg Object[%d]\n", idx);
#endif

    ret = wlReadTr69Cfg(idx+1, (wl_dsl_tr+idx),  wl_dsl_tr_wepkey+idx, wl_dsl_tr_presharedkey+idx);

    return ret;
}

/*Following functions are used to write CFG data into MDM*/
static CmsRet wlWriteIntfWepKeyCfg( int objId,  int intfId,   WIRELESS_MSSID_VAR *m_wlMssidVarPtr )
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _WlKey64CfgObject *wepkey64Obj=NULL;
   _WlKey128CfgObject *wepkey128Obj=NULL;
   
//   WIRELESS_MSSID_VAR *m_wlMssidVarPtr; 
   
   int i=0;
   
   //Write WEpKey 64
    for ( i=0; i<4; i++ ) {
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_KEY64_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1); 
	   

	   if ((ret = cmsObj_get(MDMOID_WL_KEY64_CFG, &(pathDesc.iidStack), 0, (void **) &wepkey64Obj)) != CMSRET_SUCCESS)
	   {
		 
               break;
	   }
	   if ( m_wlMssidVarPtr->wlKeys64[i] != NULL ) {
  	  	   MDM_STRCPY(wepkey64Obj->wlKey64, m_wlMssidVarPtr->wlKeys64[i] );
          }
#ifdef WLWRTMDMDBG	
	   printf("m_wlMssidVarPtr->wlKeys64[%d]=%s\n", i,  m_wlMssidVarPtr->wlKeys64[i] );
#endif

	   ret = cmsObj_set(wepkey64Obj, &(pathDesc.iidStack));
          cmsObj_free((void **) &wepkey64Obj);

          if ( ret  != CMSRET_SUCCESS ) 	{
	        printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
               break;
          }

    }

    //Write WEPKEY 128
    if ( ret == CMSRET_SUCCESS ) {
	    for ( i=0; i<4; i++ ) {
		   INIT_PATH_DESCRIPTOR(&pathDesc);
		   pathDesc.oid = MDMOID_WL_KEY128_CFG;
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1); 
		   

		   if ((ret = cmsObj_get(MDMOID_WL_KEY128_CFG, &(pathDesc.iidStack), 0, (void **) &wepkey128Obj)) != CMSRET_SUCCESS)
		   {
		       
	               break;
		   }
		   if ( m_wlMssidVarPtr->wlKeys128[i] != NULL ) {
	  	  	   MDM_STRCPY(wepkey128Obj->wlKey128, m_wlMssidVarPtr->wlKeys128[i] );
	         }
#ifdef WLWRTMDMDBG	
		   printf("m_wlMssidVarPtr->wlKeys128[%d]=%s\n", i,  m_wlMssidVarPtr->wlKeys128[i] );
#endif
		   ret = cmsObj_set(wepkey128Obj, &(pathDesc.iidStack));
	          cmsObj_free((void **) &wepkey128Obj);

	          if ( ret  != CMSRET_SUCCESS ) 	{
		        printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
	               break;
	          }
	   }
    }

   return ret;
}

#ifdef SUPPORT_WSC
static CmsRet wlWriteIntfWpsCfg( int objId,    int intfId, WIRELESS_MSSID_VAR *m_wlMssidVarPtr)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _WlWpsCfgObject      *wpsObj =NULL;
  
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WL_WPS_CFG;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
  if ((ret = cmsObj_get(MDMOID_WL_WPS_CFG, &(pathDesc.iidStack), 0, (void **) &wpsObj)) != \
  	CMSRET_SUCCESS) {
		
		return ret;
  }
  
  if (  m_wlMssidVarPtr->wsc_mode != NULL )
	MDM_STRCPY(wpsObj->wsc_mode, m_wlMssidVarPtr->wsc_mode );
		
  if (  m_wlMssidVarPtr->wsc_config_state != NULL )
	MDM_STRCPY(wpsObj->wsc_config_state, m_wlMssidVarPtr->wsc_config_state );

#ifdef WLWRTMDMDBG	
  printf("m_wlMssidVarPtr->wsc_mode=%s\n", m_wlMssidVarPtr->wsc_mode );
  printf("m_wlMssidVarPtr->wsc_config_state=%s\n", m_wlMssidVarPtr->wsc_config_state );
#endif

  ret = cmsObj_set(wpsObj, &(pathDesc.iidStack));
  cmsObj_free((void **) &wpsObj);

   if ( ret  != CMSRET_SUCCESS ) 	{
	printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
   }

   return ret;

}
#endif

/*Write Mac Flt*/
static CmsRet wlWriteIntfMacFltCfg( int objId,    int intfId, WLAN_ADAPTER_STRUCT *wl_instance)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   
   _WlMacFltObject *macFltObj=NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    WIRELESS_MSSID_VAR *m_wlMssidVarPtr =  &(wl_instance->m_wlMssidVar[intfId]);
   int i=0;
   UINT32  first=0;
   int cnt =0;

   struct wl_flt_mac_entry *pos;
	
/* Clean the old MacFlt table*/

#ifdef WLWRTMDMDBG	
   printf("%s@%d objId=%d, intfId=%d\n", __FUNCTION__, __LINE__, objId, intfId );
#endif

// delete old macFlt list

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WL_MAC_FLT;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
	   
   first= cnt=0;
   while ( (ret = cmsObj_getNextInSubTree(MDMOID_WL_MAC_FLT, 
	   				&(pathDesc.iidStack), &iidStack, (void **) &macFltObj)) == CMSRET_SUCCESS )  {

#ifdef WLWRTMDMDBG	
         printf(" currentDepth=%d\n", iidStack.currentDepth );
	     for ( i=0; i <  iidStack.currentDepth; i++ )
			printf("[%d]=%d\n", i, iidStack.instance[i] );
	     printf("macFltObj->wlMacAddr=%s\n", macFltObj->wlMacAddr);
#endif

	     if ( first==0 )
			first = iidStack.instance[iidStack.currentDepth-1];
	     cnt++;

            cmsObj_free((void **)&macFltObj);
	   }

#ifdef WLWRTMDMDBG	
   printf("first=%d cnt=%d\n", first, cnt );
#endif


   for ( i=0; i < cnt; i++ ) {
       INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_MAC_FLT;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), first+i);
#ifdef WLWRTMDMDBG	
       printf("Remove macFltObj Id=%d\n", first+i );
#endif
  	   cmsObj_deleteInstance( pathDesc.oid, &pathDesc.iidStack);
   }

  //write new one
  list_for_each(pos, (m_wlMssidVarPtr->m_tblFltMac) )  {
       INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_MAC_FLT;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), intfId+1); 

	   if ((ret = cmsObj_addInstance((pathDesc.oid), &(pathDesc.iidStack))) != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Add Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
	      return ret;
	   }

	   if ((ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), 0, (void **) &macFltObj)) != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Get Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
	      return ret;
	   }
	
	   MDM_STRCPY(macFltObj->wlIfcname, pos->ifcName);
	   MDM_STRCPY(macFltObj->wlMacAddr, pos->macAddress );
	   MDM_STRCPY(macFltObj->wlSsid,  pos->ssid );
	   
#ifdef WLWRTMDMDBG	
       printf("pos->ifcName=%s\n", pos->ifcName);
	   printf("pos->macAddress=%s\n", pos->macAddress);
	   printf("pos->ssid=%s\n", pos->ssid);
#endif


	   ret = cmsObj_set(macFltObj, &(pathDesc.iidStack));
	   cmsObj_free((void **)&macFltObj);

	   if (ret != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Get Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
		  return ret;
	   }
   }

   return CMSRET_SUCCESS;
}

static CmsRet wlWriteIntfCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _WlVirtIntfCfgObject *intfObj=NULL;

   WIRELESS_MSSID_VAR *m_wlMssidVarPtr; 
   
#ifdef BCMWAPI_WAI
   char *wapi_data;
#endif
   int j=0;

   for ( j=0; j<WL_NUM_SSID; j++ ) {
  	   m_wlMssidVarPtr =  &(wl_instance->m_wlMssidVar[j]);
	   
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   
	   pathDesc.oid = MDMOID_WL_VIRT_INTF_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), j+1); 
	   
#ifdef WLWRTMDMDBG	
	   printf("objId=%d\n", objId);
#endif

          if ((ret = cmsObj_get(MDMOID_WL_VIRT_INTF_CFG, &(pathDesc.iidStack), 0, (void **) &intfObj)) != CMSRET_SUCCESS)
	  {
		 
               break;
	  }

         intfObj->wlIdx = j;
         if ( m_wlMssidVarPtr->wlSsid != NULL )
	   	MDM_STRCPY(intfObj->wlSsid, m_wlMssidVarPtr->wlSsid);
         if ( wl_instance->m_ifcName[j] != NULL )
		 	MDM_STRCPY( intfObj->wlIfcname, wl_instance->m_ifcName[j] );
		 
#if 0
         if ( m_wlMssidVarPtr->wlBrName != NULL )
		 	MDM_STRCPY( intfObj->wlBrName, m_wlMssidVarPtr->wlBrName );
#endif
         if ( wl_instance->bssMacAddr[j] != NULL )
		 	MDM_STRCPY( intfObj->wlBssMacAddr, wl_instance->bssMacAddr[j] );

	  if ( m_wlMssidVarPtr->wlWpaPsk != NULL )
		MDM_STRCPY(intfObj->wlWpaPsk, m_wlMssidVarPtr->wlWpaPsk );
	  if ( m_wlMssidVarPtr->wlRadiusKey != NULL )
	       MDM_STRCPY(intfObj->wlRadiusKey,  m_wlMssidVarPtr->wlRadiusKey);
	  if ( m_wlMssidVarPtr->wlWep != NULL )
	       MDM_STRCPY(intfObj->wlWep, m_wlMssidVarPtr->wlWep);
	  if ( m_wlMssidVarPtr->wlWpa != NULL )
	       MDM_STRCPY(intfObj->wlWpa, m_wlMssidVarPtr->wlWpa );
		   
	  MDM_STRCPY(intfObj->wlRadiusServerIP,  inet_ntoa(m_wlMssidVarPtr->wlRadiusServerIP));
			
	  if ( m_wlMssidVarPtr->wlAuthMode != NULL )
	        MDM_STRCPY(intfObj->wlAuthMode, m_wlMssidVarPtr->wlAuthMode);
	  intfObj->wlWpaGTKRekey= m_wlMssidVarPtr->wlWpaGTKRekey;
	  intfObj->wlRadiusPort = m_wlMssidVarPtr->wlRadiusPort;
	  intfObj->wlAuth = m_wlMssidVarPtr->wlAuth;
	  intfObj->wlEnblSsid = m_wlMssidVarPtr->wlEnblSsid;
	  intfObj->wlKeyIndex128 = m_wlMssidVarPtr->wlKeyIndex128;
	  intfObj->wlKeyIndex64 = m_wlMssidVarPtr->wlKeyIndex64;
	  intfObj->wlKeyBit = m_wlMssidVarPtr->wlKeyBit;
	  intfObj->wlPreauth = m_wlMssidVarPtr->wlPreauth;
	  intfObj->wlNetReauth = m_wlMssidVarPtr->wlNetReauth;
		//     wlNasWillrun; /*runtime*/
	  intfObj->wlHide = m_wlMssidVarPtr->wlHide;
	  intfObj->wlAPIsolation = m_wlMssidVarPtr->wlAPIsolation;
	  intfObj->wlMaxAssoc = m_wlMssidVarPtr->wlMaxAssoc;
	  intfObj->wlDisableWme = m_wlMssidVarPtr->wlDisableWme; /* this is per ssid advertisement in beacon/probe_resp */
#ifdef WMF
	  intfObj->wlEnableWmf = m_wlMssidVarPtr->wlEnableWmf;
#endif
#ifdef HSPOT_SUPPORT
	  intfObj->wlEnableHspot = m_wlMssidVarPtr->wlEnableHspot;
#endif
#ifdef BCMWAPI_WAI
    wapi_data = malloc(WAPI_CERT_BUFF_SIZE);

    if (wapi_data != NULL)
    {
        int fd;
        ssize_t size;
        char f_name[32];

        sprintf(f_name, "/var/%s_apcert.cer", intfObj->wlIfcname);

        fd = open(f_name, O_RDONLY);

        if (fd > 0)
        {
            memset(wapi_data, 0, WAPI_CERT_BUFF_SIZE);
            size = read(fd, wapi_data, WAPI_CERT_BUFF_SIZE);

            if ((size > 0)  && (strlen(wapi_data) < 2048))
            {
	             MDM_STRCPY(intfObj->wlWapiCertificate, wapi_data);
            }
            close(fd);
        }

        free(wapi_data);
    }
     else
         printf("%s@%d malloc error...\n", __FUNCTION__, __LINE__);
#endif
	  if ( m_wlMssidVarPtr->wlFltMacMode != NULL )
	      MDM_STRCPY(intfObj->wlFltMacMode, m_wlMssidVarPtr->wlFltMacMode );

#ifdef WLWRTMDMDBG	
	  printf("m_wlMssidVarPtr->wlFltMacMode=%s\n", m_wlMssidVarPtr->wlFltMacMode ); //jhc&z
#endif
     

#ifdef WLWRTMDMDBG	
	  printf("m_wlMssidVarPtr->wlEnblSsid[%d] intfObj->wlIdx[%d]\n", 
	  	m_wlMssidVarPtr->wlEnblSsid, intfObj->wlIdx);
	  printf(" m_wlMssidVarPtr->wlSsid[%s] intfObj->wlIfcname=[%s]\n", 
	  	m_wlMssidVarPtr->wlSsid, wl_instance->m_ifcName[objId]);
#endif

         ret = cmsObj_set(intfObj, &(pathDesc.iidStack));
         cmsObj_free((void **) &intfObj);

         if ( ret  != CMSRET_SUCCESS ) {	
	    printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
	    break;
         }


//Write wepkey Cfg
	   if ( (ret = wlWriteIntfWepKeyCfg( objId, j, m_wlMssidVarPtr) ) != CMSRET_SUCCESS ) 
               break;

#ifdef SUPPORT_WSC
// Write WPS  Cfg
	   if ( (ret = wlWriteIntfWpsCfg( objId,   j,  m_wlMssidVarPtr) ) != CMSRET_SUCCESS ) 
               break;
#endif

// Write MacFltCfg
	   if ( (ret =wlWriteIntfMacFltCfg( objId,   j,  wl_instance)) != CMSRET_SUCCESS )
	   	break;

    }

    return ret;
}

/*Read Tr69 data-model object*/
static CmsRet wlWriteTr69Cfg( int objId, struct wl_dsl_tr_struct *wl_dsl_trPtr, 
                                         struct wl_dsl_tr_wepkey_struct *wl_dsl_tr_wepkey, struct wl_dsl_tr_presharedkey_struct *wl_dsl_tr_presharedkey)
{
//   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   _LanWlanObject *wllanWlanObj=NULL;
   
   MdmPathDescriptor pathDesc;
   LanWlanWepKeyObject *wlwepKeyObj = NULL;
   LanWlanPreSharedKeyObject *wlPreSharedKeyObj = NULL;
   int i =0;
   int guestMdmOff;
   struct mdm_object_mbss_struct *mdm_object_mbssPtr;
   struct wl_dsl_tr_mbss_struct *wl_dsl_tr_mbssPtr;
   LanWlanVirtMbssObject *wllanWlanVirtMbssObj=NULL;
   MdmPathDescriptor pathDesc_virtMbss;

//Write Base Object
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_LAN_WLAN;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 

   if ((ret = cmsObj_get(MDMOID_LAN_WLAN, &(pathDesc.iidStack), 0, (void **) &wllanWlanObj)) 
   	       != CMSRET_SUCCESS) {
      
	      return ret;
    }



    wllanWlanObj->enable = wl_dsl_trPtr->enable;
    MDM_STRCPY( wllanWlanObj->BSSID, wl_dsl_trPtr->BSSID );
    MDM_STRCPY( wllanWlanObj->maxBitRate, wl_dsl_trPtr->maxBitRate );
    wllanWlanObj->channel = wl_dsl_trPtr->channel;
    MDM_STRCPY( wllanWlanObj->SSID, wl_dsl_trPtr->SSID);
    MDM_STRCPY( wllanWlanObj->beaconType, wl_dsl_trPtr->beaconType);
    wllanWlanObj->MACAddressControlEnabled = wl_dsl_trPtr->MACAddressControlEnabled;
    MDM_STRCPY( wllanWlanObj->standard, wl_dsl_trPtr->standard);
    wllanWlanObj->WEPKeyIndex = wl_dsl_trPtr->WEPKeyIndex;
    MDM_STRCPY(wllanWlanObj->keyPassphrase, wl_dsl_trPtr->keyPassphrase);
    MDM_STRCPY( wllanWlanObj->WEPEncryptionLevel, wl_dsl_trPtr->WEPEncryptionLevel );
    MDM_STRCPY( wllanWlanObj->basicEncryptionModes, wl_dsl_trPtr->basicEncryptionModes);
    MDM_STRCPY( wllanWlanObj->basicAuthenticationMode, wl_dsl_trPtr->basicAuthenticationMode);
    MDM_STRCPY( wllanWlanObj->WPAEncryptionModes, wl_dsl_trPtr->WPAEncryptionModes);
    MDM_STRCPY( wllanWlanObj->WPAAuthenticationMode, wl_dsl_trPtr->WPAAuthenticationMode);
    MDM_STRCPY( wllanWlanObj->IEEE11iEncryptionModes, wl_dsl_trPtr->IEEE11iEncryptionModes);
    MDM_STRCPY( wllanWlanObj->IEEE11iAuthenticationMode, wl_dsl_trPtr->IEEE11iAuthenticationMode);
    MDM_STRCPY( wllanWlanObj->possibleChannels, wl_dsl_trPtr->possibleChannels);
    MDM_STRCPY( wllanWlanObj->basicDataTransmitRates, wl_dsl_trPtr->basicDataTransmitRates);
    MDM_STRCPY( wllanWlanObj->operationalDataTransmitRates, wl_dsl_trPtr->operationalDataTransmitRates);
    MDM_STRCPY( wllanWlanObj->possibleDataTransmitRates, wl_dsl_trPtr->possibleDataTransmitRates);
    wllanWlanObj->insecureOOBAccessEnabled = wl_dsl_trPtr->insecureOOBAccessEnabled;
    wllanWlanObj->beaconAdvertisementEnabled = wl_dsl_trPtr->beaconAdvertisementEnabled;
    wllanWlanObj->radioEnabled = wl_dsl_trPtr->radioEnabled;
    wllanWlanObj->autoRateFallBackEnabled = wl_dsl_trPtr->autoRateFallBackEnabled;
    MDM_STRCPY( wllanWlanObj->locationDescription, wl_dsl_trPtr->locationDescription);
    MDM_STRCPY( wllanWlanObj->regulatoryDomain, wl_dsl_trPtr->regulatoryDomain);
    wllanWlanObj->totalPSKFailures = wl_dsl_trPtr->totalPSKFailures;
    wllanWlanObj->totalIntegrityFailures = wl_dsl_trPtr->totalIntegrityFailures;
    MDM_STRCPY( wllanWlanObj->channelsInUse, wl_dsl_trPtr->channelsInUse);
    MDM_STRCPY( wllanWlanObj->deviceOperationMode, wl_dsl_trPtr->deviceOperationMode);
    wllanWlanObj->distanceFromRoot = wl_dsl_trPtr->distanceFromRoot;
    MDM_STRCPY( wllanWlanObj->peerBSSID, wl_dsl_trPtr->peerBSSID);
    MDM_STRCPY( wllanWlanObj->authenticationServiceMode, wl_dsl_trPtr->authenticationServiceMode);
    wllanWlanObj->totalBytesSent = wl_dsl_trPtr->totalBytesSent;
    wllanWlanObj->totalBytesReceived = wl_dsl_trPtr->totalBytesReceived;
    wllanWlanObj->totalPacketsSent = wl_dsl_trPtr->totalPacketsSent;
    wllanWlanObj->totalPacketsReceived = wl_dsl_trPtr->totalPacketsReceived;
    wllanWlanObj->totalAssociations = wl_dsl_trPtr->totalAssociations;

/*Private fileds*/
    MDM_STRCPY( wllanWlanObj->X_BROADCOM_COM_IfName, wl_dsl_trPtr->X_BROADCOM_COM_IfName);
    wllanWlanObj->X_BROADCOM_COM_HideSSID = wl_dsl_trPtr->X_BROADCOM_COM_HideSSID;
    wllanWlanObj->X_BROADCOM_COM_TxPowerPercent = wl_dsl_trPtr->X_BROADCOM_COM_TxPowerPercent;

    guestMdmOff = sizeof(struct mdm_object_mbss_struct);
    for (i=0; i<WL_NUM_SSID-1; i++) {
        wl_dsl_tr_mbssPtr = &wl_dsl_trPtr->GuestMbss[i];
        mdm_object_mbssPtr = (struct mdm_object_mbss_struct *)((int)&wllanWlanObj->X_BROADCOM_COM_GuestSSID + i * guestMdmOff);

        if (i < GUEST2_BSS_IDX ) {
            MDM_STRCPY( mdm_object_mbssPtr->X_BROADCOM_COM_GuestSSID, wl_dsl_tr_mbssPtr->GuestSSID);
            MDM_STRCPY( mdm_object_mbssPtr->X_BROADCOM_COM_GuestBSSID, wl_dsl_tr_mbssPtr->GuestBSSID);
            mdm_object_mbssPtr->X_BROADCOM_COM_GuestEnable = wl_dsl_tr_mbssPtr->GuestEnable;
            mdm_object_mbssPtr->X_BROADCOM_COM_GuestHiden = wl_dsl_tr_mbssPtr->GuestHiden;
            mdm_object_mbssPtr->X_BROADCOM_COM_GuestIsolateClients =  wl_dsl_tr_mbssPtr->GuestIsolateClients;
            mdm_object_mbssPtr->X_BROADCOM_COM_GuestDisableWMMAdvertise = wl_dsl_tr_mbssPtr->GuestDisableWMMAdvertise;
            mdm_object_mbssPtr->X_BROADCOM_COM_GuestMaxClients = wl_dsl_tr_mbssPtr->GuestMaxClients;
        } else {

            INIT_PATH_DESCRIPTOR(&pathDesc_virtMbss);
            pathDesc_virtMbss.oid = MDMOID_LAN_WLAN_VIRT_MBSS;
            PUSH_INSTANCE_ID(&(pathDesc_virtMbss.iidStack), 1); 
            PUSH_INSTANCE_ID(&(pathDesc_virtMbss.iidStack), objId); 
            PUSH_INSTANCE_ID(&(pathDesc_virtMbss.iidStack), i-GUEST2_BSS_IDX+1); 

            if ((ret = cmsObj_get(pathDesc_virtMbss.oid, &(pathDesc_virtMbss.iidStack), 0, (void **) &wllanWlanVirtMbssObj)) 
                != CMSRET_SUCCESS) {	 
               break;
            }
            MDM_STRCPY( wllanWlanVirtMbssObj->mbssGuestSSID, wl_dsl_tr_mbssPtr->GuestSSID);
            MDM_STRCPY( wllanWlanVirtMbssObj->mbssGuestBSSID, wl_dsl_tr_mbssPtr->GuestBSSID);
            wllanWlanVirtMbssObj->mbssGuestEnable = wl_dsl_tr_mbssPtr->GuestEnable;
            wllanWlanVirtMbssObj->mbssGuestHiden = wl_dsl_tr_mbssPtr->GuestHiden;
            wllanWlanVirtMbssObj->mbssGuestIsolateClients =  wl_dsl_tr_mbssPtr->GuestIsolateClients;
            wllanWlanVirtMbssObj->mbssGuestDisableWMMAdvertise = wl_dsl_tr_mbssPtr->GuestDisableWMMAdvertise;
            wllanWlanVirtMbssObj->mbssGuestMaxClients = wl_dsl_tr_mbssPtr->GuestMaxClients;

            ret = cmsObj_set(wllanWlanVirtMbssObj, &(pathDesc_virtMbss.iidStack));
            cmsObj_free((void **) &wllanWlanVirtMbssObj);

            if ( ret  != CMSRET_SUCCESS ) 	{
               cmsLog_error("cmsObj_set failed, ret=%d", ret );
               break;
            }
        }
    }

   ret = cmsObj_set(wllanWlanObj, &(pathDesc.iidStack));
   cmsObj_free((void **)&wllanWlanObj);	

   if ( ret  != CMSRET_SUCCESS ) 	{
	    printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
	    return ret;
   }
	

#ifdef WLRDMDMDBG	
    printf("wl_dsl_trPtr->enable=%d\n", wl_dsl_trPtr->enable);

    printf("wl_dsl_trPtr->status=%s\n", wl_dsl_trPtr->status);
	
    printf("wl_dsl_trPtr->BSSID=%s\n", wl_dsl_trPtr->BSSID );

    printf("wl_dsl_trPtr->maxBitRate=%s\n", wl_dsl_trPtr->maxBitRate );
		
    printf("wl_dsl_trPtr->channel = %d\n", wl_dsl_trPtr->channel);

    printf("wl_dsl_trPtr->SSID=%s\n", wl_dsl_trPtr->SSID);
	
    printf("wl_dsl_trPtr->beaconType=%s\n", wl_dsl_trPtr->beaconType);
	
    printf("wl_dsl_trPtr->MACAddressControlEnabled = %d\n", wl_dsl_trPtr->MACAddressControlEnabled);
    printf(" wl_dsl_trPtr->standard=%s\n",  wl_dsl_trPtr->standard);
	
    printf("wl_dsl_trPtr->WEPKeyIndex=%d\n",  wl_dsl_trPtr->WEPKeyIndex);
    printf("wl_dsl_trPtr->WEPEncryptionLevel=%s\n", wl_dsl_trPtr->WEPEncryptionLevel );
	
    printf("wl_dsl_trPtr->basicEncryptionModes=%s\n", wl_dsl_trPtr->basicEncryptionModes);
    printf("wl_dsl_trPtr->basicAuthenticationMode=%s\n", wl_dsl_trPtr->basicAuthenticationMode);
    printf("wl_dsl_trPtr->WPAEncryptionModes=%s\n", wl_dsl_trPtr->WPAEncryptionModes);
    printf("wl_dsl_trPtr->WPAAuthenticationMode=%s\n", wl_dsl_trPtr->WPAAuthenticationMode);
    printf("wl_dsl_trPtr->basicDataTransmitRates=%s\n", wl_dsl_trPtr->basicDataTransmitRates);
    printf("wl_dsl_trPtr->radioEnabled = %d\n", wl_dsl_trPtr->radioEnabled);
    printf("wl_dsl_trPtr->regulatoryDomain=%s\n", wl_dsl_trPtr->regulatoryDomain);

#endif
  



/*Read Wep Key*/
for ( i=0; i<4; i++ ) {
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_LAN_WLAN_WEP_KEY;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1); 
	   

          if ((ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), 0, (void **) &wlwepKeyObj)) 
   	               != CMSRET_SUCCESS) {
		 
               break;
	   }
  	   MDM_STRCPY(wlwepKeyObj->WEPKey, wl_dsl_tr_wepkey->WEPKey[i]  );

#ifdef WLWRTMDMDBG	
          printf("wl_dsl_tr_wepkey[%d]->WEPKey=%s\n", i, wl_dsl_tr_wepkey->WEPKey[i] );
#endif

         ret = cmsObj_set(wlwepKeyObj, &(pathDesc.iidStack));

          cmsObj_free((void **) &wlwepKeyObj);

          if ( ret  != CMSRET_SUCCESS ) 	{
	        printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
               break;
          }

    }

/*Read PreSharedKey*/
    for ( i=0; i<1; i++ ) 
    {
       INIT_PATH_DESCRIPTOR(&pathDesc);
       pathDesc.oid = MDMOID_LAN_WLAN_PRE_SHARED_KEY;
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1); 

       if ((ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), 0, (void **) &wlPreSharedKeyObj))  != CMSRET_SUCCESS) 
       {
          break;
       }
#if 0
       MDM_STRCPY(wlPreSharedKeyObj->preSharedKey, wl_dsl_tr_presharedkey->PreSharedKey[i]  );
#else
       MDM_STRCPY(wlPreSharedKeyObj->keyPassphrase, wl_dsl_tr_presharedkey->PreSharedKey[i]  );
#endif

#ifdef WLWRTMDMDBG	
          printf("wl_dsl_tr_presharedkey[%d]->PreSharedKey=%s\n", i, wl_dsl_tr_presharedkey->PreSharedKey[i] );
#endif

       ret = cmsObj_set(wlPreSharedKeyObj, &(pathDesc.iidStack));

       cmsObj_free((void **) &wlPreSharedKeyObj);

       if ( ret  != CMSRET_SUCCESS )
       {
          printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
          break;
       }
    }

   return ret;

}


CmsRet wlWriteBaseCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   _WlBaseCfgObject *wlBaseCfgObj=NULL;
   WIRELESS_VAR *m_wlVarPtr = &(wl_instance->m_wlVar);
   


//Write BaseCfg Object to m_wlVar
   iidStack.instance[0] = 1;
   iidStack.instance[1] = objId;
   iidStack.currentDepth=2;
   if ((ret = cmsObj_get(MDMOID_WL_BASE_CFG, &iidStack, 0, (void **)&wlBaseCfgObj)) != CMSRET_SUCCESS) {
	      
	      return ret;
   }

   wlBaseCfgObj->wlEnbl = m_wlVarPtr->wlEnbl;
	 
   wlBaseCfgObj->wlSsidIdx = m_wlVarPtr->wlSsidIdx;

#ifdef WLWRTMDMDBG	
    printf("m_wlVarPtr->wlSsidIdx=%d\n", m_wlVarPtr->wlSsidIdx );
#endif
   
   if ( m_wlVarPtr->wlMode != NULL )
	   MDM_STRCPY(wlBaseCfgObj->wlMode, m_wlVarPtr->wlMode);

   if ( m_wlVarPtr->wlCountry != NULL )
   {
	   char ccountry[WL_SM_SIZE_MAX];
	   sprintf(ccountry, "%s/%d",m_wlVarPtr->wlCountry,m_wlVarPtr->wlRegRev);	  
	   /* DO not sprintf to  BaseCfgObj as it may have no enough memory allocated,MDM_STRCPY 
	    * make sure enough memeory allocated */
	   MDM_STRCPY(wlBaseCfgObj->wlCountry,ccountry);

   }
//   wlFltMacMode[WL_SM_SIZE_MAX];

  if ( m_wlVarPtr->wlPhyType!= NULL )
	   MDM_STRCPY(wlBaseCfgObj->wlPhyType, m_wlVarPtr->wlPhyType);

  wlBaseCfgObj->wlCoreRev = m_wlVarPtr->wlCoreRev;
  
  if ( m_wlVarPtr->wlBasicRate!= NULL )
  	MDM_STRCPY(wlBaseCfgObj->wlBasicRate, m_wlVarPtr->wlBasicRate);

  if ( m_wlVarPtr->wlProtection != NULL )
	   MDM_STRCPY(wlBaseCfgObj->wlProtection , m_wlVarPtr->wlProtection);
  
  if ( m_wlVarPtr->wlPreambleType != NULL )
	   MDM_STRCPY(wlBaseCfgObj->wlPreambleType, m_wlVarPtr->wlPreambleType);
  
  if ( m_wlVarPtr->wlAfterBurnerEn != NULL )
	   MDM_STRCPY(wlBaseCfgObj->wlAfterBurnerEn, m_wlVarPtr->wlAfterBurnerEn);

  if ( m_wlVarPtr->wlFrameBurst != NULL )
	   MDM_STRCPY(wlBaseCfgObj->wlFrameBurst, m_wlVarPtr->wlFrameBurst);

//  strcpy(m_wlVarPtr->wlWlanIfName, "wl0" ); // should not be hardcodded


//Write wdsCfg   char wlWds[WL_WDS_NUM][WL_MID_SIZE_MAX];

  
   wlBaseCfgObj->wlChannel = m_wlVarPtr->wlChannel;
   wlBaseCfgObj->wlCurrentChannel = wl_instance->wlCurrentChannel;

  
  wlBaseCfgObj->wlFrgThrshld = m_wlVarPtr->wlFrgThrshld;
  wlBaseCfgObj->wlRtsThrshld = m_wlVarPtr->wlRtsThrshld;
  wlBaseCfgObj->wlDtmIntvl = m_wlVarPtr->wlDtmIntvl;
  wlBaseCfgObj->wlBcnIntvl = m_wlVarPtr->wlBcnIntvl;
  wlBaseCfgObj->wlRate = m_wlVarPtr->wlRate;
  wlBaseCfgObj->wlgMode= m_wlVarPtr->wlgMode;
//   m_wlVarPtr->wlLazyWds = wlBaseCfgObj->wlLazyWds;
  wlBaseCfgObj->wlBand = m_wlVarPtr->wlBand;
  wlBaseCfgObj->wlMCastRate = m_wlVarPtr->wlMCastRate;
  wlBaseCfgObj->wlInfra = m_wlVarPtr->wlInfra;
  wlBaseCfgObj->wlAntDiv = m_wlVarPtr->wlAntDiv;
  wlBaseCfgObj->wlWme = m_wlVarPtr->wlWme;
  wlBaseCfgObj->wlWmeNoAck = m_wlVarPtr->wlWmeNoAck;
  wlBaseCfgObj->wlWmeApsd = m_wlVarPtr->wlWmeApsd;
  wlBaseCfgObj->wlTxPwrPcnt = m_wlVarPtr->wlTxPwrPcnt;
  wlBaseCfgObj->wlRegMode = m_wlVarPtr->wlRegMode;
  wlBaseCfgObj->wlDfsPreIsm = m_wlVarPtr->wlDfsPreIsm;
  wlBaseCfgObj ->wlDfsPostIsm = m_wlVarPtr->wlDfsPostIsm; 
  wlBaseCfgObj->wlTpcDb = m_wlVarPtr->wlTpcDb;
  wlBaseCfgObj->wlCsScanTimer = m_wlVarPtr->wlCsScanTimer;  
  wlBaseCfgObj->wlGlobalMaxAssoc = m_wlVarPtr->wlGlobalMaxAssoc;
  wlBaseCfgObj->wlMBands = wl_instance->m_bands ;


  if ( wl_instance->wlVer != NULL )
	MDM_STRCPY(wlBaseCfgObj->wlVer, wl_instance->wlVer);
  
  wlBaseCfgObj->wlLazyWds = m_wlVarPtr->wlLazyWds;
  wlBaseCfgObj->wlMaxMbss = wl_instance->maxMbss;
  wlBaseCfgObj->wlMbssSupported = wl_instance->mbssSupported;
  wlBaseCfgObj->wlNumBss = wl_instance->numBss;

  wlBaseCfgObj->wlAburnSupported = wl_instance->aburnSupported;
  wlBaseCfgObj->wlAmpduSupported = wl_instance->ampduSupported;
  wlBaseCfgObj->wlAmsduSupported = wl_instance->amsduSupported;
  wlBaseCfgObj->wlWmeSupported = wl_instance->wmeSupported;    
#ifdef WMF
  wlBaseCfgObj->wlWmfSupported = wl_instance->wmfSupported;    
#endif
#ifdef DUCATI
    wlBaseCfgObj->wlVecSupported    = wl_instance->wlVecSupported;
    wlBaseCfgObj->wlIperf           = wl_instance->wlIperf;
    wlBaseCfgObj->wlVec             = wl_instance->wlVec;
#endif

   wlBaseCfgObj->wlRifsAdvert = m_wlVarPtr->wlRifsAdvert;
   wlBaseCfgObj->wlChanImEnab = m_wlVarPtr->wlChanImEnab;
   wlBaseCfgObj->wlObssCoex   = m_wlVarPtr->wlObssCoex;
   wlBaseCfgObj->wlRxChainPwrSaveEnable         = m_wlVarPtr->wlRxChainPwrSaveEnable;
   wlBaseCfgObj->wlRxChainPwrSaveQuietTime      = m_wlVarPtr->wlRxChainPwrSaveQuietTime;
   wlBaseCfgObj->wlRxChainPwrSavePps            = m_wlVarPtr->wlRxChainPwrSavePps;
   wlBaseCfgObj->wlRadioPwrSaveEnable           = m_wlVarPtr->wlRadioPwrSaveEnable;
   wlBaseCfgObj->wlRadioPwrSaveQuietTime        = m_wlVarPtr->wlRadioPwrSaveQuietTime;
   wlBaseCfgObj->wlRadioPwrSavePps              = m_wlVarPtr->wlRadioPwrSavePps;
   wlBaseCfgObj->wlRadioPwrSaveLevel           = m_wlVarPtr->wlRadioPwrSaveLevel;

   //WEP over WDS
   wlBaseCfgObj->wlWdsSec = m_wlVarPtr->wlWdsSec;
   if (m_wlVarPtr->wlWdsKey != NULL)
       MDM_STRCPY(wlBaseCfgObj->wlWdsKey, m_wlVarPtr->wlWdsKey);
   wlBaseCfgObj->wlWdsSecEnable = m_wlVarPtr->wlWdsSecEnable;
   wlBaseCfgObj->wlEnableUre   = m_wlVarPtr->wlEnableUre;
   wlBaseCfgObj->wlStaRetryTime   = m_wlVarPtr->wlStaRetryTime;
   ret = cmsObj_set(wlBaseCfgObj, &iidStack);
   cmsObj_free((void **) &wlBaseCfgObj);

   if ( ret  != CMSRET_SUCCESS ) 	{
	    printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
   }


#ifdef WLWRTMDMDBG	
//Debug purpose Show the data

  printf("m_wlVarPtr->wlSsidIdx=%d\n", m_wlVarPtr->wlSsidIdx );
  printf("m_wlVarPtr->wlMode=%s\n", m_wlVarPtr->wlMode );
  
  printf("m_wlVarPtr->wlChannel = %d\n", m_wlVarPtr->wlChannel );
  printf("m_wlVarPtr->wlCountry=%s\n", m_wlVarPtr->wlCountry );
  printf("m_wlVarPtr->wlPhyType=%s\n", m_wlVarPtr->wlPhyType );
  
  printf("m_wlVarPtr->wlBasicRate=%s\n", m_wlVarPtr->wlBasicRate );

  printf("m_wlVarPtr->wlProtection=%s\n", m_wlVarPtr->wlProtection );
  printf("m_wlVarPtr->wlPreambleType=%s\n", m_wlVarPtr->wlPreambleType );

  printf("m_wlVarPtr->wlAfterBurnerEn=%s\n", m_wlVarPtr->wlAfterBurnerEn );
  
  printf("m_wlVarPtr->wlFrameBurst=%s\n", m_wlVarPtr->wlFrameBurst );
  
  printf("wl_instance->wlCurrentChannel = %d\n", wl_instance->wlCurrentChannel );
   
//  printf("wl_instance->wlWlanIfName=%s\n", wl_instance->wlWlanIfName ); 
   
  printf("m_wlVarPtr->wlEnbl=%d\n",  m_wlVarPtr->wlEnbl);
   
  printf("m_wlVarPtr->wlFrgThrshld=%d\n", m_wlVarPtr->wlFrgThrshld);
  printf("m_wlVarPtr->wlRtsThrshld=%d\n",    m_wlVarPtr->wlRtsThrshld);
  printf(" m_wlVarPtr->wlDtmIntvl=%d\n",    m_wlVarPtr->wlDtmIntvl);
  printf("m_wlVarPtr->wlBcnIntvl=%d\n",   m_wlVarPtr->wlBcnIntvl);
  printf("m_wlVarPtr->wlRate=%ld\n",    m_wlVarPtr->wlRate);
  printf("m_wlVarPtr->wlgMode=%d\n",    m_wlVarPtr->wlgMode);

  printf("m_wlVarPtr->wlBand=%d\n",    m_wlVarPtr->wlBand);
  printf("m_wlVarPtr->wlMCastRate=%d\n",    m_wlVarPtr->wlMCastRate);
  printf("m_wlVarPtr->wlInfra=%d\n",    m_wlVarPtr->wlInfra);
  printf("m_wlVarPtr->wlAntDiv=%d\n",   m_wlVarPtr->wlAntDiv);
  printf("m_wlVarPtr->wlWme=%d\n",    m_wlVarPtr->wlWme);
  printf("m_wlVarPtr->wlWmeNoAck=%d\n",    m_wlVarPtr->wlWmeNoAck);
  printf("m_wlVarPtr->wlWmeApsd=%d\n",    m_wlVarPtr->wlWmeApsd);
  printf("m_wlVarPtr->wlTxPwrPcnt=%d\n",    m_wlVarPtr->wlTxPwrPcnt);
  printf("m_wlVarPtr->wlRegMode=%d\n",    m_wlVarPtr->wlRegMode);
  printf("m_wlVarPtr->wlDfsPreIsm=%d\n",    m_wlVarPtr->wlDfsPreIsm);
  printf("m_wlVarPtr->wlDfsPostIsm=%d\n",    m_wlVarPtr->wlDfsPostIsm); 
  printf("m_wlVarPtr->wlTpcDb=%d\n",    m_wlVarPtr->wlTpcDb);
  printf("m_wlVarPtr->wlCsScanTimer=%d\n",    m_wlVarPtr->wlCsScanTimer);  
  printf("m_wlVarPtr->wlGlobalMaxAssoc=%d\n",    m_wlVarPtr->wlGlobalMaxAssoc);
  printf("wl_instance->m_bands=%d\n", wl_instance->m_bands );
  printf("wl_instance->wlVer=%s\n", wl_instance->wlVer );
  printf("m_wlVarPtr->wlLazyWds = %d\n", m_wlVarPtr->wlLazyWds);
  printf("wl_instance->maxMbss = %d\n", wl_instance->maxMbss);
  printf("wl_instance->mbssSupported = %d\n", wl_instance->mbssSupported);
  printf("wl_instance->numBss = %d\n", wl_instance->numBss );
#endif

   return ret;



}

#ifdef SUPPORT_MIMO

static CmsRet wlWriteMimoCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   WIRELESS_VAR *m_wlVarPtr = &(wl_instance->m_wlVar);
   
   _WlMimoCfgObject *wlMimoCfgObj = NULL;

   iidStack.instance[0] = 1;
   iidStack.instance[1] = objId;
   iidStack.currentDepth=2;

   if ((ret = cmsObj_get(MDMOID_WL_MIMO_CFG, &iidStack, 0, (void **)&wlMimoCfgObj)) != CMSRET_SUCCESS) {
	      
	      return ret;
   }  

   wlMimoCfgObj->wlNBwCap = m_wlVarPtr->wlNBwCap;
   wlMimoCfgObj->wlNCtrlsb = m_wlVarPtr->wlNCtrlsb;
   wlMimoCfgObj->wlNBand = m_wlVarPtr->wlNBand;
   wlMimoCfgObj->wlNMcsidx = m_wlVarPtr->wlNMcsidx;
   if ( m_wlVarPtr->wlNProtection !=NULL )
   	MDM_STRCPY(wlMimoCfgObj->wlNProtection, m_wlVarPtr->wlNProtection);
   if ( m_wlVarPtr->wlRifs != NULL )
  	MDM_STRCPY(wlMimoCfgObj->wlRifs, m_wlVarPtr->wlRifs);
   if ( m_wlVarPtr->wlAmpdu != NULL )
   	MDM_STRCPY(wlMimoCfgObj->wlAmpdu, m_wlVarPtr->wlAmpdu);
   if ( m_wlVarPtr->wlAmsdu != NULL )
   	MDM_STRCPY(wlMimoCfgObj->wlAmsdu, m_wlVarPtr->wlAmsdu);
   if ( m_wlVarPtr->wlNmode != NULL )
   	MDM_STRCPY(wlMimoCfgObj->wlNmode, m_wlVarPtr->wlNmode);
   wlMimoCfgObj->wlNReqd = m_wlVarPtr->wlNReqd;
   wlMimoCfgObj->wlStbcTx = m_wlVarPtr->wlStbcTx;
   wlMimoCfgObj->wlStbcRx = m_wlVarPtr->wlStbcRx;
   
#ifdef WLWRTMDMDBG	
   printf("m_wlVarPtr->wlNBand=%d\n", m_wlVarPtr->wlNBand );
#endif

   ret = cmsObj_set(wlMimoCfgObj, &iidStack);
   cmsObj_free((void **) &wlMimoCfgObj);

   if ( ret  != CMSRET_SUCCESS ) 	{
	    printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
   }

   return ret;
}
#endif


#ifdef SUPPORT_SES   
static CmsRet wlWriteSesCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   WIRELESS_VAR *m_wlVarPtr = &(wl_instance->m_wlVar);
   _WlSesCfgObject *wlSesCfgObj = NULL;

   iidStack.instance[0] = 1;
   iidStack.instance[1] = objId;
   iidStack.currentDepth=2;

   if ((ret = cmsObj_get(MDMOID_WL_SES_CFG, &iidStack, 0, (void **)&wlSesCfgObj)) != CMSRET_SUCCESS)  {
      
      return ret;
   }  

   wlSesCfgObj->wlSesEnable = m_wlVarPtr->wlSesEnable;
   wlSesCfgObj->wlSesEvent = m_wlVarPtr->wlSesEvent;
   if ( m_wlVarPtr->wlSesStates != NULL )
   	MDM_STRCPY(m_wlVarPtr->wlSesStates, wlSesCfgObj->wlSesStates);
   if ( m_wlVarPtr->wlSesSsid != NULL )
   	MDM_STRCPY(wlSesCfgObj->wlSesSsid, m_wlVarPtr->wlSesSsid);
   if ( m_wlVarPtr->wlSesWpaPsk != NULL )
   	MDM_STRCPY(wlSesCfgObj->wlSesWpaPsk, m_wlVarPtr->wlSesWpaPsk);  
   wlSesCfgObj->wlSesHide = m_wlVarPtr->wlSesHide;
   wlSesCfgObj->wlSesAuth = m_wlVarPtr->wlSesAuth; 
   if ( m_wlVarPtr->wlSesAuthMode != NULL )
   	MDM_STRCPY(wlSesCfgObj->wlSesAuthMode, m_wlVarPtr->wlSesAuthMode); 
   if ( m_wlVarPtr->wlSesWep != NULL )
   	MDM_STRCPY(wlSesCfgObj->wlSesWep, m_wlVarPtr->wlSesWep);   
   if ( m_wlVarPtr->wlSesWpa != NULL )
   	MDM_STRCPY(wlSesCfgObj->wlSesWpa, m_wlVarPtr->wlSesWpa);      
   if ( m_wlVarPtr->wlSesWpa != NULL )
   	MDM_STRCPY(wlSesCfgObj->wlSesWpa, m_wlVarPtr->wlSesWpa);      

   wlSesCfgObj->wlSesClEnable = m_wlVarPtr->wlSesClEnable;   
   wlSesCfgObj->wlSesClEvent = m_wlVarPtr->wlSesClEvent;   

   if ( m_wlVarPtr->wlSesWdsWsec != NULL )
   	MDM_STRCPY(wlSesCfgObj->wlSesWdsWsec, m_wlVarPtr->wlWdsWsec); 

   ret = cmsObj_set(wlSesCfgObj, &iidStack);
   cmsObj_free((void **) &wlSesCfgObj);

   if ( ret  != CMSRET_SUCCESS ) 	{
	    printf(" %s::%s@%d Failure to Set Obj, ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret );
   }

    return CMSRET_SUCCESS;

}
#endif


/*This function is used for write association Device List into Tr69c Object*/
/* Need think about how tr69c retrieve this object instance*/
CmsRet wlWriteAssocDev( int objId )
{
/* Clean the old assoc object, then write down the new one*/

   MdmPathDescriptor pathDesc;
   CmsRet ret;
   WLAN_ADAPTER_STRUCT *wl_instance = (m_instance_wl+objId -1 );
   
   _LanWlanAssociatedDeviceEntryObject *assocDevObj=NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   _LanWlanObject *wllanWlanObj=NULL;

   int i=0;
   UINT32  first=0;
   int cnt =0;


#ifdef WLWRTMDMDBG	
   printf("objId=%d\n", objId );
#endif

// delete old association Device list

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
	   
   first= cnt=0;
   while ( (ret = cmsObj_getNextInSubTree(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY, 
	   				&(pathDesc.iidStack), &iidStack, (void **) &assocDevObj)) == CMSRET_SUCCESS )  {

#ifdef WLWRTMDMDBG_ASSOC
            printf(" currentDepth=%d\n", iidStack.currentDepth );
	     for ( i=0; i <  iidStack.currentDepth; i++ )
			printf("[%d]=%d\n", i, iidStack.instance[i] );
#endif
	     if ( first==0 )
			first = iidStack.instance[iidStack.currentDepth-1];
	     cnt ++;

            cmsObj_free((void **)&assocDevObj);
	   }

#ifdef WLWRTMDMDBG_ASSOC
   printf("first=%d cnt=%d\n", first, cnt );
#endif			


   for ( i=0; i < cnt; i++ ) {
         INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), first+i);
#ifdef WLWRTMDMDBG_ASSOC
          printf("Remove AccosObject Id=%d\n", first+i );
#endif
  	   cmsObj_deleteInstance( pathDesc.oid, &pathDesc.iidStack);
   }

//add new Assoction device list
#ifdef WLWRTMDMDBG_ASSOC
   printf("objId=%d wl_instance->numStations=%d\n", objId, wl_instance->numStations );
#endif

   for ( i=0; i<wl_instance->numStations ; i++ ) {
   	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);

	   if ((ret = cmsObj_addInstance(pathDesc.oid, &pathDesc.iidStack)) != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Add Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
	      return ret;
	   }


	   if ((ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), 0, (void **) &assocDevObj)) != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Get Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
	      return ret;
	   }

	
	   MDM_STRCPY(assocDevObj->associatedDeviceMACAddress, wl_instance->stationList[i].macAddress );
/* need to put values. Need to find the crresponding data. Impl later*/
	   MDM_STRCPY(assocDevObj->associatedDeviceIPAddress, "" );
          assocDevObj->associatedDeviceAuthenticationState=TRUE;
	   MDM_STRCPY(assocDevObj->lastRequestedUnicastCipher, "" );
	   MDM_STRCPY(assocDevObj->lastRequestedMulticastCipher, "" );
	   MDM_STRCPY(assocDevObj->lastPMKId, "" );
	   MDM_STRCPY(assocDevObj->lastDataTransmitRate, "" );
	   
	   assocDevObj->X_BROADCOM_COM_Associated = wl_instance->stationList[i].associated;
	   assocDevObj->X_BROADCOM_COM_Authorized = wl_instance->stationList[i].authorized;
	   MDM_STRCPY(assocDevObj->X_BROADCOM_COM_Ssid, wl_instance->stationList[i].ssid );
	   MDM_STRCPY(assocDevObj->X_BROADCOM_COM_Ifcname, wl_instance->stationList[i].ifcName );
	   
#ifdef WLWRTMDMDBG_ASSOC
          printf("wl_instance->stationList[%d].macAddress=%s\n", i, wl_instance->stationList[i].macAddress );
	   printf("wl_instance->stationList[%d].associated=%d\n", i, wl_instance->stationList[i].associated);
	   printf("wl_instance->stationList[%d].authorized=%d\n", i, wl_instance->stationList[i].authorized);
	   printf("wl_instance->stationList[%d].ssid=%s\n", i, wl_instance->stationList[i].ssid);
	   printf("wl_instance->stationList[%d].ifcName =%s\n", i, wl_instance->stationList[i].ifcName);
#endif

	   ret = cmsObj_set(assocDevObj, &(pathDesc.iidStack));
 	   cmsObj_free((void **)&assocDevObj);

	   if (ret != CMSRET_SUCCESS)
	   {
		  return ret;
	   }
   }
   /* Update TotalAssociations */

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_LAN_WLAN;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 

   if ((ret = cmsObj_get(MDMOID_LAN_WLAN, &(pathDesc.iidStack), 0, (void **) &wllanWlanObj)) 
   	       != CMSRET_SUCCESS) {
      
	      return ret;
    }

    wllanWlanObj->totalAssociations = wl_instance->numStations;


    ret = cmsObj_set(wllanWlanObj, &(pathDesc.iidStack));
    cmsObj_free((void **)&wllanWlanObj);

    if (ret != CMSRET_SUCCESS)
    {
        return ret;
    }

   return ret;

}

CmsRet wlLockWriteAssocDev( int objId)
{
    CmsRet ret;

#ifdef WLWRTMDMDBG_ASSOC	
    printf("Lock Writing Assoc Dev Object[%d]\n", objId);
#endif

      ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT ); //3 //3s delay
      if ( ret != CMSRET_SUCCESS ) {
	  	printf("Could not get lock!\n");
	      return ret;
      	}
       ret = wlWriteAssocDev(objId);
      cmsLck_releaseLock();
 
   return ret;
}

CmsRet wlUnlockWriteAssocDev( int objId)
{
    CmsRet ret;

#ifdef WLWRTMDMDBG_ASSOC	
    printf("Unlock Writing Assoc Dev Object[%d]\n", objId);
#endif

    ret = wlWriteAssocDev(objId);

   return ret;
}

static CmsRet wlWriteStaticWdsCfg( int objId, WLAN_ADAPTER_STRUCT *wl_instance )
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   
   _WlStaticWdsCfgObject *staticWdsCfgObj=NULL;

   int i=0;

#ifdef WLWRTMDMDBG
   printf("%s@%d objId=%d\n", __FUNCTION__, __LINE__, objId );
#endif
	
/* Clean the old WdsCfg table*/
   for ( i=0; i< 4; i++ ) {
          INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_STATIC_WDS_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), i+1);

          if ((ret = cmsObj_get(MDMOID_WL_STATIC_WDS_CFG, &(pathDesc.iidStack), 0, (void **) &staticWdsCfgObj)) != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Get Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
	      return ret;
	   }

	    MDM_STRCPY(staticWdsCfgObj->wlMacAddr, wl_instance->m_wlVar.wlWds[i] );

#ifdef WLWRTMDMDBG
          printf("wlWriteStaticWdsCfg wl_instance->m_wlVar.wlWds[%d]=%s\n", i, wl_instance->m_wlVar.wlWds[i]);
#endif

	   ret = cmsObj_set(staticWdsCfgObj, &(pathDesc.iidStack));
 	   cmsObj_free((void **)&staticWdsCfgObj);
		  
          if ( ret != CMSRET_SUCCESS)
	   {
		  return ret;
	   }
   }
   return CMSRET_SUCCESS;
}

//Write Dynamic WDS list
static CmsRet wlWriteWdsCfg( int objId,    WLAN_ADAPTER_STRUCT *wl_instance)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   
   _WlWdsCfgObject *wdsCfgObj=NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   int i=0;
   UINT32  first=0;
   int cnt =0;

   struct wl_flt_mac_entry *pos;


#ifdef WLWRTMDMDBG
   printf("%s@%d objId=%d\n", __FUNCTION__, __LINE__, objId );
#endif
   

/* Clean the old Wds table*/
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WL_WDS_CFG;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
	   
   first= cnt=0;
   while ( (ret = cmsObj_getNextInSubTree(MDMOID_WL_WDS_CFG, 
	   				&(pathDesc.iidStack), &iidStack, (void **) &wdsCfgObj)) == CMSRET_SUCCESS )  {

#ifdef WLWRTMDMDBG
            printf(" currentDepth=%d\n", iidStack.currentDepth );
	     for ( i=0; i <  iidStack.currentDepth; i++ )
			printf("[%d]=%d\n", i, iidStack.instance[i] );
	     printf("wdsCfgObj->wlMacAddr=%s\n", wdsCfgObj->wlMacAddr);
#endif

	     if ( first==0 )
			first = iidStack.instance[iidStack.currentDepth-1];
	     cnt++;

            cmsObj_free((void **)&wdsCfgObj);
	   }

#ifdef WLWRTMDMDBG
   printf("first=%d cnt=%d\n", first, cnt );
#endif			

   for ( i=0; i < cnt; i++ ) {
       INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_WDS_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), first+i);
#ifdef WLWRTMDMDBG
          printf("Remove WdsCfgObj Id=%d\n", first+i );
#endif
  	   cmsObj_deleteInstance( pathDesc.oid, &pathDesc.iidStack);
   }

  //write new one
  list_for_each(pos, (wl_instance->m_tblWdsMac) )  {
       INIT_PATH_DESCRIPTOR(&pathDesc);
	   pathDesc.oid = MDMOID_WL_WDS_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);

	   if ((ret = cmsObj_addInstance(pathDesc.oid, &pathDesc.iidStack)) != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Add Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
	      return ret;
	   }


	   if ((ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack), 0, (void **) &wdsCfgObj)) != CMSRET_SUCCESS)
	   {
	      printf("%s@%d Get Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
	      return ret;
	   }

	
	   MDM_STRCPY(wdsCfgObj->wlIfcname, pos->ifcName);
	   MDM_STRCPY(wdsCfgObj->wlMacAddr, pos->macAddress );
	   MDM_STRCPY(wdsCfgObj->wlSsid,  pos->ssid );
	   
#ifdef WLWRTMDMDBG
       printf("pos->ifcName=%s\n", pos->ifcName);
	   printf("pos->macAddress=%s\n", pos->macAddress);
	   printf("pos->ssid=%s\n", pos->ssid);
#endif

	   ret = cmsObj_set(wdsCfgObj, &(pathDesc.iidStack));
	   cmsObj_free((void **)&wdsCfgObj);

	   if (ret != CMSRET_SUCCESS)
	   {
	         printf("%s@%d Set Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
             return ret;
	   }
   }
 
   return CMSRET_SUCCESS;
    
}

static CmsRet wlWriteMdmOne( int idx )
{
    CmsRet ret;
	
#ifdef SUPPORT_SES   
     if ( (ret = wlWriteSesCfg( idx+1, (m_instance_wl +idx) ) )  != CMSRET_SUCCESS) {
            printf("%s@%d WriteSesCfg Error\n", __FUNCTION__, __LINE__ );
	     return ret;	 
     }
#endif

#ifdef SUPPORT_MIMO
     if ( (ret = wlWriteMimoCfg( idx+1, (m_instance_wl +idx) ) )  != CMSRET_SUCCESS) {
	     printf("%s@%d WriteMimoCfg Error\n", __FUNCTION__, __LINE__ );
	     return ret;	 
     }
#endif

      if ( (ret = wlWriteIntfCfg( idx+1, (m_instance_wl+idx)  ))  != CMSRET_SUCCESS) {
	     printf("%s@%d WriteIntfCfg Error\n", __FUNCTION__, __LINE__ );
	     return ret;	 
      }
#if 0
       if ( (ret = wlWriteAssocDev( i+1 ))  != CMSRET_SUCCESS) {
	     printf("%s@%d WriteIntfCfg Error\n", __FUNCTION__, __LINE__ );
	     return ret;	 
       }
#endif
       if ( (ret = wlWriteStaticWdsCfg( idx+1, (m_instance_wl+idx) ))  != CMSRET_SUCCESS) {
	     printf("%s@%d wlWriteStaticWdsCfg Error\n", __FUNCTION__, __LINE__ );
	     return ret;	 
       }

//This is dynamic WDS 
       if ( (ret = wlWriteWdsCfg( idx+1, (m_instance_wl+idx) ))  != CMSRET_SUCCESS)  {
	     printf("%s@%d WriteWdsCfg Error\n", __FUNCTION__, __LINE__ );
	     return ret;	 
	}
	if ( (ret = wlWriteBaseCfg( idx+1, (m_instance_wl +idx) ) )  != CMSRET_SUCCESS)  {
	     printf("%s@%d WriteBaseCfg Error\n", __FUNCTION__, __LINE__ );
	     return ret;	 
	}
	
       return ret;
}

static CmsRet wlWriteMdm( void )
{
    CmsRet ret = CMSRET_SUCCESS;
    int i=0;
	
//write adapter 
      for ( i=0; i<WL_ADAPTER_NUM; i++ ) {
            ret =  wlWriteMdmOne( i );
	     if ( ret != CMSRET_SUCCESS )
		 	break;
      }
      return ret;
}

CmsRet wlLockWriteMdm( void )
{
    CmsRet ret;
	
#ifdef WLWRTMDMDBG	
    printf("Lock Writing WLAN Object\n");
#endif

      ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT ); //3 //3s delay
      if ( ret != CMSRET_SUCCESS ) {
	  	printf("Could not get lock!\n");
	      return ret;
      	}
       ret = wlWriteMdm();
      cmsLck_releaseLock();
 
   return ret;
}

CmsRet wlUnlockWriteMdm( void )
{
#ifdef WLWRTMDMDBG	
    printf("Unlock Writing WLAN Object\n");
#endif

    return wlWriteMdm();
}

CmsRet wlLockWriteMdmOne( int idx )
{
    CmsRet ret;
	
#ifdef WLWRTMDMDBG	
    printf("Lock Writing WLAN Object[%d]\n", idx);
#endif

      ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT ); //3 //3s delay
      if ( ret != CMSRET_SUCCESS ) {
	  	printf("Could not get lock!\n");
          return ret;
      	}
       ret = wlWriteMdmOne(idx);
      cmsLck_releaseLock();
 
   return ret;
}

CmsRet wlUnlockWriteMdmOne( int idx )
{
#ifdef WLWRTMDMDBG	
    printf("Unlock Writing WLAN Object[%d]\n", idx);
#endif

    return wlWriteMdmOne(idx);
}


CmsRet wlLockWriteMdmTr69Cfg( int idx )
{
    CmsRet ret;
	
#ifdef WLWRTMDMDBG	
    printf("Lock Writing Wlan Tr69Cfg Object[%d]\n", idx);
#endif

    ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT ); //3 //3s delay
    if ( ret != CMSRET_SUCCESS ) {
	  	printf("Could not get lock!\n");
	      return ret;
    }

    /*AvailableInterface.{i} Object  update?? */	
    ret = wlWriteTr69Cfg(idx+1, (wl_dsl_tr+idx), wl_dsl_tr_wepkey+idx, wl_dsl_tr_presharedkey+idx);
    cmsLck_releaseLock();
 
    return ret;
}

CmsRet wlUnlockWriteMdmTr69Cfg( int idx )
{
#ifdef WLWRTMDMDBG	
    printf("Unlock Writing Wlan Tr69Cfg Object[%d]\n", idx);
#endif

    return wlWriteTr69Cfg(idx+1, (wl_dsl_tr+idx), wl_dsl_tr_wepkey+idx, wl_dsl_tr_presharedkey+idx);
}


/*Following functions are used to write CFG data into Flash*/
CmsRet wlWriteMdmToFlash ( void )
{
    CmsRet ret;
	
    ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT ); //3 //3s delay
    if ( ret != CMSRET_SUCCESS ) {
 	printf("Could not get lock!\n");
	return ret;
    }
    ret = cmsMgm_saveConfigToFlash();
    cmsLck_releaseLock();
 
    return ret;
}


#ifdef BCMWAPI_WAI
/* This function needs to move to wlutil.c or somewhere, not Stay here*/
CmsRet wlSendCmsMsgToWlmngr(CmsEntityId sender_id, char *message_text)
{
   static void *local_msgHandle=NULL;
   CmsRet ret;

   ret = cmsMsg_init(sender_id, &local_msgHandle);

   if (ret == CMSRET_SUCCESS)
   {
      ret = wlSendCMsMsgToWlmngrByHandle(local_msgHandle, message_text);

      cmsMsg_cleanup(&local_msgHandle);
   }

   return ret;
}

CmsRet wlSendCmsMsgToWlmngrByHandle(void *msgHandle, char *message_text)
{
   static char buf[sizeof(CmsMsgHeader) + 32] = {0};
   CmsMsgHeader *msg = (CmsMsgHeader *)buf;

   char *ptr;
   int unit;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   ptr = nvram_get("wl_unit");

   if ((ptr != NULL) && (*ptr == '1'))
      unit = 1;
   else
      unit = 0; 

   sprintf((char *)(msg + 1), "%s:%d", message_text, unit + 1);

   msg->dataLength = 32;
   msg->type = CMS_MSG_WLAN_CHANGED;
   msg->src = cmsMsg_getHandleEid(msgHandle);
   msg->dst = EID_WLMNGR;
   msg->flags_event = 1;
   msg->flags_request = 0;

   ret = cmsMsg_send(msgHandle, msg);

#if defined(WLRDMDMDBG) || defined(WLWRTMDMDBG)
   if (ret != CMSRET_SUCCESS)
      printf("could not send BOUNDIFNAME_CHANGED msg to ssk, ret=%d\n", ret);
   else
      printf("message CMS_MSG_WLAN_CHANGED sent successfully\n");

   fflush(stdout);
#endif

   return ret;
 }
#endif


#ifdef DMP_X_BROADCOM_COM_PWRMNGT_1 /* aka SUPPORT_PWRMNGT */
/* This function is to read power management configuration */
CmsRet wlReadPowerManagement(PwrMngtObject *pPwrMngt)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   PwrMngtObject *pwrMngtObj = NULL;

   ret = cmsLck_acquireLockWithTimeout( LOCK_WAIT );
   if ( ret != CMSRET_SUCCESS ) {
         return ret;
   }

   if ((ret = cmsObj_get(MDMOID_PWR_MNGT, &iidStack, 0, (void **) &pwrMngtObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get pwrMngtObj, ret=%d", ret);
      cmsLck_releaseLock();
      return ret;
   }

   memcpy(pPwrMngt, pwrMngtObj, sizeof(PwrMngtObject));

   cmsObj_free((void **) &pwrMngtObj);
   
   cmsLck_releaseLock();
   return ret;
}
#endif
