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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "cms_dal.h"
#include "cms_cli.h"

#include <bcmnvram.h>
#include "wlapi.h"
#include "wlmngr.h"
#include "wldsltr.h"
#include "wlsyscall.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>


static void *msgHandle=NULL;
int wl_cnt =0;

#ifdef BUILD_EAPD
 void wlevt_main( void );
#endif
 
// #define WLDAEMON_DBG

static void wlan_restart(unsigned char dataSync, int idx)
{

    /* 
    wl_cnt: a global variable in wldaemon.c that indicates the number of
     adaptors in the system.  This is obtained by calling wlgetintfNo()
     below.  The function wlgetintfNo() uses IOCTL's to find out this info.
    */
    
    if ((idx < 0) || (idx >= wl_cnt))
        idx = 0;

    if (idx < wl_cnt) {
        wlmngr_initCfg(dataSync, idx);
        wlmngr_init(idx);
    }
    wlmngr_update_assoc_list(idx);
	
#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
   wlmngr_initWanWifi();
#endif
}

int main(int argc, char **argv)
{
    SINT32 shmId=UNINITIALIZED_SHM_ID;
    CmsRet ret;
    CmsMsgHeader *msg;
    CmsEntityId msgSrc= EID_WLMNGR;
    SINT32 c;
    char cmd[32]={0};
    int idx = -1,i=0; 
    int not_failure = 1;

    printf("WLmngr Daemon is running\n");

    signal(SIGINT, SIG_IGN);

    while ((c = getopt(argc, argv, "v:m:")) != -1)
    {
        switch(c)
        {
            case 'm':
                shmId = atoi(optarg);
                printf("WLMNGR: optarg=%s shmId=%d \n", optarg, shmId );
                break;

            default:
                break;
        }
    }



    /*Fetch the Adapter number */
    wl_cnt = wlgetintfNo();

    is_smp_system = wlmngr_detectSMP();

#ifdef WL_WLMNGR_DBG
    printf("%s@%d wl_cnt=%d\n", __FUNCTION__, __LINE__, wl_cnt);
#endif

    if ( wldsltr_alloc( wl_cnt ) ) 
        not_failure = 0;

    if (wlmngr_alloc( wl_cnt) )
        not_failure = 0;

#ifdef BUILD_EAPD
     /* Start Event handling Daemon */
    bcmSystem("/bin/wlevt");
#endif
    sleep(1); 

    /*
     * Do CMS initialization after wlevt is spawned so that wlevt does
     * not inherit any file descriptors opened by CMS init code.
     */
    cmsLog_initWithName(EID_WLMNGR, argv[0]);

    if ((ret = cmsMsg_initWithFlags(EID_WLMNGR, 0, &msgHandle)) != CMSRET_SUCCESS)
    {
        cmsLog_error("could not initialize msg, ret=%d", ret);
        cmsLog_cleanup();
        exit(-1);
    }

    if ((ret = cmsMdm_initWithAcc(EID_WLMNGR, 0, msgHandle, &shmId)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not initialize mdm, ret=%d", ret);
        cmsMsg_cleanup(&msgHandle);
        cmsLog_cleanup();
        exit(-1);
    }

    while ( not_failure )
    {
        ret = cmsMsg_receive(msgHandle, &msg);
        sleep(1);

        if (ret == CMSRET_SUCCESS)
        {
            cmsLog_debug("received msg: src=%d dst=%d type=0x%x len=%d",
                         msg->dst, msg->src, msg->type, msg->dataLength);

            if (CMS_MSG_SET_LOG_LEVEL == msg->type)
            {
                cmsLog_setLevel(msg->wordData);
                cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS);
                CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
                continue;
            }

            if (CMS_MSG_SET_LOG_DESTINATION == msg->type)
            {
                cmsLog_setDestination(msg->wordData);
                cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS);
                CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
                continue;
            }

            if  (CMS_MSG_INTERNAL_NOOP== msg->type)
            {
                CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
                continue;
            }

#ifdef WLDAEMON_DBG
            printf(" msg->dst[%d]\n", msg->dst );
            printf(" msg->src[%d]\n", msg->src );
            printf(" msg->type[%x]\n", msg->type);
            printf(" msg->dataLength[%d]\n", msg->dataLength );
#endif
            if ( msg->dataLength >0 ) 
            {
#ifdef WLDAEMON_DBG
                printf("(msg+1)=%s\n", (char *)(msg+1) );
#endif
                strcpy(cmd, (char *)(msg+1) );
            } 
	    else cmd[0]='\0';
	    
            msgSrc = msg->src; 
            CMSMEM_FREE_BUF_AND_NULL_PTR(msg);

            /* Assoc request from WLEVENT */
            if (msgSrc == EID_WLEVENT  && !strncmp(cmd, "Assoc", 5) ) {
                idx = cmd[6]- '0'-1;
                if (idx < wl_cnt && idx >=0 ) {
                    wlmngr_update_assoc_list(idx);
                    wlmngr_togglePowerSave(); 
                }
            }
            /* restart request from WPS */
            else if (msgSrc == EID_WLWPS  && !strncmp(cmd, "Modify", 6) ) 
            {
#ifdef WLDAEMON_DBG
                printf("WPS Request Restart\n");
#endif
              idx = 0;
	      if ( strlen(cmd) >=8 && (cmd[7] == '1' || cmd[7] == '2' ) )
	      {

		      /* From WPS, byte 8 indicates if need to restart all wlan interfaces */
		      if(cmd[8]) {

			      for (i=0; i<wl_cnt; i++)
				      wlan_restart(WL_SYNC_TO_MDM_NVRAM, i);
		      } else 
		      {
			      idx = cmd[7] - '0' -1;
			             wlan_restart(WL_SYNC_TO_MDM_NVRAM, idx);
		      }
	      }
            }
            /*Request from ssk initialization */
            else if ( !strncmp(cmd, "Create", 6) )
            {
                idx = 0;
                if ( strlen(cmd) >=8 && (cmd[7] == '1' || cmd[7] == '2' ) )
                {
                    idx = cmd[7] - '0' -1;
#ifdef WLDAEMON_DBG
                    printf("SSK Request Restart\n");
#endif
                    wlan_restart(WL_SYNC_FROM_MDM, idx);
                }
#ifdef BCMWAPI_WAI

                BcmWapi_ReadAsCertFromMdm();


                if (BcmWapi_ReadCertListFromMdm() == 1)
                    bcmSystem("ias -D -F /etc/AS.conf");

#endif
            }

            else if ( !strncmp(cmd, "Modify", 6) && msgSrc==EID_SSK)
            {

		/* when Lan bridge IP address change, wlan to restart*/
                idx = 0;
                if ( strlen(cmd) >=8 && (cmd[7] == '1' || cmd[7] == '2' ) )
                {
                    idx = cmd[7] - '0' -1;
#ifdef WLDAEMON_DBG
                    printf("SSK Request Restart\n");
#endif
                    wlan_restart(WL_SYNC_FROM_MDM, idx);
                }
            }
            /*sync from httpd or tr69c */
            else if ( !strncmp(cmd, "Modify", 6) && msgSrc == EID_HTTPD)
            {
                idx = 0;
                if ( strlen(cmd) >=8 && (cmd[7] == '1' || cmd[7] == '2' ) )
                {
                    idx = cmd[7] - '0' -1;
#ifdef WLDAEMON_DBG
                    printf("Httpd Request Restart\n");
#endif
                    wlan_restart(WL_SYNC_FROM_MDM_HTTPD, idx);
                }
            }// fac command
            else if ( !strncmp(cmd, "Fac", 3) && msgSrc == EID_HTTPD)
            {
                idx = 0;
                if ( strlen(cmd) >=5 && (cmd[4] == '1' || cmd[4] == '2' ) )
                {
                    idx = cmd[4] - '0' -1;
#ifdef WLDAEMON_DBG
                    printf("Fac Request Restart\n");
#endif
                    printf("Fac Request Restart\n");
                    wlan_restart(WL_SYNC_TO_MDM_NVRAM, idx);
                }
            }
            else if ( !strncmp(cmd, "PwrMngt", 7) && msgSrc == EID_HTTPD)
            {
#ifdef WLDAEMON_DBG
                printf("PwrMngt Changed Reqeust\n");
#endif
                wlmngr_togglePowerSave();
            }
            else if ( !strncmp(cmd, "Modify", 6) && msgSrc == EID_TR69C)
            {
                sleep(2); //wait for 2 seconds for ACS finishing the setting
                idx = 0;
                if ( strlen(cmd) >=8 && (cmd[7] == '1' || cmd[7] == '2' ) )
                {
                    idx = cmd[7] - '0' -1;
#ifdef WLDAEMON_DBG
                    printf("TR69 Request Restart\n");
#endif
                    wlan_restart(WL_SYNC_FROM_MDM_TR69C, idx);
                }
            }
            else if ( !strncmp(cmd, "Modify", 6) && msgSrc == EID_TR64C)
            {
                idx = 0;
                if ( strlen(cmd) >=8 && (cmd[7] == '1' || cmd[7] == '2' ) )
                {
                    idx = cmd[7] - '0' -1;

                    printf("TR64 Request Restart idx=%d wl_cnt = %d\n", idx, wl_cnt);

                    wlan_restart(WL_SYNC_FROM_MDM_TR69C, idx);
                }
            }
#ifdef BCMWAPI_WAI
            else if (msgSrc == EID_WLWAPID)
            {
                if (!strncmp(cmd, "Modify", 6))
                {
                    // AP certificate is installed, so we sync
                    // to MDM and restart.

                    idx = 0;
                    if (strlen(cmd) >= 8 && (cmd[7] == '1' || cmd[7] == '2' ))
                    {
                        idx = cmd[7] - '0' -1;
                        wlan_restart(WL_SYNC_TO_MDM, idx);
                    }
                }
                else if (!strncmp(cmd, "Record", 6))
                {
                    // AS has issued or revoked a certificate.
                    // We do record-keeping.

                    // In order to get AS to save cert list to file,
                    // we need to stop it, record, and then restart.
                    // This could be improved.

                    bcmSystem("killall -15 ias");
                    BcmWapi_SaveAsCertToMdm();
                    BcmWapi_SaveCertListToMdm(1);
                    wlWriteMdmToFlash();
                    bcmSystem("ias -D -F /etc/AS.conf");
                    BcmWapi_SetAsPending(0);
                }
                else if (!strncmp(cmd, "Start", 5))
                {
                    if (BcmWapi_AsStatus() == 0)
                    {
                        bcmSystem("ias -D -F /etc/AS.conf");
                        BcmWapi_SaveCertListToMdm(1);
                        wlWriteMdmToFlash();
                        BcmWapi_SetAsPending(0);
                    }
                }
                else if (!strncmp(cmd, "Stop", 4))
                {
                    if (BcmWapi_AsStatus() == 1)
                    {   
                        bcmSystem("killall -15 ias");
                        BcmWapi_SaveCertListToMdm(0);
                        wlWriteMdmToFlash();
                        BcmWapi_SetAsPending(0);
                    }
                }
            }
#endif
            else if (msgSrc == EID_WLNVRAM) {
#ifdef WLDAEMON_DBG
                  printf("Nvram commit Request [%s]\n", cmd);
#endif
                   for (idx=0; idx<wl_cnt; idx++)
                       wlan_restart(WL_SYNC_TO_MDM_NVRAM, idx);
            }
#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
           /*Request from dal_wan.c to Set URE Mode*/
            else if ( !strncmp(cmd, "WanUre", 6) )  {
               wlmngr_initUre();
               wlan_restart(WL_SYNC_FROM_MDM_TR69C, 0);
            }
#endif
        }
        else 
        {
            if (CMSRET_DISCONNECTED == ret)
            {
               if (cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS))
               {
                  /* normal smd shutdown, I will exit too. */
                  break;
               }
               else
               {
                /*
                 * uh-oh, lost comm link to smd; smd is prob dead.
                 * sleep 60 here to avoid lots of these messages or wlmngr
                 * can just exit.
                 */
                cmsLog_error("==>smd has crashed...");
                sleep(60);
               }
            }
            else
            {
               cmsLog_error("cmsMsg_receive failed, ret=%d", ret);
            }
        }
    }

    /*Clean up*/
    wldsltr_free();
    wlmngr_free();
  
    cmsMdm_cleanup();
    cmsMsg_cleanup(&msgHandle);
    cmsLog_cleanup();

    return 0;
}
