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
:>
*/

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_msg.h"
#include "cms_boardioctl.h"
#include "cms_boardcmds.h"
#include "bcmnet.h"
#include "ssk.h"
#include "AdslMibDef.h"
#include "devctl_adsl.h"

#include "bcmnetlink.h"
#include <errno.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_addr.h>
#include <time.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <board.h>
#include <boardparms.h>

/* globals */
SINT32   shmId = UNINITIALIZED_SHM_ID;
SINT32   kernelMonitorFd = CMS_INVALID_FD;
UBOOL8     keepLooping=TRUE;
void *   msgHandle=NULL;
const CmsEntityId myEid=EID_SSK;
UBOOL8 useConfiguredLogLevel=TRUE;
UBOOL8 isLowMemorySystem=FALSE;

#ifdef SUPPORT_MOCA
SINT32 mocaMonitorFd = CMS_INVALID_FD;
#endif

#ifdef DMP_DSLDIAGNOSTICS_1
extern dslLoopDiag dslLoopDiagInfo;
#endif

#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1
/* This is flag initialied when ssk starts and updated when it is 
* changed by user.  It is also used in connstatus.c 
*/
UBOOL8 isAutoDetectionEnabled=FALSE;
#endif

extern void rut_doSystemAction(const char* from, char *cmd);
/* forward function declarations */
static SINT32 ssk_main(void);
static CmsRet ssk_init(void);
static CmsRet initKernelMonitorFd(void);
static void initLoggingFromConfig(void);
static UINT32 getSmdLogSettingsFromConfig(void);
static void processKernelMonitor(void);
static void processPingStateChanged(CmsMsgHeader *msg);
static void processPeriodicTask(void);
static void processGetDeviceInfo(CmsMsgHeader *msg);
static void processVendorConfigUpdate(CmsMsgHeader *msg);
#ifdef VOXXXLOAD
static void registerInterestInEvent(CmsMsgType msgType, UBOOL8 positive, void *msgData, UINT32 msgDataLen);
#endif
void processWlanBtnPressedMsg(void);
void ssk_restart_wlan(void);
#if defined (DMP_X_BROADCOM_COM_STANDBY_1)
extern void processPeriodicStandby(StandbyCfgObject *standbyCfgObj);
#endif
void updateLinkStatus(void);
void dumpLockInfo(void);

void usage(char *progName)
{
   printf("usage: %s [-v num]\n", progName);
   printf("       v: set verbosity, where num==0 is LOG_ERROR, 1 is LOG_NOTICE, all others is LOG_DEBUG\n");
}


SINT32 main(SINT32 argc, char *argv[])
{
   SINT32 c, logLevelNum;
   CmsLogLevel logLevel=DEFAULT_LOG_LEVEL;
   CmsRet ret;
   SINT32 exitCode=0;


   cmsLog_initWithName(myEid, argv[0]);

#ifdef CMS_STARTUP_DEBUG
   logLevel=LOG_LEVEL_DEBUG;
   cmsLog_setLevel(logLevel);
   useConfiguredLogLevel = FALSE;
#endif

   /* parse command line args */
   while ((c = getopt(argc, argv, "v:")) != -1)
   {
      switch(c)
      {
      case 'v':
         logLevelNum = atoi(optarg);
         if (logLevelNum == 0)
         {
            logLevel = LOG_LEVEL_ERR;
         }
         else if (logLevelNum == 1)
         {
            logLevel = LOG_LEVEL_NOTICE;
         }
         else
         {
            logLevel = LOG_LEVEL_DEBUG;
         }
         cmsLog_setLevel(logLevel);
         useConfiguredLogLevel = FALSE;
         break;

      default:
         usage(argv[0]);
         cmsLog_error("bad arguments, exit");
         exit(-1);
      }
   }

   if ((ret = ssk_init()) != CMSRET_SUCCESS)
   {
      cmsLog_error("initialization failed (%d), exit.", ret);
      return -1;
   }

   /*
    * Main body of code here.
    */
   exitCode = ssk_main();


   cmsLog_notice("exiting with code %d", exitCode);

   if (kernelMonitorFd != CMS_INVALID_FD)
   {
      close(kernelMonitorFd);
   }

#ifdef SUPPORT_MOCA
   cleanupMocaMonitorFd();
#endif


   cmsMsg_cleanup(&msgHandle);
   cmsLog_cleanup();

   freeWanLinkInfoList();
   cleanupLanLinkStatus();
   
   return exitCode;
}


/** This is the main loop of ssk.
 *
 * Just read messages and events and process them.
 * The DSL link-up, link-down detection should be done here and
 * not in smd.
 */
SINT32 ssk_main()
{
   CmsRet ret;
   CmsMsgHeader *msg=NULL;
   SINT32 exitCode=0;
   SINT32 commFd;
   SINT32 n, maxFd;
   fd_set readFdsMaster, readFds;
   fd_set errorFdsMaster, errorFds;
   struct timeval tv;
   CmsTimestamp nowTs, expireTs, deltaTs;
   CmsTimestamp lastPeriodicTaskTs, deltaPeriodicTaskTs;

   cmsTms_get(&nowTs);

   lastPeriodicTaskTs.sec = nowTs.sec;
   lastPeriodicTaskTs.nsec = nowTs.nsec;


   /* set up all the fd stuff for select */
   FD_ZERO(&readFdsMaster);
   FD_ZERO(&errorFdsMaster);

   cmsMsg_getEventHandle(msgHandle, &commFd);
   FD_SET(commFd, &readFdsMaster);
   maxFd = commFd;

   /* in DESKTOP_LINUX mode, kernelMonitorFd is not opened, so must check */
   if (kernelMonitorFd != CMS_INVALID_FD)
   {
      FD_SET(kernelMonitorFd, &readFdsMaster);
      maxFd = (maxFd > kernelMonitorFd) ? maxFd : kernelMonitorFd;
   }

#ifdef SUPPORT_MOCA
   if (mocaMonitorFd != CMS_INVALID_FD)
   {
      FD_SET(mocaMonitorFd, &readFdsMaster);
   }

   maxFd = (maxFd > mocaMonitorFd) ? maxFd : mocaMonitorFd;
#endif

   while (keepLooping)
   {
      readFds = readFdsMaster;
      errorFds = errorFdsMaster;

      cmsTms_get(&nowTs);

#ifdef DMP_DSLDIAGNOSTICS_1
      if (dslLoopDiagInfo.dslLoopDiagInProgress == TRUE)
      {
         /* we need to check the result every 2 seconds */
         expireTs.sec = nowTs.sec + 2;
         expireTs.nsec = nowTs.nsec;
         cmsLog_debug("diag in progress, go to fast monitor loop mode");
      }
      else
#endif
      {
         expireTs.sec = lastPeriodicTaskTs.sec + PERIODIC_TASK_INTERVAL;
         expireTs.nsec = lastPeriodicTaskTs.nsec;
      }


      /* calculate how long we should sleep */
      cmsTms_delta(&expireTs, &nowTs, &deltaTs);
      tv.tv_sec = deltaTs.sec;
      tv.tv_usec = deltaTs.nsec / NSECS_IN_USEC;

      /* If we spent too much time processing the message
      * nowTs will be greater than the expireTs and we will have
      * rollover.  Force timeout to 0 
      */
      if (tv.tv_sec > PERIODIC_TASK_INTERVAL * 2 || tv.tv_sec < 0)
      {
         cmsLog_debug("Rollover detected. tv.tv_sec =%d", tv.tv_sec);
         cmsLog_debug("nowTS=%u.%u lastPeriodicTs=%u.%u expireTS=%u.%u",
                      nowTs.sec, nowTs.nsec,
                      lastPeriodicTaskTs.sec, lastPeriodicTaskTs.nsec,
                      expireTs.sec, expireTs.nsec);
         tv.tv_sec = 0;
         tv.tv_usec = 0;
      }

      cmsLog_debug("calling select with tv=%d.%d", tv.tv_sec, tv.tv_usec);
      n = select(maxFd+1, &readFds, NULL, &errorFds, &tv);
      
      if (n < 0)
      {
         /* interrupted by signal or something, continue */
         continue;
      }

      if ( 0 == n )
      {
         cmsTms_get(&nowTs);
         cmsTms_delta(&nowTs,&lastPeriodicTaskTs,&deltaPeriodicTaskTs);
         cmsLog_debug("nowTS=%u.%u lastPeriodicTs=%u.%u deltaTS=%u.%u",
                      nowTs.sec, nowTs.nsec,
                      lastPeriodicTaskTs.sec, lastPeriodicTaskTs.nsec,
                      deltaPeriodicTaskTs.sec, deltaPeriodicTaskTs.nsec);
         if (deltaPeriodicTaskTs.sec >= PERIODIC_TASK_INTERVAL)
         {
            processPeriodicTask();

            /*
             * Update lastPeriodicTaskTs based on when it SHOULD have been called,
             * rather than when it WAS called.  This will eliminate drifting of when
             * the periodic tasks are run due to other scheduling/locking delays.
             */
            lastPeriodicTaskTs.sec += PERIODIC_TASK_INTERVAL;

            /*
             * On the other hand, if we are running hopelessly behind, then
             * don't try to catch up and start at the next periodic interval.
             */
            if (deltaPeriodicTaskTs.sec > (20* PERIODIC_TASK_INTERVAL))
            {
               cmsLog_error("hopelessly behind: "
                            "nowTs=%u.%u lastPeriodicTs=%u.%u deltaTS=%u.%u",
                            nowTs.sec, nowTs.nsec,
                            lastPeriodicTaskTs.sec, lastPeriodicTaskTs.nsec,
                            deltaPeriodicTaskTs.sec, deltaPeriodicTaskTs.nsec);
               cmsTms_get(&nowTs);
               lastPeriodicTaskTs.sec = nowTs.sec + PERIODIC_TASK_INTERVAL;
               lastPeriodicTaskTs.nsec = 0;
               cmsLog_error("catch up, jump to lastPeriodicTs=%u.%u",
                            lastPeriodicTaskTs.sec, lastPeriodicTaskTs.nsec);
            }
         }
      }

      if ((kernelMonitorFd != CMS_INVALID_FD) &&
          FD_ISSET(kernelMonitorFd, &readFds))
      {
         processKernelMonitor();
      }
      else if (FD_ISSET(commFd, &readFds))
      {
         if ((ret = cmsMsg_receive(msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            if (ret == CMSRET_DISCONNECTED)
            {
               if (cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS))
               {
                  /* smd is shutting down, I should quietly exit too */
                  return 0;
               }
               else
               {
                  cmsLog_error("detected exit of smd, ssk will also exit");
                  /* unexpectedly lost connection to smd, maybe I should
                   * reboot the system here to recover.... */
                  return -1;
               }
            }
            else
            {
               struct timespec sleep_ts={0, 100*NSECS_IN_MSEC};
               
               /* try to keep running after a 100ms pause */
               cmsLog_error("error during cmsMsg_receive, ret=%d", ret);
               nanosleep(&sleep_ts, NULL);
               continue;
            }
         }
         switch(msg->type)
         {
         case CMS_MSG_DHCPC_STATE_CHANGED:
            processDhcpcStateChanged(msg);
            break;

         case CMS_MSG_DHCPD_HOST_INFO:
            processLanHostInfoMsg(msg);
            break;

         case CMS_MSG_PPPOE_STATE_CHANGED:
            processPppStateChanged(msg);
            break;

         case CMS_MSG_GET_WAN_LINK_STATUS:
            processGetWanLinkStatus(msg);
            break;

         case CMS_MSG_GET_LAN_LINK_STATUS:
            processGetLanLinkStatus(msg);
            break;

         case CMS_MSG_GET_WAN_CONN_STATUS:
            processGetWanConnStatus(msg);
            break;
            
         case CMS_MSG_SET_LOG_LEVEL:
            cmsLog_setLevel(msg->wordData);
            if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
            {
               cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
            }
            break;

         case CMS_MSG_SET_LOG_DESTINATION:
            cmsLog_setDestination(msg->wordData);
            if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
            {
               cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
            }
            break;

         case CMS_MSG_TERMINATE:
            if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
            {
               cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
            }
            keepLooping = FALSE;
            break;

         case CMS_MSG_PING_STATE_CHANGED:
            processPingStateChanged(msg);
            break;

         case CMS_MSG_WATCH_WAN_CONNECTION:
            processWatchWanConnectionMsg(msg);
            break;

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
         case CMS_MSG_DHCP6C_STATE_CHANGED:
            processDhcp6cStateChanged(msg);
            break;

         case CMS_MSG_RASTATUS6_INFO:
            processRAStatus6Info(msg);
            break;
#endif

#ifdef VOXXXLOAD
         case CMS_MSG_SHUTDOWN_VODSL:
            processVodslShutdown();
            break;
            
         case CMS_MSG_START_VODSL:
            processVodslStart();
            break;    
            
         case CMS_MSG_RESTART_VODSL:
            processVodslRestart();
            break;
            
         case CMS_MSG_REBOOT_VODSL:
            processVodslReboot();
            break;
            
         case CMS_MSG_CONFIG_UPLOAD_COMPLETE:
            /* TODO: Unregister: SSK only cares about the first configuration */
            processConfigUploadComplete();
            break;
            
         case CMS_MSG_OMCI_VOIP_MIB_RESET:
            processMibReset();
            msg->wordData = 0;
            if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
            {
               cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
            }
            break;
            
         case CMS_MSG_INIT_VODSL:
            initializeVoice();
            break;
            
         case CMS_MSG_DEINIT_VODSL:
            deInitializeVoice();
            break;

         case CMS_MSG_DEFAULT_VODSL:
            defaultVoice();
            break;
#endif /* VOXXXLOAD */

#ifdef DMP_X_BROADCOM_COM_EPON_1
         case CMS_MSG_EPONMAC_BOOT_COMPLETE:
#ifdef VOXXXLOAD
            processEponMacBootInd();
#endif /* VOXXXLOAD */            
            break;
#endif /* DMP_X_BROADCOM_COM_EPON_1 */


#ifdef DMP_STORAGESERVICE_1
         case CMS_MSG_STORAGE_ADD_PHYSICAL_MEDIUM:
            processAddPhysicalMediumMsg(msg);
            break;
         case CMS_MSG_STORAGE_REMOVE_PHYSICAL_MEDIUM:
            processRemovePhysicalMediumMsg(msg);
            break;
         case CMS_MSG_STORAGE_ADD_LOGICAL_VOLUME:
            processAddLogicalVolumeMsg(msg);
            break;
         case CMS_MSG_STORAGE_REMOVE_LOGICAL_VOLUME:
            processRemoveLogicalVolumeMsg(msg);
            break;
#endif
         case CMS_MSG_GET_DEVICE_INFO:
            processGetDeviceInfo(msg);
            break;

#ifdef DMP_BRIDGING_1
         case CMS_MSG_DHCPD_DENY_VENDOR_ID:
            processDhcpdDenyVendorId(msg);
            break;
#endif
         case CMS_MSG_REQUEST_FOR_PPP_CHANGE:
            processRequestPppChange(msg);
            break;

#ifdef DMP_DSLDIAGNOSTICS_1
         case CMS_MSG_WATCH_DSL_LOOP_DIAG:
            processWatchDslLoopDiag(msg);
            break;
#endif 

#ifdef DMP_TIME_1
         case CMS_MSG_SNTP_STATE_CHANGED:
            processSntpStateChanged(msg);
            break;
#endif

         case CMS_MSG_VENDOR_CONFIG_UPDATE:
            processVendorConfigUpdate(msg);
            break;

#ifdef SUPPORT_DEBUG_TOOLS
         case CMS_MSG_MEM_DUMP_STATS:
            cmsMem_dumpMemStats();
            break;
#endif

#ifdef CMS_MEM_LEAK_TRACING
         case CMS_MSG_MEM_DUMP_TRACEALL:
            cmsMem_dumpTraceAll();
            break;

         case CMS_MSG_MEM_DUMP_TRACE50:
            cmsMem_dumpTrace50();
            break;

         case CMS_MSG_MEM_DUMP_TRACECLONES:
            cmsMem_dumpTraceClones();
            break;
#endif

#ifdef SUPPORT_MOCA
#ifdef BRCM_MOCA_DAEMON
         case CMS_MSG_MOCA_WRITE_LOF:
            processMoCAWriteLof(msg);
            break;

         case CMS_MSG_MOCA_READ_LOF:
            processMoCAReadLof(msg);
            break;

         case CMS_MSG_MOCA_WRITE_MRNONDEFSEQNUM:
            processMoCAWriteMRNonDefSeqNum(msg);
            break;

         case CMS_MSG_MOCA_READ_MRNONDEFSEQNUM:
            processMoCAReadMRNonDefSeqNum(msg);
            break;

         case CMS_MSG_MOCA_NOTIFICATION:
            processMoCANotification(msg);
            break;
#endif
#endif

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
         case CMS_MSG_GET_NTH_GPON_WAN_LINK_INFO:
            processGetNthGponWanLinkInfo(msg);
            break;

         case CMS_MSG_OMCI_GPON_WAN_SERVICE_STATUS_CHANGE:
            processGponWanServiceStatusChange(msg);
            break;
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
         case CMS_MSG_EPON_LINK_STATUS_CHANGED:
            processEponWanLinkChange(msg);
            break;
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
         case CMS_MSG_WLAN_LINK_STATUS_CHANGED:
            updateLinkStatus();
            break;
#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

#ifdef DMP_DEVICE2_HOMEPLUG_1
         case CMS_MSG_HOMEPLUG_LINK_STATUS_CHANGED:
            /* this is inefficient, since we know specifically that just the
             * homeplug link status has changed, but we are calling a general
             * function to check everything.  Hopefully link status does not
             * change too often.
             */
            updateLinkStatus();
            break;
#endif

         case CMS_MSG_INTERNAL_NOOP:
            /* just ignore this message.  It will get freed below. */
            break;

         default:
            cmsLog_error("cannot handle msg type 0x%x from %d (flags=0x%x)",
                         msg->type, msg->src, msg->flags);
            break;
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
      }
#ifdef SUPPORT_MOCA
#ifndef BRCM_MOCA_DAEMON
      else if (mocaMonitorFd != CMS_INVALID_FD && FD_ISSET(mocaMonitorFd, &readFds))
      {
         processMocaMonitor();
      }
#endif
#endif

#ifdef DMP_DSLDIAGNOSTICS_1
      if (dslLoopDiagInfo.dslLoopDiagInProgress == TRUE)
      {
         getDslLoopDiagResults();
      }      
#endif /* #ifdef DMP_DSLDIAGNOSTICS_1 */

   }

   return exitCode;
}


CmsRet ssk_init(void)
{
   CmsRet ret;
   CmsMsgHeader *msg=NULL;
   SINT32 sessionPid;
   UINT32 sdramSz;
   
   
   /*
    * Detach myself from the terminal so I don't get any control-c/sigint.
    * On the desktop, it is smd's job to catch control-c and exit.
    * When ssk detects that smd has exited, ssk will also exit.
    */
   if ((sessionPid = setsid()) == -1)
   {
      cmsLog_error("Could not detach from terminal");
   }
   else
   {
      cmsLog_debug("detached from terminal");
   }

   if ((ret = cmsMsg_initWithFlags(myEid, 0, &msgHandle)) != CMSRET_SUCCESS)
   {
      cmsLog_error("msg initialization failed, ret=%d", ret);
      return ret;
   }

   /*
    * ssk will get an event message on system boot.
    */
   if ((ret = cmsMsg_receive(msgHandle, &msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not receive initial msg, ret=%d", ret);
      return ret;
   }

   if (msg->type != CMS_MSG_SYSTEM_BOOT)
   {
      cmsLog_error("Unexpected initial msg, got 0x%x", msg->type);
      cmsMem_free(msg);
      return CMSRET_INTERNAL_ERROR;
   }


   /*
    * The first call to cmsMdm_init is done by ssk because MDM initialization
    * may cause RCL handler functions to send messages to smd.  So smd 
    * cannot call any MDM functions which would cause RCL or STL handler 
    * functions to send messagse to itself.
    */
   if ((ret = cmsMdm_initWithAcc(myEid, 0, msgHandle, &shmId)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_init failed, ret=%d", ret);
      cmsMem_free(msg);
      return ret;
   }

   cmsLog_debug("cmsMdm_init successful, shmId=%d", shmId);


   initLoggingFromConfig();


   /*
    * Reuse the event msg to send a CMS_MSG_MDM_INITIALIZED event msg
    * back to smd.  This will trigger stage 2 of the smd oal_launchOnBoot
    * process.
    */
   msg->type = CMS_MSG_MDM_INITIALIZED;
   msg->src = myEid;
   msg->dst = EID_SMD;
   msg->wordData = getSmdLogSettingsFromConfig();

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("MDM init event msg failed. ret=%d", ret);
   }


   /*
    * Initialize special socket to kernel for WAN link-up, link-down events.
    * The kernel notification mechanism uses the error channel of some existing fd.
    * See webmain in cfm/web.
    */
   if ((ret = initKernelMonitorFd()) != CMSRET_SUCCESS)
   {
      return ret;
   }


#ifdef SUPPORT_MOCA
   initMocaMonitorFd();
#endif

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   /*
    * Register interest for the CMS_MSG_DHCP6C_STATE_CHANGED event msg.
    * (reuse previous msg)
    * mwang: this needs to change, dhcpc should send event change directly
    * to ssk, ssk then sends event change to smd for general broadcast.
    */
   cmsLog_debug("registering interest for DHCP6C_STATE_CHANGED event msg");
   msg->type = CMS_MSG_REGISTER_EVENT_INTEREST;
   msg->flags_request = 1;
   msg->flags_response = 0;
   msg->flags_event = 0;
   msg->wordData = CMS_MSG_DHCP6C_STATE_CHANGED;
   if ((ret = cmsMsg_sendAndGetReply(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not register for DHCP6C_STATE_CHANGED");
   }
#endif

#ifdef DMP_TIME_1
   /*
    * Register interest for the CMS_MSG_SNTP_STATE_CHANGED event msg.
    * (reuse previous msg)
    */
   cmsLog_debug("registering interest for SNTP_STATE_CHANGED event msg");
   msg->type = CMS_MSG_REGISTER_EVENT_INTEREST;
   msg->flags_request = 1;
   msg->flags_response = 0;
   msg->flags_event = 0;
   msg->wordData = CMS_MSG_SNTP_STATE_CHANGED;
   if ((ret = cmsMsg_sendAndGetReply(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not register for SNTP_STATE_CHANGED");
   }
#endif /* DMP_TIME_1 */

   CMSMEM_FREE_BUF_AND_NULL_PTR(msg);


#ifdef VOXXXLOAD
   /*
    * Register interest for the CMS_MSG_CONFIG_UPLOAD_COMPLETE event msg.
    * (reuse previous msg)
    */
   registerInterestInEvent(CMS_MSG_CONFIG_UPLOAD_COMPLETE, TRUE, NULL, 0);

   /*
    * Initialize ssk_voice state 
    */
   initializeVoice();
#endif

   /*
    * See if we are running on a 8MB board.
    */
   sdramSz = devCtl_getSdramSize();
   if (sdramSz <= SZ_8MB)
   {
      cmsLog_debug("This is a low memory system, sdramSz=%d", sdramSz);
      isLowMemorySystem = TRUE;
   }


   /*
    * Keep trying if we get a TIMED_OUT error in case some other app is
    * holding the lock for an extra long time.
    */
   ret = CMSRET_TIMED_OUT;
   while (ret == CMSRET_TIMED_OUT)
   {
      if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get lock, ret=%d", ret);
         cmsLck_dumpInfo();
      }
      else
      {
         /*
          * Do an initial scan on the link status of all the WAN and LAN interfaces.
          * We have to scan the LAN interfaces because we might not get a kernel
          * event if the eth link comes up during kernel boot.  We have to scan
          * the WAN interface because we could be configured for ethWan and
          * may not get a kernel event if the ethWan link comes up during kernel boot.
          * This has the side effect of starting vodsl if the BoundIfName and
          * the link that is up matches.
          */
         initWanLinkInfo();
         checkAllWanLinkStatusLocked();
         checkLanLinkStatusLocked();
         cmsLck_releaseLock();
      }
   }

#ifdef DMP_STORAGESERVICE_1
   initStorageService();
#endif

   cmsLog_notice("done, ret=%d", ret);

   return ret;
}


static void initLoggingFromConfig()
{
   SskCfgObject *obj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   if ((ret = cmsObj_get(MDMOID_SSK_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of SSK_CFG object failed, ret=%d", ret);
   }
   else
   {
      if (useConfiguredLogLevel)
      {
         cmsLog_setLevel(cmsUtl_logLevelStringToEnum(obj->loggingLevel));
      }

      cmsLog_setDestination(cmsUtl_logDestinationStringToEnum(obj->loggingDestination));

      cmsObj_free((void **) &obj);
   }

   cmsLck_releaseLock();
}


/** Get the SMD log settings from the MDM so that it can be sent to smd.
 *
 * We do this so smd does not have to acquire the CMS lock to get its log
 * settings.  Smd should avoid getting the CMS lock to avoid deadlock
 * situations.
 */
UINT32 getSmdLogSettingsFromConfig()
{
   SmdCfgObject *obj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UINT32 logSettings=0;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return logSettings;
   }

   if ((ret = cmsObj_get(MDMOID_SMD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of SMD_CFG object failed, ret=%d", ret);
   }
   else
   {
      UINT32 level;
      UINT32 dest;

      level = cmsUtl_logLevelStringToEnum(obj->loggingLevel);
      dest = cmsUtl_logDestinationStringToEnum(obj->loggingDestination);
      logSettings = (dest & 0xffff) << 16;
      logSettings |= (level & 0xffff);

      cmsObj_free((void **) &obj);
   }

   cmsLck_releaseLock();

   return logSettings;
}



#ifdef VOXXXLOAD
static void registerInterestInEvent(CmsMsgType msgType, UBOOL8 positive, void *msgData, UINT32 msgDataLen)
{
   CmsMsgHeader *msg = NULL;
   char *data = NULL;
   void *msgBuf = NULL;
   char *action = (positive) ? "REGISTER" : "UNREGISTER";
   CmsRet ret;

   if (msgData != NULL && msgDataLen != 0)
   {
      /* for msg with user data */
      msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader) + msgDataLen, ALLOC_ZEROIZE);
   }
   else
   {
      msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
   }

   msg = (CmsMsgHeader *)msgBuf;

   /* fill in the msg header */
   msg->type = (positive) ? CMS_MSG_REGISTER_EVENT_INTEREST : CMS_MSG_UNREGISTER_EVENT_INTEREST;
   msg->src = myEid;
   msg->dst = EID_SMD;
   msg->flags_request = 1;
   msg->wordData = msgType;

   if (msgData != NULL && msgDataLen != 0)
   {
      data = (char *) (msg + 1);
      msg->dataLength = msgDataLen;
      memcpy(data, (char *)msgData, msgDataLen);
   }

   /* Only send the message. Do not wait for a reply in the context of the calling thread. 
   ** The reply will be received by the main message handling loop 
   */
   ret = cmsMsg_send(msgHandle, msg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("%s_EVENT_INTEREST for 0x%x failed, ret=%d", action, msgType, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(msgBuf);

   return;
}
#endif    // VOXXXLOAD

/** Kernel link status changed, call stl handler function for corresponding
 *  object which will cause new state to be read into the MDM.
 *
 */
void updateLinkStatus(void)
{
   CmsRet ret;   

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      /* just a kernel event, I guess we can try later. */
      return;
   }

   /* Check WAN */
   checkAllWanLinkStatusLocked();

   /* Check LAN */
   checkLanLinkStatusLocked();

   cmsLck_releaseLock();

   return;
}

/** Kernel traffic type mismatch , call handler function to determine if we
 *  need to reboot the system taking account of the traffic type mismatch.
 */
void processTrafficMismatchMessage(unsigned int msgData __attribute__((unused)))
{
   CmsRet ret;   

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      /* just a kernel event, I guess we can try later. */
      return;
   }

#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
#if defined (CHIP_6368)
       setWanDslTrafficTypeAndRestart () ;
#else
       setWanDslTrafficType () ;
#endif
#endif

   cmsLck_releaseLock();

   return;
}

#ifdef DMP_ADSLWAN_1
void processXdslCfgSaveMessage(void)
{
    long    dataLen;
    char    oidStr[] = { 95 };      /* kOidAdslPhyCfg */
    adslCfgProfile  adslCfg;
    CmsRet          cmsRet;
    WanDslIntfCfgObject *dslIntfCfg = NULL;
    InstanceIdStack         iidStack = EMPTY_INSTANCE_ID_STACK;
    
    dataLen = sizeof(adslCfgProfile);
    cmsRet = xdslCtl_GetObjectValue(0, oidStr, sizeof(oidStr), (char *)&adslCfg, &dataLen);
    
    if( cmsRet != (CmsRet) BCMADSL_STATUS_SUCCESS) {
        cmsLog_error("Could not get adsCfg, ret=%d", cmsRet);
        return;
    }
    
    if ((cmsRet = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS) {
        cmsLog_error("Could not get lock, ret=%d", cmsRet);
        cmsLck_dumpInfo();
        /* just a kernel event, I guess we can try later. */
        return;
    }
    
    cmsRet = cmsObj_getNext(MDMOID_WAN_DSL_INTF_CFG, &iidStack, (void **) &dslIntfCfg);
    if (cmsRet != CMSRET_SUCCESS) {
        cmsLck_releaseLock();
        cmsLog_error("Could not get DSL intf cfg, ret=%d", cmsRet);
        return;
    }
    
    xdslUtil_IntfCfgInit(&adslCfg, dslIntfCfg);
    
    cmsRet = cmsObj_set(dslIntfCfg, &iidStack);
    if (cmsRet != CMSRET_SUCCESS)
        cmsLog_error("Could not set DSL intf cfg, ret=%d", cmsRet);
    else
        cmsRet = cmsMgm_saveConfigToFlash();
    
    cmsObj_free((void **) &dslIntfCfg);
    
    cmsLck_releaseLock();
    
    if(cmsRet != CMSRET_SUCCESS)
        cmsLog_error("Writing  Xdsl Cfg to flash.failed!");
}
#endif
/* opens a netlink socket and intiliaze it to recieve messages from
 * kernel for protocol NETLINK_BRCM_MONITOR
 */

CmsRet initKernelMonitorFd()
{
   CmsRet ret=CMSRET_SUCCESS;

#ifndef DESKTOP_LINUX
   struct sockaddr_nl addr;

   if ((kernelMonitorFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_BRCM_MONITOR)) < 0)
   //if ((kernelMonitorFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_UNUSED)) < 0)
   {
      cmsLog_error("Could not open netlink socket for kernel monitor");
      return CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("kernelMonitorFd=%d", kernelMonitorFd);
   }

    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0;

    if (bind(kernelMonitorFd,(struct sockaddr *)&addr,sizeof(addr))<0)
    {
       cmsLog_error("Could not bind netlink socket for kernel monitor");
       close(kernelMonitorFd);
       kernelMonitorFd = CMS_INVALID_FD;
       return CMSRET_INTERNAL_ERROR;
    }

   /*send the pid of ssk to kernel,so that it can send events */

   ret = devCtl_boardIoctl(BOARD_IOCTL_SET_MONITOR_FD, 0, "", 0, getpid(), "");
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("registered fd %d with kernel monitor", kernelMonitorFd);
   }
   else
   {
      cmsLog_error("failed to register fd %d with kernel monitor, ret=%d", kernelMonitorFd, ret);
      close(kernelMonitorFd);
      kernelMonitorFd = CMS_INVALID_FD;
   }

#endif

   return ret;
}

/*  Richard, 2008/11/21, for CT. Hardware Button Monintor.  */
int boardIoctl(int board_ioctl, BOARD_IOCTL_ACTION action, char *string, int strLen, int offset, char *buf)
{
    BOARD_IOCTL_PARMS IoctlParms;
    int boardFd = 0;

    boardFd = open("/dev/brcmboard", O_RDWR);
    if ( boardFd != -1 ) {
        IoctlParms.string = string;
        IoctlParms.strLen = strLen;
        IoctlParms.offset = offset;
        IoctlParms.action = action;
        IoctlParms.buf    = buf;
        ioctl(boardFd, board_ioctl, &IoctlParms);
        close(boardFd);
        boardFd = IoctlParms.result;
    } else
        printf("Unable to open device /dev/brcmboard.\n");

    return boardFd;
}

int sysGetWlanButtonRequest()
{
   return(boardIoctl(BOARD_IOCTL_QUERY_WLAN_BUTTON_REQUEST, 0, "", 0, 0, ""));
}

void doWlanButtonProcess(unsigned int button )
{
   CmsMsgHeader msg;
    CmsRet ret = CMSRET_SUCCESS;

    memset(&msg, 0, sizeof(CmsMsgHeader));
    msg.src = EID_SSK;
    msg.type = CMS_MSG_WLAN_BTN_PUSHED;
    msg.wordData = button;
    msg.dst = EID_HTTPD;
    if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
    {
        cmsLog_error("sending msg to RequestId %d failed, ret=%d", msg.dst,ret);
    }
    else
    {
        cmsLog_debug("sending msg to RequestId %d done, ret=%d", msg.dst,ret);
    }

#ifdef CT_MIDDLEWARE
      msg.dst = EID_TR69C;
      switch(wlButtonState)
      {
          case WLAN_PRESS_SHORT:
              msg.type = CMS_MSG_TR69_CTMDW_WLANSHORT;
              break;
              
          case WLAN_PRESS_LONG:
              msg.type = CMS_MSG_TR69_CTMDW_WLANLONG;
              break;
              
          case WLAN_PRESS_DOUBLE:
              msg.type = CMS_MSG_TR69_CTMDW_WLANDOUBLE;
              break;
      }
      if ((ret = cmsMsg_send(msgHandle, &msg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("sending msg to RequestId %d failed, ret=%d", msg.dst,ret);
      }
      else
      {
         cmsLog_debug("sending msg to RequestId %d done, ret=%d", msg.dst,ret);
      }
#endif
   
}

/* Actions for wps button */
static int sysGetWPSButtonRequest(void)
{
   return(boardIoctl(BOARD_IOCTL_QUERY_WPS_BUTTON_REQUEST, 0, "", 0, 0, ""));
}

void checkWpsButtonStats(void)
{
   int wlButtonState;
   CmsMsgHeader msg;

   memset(&msg, 0, sizeof(CmsMsgHeader));

   wlButtonState = sysGetWPSButtonRequest();
   if( (wlButtonState > 0) && (wlButtonState < 4) )
   {
      msg.src = EID_SSK;
      msg.dst = EID_TR69C;
      switch(wlButtonState)
      {
          case WLAN_PRESS_SHORT:
              rut_doSystemAction(__func__,"echo 1 > /var/wl_wps_btn_pressed"); //write file for wps usage
             /*
              * Richard, if wps button is pushed, eapd and nas must be enabled.       
              */
              rut_doSystemAction(__func__,"killall -9 eapd 2>/dev/null");
              rut_doSystemAction(__func__,"killall -9 nas 2>/dev/null");
              rut_doSystemAction(__func__,"/bin/eapd &");
              rut_doSystemAction(__func__,"/bin/nas &");
              break;

          case WLAN_PRESS_LONG:
              break;

          case WLAN_PRESS_DOUBLE:
              break;
      }

   }
}

void checkRestoreButtonStats(void)
{

}

/** Receive and process messages sent from kernel on netlink sockets
 *
 */
void processKernelMonitor(void)
{
   int recvLen;
   char buf[4096];
   struct iovec iov = { buf, sizeof buf };
   struct sockaddr_nl nl_srcAddr;
   struct msghdr msg ;
   struct nlmsghdr *nl_msgHdr;
   unsigned int nl_msgData ;

   memset(&msg,0,  sizeof(struct msghdr));

   msg.msg_name = (void*)&nl_srcAddr;
   msg.msg_namelen = sizeof(nl_srcAddr);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1 ;


   cmsLog_debug("Enter\n");

   recvLen = recvmsg(kernelMonitorFd, &msg, 0);

   if(recvLen < 0)
   {
      if (errno == EWOULDBLOCK || errno == EAGAIN)
         return ;

      /* Anything else is an error */
      cmsLog_error("read_netlink: Error recvmsg: %d\n", recvLen);
      perror("read_netlink: Error: ");
      return ;
   }

   if(recvLen == 0)
   {
      cmsLog_error("read_netlink: EOF\n");
   }

   /* There can be  more than one message per recvmsg */
   for(nl_msgHdr = (struct nlmsghdr *) buf; NLMSG_OK (nl_msgHdr, (unsigned int)recvLen); 
         nl_msgHdr = NLMSG_NEXT (nl_msgHdr, recvLen))
   {
      /* Finish reading */
      if (nl_msgHdr->nlmsg_type == NLMSG_DONE)
         return ;

      /* Message is some kind of error */
      if (nl_msgHdr->nlmsg_type == NLMSG_ERROR)
      {
         cmsLog_error("read_netlink: Message is an error \n");
         return ; // Error
      }

      /*Currently we expect messages only from kernel, make sure
       * the message is from kernel 
       */
      if(nl_msgHdr->nlmsg_pid !=0)
      {
         cmsLog_error("netlink message source(%d)is not kernel",nl_msgHdr->nlmsg_pid);
         return;
      }

      /* Call message handler */
      switch (nl_msgHdr->nlmsg_type)
      {
         case MSG_NETLINK_BRCM_WAKEUP_MONITOR_TASK:
         case MSG_NETLINK_BRCM_LINK_STATUS_CHANGED:
            /*process the message */
            cmsLog_debug("received LINK_STATUS_CHANGED message\n"); 
            updateLinkStatus();
            break;
         case MSG_NETLINK_BRCM_LINK_TRAFFIC_TYPE_MISMATCH   :
            /*process the message */
            cmsLog_debug("received LINK_TRAFFIC_TYPE_MISMATCH message\n");
            memcpy ((char *) &nl_msgData, NLMSG_DATA(nl_msgHdr), sizeof (nl_msgData)) ;
            processTrafficMismatchMessage(nl_msgData) ;
            break;
#ifdef DMP_ADSLWAN_1
         case MSG_NETLINK_BRCM_SAVE_DSL_CFG:
            cmsLog_debug("received SAVE_DSL_CFG  message\n");
            processXdslCfgSaveMessage();
            break;
         case MSG_NETLINK_BRCM_CALLBACK_DSL_DRV:
            cmsLog_debug("received CALLBACK_DSL_DRV  message\n");
            xdslCtl_CallBackDrv(0);
            break;
         case MSG_NETLINK_WLAN2G_BUTTON_PRESSED:
            printf ("received MSG_NETLINK_WLAN2G_BUTTON_PRESSED message\n"); 
            doWlanButtonProcess(WLAN2G_BUTTON);         
            break;
         case MSG_NETLINK_WLAN_WPS_BUTTON_PRESSED:   
            printf ("received MSG_NETLINK_WLAN_WPS_BUTTON_PRESSED message\n"); 
            doWlanButtonProcess(WLAN_WPS_BUTTON);         
            break;
         case MSG_NETLINK_WLAN11AC_BUTTON_PRESSED:
            printf ("received MSG_NETLINK_WLAN11AC_BUTTON_PRESSED message\n"); 
             doWlanButtonProcess(WLAN11AC_BUTTON);         
            break;
         case MSG_NETLINK_WLAN_BUTTON_PRESSED:
            printf ("received MSG_NETLINK_WLAN_BUTTON_PRESSED message\n"); 
            doWlanButtonProcess(WLAN_BUTTON);
            break;
#endif
//         case MSG_NETLINK_BUTTON_PRESSED:
//            printf ("received BUTTON_PRESSED message\n\n"); 
//            processWlanBtnPressedMsg();
//            break;
         default:
            cmsLog_error(" Unknown netlink nlmsg_type %d\n",
                  nl_msgHdr->nlmsg_type);
            break;
      }
   }

   cmsLog_debug("Exit\n");
   return;
}

void processPingStateChanged(CmsMsgHeader *msg)
{
   PingDataMsgBody *pingInfo = (PingDataMsgBody *) (msg + 1);
   IPPingDiagObject *ipPingObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("dataLength=%d state=%s",msg->dataLength, pingInfo->diagnosticsState);
   if (pingInfo->interface)
   {
      cmsLog_debug("interface %s",pingInfo->interface);
   }
   if (pingInfo->host)
   {
      cmsLog_debug("host %s",pingInfo->host);
   }
   cmsLog_debug("success %d, fail %d, avg/min/max %d/%d/%d requestId %d",pingInfo->successCount,
                pingInfo->failureCount,pingInfo->averageResponseTime,pingInfo->minimumResponseTime,
                pingInfo->maximumResponseTime,(int)pingInfo->requesterId);
      
      
   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   cmsLog_debug("calling cmsObjGet");

   if ((ret = cmsObj_get(MDMOID_IP_PING_DIAG, &iidStack, 0, (void **) &ipPingObj)) == CMSRET_SUCCESS)
   {
      cmsLog_debug("cmsObj_get, MDMOID_IP_PING_DIAG success");

      CMSMEM_FREE_BUF_AND_NULL_PTR(ipPingObj->diagnosticsState);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipPingObj->host);
      ipPingObj->diagnosticsState = cmsMem_strdup(pingInfo->diagnosticsState);
      ipPingObj->host = cmsMem_strdup(pingInfo->host);
      ipPingObj->successCount = pingInfo->successCount;
      ipPingObj->failureCount = pingInfo->failureCount;
      ipPingObj->averageResponseTime = pingInfo->averageResponseTime;
      ipPingObj->maximumResponseTime = pingInfo->maximumResponseTime;
      ipPingObj->minimumResponseTime = pingInfo->minimumResponseTime;

      if ((ret = cmsObj_set(ipPingObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("set of ipPingObj failed, ret=%d", ret);
      }
      else
      {
         cmsLog_debug("set ipPingObj OK, successCount %d, failCount %d",
                      ipPingObj->successCount,ipPingObj->failureCount);
      }     
      
      cmsObj_free((void **) &ipPingObj);
   } 
   else
   {
      cmsLog_debug("cmsObj_get, MDMOID_IP_PING_DIAG ret %d",ret);
   }

   cmsLck_releaseLock();

   return;
}




void sendStatusMsgToSmd(CmsMsgType msgType, const char *ifName)
{
   CmsMsgHeader *msgHdr;
   char *dataPtr;
   UINT32 dataLen=0;
   CmsRet ret;

   cmsLog_debug("sending status msg 0x%x", msgType);

   if (ifName != NULL)
   {
      dataLen = strlen(ifName)+1;
      cmsLog_debug("ifName=%s", ifName);
   }



   msgHdr = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader) + dataLen, ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      cmsLog_error("message header allocation failed, len of ifName=%d", strlen(ifName));
      return;
   }

   msgHdr->src = EID_SSK;
   msgHdr->dst = EID_SMD;
   msgHdr->type = msgType;
   msgHdr->flags_event = 1;
   msgHdr->dataLength = dataLen;
   
   if (ifName != NULL)
   {
      dataPtr = (char *) (msgHdr+1);
      strcpy(dataPtr, ifName);
   }

   if ((ret = cmsMsg_send(msgHandle, msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send event msg 0x%x to smd, ret=%d", msgType, ret);
   }

   cmsMem_free(msgHdr);   

   return;
}

void expirePortMappings(void)
{

   InstanceIdStack parentIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack sonIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack tmpIidStack = EMPTY_INSTANCE_ID_STACK;

   CmsRet ret = CMSRET_SUCCESS;

   CmsTimestamp nowTs;

   cmsTms_get(&nowTs);

   WanIpConnPortmappingObject *ipPort_mapping = NULL;
   WanPppConnPortmappingObject *pppPort_mapping = NULL;

   int tobe_deleted = 0;

 /***** expire MDMOID_WAN_IP_CONN_PORTMAPPING ***/

/* get and update the timeout of all the existing port mapping entries
 * when parentIidstack is sent as EMPTY then all the instances for an object
 * can be reterived.
 */ 
   while (cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN_PORTMAPPING, &parentIidStack, &sonIidStack,(void **)&ipPort_mapping) == CMSRET_SUCCESS)
   {

      if(tobe_deleted)
      {
         if((ret=cmsObj_deleteInstance(MDMOID_WAN_IP_CONN_PORTMAPPING, &tmpIidStack)) !=CMSRET_SUCCESS)
         {
      	    cmsLog_error("could not delete new virtual server entry, ret=%d", ret);
         }
         tobe_deleted = 0;
      }


      if((0 != ipPort_mapping->portMappingLeaseDuration))/*skip static entries */
      {
	 
	 if(nowTs.sec < ipPort_mapping->X_BROADCOM_COM_ExpiryTime) 
	 {
           /*update time out */ 		
            ipPort_mapping->portMappingLeaseDuration = ipPort_mapping->X_BROADCOM_COM_ExpiryTime - nowTs.sec ;
            cmsObj_set(ipPort_mapping, &sonIidStack);
         }
	 else
	 {
	  /* entry expired, mark for removal
 	   * we cant delete this entry immediately as we need to get
 	   * next entry based on sonIidstack,so we just mark here and it will
 	   * be removed in next iteration
 	   */
	    tmpIidStack = sonIidStack;
            tobe_deleted = 1;	
	 }	
      }  
      /*free port_mapping as a copy was returned when it was retreived*/
      cmsObj_free((void **) &ipPort_mapping);
   }                    

   if(tobe_deleted)
   {
      if((ret=cmsObj_deleteInstance(MDMOID_WAN_IP_CONN_PORTMAPPING, &tmpIidStack)) !=CMSRET_SUCCESS)
      {
      	 cmsLog_error("could not delete new virtual server entry, ret=%d", ret);

      }
      tobe_deleted = 0;
   }

 /***** expire MDMOID_WAN_PPP_CONN_PORTMAPPING ***/

/* get and update the timeout of all the existing port mapping entries
 * when parentIidstack is sent as EMPTY then all the instances for an object
 * can be reterived.
 */ 
   tobe_deleted = 0;

   while (cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN_PORTMAPPING, &parentIidStack, &sonIidStack,(void **)&pppPort_mapping) == CMSRET_SUCCESS)
   {

      if(tobe_deleted)
      {
         if((ret=cmsObj_deleteInstance(MDMOID_WAN_PPP_CONN_PORTMAPPING, &tmpIidStack)) !=CMSRET_SUCCESS)
         {
      	    cmsLog_error("could not delete new virtual server entry, ret=%d", ret);

         }
         tobe_deleted = 0;
      }


      if((0 != pppPort_mapping->portMappingLeaseDuration))/*skip static entries */
      {
	 
	 if(nowTs.sec < pppPort_mapping->X_BROADCOM_COM_ExpiryTime) 
	 {
           /*update time out */ 		
            pppPort_mapping->portMappingLeaseDuration = pppPort_mapping->X_BROADCOM_COM_ExpiryTime - nowTs.sec ;
            cmsObj_set(pppPort_mapping, &sonIidStack);
         }
	 else
	 {
	  /* entry expired, mark for removal
 	   * we cant delete this entry immediately as we need to get
 	   * next entry based on sonIidstack,so we just mark here and it will
 	   * be removed in next iteration
 	   */
	    tmpIidStack = sonIidStack;
            tobe_deleted = 1;	
	 }	
      }  
      /*free port_mapping as a copy was returned when it was retreived*/
      cmsObj_free((void **) &pppPort_mapping);
   }                    

   if(tobe_deleted)
   {
      if((ret=cmsObj_deleteInstance(MDMOID_WAN_PPP_CONN_PORTMAPPING, &tmpIidStack)) !=CMSRET_SUCCESS)
      {
      	 cmsLog_error("could not delete new virtual server entry, ret=%d", ret);

      }
      tobe_deleted = 0;
   }


}

/* For period task you need to handle in ssk, please put it into
 * this routine
 */
void processPeriodicTask()
{
   IGDDeviceInfoObject *obj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1
   isAutoDetectionEnabled = dalAutoDetect_isAutoDetectEnabled();
   if (isAutoDetectionEnabled)
   {
      processAutoDetectTask();
   }      
#endif

   /*
    * The reason we call cmsObj_get on the DEVICE_INFO object is to
    * is force the stl_igdDeviceInfoObject() to update the upTime field
    * in the DeviceInfo object in the MDM.
    * If ssk does not periodically call cmsObj_get on this object,
    * then the field would never be updated and when a user types
    * dumpMdm, the uptime will be a very old one.  If this feature
    * is not important to your product, you can simply delete this
    * call to cmsObj_get.
    * A getParameterValues from tr69c will always trigger an update
    * to the upTime field and return the latest value.
    */
   cmsLog_debug("Get IGD.DeviceInfo");
   if ((ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of DEVICE_INFO object failed, ret=%d", ret);
   }
   else
   {
      cmsObj_free((void **) &obj);
   }

   /*update lease Times of portmapping entries */	
   expirePortMappings();

#ifdef DMP_X_BROADCOM_COM_STANDBY_1
   processPeriodicStandby(NULL);
#endif

   cmsLck_releaseLock();
}


void processGetDeviceInfo(CmsMsgHeader *msg)
{
   char buf[sizeof(CmsMsgHeader) + sizeof(GetDeviceInfoMsgBody)]={0};
   CmsMsgHeader *replyMsg=(CmsMsgHeader *) buf;
   GetDeviceInfoMsgBody *body = (GetDeviceInfoMsgBody*) (replyMsg+1);
   CmsRet ret;
   IGDDeviceInfoObject *deviceInfoObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   replyMsg->type = msg->type;
   replyMsg->src = EID_SSK;
   replyMsg->dst = msg->src;
   replyMsg->flags_request = 0;
   replyMsg->flags_response = 1;
   replyMsg->dataLength = sizeof(GetDeviceInfoMsgBody);

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   if ((ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &deviceInfoObj)) == CMSRET_SUCCESS)
   {
      cmsLog_debug("cmsObj_get, MDMOID_IGD_DEVICE_INFO success: oui %s, serialNumber %s productClass %s",
                   deviceInfoObj->manufacturerOUI,deviceInfoObj->serialNumber,deviceInfoObj->productClass);

      strcpy(body->oui,deviceInfoObj->manufacturerOUI);
      strcpy(body->serialNum,deviceInfoObj->serialNumber);
      strcpy(body->productClass,deviceInfoObj->productClass);
      cmsObj_free((void **) &deviceInfoObj);
   } 
   else
   {
      cmsLog_debug("cmsObj_get, MDMOID_IGD_DEVICE_INFO ret %d",ret);
   }
   cmsLck_releaseLock();

   if ((ret = cmsMsg_send(msgHandle, replyMsg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("sending msg to %d failed, ret=%d", msg->dst,ret);
   }
   else
   {
      cmsLog_debug("sending msg to %d done, ret=%d", msg->dst,ret);
   }
} /* processGetDeviceInfo */

#ifdef DMP_TIME_1
void processSntpStateChanged(CmsMsgHeader *msg)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   TimeServerCfgObject *timeObj=NULL;
   IGDDeviceInfoObject *deviceObj=NULL;
   CmsRet ret;
   char timeBuf[BUFLEN_64];
   int state = msg->wordData;

   cmsLog_debug("state=%d",state);
      
   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   cmsLog_debug("calling cmsObjGet");
   if (state == SNTP_STATE_SYNCHRONIZED)
   {
      if ((ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &deviceObj)) == CMSRET_SUCCESS)
      {
         /* only updated once, the first time NTP time is set */
         if (cmsUtl_strcmp(deviceObj->firstUseDate,UNKNOWN_DATETIME_STRING) == 0)
         {
            cmsTms_getXSIDateTime(0,timeBuf,sizeof(timeBuf));
            CMSMEM_FREE_BUF_AND_NULL_PTR(deviceObj->firstUseDate);
            deviceObj->firstUseDate = cmsMem_strdup(timeBuf);
            if ((ret = cmsObj_set(deviceObj, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("fail to set deviceObj, ret %d",ret);
            }
            else
            {
               /* need to save to flash */
               cmsMgm_saveConfigToFlash();
            }
         }
         cmsObj_free((void **) &deviceObj);
      }
   } /* synchronized */

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_get(MDMOID_TIME_SERVER_CFG, &iidStack, 0, (void **) &timeObj)) == CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(timeObj->status);
      switch (state)
      {
      case SNTP_STATE_UNSYNCHRONIZED:
         timeObj->status = cmsMem_strdup(MDMVS_UNSYNCHRONIZED);
         break;
      case SNTP_STATE_DISABLED:
         timeObj->status = cmsMem_strdup(MDMVS_DISABLED);
         break;
      case SNTP_STATE_SYNCHRONIZED:
         timeObj->status = cmsMem_strdup(MDMVS_SYNCHRONIZED);
         break;
      case SNTP_STATE_FAIL_TO_SYNCHRONIZE:
         timeObj->status = cmsMem_strdup(MDMVS_ERROR_FAILEDTOSYNCHRONIZED);
         break;
      default:
         timeObj->status = cmsMem_strdup(MDMVS_ERROR);
         break;
      }

      if ((ret = cmsObj_set(timeObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("set of timeObj failed, ret=%d", ret);
      }
      cmsObj_free((void **) &timeObj);
   } 
   cmsLck_releaseLock();
}
#endif /* DMP_TIME_1 */

void processVendorConfigUpdate(CmsMsgHeader *msg)
{
   VendorConfigFileObject *pVendorConfigObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   IGDDeviceInfoObject *pDevInfo;
   int found = 0;
   char fileName[BUFLEN_64];
   int instance=0;
   int numberOfVendorFiles;
   vendorConfigUpdateMsgBody *vendorConfig = (vendorConfigUpdateMsgBody *) (msg + 1);
   CmsMsgHeader replyMsg = EMPTY_MSG_HEADER;

   replyMsg.type = msg->type;
   replyMsg.src = EID_SSK;
   replyMsg.dst = msg->src;
   replyMsg.flags_request = 0;
   replyMsg.flags_response = 1;

   ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT);
   if (ret != CMSRET_SUCCESS)
   {
      return;
   }
   if (cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &pDevInfo) != CMSRET_SUCCESS)
   {
      cmsLck_releaseLock();
      return;
   }
   numberOfVendorFiles = pDevInfo->vendorConfigFileNumberOfEntries;

   cmsLog_debug("numberOfVendorFiles %d",numberOfVendorFiles);

   cmsObj_free((void **) &pDevInfo);

   /* first look for instance of existing file */
   if (numberOfVendorFiles > 0)
   {
      if (vendorConfig->name[0] != '\0')
      {
         INIT_INSTANCE_ID_STACK(&iidStack);
         while (!found && (cmsObj_getNext(MDMOID_VENDOR_CONFIG_FILE,&iidStack, (void **) &pVendorConfigObj)) == CMSRET_SUCCESS)
         {
            if ((cmsUtl_strcmp(pVendorConfigObj->name,vendorConfig->name) == 0))
            {
               cmsLog_debug("Same entry found, update vendor config entry");
               CMSMEM_REPLACE_STRING(pVendorConfigObj->version,vendorConfig->version);
               CMSMEM_REPLACE_STRING(pVendorConfigObj->date,vendorConfig->date);
               CMSMEM_REPLACE_STRING(pVendorConfigObj->description,vendorConfig->description);
               found = 1;
               cmsObj_set(pVendorConfigObj,&iidStack);
            }
            cmsObj_free((void **) &pVendorConfigObj);
         } /* while */
      } /* vendorConfig->name is not empty */
   }
   if (!found)
   {
      if (numberOfVendorFiles == MAX_NUMBER_OF_VENDOR_CONFIG_RECORD) 
      {
         INIT_INSTANCE_ID_STACK(&iidStack);
         while ((cmsObj_getNext(MDMOID_VENDOR_CONFIG_FILE,&iidStack, (void **) &pVendorConfigObj)) == CMSRET_SUCCESS)
         {
            /* if the list is full, we delete the first instance, and add another */
            cmsLog_debug("numberOfVendorFiles has reached %d. Deleteing vendor config entry",numberOfVendorFiles);
            cmsObj_deleteInstance(MDMOID_VENDOR_CONFIG_FILE, &iidStack);               
            cmsObj_free((void **) &pVendorConfigObj);
            break;
         }
      }
      /* add the instance */
      INIT_INSTANCE_ID_STACK(&iidStack);
      if ((ret = cmsObj_addInstance(MDMOID_VENDOR_CONFIG_FILE,&iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("add instance of vendor config file returned %d", ret);
      }
      else
      {
         cmsObj_get(MDMOID_VENDOR_CONFIG_FILE, &iidStack, 0, (void **) &pVendorConfigObj);
         if (vendorConfig->name[0] == '\0')
         {
            strcpy(fileName,"ConfigFile");
            instance = iidStack.instance[iidStack.currentDepth-1];
            sprintf(pVendorConfigObj->name,"%s%d",fileName,instance);
         }
         else
         {
            CMSMEM_REPLACE_STRING(pVendorConfigObj->name,vendorConfig->name);
         }
         CMSMEM_REPLACE_STRING(pVendorConfigObj->version,vendorConfig->version);
         CMSMEM_REPLACE_STRING(pVendorConfigObj->date,vendorConfig->date);
         CMSMEM_REPLACE_STRING(pVendorConfigObj->description,vendorConfig->description);

         if ((ret = cmsObj_set(pVendorConfigObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("set of vendor config object failed, ret=%d", ret);
         }
         else
         {
            cmsLog_debug("vendor config entry added");
         }
         cmsObj_free((void **) &pVendorConfigObj);
      } /* add instance, ok */
   } /* !found */
   cmsMgm_saveConfigToFlash();
   cmsLck_releaseLock();

   if ((ret = cmsMsg_send(msgHandle, &replyMsg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("msg send failed, ret=%d",ret);
   }
   
} /* processVendorConfigUpdate */

void ssk_restart_wlan(void)
{
   static char buf[sizeof(CmsMsgHeader) + 32]={0};
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   sprintf((char *)(msg + 1), "Modify:%d", 1);

   msg->dataLength = 32;
   msg->type = CMS_MSG_WLAN_CHANGED;
   msg->src = EID_HTTPD; /* do not change it to EID_SSK,tks. */
   msg->dst = EID_WLMNGR;
   msg->flags_event = 1;
   msg->flags_request = 0;

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
      printf("could not send CMS_MSG_WLAN_CHANGED msg to wlmngr, ret=%d", ret);
   else
      printf("message CMS_MSG_WLAN_CHANGED sent successfully");
   return;
 }

void processWlanBtnPressedMsg(void)
{
        CmsRet ret;
        _WlBaseCfgObject *wlBaseCfgObj = NULL;
        InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
        
        if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
        {
            cmsLog_error("could not get lock, ret=%d", ret);
            return;
        }

        if ((ret = cmsObj_getNext(MDMOID_WL_BASE_CFG, &iidStack, (void **) &wlBaseCfgObj)) == CMSRET_SUCCESS)
        {
            if(TRUE == wlBaseCfgObj->wlEnbl)
            {
                wlBaseCfgObj->wlEnbl = FALSE;
            }
            else
            {
                wlBaseCfgObj->wlEnbl = TRUE;
            }
            cmsObj_set(wlBaseCfgObj, &iidStack);
            cmsObj_free((void **) &wlBaseCfgObj);
            ssk_restart_wlan();
            cmsMgm_saveConfigToFlash();
        } 
        else
        {
            cmsLog_debug("cmsObj_get, MDMOID_WL_BASE_CFG ret %d",ret);
        }
        cmsLck_releaseLock();
    return;
}
