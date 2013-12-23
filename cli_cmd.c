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

/** cmd driven CLI code goes into this file */

#ifdef SUPPORT_CLI_CMD

#include "cms_util.h"
#include "cms_core.h"
#include "cms_cli.h"
#include "cms_msg.h"
#include "cms_seclog.h"
#include "cms_boardcmds.h"
#include "../../../../shared/opensource/include/bcm963xx/bcm_hwdefs.h"
#include "prctl.h"
#include "cli.h"
#include "adslctlapi.h"
#include "devctl_adsl.h"
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "board.h"

/* processing commands that are implemented in this file can be declared here */
static void processHelpCmd(char *cmdLine);
static void processLogoutCmd(char *cmdLine);
static void processRebootCmd(char *cmdLine);
#ifdef DMP_ADSLWAN_1
static void processDslCmd(char *cmdName, char *cmdLine);
static void processAdslCtlCmd(char *cmdLine);
static void processXdslCtlCmd(char *cmdLine);
#ifdef SUPPORT_DSL_BONDING
static void processXdslCtl0Cmd(char *cmdLine);
static void processXdslCtl1Cmd(char *cmdLine);
#endif
static void DslCfgProfileUpdate(unsigned char lineId);
static void DslIntfCfgMdmUpdate(unsigned char lineId);
static void processXtmCtlCmd(char *cmdLine);
#endif
static void processFacCtlCmd(char *cmdLine);

#ifdef SUPPORT_DDNSD
static void processDDnsCmd(char *cmdLine);
#endif

#ifdef CMS_SECURITY_LOG
static void processSeclogCmd(char *cmdLine);
#endif
#ifdef BRCM_PROFILER_ENABLED
static void processProfilerCtlCmd(char *cmdLine);
#endif
#ifdef SUPPORT_SNTP
static void processSntpCmd(char *cmdLine);
#endif
#ifdef DMP_X_ITU_ORG_GPON_1
static void processShowOmciStatsCmd(char *cmdLine);
static void processDumpOmciVoiceCmd(char *cmdLine);
static void processDumpOmciEthernetCmd(char *cmdLine);
static void processDumpOmciGemCmd(char *cmdLine);
#endif
static void processArpCmd(char *cmdLine);
static void processDefaultGatewayCmd(char *cmdLine);
static void processDhcpServerCmd(char *cmdLine);
static void processDnsCmd(char *cmdLine);
void processLanCmd(char *cmdLine);
static void processLanHostsCmd(char *cmdLine);
static void processPasswdCmd(char *cmdLine);
static void processPppCmd(char *cmdLine);
static void processRestoreDefaultCmd(char *cmdLine);
static void processRouteCmd(char *cmdLine);
static void processSaveCmd(char *cmdLine);
static void processUptimeCmd(char *cmdLine);
static void processCfgUpdateCmd(char *cmdLine);
static void processSwUpdateCmd(char *cmdLine);
static void processExitOnIdleCmd(char *cmdLine);

typedef void (*CLI_CMD_FNC) (char *cmdLine);

typedef struct {
   char *cmdName;
   char *cmdHelp;
   UINT8  perm;          /**< permission bit required to execute this item */
   UBOOL8 isLockNeeded;  /**< Framework will acquire lock before calling processing func */
   CLI_CMD_FNC cliProcessingFunc;
} CLI_CMD_ITEM;

#define LOCK_NEEDED TRUE


static const CLI_CMD_ITEM cliCmdTable[] = {
   { "fac", "fac commands.",           PERM1, 0,           processFacCmd },
   { "?", "List of all commands.",     PERM3, 0,           processHelpCmd },
   { "help", "List of all commands.",  PERM3, 0,           processHelpCmd },
   { "logout", "Logout from CLI.",     PERM3, 0,           processLogoutCmd },
   { "exit", "Logout from CLI.",       PERM3, 0,           processLogoutCmd },
   { "quit", "Logout from CLI.",       PERM3, 0,           processLogoutCmd },
   { "reboot", "Reboot the system.",   PERM3, LOCK_NEEDED, processRebootCmd },
#ifdef DMP_ADSLWAN_1
   { "adsl", "adsl",                   PERM2, LOCK_NEEDED, processAdslCtlCmd },
   { "xdslctl", "xdslctl",                   PERM2, LOCK_NEEDED, processXdslCtlCmd },
#ifdef SUPPORT_DSL_BONDING
   { "xdslctl0", "xdslctl0",                   PERM2, LOCK_NEEDED, processXdslCtl0Cmd },
   { "xdslctl1", "xdslctl1",                   PERM2, LOCK_NEEDED, processXdslCtl1Cmd },
#endif
   { "xtm", "xtm",                     PERM2, 0,           processXtmCtlCmd },
#endif
   { "brctl", "brctl",                 PERM2, 0,           NULL },
   { "cat", "cat",                     PERM2, 0,           NULL },
   { "virtualserver", "show/enable/disable virtual servers", PERM2, LOCK_NEEDED, processVirtualServerCmd },
#ifdef SUPPORT_DDNSD
   { "ddns", "ddns",                  PERM2, LOCK_NEEDED, processDDnsCmd },
#endif
#ifdef SUPPORT_DEBUG_TOOLS
   { "df", "file system info",        PERM2, 0,           NULL },
   { "loglevel", "get or set logging level", PERM2, LOCK_NEEDED, processLogLevelCmd },
   { "logdest", "get or set logging destination", PERM2, LOCK_NEEDED, processLogDestCmd },
   { "dumpcfg", "inspect config info in flash", PERM2, LOCK_NEEDED, processDumpCfgCmd },
   { "dumpmdm", "inspect MDM",                  PERM1, LOCK_NEEDED, processDumpMdmCmd },
   { "dumpeid", "ask smd to dump EID DB",       PERM2, 0,           processDumpEidInfoCmd },
   { "mdm",     "various MDM debug operations", PERM2, LOCK_NEEDED, processMdmCmd },
   { "meminfo", "Get system and CMS memory info", PERM2, 0,         processMeminfoCmd },
   { "psp", "persistent scratch pad",           PERM2, 0,           processPspCmd },
   { "kill", "send signal to process",   PERM2, 0,        NULL },
   { "dumpsysinfo", "Dump system info for error report", PERM2, LOCK_NEEDED, processDumpSysInfoCmd },
#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1
   { "dnsproxy", "dnsproxy",             PERM3, 0,        processDnsproxyCmd },
#endif
#endif /* SUPPORT_DEBUG_TOOLS */
   { "syslog", "system log commands",           PERM2, LOCK_NEEDED, processSyslogCmd },
   { "echo", "echo",                     PERM2, 0,           NULL },
   { "ifconfig", "inspect network info", PERM2, 0,        NULL },
   { "ping", "ping",                     PERM3, 0,        NULL },
   { "ps", "process info",               PERM2, 0,        NULL },
   { "pwd", "present working directory", PERM2, 0,        NULL },
#ifdef CMS_SECURITY_LOG
   { "seclog", "security log commands",  PERM2, 0,           processSeclogCmd },
#endif
#ifdef BRCM_PROFILER_ENABLED
   { "profiler", "profiler",         PERM2, LOCK_NEEDED,  processProfilerCtlCmd },
#endif /* BRCM_PROFILER_ENABLED */
#ifdef SUPPORT_SNTP
   { "sntp", "sntp",                 PERM2, LOCK_NEEDED,  processSntpCmd },
#endif
   { "sysinfo", "sysinfo",           PERM2, 0,            NULL },
   { "tftp", "tftp",                 PERM2, 0,            NULL },
#ifdef VOXXXLOAD
   { "voice", "voice",               PERM3, 0,  processVoiceCtlCmd },
#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1
   { "dect", "dect",                 PERM3, 0,  processDectCmd },
#endif
#endif

#ifdef BRCM_WLAN
   { "wlctl", "wlctl",               PERM2, 0,            NULL },
#endif
#ifdef DMP_X_ITU_ORG_GPON_1
   { "showOmciStats", "Show the OMCI Statistics", PERM2, LOCK_NEEDED, processShowOmciStatsCmd },
   { "laser", "send commands to laser driver",  PERM2, 0,           processLaserCmd },
   { "omci", "send commands to OMCID",  PERM2, 0,           processOmciCmd },
   { "omcipm", "send commands to OMCIPMD",  PERM2, 0,       processOmcipmCmd },
   { "dumpOmciVoice", "inspect OMCI voice configuration", PERM2, LOCK_NEEDED, processDumpOmciVoiceCmd },
   { "dumpOmciEnet", "inspect OMCI Ethernet configuration", PERM2, LOCK_NEEDED, processDumpOmciEthernetCmd },
   { "dumpOmciGem", "inspect OMCI GEM configuration", PERM2, LOCK_NEEDED, processDumpOmciGemCmd },
#endif
#ifdef DMP_X_BROADCOM_COM_EPON_1
   { "laser", "send commands to laser driver",  PERM2, 0,           processLaserCmd },
#endif

   { "arp", "arp",                      PERM2, 0,           processArpCmd },     
   { "defaultgateway", "defaultgatway", PERM2, LOCK_NEEDED, processDefaultGatewayCmd },
   { "dhcpserver", "dhcpserver",        PERM2, LOCK_NEEDED, processDhcpServerCmd },
   { "dns", "dns",                      PERM2, LOCK_NEEDED, processDnsCmd },
   { "lan", "lan",                      PERM2, LOCK_NEEDED, processLanCmd },
   { "lanhosts", "show hosts on LAN",   PERM3, LOCK_NEEDED, processLanHostsCmd },
   { "passwd", "passwd",                PERM3, 0, processPasswdCmd },
   { "ppp", "ppp",                      PERM2, LOCK_NEEDED, processPppCmd },
   { "restoredefault", "restore config flash to default", PERM3, LOCK_NEEDED, processRestoreDefaultCmd },
   { "route", "route",                  PERM2, LOCK_NEEDED, processRouteCmd },
   { "save", "save",                    PERM3, LOCK_NEEDED, processSaveCmd },
   { "swversion", "get software version", PERM3, LOCK_NEEDED, processSwVersionCmd },
   { "uptime", "get system uptime",     PERM3, LOCK_NEEDED, processUptimeCmd },
   { "cfgupdate", "update configurations", PERM3, LOCK_NEEDED, processCfgUpdateCmd },
   { "swupdate", "update software",     PERM3, LOCK_NEEDED, processSwUpdateCmd },
   { "exitOnIdle", "get/set exit-on-idle timeout value (in seconds)", PERM3, 0, processExitOnIdleCmd },
   { "wan", "wan",                        PERM3, LOCK_NEEDED, processWanCmd },
#ifdef SUPPORT_CMFD
   { "cmf", "cmf",                      PERM2, 0,           processCmfCmd },
#endif
#ifdef SUPPORT_MOCA
   { "moca", "moca control & stats",    PERM2, LOCK_NEEDED, processMocaCmd },
#endif
#ifdef SUPPORT_MODSW_CLI
#ifdef SUPPORT_OSGI_FELIX
   { "osgid", "osgid",                  PERM2, 0,           processOsgidCmd },
#endif
#endif
#if defined(SUPPORT_IGMP) || defined(SUPPORT_MLD)
   { "mcpctl", "display mcpd information", PERM2, 0,        NULL },
#endif
};

#define NUM_CLI_CMDS (sizeof(cliCmdTable) / sizeof(CLI_CMD_ITEM))


/* all hidden commands are just directly executed */
static const char *cliHiddenCmdTable[] = {
   "dumpmem", "ebtables", "iptables",
   "logread", "setmem", "sh"
};

#define NUM_HIDDEN_CMDS (sizeof(cliHiddenCmdTable) / sizeof(char *))

UBOOL8 cliCmdSaveNeeded = FALSE;


/* this is defined in rut_util.c and is used for user requst to disconnect/connect wan connection temperaroly */
extern CmsRet rut_sendMsgToSsk(CmsMsgType msgType, UINT32 wordData, void *msgData, UINT32 msgDataLen);

CmsRet cli_sendMsgToSsk(CmsMsgType msgType, UINT32 wordData, void *msgData, UINT32 msgDataLen)
{
   return rut_sendMsgToSsk(msgType, wordData, msgData, msgDataLen);
}


UBOOL8 cli_processCliCmd(const char *cmdLine)
{
   UBOOL8 found=FALSE;
   char compatLine[CLI_MAX_BUF_SZ] = {0};
   UINT32 i, cmdNameLen, compatLineLen;


   /*
    * To maintain compatiblity with previous software, if cmd starts with netCtl,
    * just strip it off.
    */
   if (!strncasecmp(cmdLine, "netctl", 6))
   {
      strcpy(compatLine, &(cmdLine[7]));
   }
   else
   {
      strcpy(compatLine, cmdLine);
   }


   /*
    * Figure out the length of the command (the first word).
    */
   compatLineLen = strlen(compatLine);
   cmdNameLen = compatLineLen;
   for (i=0; i < compatLineLen; i++)
   {
      if (compatLine[i] == ' ')
      {
         cmdNameLen = i;
         break;
      }
   }


   for (i=0; i < NUM_CLI_CMDS; i++)
   {
      /*
       * Note that strcasecmp is used here, which means a command should not differ from
       * another command only in capitalization.
       */
      if ((cmdNameLen == strlen(cliCmdTable[i].cmdName)) &&
          (!strncasecmp(compatLine, cliCmdTable[i].cmdName, cmdNameLen)) &&
          (currPerm & cliCmdTable[i].perm))
      {
         if (cliCmdTable[i].isLockNeeded) 
         {
            CmsRet ret;
            
            /* get lock if indicated on the cliCmdTable */
            if ((ret = cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
            {
               cmsLog_error("failed to get lock, ret=%d", ret);
               printf("Could not run command due to lock failure.\r\n");
               cmsLck_dumpInfo();
               found = TRUE;
               break;
            }
         }

         if (cliCmdTable[i].cliProcessingFunc != NULL)
         {
            if (compatLineLen == cmdNameLen)
            {
               /* there is no additional args, just pass in null terminator */
               (*(cliCmdTable[i].cliProcessingFunc))(&(compatLine[cmdNameLen]));
            }
            else
            {
               /* pass the additional args to the processing func */
               (*(cliCmdTable[i].cliProcessingFunc))(&(compatLine[cmdNameLen + 1]));
            }

         }
         else
         {
            prctl_runCommandInShellWithTimeout(compatLine);
         }

         if (cliCmdTable[i].isLockNeeded)
         {
            cmsLck_releaseLock();
         }

 
         /* See if we need to flush config */
         if (cliCmdSaveNeeded)
         {
            CmsRet ret;

            /* regardless of success or failure, clear cliCmdSaveNeeded */
            cliCmdSaveNeeded = FALSE;

            if ((ret = cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
            {
               cmsLog_error("failed to get lock, ret=%d", ret);
               printf("Could not run command due to lock failure.\r\n");
               found = TRUE;
               break;
            }

            if ((ret = cmsMgm_saveConfigToFlash()) != CMSRET_SUCCESS)
            {
               cmsLog_error("saveConfigToFlash failed, ret=%d", ret);
            }
            
            cmsLck_releaseLock();
         }

         found = TRUE;
         break;
      }
   }

   return found;
}


UBOOL8 cli_processHiddenCmd(const char *cmdLine)
{
   UINT32 i;
   UBOOL8 found = FALSE;
   UINT32 cmdNameLen, compatLineLen;

   /* only admin and support are allowed to run hidden/shell commands */
   if ((currPerm & PERM2) == 0)
   {
      return FALSE;
   }

   /*
    * Figure out the length of the command (the first word).
    */
   compatLineLen = strlen(cmdLine);
   cmdNameLen = compatLineLen;
   for (i=0; i < compatLineLen; i++)
   {
      if (cmdLine[i] == ' ')
      {
         cmdNameLen = i;
         break;
      }
   }
   
   for (i=0; i < NUM_HIDDEN_CMDS; i++)
   {
      if ((cmdNameLen == strlen(cliHiddenCmdTable[i])) &&
          (!strncasecmp(cmdLine, cliHiddenCmdTable[i], cmdNameLen)))
      {
         if (!strncasecmp(cmdLine, "sh", cmdNameLen))
         {
            /* special handling for sh, spawn with SIGINT set to default */
            SpawnProcessInfo spawnInfo;
            SpawnedProcessInfo procInfo;
            CmsRet r2;

            memset(&spawnInfo, 0, sizeof(spawnInfo));

            spawnInfo.exe = "/bin/sh";
            spawnInfo.args = "-c sh";
            spawnInfo.spawnMode = SPAWN_AND_RETURN;
            spawnInfo.stdinFd = 0;
            spawnInfo.stdoutFd = 1;
            spawnInfo.stderrFd = 2;
            spawnInfo.serverFd = -1;
            spawnInfo.maxFd = 50;
            spawnInfo.inheritSigint = FALSE;

            memset(&procInfo, 0, sizeof(procInfo));

            r2 = prctl_spawnProcess(&spawnInfo, &procInfo);
            if (r2 == CMSRET_SUCCESS)
            {
                /* now wait for sh to finish */
                CollectProcessInfo collectInfo;

                collectInfo.collectMode = COLLECT_PID;
                collectInfo.pid = procInfo.pid;

                r2 = prctl_collectProcess(&collectInfo, &procInfo);
            }
            else
            {
                cmsLog_error("failed to spawn sh, r2=%d", r2);
            }
         }
         else
         {
             prctl_runCommandInShellBlocking((char *) cmdLine);
         }
         found = TRUE;
         break;
      }
   }

   return found;
}


/*******************************************************************
 *
 * Command processing commands start here.
 *
 *******************************************************************
 */

void processHelpCmd(char *cmdLine __attribute((unused)))
{

   UINT32 i;

   for (i=0; i < NUM_CLI_CMDS; i++)
   {
      if (currPerm & cliCmdTable[i].perm)
      {
         printf("%s\r\n", cliCmdTable[i].cmdName);
      }
   }

   return;
}


void processLogoutCmd(char *cmdLine __attribute__((unused)))
{
   cli_keepLooping = FALSE;
   return;
}

void processRebootCmd(char *cmdLine __attribute__((unused)))
{
   printf("\r\nThe system shell is being reset. Please wait...\r\n");
   fflush(stdout);

   cmsUtil_sendRequestRebootMsg(msgHandle);

   return;
}


#ifdef DMP_ADSLWAN_1
static void processAdslCtlCmd(char *cmdLine)
{
    processDslCmd("adsl", cmdLine);
}

static void processXdslCtlCmd(char *cmdLine)
{
    processDslCmd("xdslctl", cmdLine);
}

#ifdef SUPPORT_DSL_BONDING
static void processXdslCtl0Cmd(char *cmdLine)
{
    processDslCmd("xdslctl0", cmdLine);
}
static void processXdslCtl1Cmd(char *cmdLine)
{
    processDslCmd("xdslctl1", cmdLine);
}
#endif

void processDslCmd(char *cmdName, char *cmdLine)
{
    FILE* fs = NULL;
    char cmd[BUFLEN_256];
    
    if (NULL != strstr(cmdLine, "profile")) {
        if(NULL != strstr(cmdLine, "save")) {
            /* Save configuration to flash; useful when configuration was done through busybox */
            cliCmdSaveNeeded = TRUE;
            DslIntfCfgMdmUpdate(0);
            return;
        }
        else if(NULL != strstr(cmdLine, "restore")) {
            /* Restore driver configuration; useful when user play around with the driver
                 configuration through busybox and want to restore it without rebooting */
#ifdef SUPPORT_DSL_BONDING
            if( 0 == strcmp("xdslctl1", cmdName))
               DslCfgProfileUpdate(1);
            else
#endif
               DslCfgProfileUpdate(0);
            return;
        }
    }
    
    // execute command with err output to adslerr
    sprintf(cmd, "%s %s 2> /var/adslerr", cmdName, cmdLine);
    prctl_runCommandInShellBlocking(cmd);
    
    /* check for presence of error output file */
    fs = fopen("/var/adslerr", "r");
    if (fs == NULL) return;

    prctl_runCommandInShellBlocking("cat /var/adslerr");
    fclose(fs);

    /* remove error file if it is there, could just call unlink("/var/adlserr") */
    prctl_runCommandInShellBlocking("rm /var/adslerr");

    return;
}

static void DslCfgProfileUpdate(unsigned char lineId)
{
    adslCfgProfile  adslCfg;
    CmsRet          cmsRet;
    WanDslIntfCfgObject *dslIntfCfg = NULL;
    InstanceIdStack         iidStack = EMPTY_INSTANCE_ID_STACK;
    
    cmsRet = cmsObj_getNext(MDMOID_WAN_DSL_INTF_CFG, &iidStack, (void **) &dslIntfCfg);
    if (cmsRet != CMSRET_SUCCESS) {
        printf("%s: could not get DSL intf cfg, ret=%d", __FUNCTION__, cmsRet);
        return;
    }
    memset((void *)&adslCfg, 0, sizeof(adslCfg));
    xdslUtil_CfgProfileInit(&adslCfg, dslIntfCfg);
    cmsObj_free((void **) &dslIntfCfg);
    
    cmsRet = xdslCtl_Configure(lineId, &adslCfg);
    if (cmsRet != CMSRET_SUCCESS)
        printf ("%s: could not configure DLS driver, ret=%d\n", __FUNCTION__, cmsRet);
    
    cmsLog_debug("DslCfgProfile is updated.");
}

static void DslIntfCfgMdmUpdate(unsigned char lineId)
{
    int     nRet;
    long    dataLen;
    char    oidStr[] = { 95 };      /* kOidAdslPhyCfg */
    adslCfgProfile  adslCfg;
    CmsRet          cmsRet;
    WanDslIntfCfgObject *dslIntfCfg = NULL;
    InstanceIdStack         iidStack = EMPTY_INSTANCE_ID_STACK;
    
    dataLen = sizeof(adslCfgProfile);
    nRet = xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&adslCfg, &dataLen);
    
    if( nRet != BCMADSL_STATUS_SUCCESS) {
        printf("%s: could not get adsCfg, ret=%d", __FUNCTION__, nRet);
        return;
    }
    
    cmsRet = cmsObj_getNext(MDMOID_WAN_DSL_INTF_CFG, &iidStack, (void **) &dslIntfCfg);
    if (cmsRet != CMSRET_SUCCESS) {
        printf("%s: could not get DSL intf cfg, ret=%d", __FUNCTION__, cmsRet);
        return;
    }
    
    xdslUtil_IntfCfgInit(&adslCfg, dslIntfCfg);
    
    cmsRet = cmsObj_set(dslIntfCfg, &iidStack);
    if (cmsRet != CMSRET_SUCCESS)
        printf("%s: could not set DSL intf cfg, ret=%d", __FUNCTION__, cmsRet);
    
    cmsObj_free((void **) &dslIntfCfg);
    
    cmsLog_debug("DslIntfCfgMdm is updated.");
}

void processXtmCtlCmd(char *cmdLine)
{
   char xtmCmd[BUFLEN_128];
   char cmd[BUFLEN_128];
   FILE* fs = NULL;

   /* put back the command name 'xtm' */
   sprintf(xtmCmd, "xtm %s", cmdLine);
   printf("processXtmCtlCmd: %s\n", xtmCmd);

   /* if not operate command then just execute command and return */
   if ( strstr(cmdLine, "operate") == NULL )
   {
      prctl_runCommandInShellBlocking(xtmCmd);
      return;
   }

   if ( strstr(cmdLine, "tdte") != NULL )
   {
      /* if add or delete tdte */
      if ( strstr(cmdLine, "add") != NULL )
      {
//         if ( AtmTd_validateCmd(atmCmd) == FALSE )
//         {
//            fprintf(stderr, "atm: operate tdte error -- invalid parameters.\n");
//            return 0;
//         }
         /* execute command with err output to atmerr */
         sprintf(cmd, "%s 2> /var/xtmerr", xtmCmd);
         prctl_runCommandInShellBlocking(cmd);
         /* read xtmerr, if there is no err then need to configure XTM profile */
         fs = fopen("/var/xtmerr", "r");
         if (fs != NULL)
         {
            if ( fgets(cmd, BUFLEN_128, fs) <= 0 || strlen(cmd) <= 1 )
               cliCmdSaveNeeded = TRUE;
//               AtmTd_trffDscrConfig(atmCmd);
            else
               prctl_runCommandInShellBlocking("cat /var/xtmerr");
            
            fclose(fs);
            prctl_runCommandInShellBlocking("rm /var/xtmerr");
         }
      }
      else if (strstr(cmdLine, "delete") != NULL)
      {
         /* need to getObjectId before execute command to remove it */
//         AtmTd_getObjectIdFromDeleteCmd(atmCmd);
         /* execute command with err output to atmerr */
         sprintf(cmd, "%s 2> /var/xtmerr", xtmCmd);
         prctl_runCommandInShellBlocking(cmd);
         /* read xtmerr, if there is no err then need to configure XTM profile in PSI */
         fs = fopen("/var/xtmerr", "r");
         if (fs != NULL)
         {
            if ( fgets(cmd, BUFLEN_128, fs) <= 0 || strlen(cmd) <= 1 )
               cliCmdSaveNeeded = TRUE;
//               AtmTd_removeFromPsiOnly(objectId);
            else
               prctl_runCommandInShellBlocking("cat /var/xtmerr");
            
            fclose(fs);
            prctl_runCommandInShellBlocking("rm /var/xtmerr");
         }
      }
      else
      {
         sprintf(cmd, "%s", xtmCmd);
         prctl_runCommandInShellBlocking(cmd);
      }
   }
   else if (strstr(cmdLine, "intf") != NULL)
   {
      /* if state intf */
      if (strstr(cmdLine, "state") != NULL)
      {
         /* execute command with err output to atmerr */
         sprintf(cmd, "%s 2> /var/xtmerr", xtmCmd);
         prctl_runCommandInShellBlocking(cmd);
         /* read xtmerr, if there is no err then need to configure XTM profile in PSI */
         fs = fopen("/var/xtmerr", "r");
         if (fs != NULL)
         {
            if ( fgets(cmd, BUFLEN_128, fs) <= 0 || strlen(cmd) <= 1 )
               cliCmdSaveNeeded = TRUE;
//               AtmPrt_portIfcConfig(atmCmd);
            else
               prctl_runCommandInShellBlocking("cat /var/xtmerr");

            fclose(fs);
            prctl_runCommandInShellBlocking("rm /var/xtmerr");
         }
      }
      else
      {
         prctl_runCommandInShellBlocking(xtmCmd);
      }
   }
   else if (strstr(cmdLine, "vcc") != NULL)
   {
      /* if add, delete, addq, deleteq, or state vcc */
      if ((strstr(cmdLine, "add") != NULL && strstr(cmdLine, "addpripkt") == NULL) ||
          (strstr(cmdLine, "delete") != NULL && strstr(cmdLine, "deletepripkt") == NULL) ||
           strstr(cmdLine, "addq") != NULL ||
           strstr(cmdLine, "deleteq") != NULL ||
          strstr(cmdLine, "state") != NULL )
      {
         if ((strstr(cmdLine, "delete") != NULL || strstr(cmdLine, "deleteq") != NULL) /*&&
             (AtmVcc_isDeletedVccInUse(atmCmd) == TRUE)*/)
         {
            printf("app: cannot delete the PVC since it is in use.\n");
            return;
         }
         /* execute command with err output to xtmerr */
         sprintf(cmd, "%s 2> /var/xtmerr", xtmCmd);
         prctl_runCommandInShellBlocking(cmd);
         /* read xtmerr, if there is no err then need to configure XTM profile in PSI */
         fs = fopen("/var/xtmerr", "r");
         if (fs != NULL)
         {
            if (fgets(cmd, BUFLEN_128, fs) <= 0 || strlen(cmd) <= 1)
               cliCmdSaveNeeded = TRUE;
//               AtmVcc_vccCfg(atmCmd);
            else
               prctl_runCommandInShellBlocking("cat /var/xtmerr");

            fclose(fs);
            prctl_runCommandInShellBlocking("rm /var/xtmerr");
         }
   }
   else
      {
         prctl_runCommandInShellBlocking(xtmCmd);
      }
   }
   else
   {
      prctl_runCommandInShellBlocking(xtmCmd);
   }

   return;
}

#endif  /* DMP_ADSLWAN_1 */


#ifdef SUPPORT_DDNSD
static void processDDnsCmd(char *cmdLine)
{
   /* todo */
#ifdef later
  DDNS_STATUS sts;
  char *pToken = strtok( cmdLine, " " );

  // First token should be "ddns" -- next we check first argument
  pToken = strtok( NULL, " " );

  if ( pToken == NULL || strcmp( pToken, "--help" ) == 0 ) {
    printf( "ddns add hostname --username username --password password --interface interface --service tzo|dyndns\n" );
    printf( "     remove hostname\n" );
    printf( "     show\n" );
    printf( "ddns --help" );
  } else if ( strcmp( pToken, "add" ) == 0 ) {
    char hostname[IFC_MEDIUM_LEN];
    char username[IFC_MEDIUM_LEN];
    char password[IFC_PASSWORD_LEN];
    char iface[IFC_TINY_LEN];
    UINT16 service = 255;

    hostname[0] = username[0] = password[0] = iface[0] = '\0';

    pToken = strtok( NULL, " " ); // Get the hostname

    if (pToken == NULL ) {
      printf( "Add what?\n" );
    } else {
      strncpy( hostname, pToken, IFC_MEDIUM_LEN );

      pToken = strtok( NULL, " " );
      while( pToken != NULL ) {
        if ( strcmp( pToken, "--service" ) == 0 ) {
          if ( service != 255 ) {
            printf( "Already specified service.\n" );
            return;
          } else {
            pToken = strtok( NULL, " " );
            if ( pToken ) {
              if( strcmp( pToken, "tzo" ) == 0 ) {
                service = 0;
              } else if ( strcmp( pToken, "dyndns" ) == 0 ) {
                service = 1;
              } else {
                printf( "Bad service type: %s\n", pToken );
                return;
              }
            } else {
              printf( "Missing argument to --service.\n");
              return;
            }
          }
        } else if ( strcmp( pToken, "--username" ) == 0 ) {
          // Process --username option and argument
          if ( username[0] != '\0' ) {
            printf("Already specified username.\n" );
            return;
          } else {
            pToken = strtok( NULL, " " );
            if( pToken ) {
              strncpy( username, pToken, IFC_MEDIUM_LEN );
            } else {
              printf( "Missing argument to --username" );
              return;
            }
          }
        } else if ( strcmp( pToken, "--password" ) == 0 ) {
          // Process --password option and argument
          if ( password[0] != '\0' ) {
            printf("Already specified password.\n" );
            return;
          } else {
            pToken = strtok( NULL, " " );
            if( pToken ) {
              strncpy( password, pToken, IFC_PASSWORD_LEN );
            } else {
              printf( "Missing argument to --password" );
              return;
            }
          }
        } else if ( strcmp( pToken, "--interface" ) == 0 ) {
          // Process --password option and argument
          if ( iface[0] != '\0' ) {
            printf("Already specified interface.\n" );
            return;
          } else {
            pToken = strtok( NULL, " " );
            if( pToken ) {
              strncpy( iface, pToken, IFC_TINY_LEN );
            } else {
              printf( "Missing argument to --interface" );
              return;
            }
          }
        } else {
          printf("Unknown option %s\n", pToken );
          return;
        }
        pToken = strtok( NULL, " " );
      }
      if( hostname[0] != '\0' && username[0] != '\0' && password[0] != '\0' && iface[0] != '\0' ) {
        sts = BcmDDns_add( hostname, username, password, iface, service );
        if (sts == DDNS_OK)
           BcmDDns_Store();
        BcmDDns_serverRestart();
        BcmPsi_flush();
      } else {
        printf("Missing options to 'add' command.\n" );
      }
    }
  } else if ( strcmp( pToken, "remove" ) == 0 ) {
    pToken = strtok( NULL, " " );
    if ( pToken == NULL ) {
      printf( "Missing hostname to remove.\n" );
    } else {
      sts = BcmDDns_remove( pToken );
      if( sts != DDNS_OK ) {
        printf( "Hostname %s does not exist.\n", pToken );
      }
      else {
        BcmDDns_Store();
        BcmPsi_flush();
      }
    }
  } else if ( strcmp( pToken, "show" ) == 0 ) {
    char hostname[IFC_MEDIUM_LEN];
    char username[IFC_MEDIUM_LEN];
    char password[IFC_PASSWORD_LEN];
    char iface[IFC_TINY_LEN];
    UINT16 service;

    void *node = BcmDDns_getDDnsCfg( NULL, hostname, username, password, iface, &service );

    while( node != NULL ) {
      printf( "%s\t%s\t%s\t%s\n", hostname, username, iface, service?"dyndns":"tzo" );
      node = BcmDDns_getDDnsCfg( node, hostname, username, password, iface, &service );
    }
  } else {
    printf( "Unknown command %s\n", pToken );
  }
#endif /* later */
  return;
}
#endif  /* DDNS */


static void cmdSyslogHelp(void)
{
   printf("\nUsage: syslog dump\n");
   printf("       syslog help\n");
}

void processSyslogCmd(char *cmdLine)
{
   IGDDeviceInfoObject *infoObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("cmdLine =>%s<=", cmdLine);

   if (!strcasecmp(cmdLine, "dump"))
   {
      if ((ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &infoObj)) == CMSRET_SUCCESS)
      {
         printf("==== Dump of Syslog ====\n");
         printf("%s\n", infoObj->deviceLog);
         cmsObj_free((void **) &infoObj);
      }
      else
      {
         printf("Could not get deviceInfo object, ret=%d", ret);
      }
   }
   else
   {
      cmdSyslogHelp();
   }
}


#ifdef CMS_SECURITY_LOG
static void cmdSeclogHelp(void)
{
   printf("\nUsage: seclog dump\n");
   printf("       seclog reset\n");
   printf("       seclog help\n");
}

void processSeclogCmd(char *cmdLine)
{
   CmsRet ret;
   CmsSecurityLogFile * log;

   cmsLog_debug("cmdLine =>%s<=", cmdLine);

   if (!strcasecmp(cmdLine, "dump"))
   {
      if ((log = cmsMem_alloc(sizeof(*log), 0)) == NULL)
      {
         cmsLog_error("malloc of %d bytes failed", sizeof(*log));
         return;
      }

      if ((ret = cmsLog_getSecurityLog(log)) == CMSRET_SUCCESS)
      {
         printf("==== Dump of Seclog ====\n");
         cmsLog_printSecurityLog(log);
      }
      else
      {
         printf("Could not get security log, ret=%d", ret);
      }

      cmsMem_free(log);
   }
   else if (!strcasecmp(cmdLine, "reset"))
   {
      if ((ret = cmsLog_resetSecurityLog()) == CMSRET_SUCCESS)
      {
         printf("==== Security Log reset ====\n");
      }
      else
      {
         printf("Error resetting security log, ret=%d", ret);
      }
   }
   else
   {
      cmdSeclogHelp();
   }
}
#endif


#ifdef BRCM_PROFILER_ENABLED
void processProfilerCtlCmd(char *cmdLine)
{
   /* todo: need to port this code */
#ifdef later
   if ( strstr(cmdLine, "init") != NULL )
   {
      BcmProfiler_InitData();
   }
   else if ( strstr(cmdLine, "rsdump") != NULL )
   {
      BcmProfiler_RecSeq_Dump();
   }
   else if ( strstr(cmdLine, "dump") != NULL )
   {
      BcmProfiler_Dump();
   }
   else if ( strstr(cmdLine, "start") != NULL )
   {
      BcmProfiler_Start();
   }
   else if ( strstr(cmdLine, "stop") != NULL )
   {
      BcmProfiler_Stop();
   }
   else
   {
      BcmProfiler_ShowCmdSyntax();
   }
#endif /* later */

 return;
} 
#endif /* BRCM_PROFILER_ENABLED */


#ifdef SUPPORT_SNTP
/* mwang_todo: these timezones should probably be put into the data model.
 * there is another copy of this list in sntpcfg.html and maybe more copies elsewhere.
 */
char *timeZones[] =
{"International Date Line West",
 "Midway Island, Samoa",
 "Hawaii",
 "Alaska",
 "Pacific Time, Tijuana",
 "Arizona",
 "Chihuahua, La Paz, Mazatlan",
 "Mountain Time",
 "Central America",
 "Central Time",
 "Guadalajara, Mexico City, Monterrey",
 "Saskatchewan",
 "Bogota, Lima, Quito",
 "Eastern Time",
 "Indiana",
 "Atlantic Time",
 "Caracas, La Paz",
 "Santiago",
 "Newfoundland",
 "Brasilia",
 "Buenos Aires, Georgetown",
 "Greenland",
 "Mid-Atlantic",
 "Azores",
 "Cape Verde Is.",
 "Casablanca, Monrovia",
 "Greenwich Mean Time: Dublin, Edinburgh, Lisbon, London",
 "Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna",
 "Belgrade, Bratislava, Budapest, Ljubljana, Prague",
 "Brussels, Copenhagen, Madrid, Paris",
 "Sarajevo, Skopje, Warsaw, Zagreb",
 "West Central Africa",
 "Athens, Istanbul, Minsk",
 "Bucharest",
 "Cairo",
 "Harare, Pretoria",
 "Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius",
 "Jerusalem",
 "Baghdad",
 "Kuwait, Riyadh",
 "Moscow, St. Petersburg, Volgograd",
 "Nairobi",
 "Tehran",
 "Abu Dhabi, Muscat",
 "Baku, Tbilisi, Yerevan",
 "Kabul",
 "Ekaterinburg",
 "Islamabad, Karachi, Tashkent",
 "Chennai, Kolkata, Mumbai, New Delhi",
 "Kathmandu",
 "Almaty, Novosibirsk",
 "Astana, Dhaka",
 "Sri Jayawardenepura",
 "Rangoon",
 "Bangkok, Hanoi, Jakarta",
 "Krasnoyarsk",
 "Beijing, Chongquing, Hong Kong, Urumqi",
 "Irkutsk, Ulaan Bataar",
 "Kuala Lumpur, Singapore",
 "Perth",
 "Taipei",
 "Osaka, Sapporo, Tokyo",
 "Seoul",
 "Yakutsk",
 "Adelaide",
 "Darwin",
 "Brisbane",
 "Canberra, Melbourne, Sydney",
 "Guam, Port Moresby",
 "Hobart",
 "Vladivostok",
 "Magadan, Solomon Is., New Caledonia",
 "Auckland, Wellington",
 "Fiji, Kamchatka, Marshall Is.",
 NULL };

#define STNP_SERVER_MAX         5

static CmsRet disableSntp(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   TimeServerCfgObject *ntpCfg=NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsObj_get(MDMOID_TIME_SERVER_CFG, &iidStack, 0, (void *) &ntpCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of TIME_SERVER_CFG failed, ret=%d", ret);
      return ret;
   }

   ntpCfg->enable = FALSE;

   ret = cmsObj_set(ntpCfg, &iidStack);
   cmsObj_free((void **) &ntpCfg);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("set of TIME_SERVER_CFG failed, ret=%d", ret);
   }

   return ret;
}

static CmsRet configureSntp(char **sntpServers, char *timeZoneName)
{
   TimeServerCfgObject *ntpCfg=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsObj_get(MDMOID_TIME_SERVER_CFG, &iidStack, 0, (void *) &ntpCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of TIME_SERVER_CFG failed, ret=%d", ret);
      return ret;
   }

   ntpCfg->enable = TRUE;
   CMSMEM_REPLACE_STRING(ntpCfg->NTPServer1, sntpServers[0]);
   CMSMEM_REPLACE_STRING(ntpCfg->NTPServer2, sntpServers[1]);
   CMSMEM_REPLACE_STRING(ntpCfg->NTPServer3, sntpServers[2]);
   CMSMEM_REPLACE_STRING(ntpCfg->NTPServer4, sntpServers[3]);
   CMSMEM_REPLACE_STRING(ntpCfg->NTPServer5, sntpServers[4]);
   CMSMEM_REPLACE_STRING(ntpCfg->localTimeZoneName, timeZoneName);

   ret = cmsObj_set(ntpCfg, &iidStack);
   cmsObj_free((void **) &ntpCfg);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("set of TIME_SERVER_CFG failed, ret=%d", ret);
   }

   return ret;
}

static void processSntpCmd(char *cmdLine)
{
   int i = 0;
   char *pToken = strtok(cmdLine, " ");
   char *sntpServers[STNP_SERVER_MAX];
   char *timezone = NULL;
   int error = 0;
   int done = 0;
   TimeServerCfgObject *ntpCfg=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;


   /* get default sntp object first: in case disable is called with no parameters */
   for (i = 0; i < STNP_SERVER_MAX; i++)
   {
      sntpServers[i] = NULL;
   }
   if ((cmsObj_get(MDMOID_TIME_SERVER_CFG, &iidStack, 0, (void **) &ntpCfg)) == CMSRET_SUCCESS)
   {
      if (ntpCfg->NTPServer1 != NULL)
      {      
         sntpServers[0] = cmsMem_strdup(ntpCfg->NTPServer1);
      }
      if (ntpCfg->NTPServer2 != NULL)
      {
         sntpServers[1] = cmsMem_strdup(ntpCfg->NTPServer2);
      }
      if (ntpCfg->NTPServer3 != NULL)
      {
         sntpServers[2] = cmsMem_strdup(ntpCfg->NTPServer3);
      }
      if (ntpCfg->NTPServer4 != NULL)
      {
         sntpServers[3] = cmsMem_strdup(ntpCfg->NTPServer4);
      }
      if (ntpCfg->NTPServer5 != NULL)
      {
         sntpServers[4] = cmsMem_strdup(ntpCfg->NTPServer5);
      }
      timezone = cmsMem_strdup(ntpCfg->localTimeZoneName);
   }
   

   if (pToken != NULL)
   {
      if (strcasecmp(pToken, "disable") == 0)
      {
         cliCmdSaveNeeded = TRUE;
         disableSntp();
         done = 1;
      }
      else if (strcasecmp( pToken, "date" ) == 0)
      {
         // Print the date
         time_t cur_time = time(NULL);
         printf("%s", ctime(&cur_time));
         done = 1;
      }
      else if (strcasecmp(pToken, "zones") == 0)
      {
         printf( "Timezones supported:\n" );
         for(i = 0; timeZones[i] != NULL; i++)
         {
            if (i > 0 && i % 22 == 0)
            {
               printf( "Press <enter> for more." );
               getchar();
            }
            printf("%s\n", timeZones[i]);
            done = 1;
         }
      }
      else if (strcasecmp(pToken,"-s") == 0) 
      {
         for (i = 0; i < STNP_SERVER_MAX && strcmp(pToken, "-s") == 0; i++)
         {
            pToken = strtok(NULL, " ");
            if (pToken == NULL && i == 0)
            {
               cmsLog_error("No argument to -s option.\n");
               error = 1;
               break;
            }
            else if (pToken != NULL)
            {
               CMSMEM_REPLACE_STRING(sntpServers[i], pToken);
            }
            pToken = strtok(NULL, " ");
         }
         printf("pToken after server %s\n",pToken);

         if ((!error) && (0 == strcmp(pToken, "-t")))
         {
            pToken = strtok(NULL, "\n"); // Rest of string
            if (pToken == NULL)
            {
               cmsLog_error("Missing argument to -t option.\n");
               error = 1;
            }
            else
            {
               if(pToken[0] == '"')  // Trim leading quotes
                  pToken++;
               if (pToken[strlen(pToken)-1] == '"') // Trim trailing quotes
                  pToken[strlen(pToken)-1] = '\0';
               CMSMEM_REPLACE_STRING(timezone, pToken);
            }
         }
         else
         {
            cmsLog_error("-t timezone is required\n");
            error = 1;
         } /* get default obj ok */
      } /* -s pToken != NULL */
      else 
      {
         error = 1;
      }
   } /* pToken != NULL */
   else 
   {
      error = 1;
   }
   if (!done && !error)
   {
      cliCmdSaveNeeded = TRUE;
      configureSntp(sntpServers, pToken);
   }
   if (error)
   {
      printf("\n");
      printf( "sntp -s server [ -s server2 ] -t \"timezone\"\n" );
      printf( "     disable\n");
      printf( "     date\n" );
      printf( "     zones\n" );
      printf( "sntp --help\n" );
   }
   /* clean up */
   for (i = 0; i < STNP_SERVER_MAX; i++)
   {
      if (sntpServers[i] != NULL)
      {
         cmsMem_free(sntpServers[i]);
      }
   }
   if (timezone != NULL)
   {
      cmsMem_free(timezone);
   }
} 

#endif  /* SUPPORT_SNTP */

/***************************************************************************
// Function Name: cmdArpAddDel
// Description  : arp add or delete
// Parameters   : type - 0=add, 1=delete.
//                         ipAddr - ip address
//                         macAddr - mac address
//                         msg - return error message
// Returns      : 0 - failed.
//                      1 -  success.
****************************************************************************/
int cmdArpAddDel(int type, char *ipAddr, char *macAddr, char *msg) 
{
   int i = 0, sockfd = 0;
   struct arpreq req;
   struct sockaddr sa;
   struct sockaddr_in *addr = (struct sockaddr_in*)&sa;
   unsigned char *ptr = NULL, *mac = NULL, *mac1 = NULL;

   if (cli_isIpAddress(ipAddr) == FALSE)
      {   
      strcpy(msg, "Invalid ip address.");
      return 0;
      }
   //only type=0 for adding has mac address
   if (type == 0 && cli_isMacAddress(macAddr) == FALSE)
      {
      strcpy(msg, "Ivalid mac address.");
      return 0;
      }

   addr->sin_family = AF_INET;
   addr->sin_port = 0;
   inet_aton(ipAddr, &addr->sin_addr);
   memcpy((char *)&req.arp_pa, (char *)&sa, sizeof(struct sockaddr));
   bzero((char*)&sa,sizeof(struct sockaddr));
   ptr = (unsigned char *)sa.sa_data;
   if (type == 0)
   {
      mac = (unsigned char *)macAddr;
      for ( i = 0; i < 6; i++ ) 
      {
         *ptr = (unsigned char)(strtol((char*)mac,(char**)&mac1,16));
         mac1++;
         ptr++;
         mac = mac1;
      }
   }   
   sa.sa_family = ARPHRD_ETHER;
   memcpy((char *)&req.arp_ha, (char *)&sa, sizeof(struct sockaddr));
   req.arp_flags = ATF_PERM;
   req.arp_dev[0] = '\0';

   if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
   {
      strcpy(msg, "Error in create socket.");
      return 0;
   } 
   else 
   {
      if (type ==0 && ioctl(sockfd, SIOCSARP, &req) < 0) 
      {
         strcpy(msg, "Error in SIOCSARP.");
         return 0;
      }
      if (type ==1 && ioctl(sockfd, SIOCDARP, &req) < 0) 
      {
         strcpy(msg, "Error in SIOCDARP.");
         return 0;
      }
   }
   if ( sockfd > 0 )
      close(sockfd);

   return 1;
}

void cmdArpHelp(int argc)
{
   if ( argc <= 1 )
      fprintf(stdout,
         "\nUsage: arp add <IP address> <MAC address>\n"
         "       arp delete <IP address>\n"
         "       arp show\n"
         "       arp --help\n");
   else
      fprintf(stderr, "arp: invalid number of parameters " \
         "for option '--help'\n");
}
void processArpCmd(char *cmdLine)
{
   const int maxOpt=3;
   SINT32 argc = 0;
   char *argv[maxOpt];
   char *last = NULL;
   char msg[BUFLEN_64];

   /* parse the command line and build the argument vector */
   argv[0] = strtok_r(cmdLine, " ", &last);
   if (argv[0] != NULL)
   {
      for (argc = 1; argc < maxOpt; argc++)
      {
         argv[argc] = strtok_r(NULL, " ", &last);
         if (argv[argc] == NULL)
            break;
      }
   }

   if (argv[0] == NULL)
   {
      cmdArpHelp(argc);
   }
   else if (strcasecmp(argv[0], "add") == 0)
   {  
      if (argc != 3)
	  {
         fprintf(stderr, "arp: invalid number of parameters " \
                "for option 'add'\n");
		 return;
	  }
      if (cmdArpAddDel(0, argv[1], argv[2], msg) == 0)
         fprintf(stderr, "arp: %s\n", msg);       
   }
   else if (strcasecmp(argv[0], "delete") == 0)
   {  
      if (argc != 2)
	  {
         fprintf(stderr, "arp: invalid number of parameters " \
                "for option 'delete'\n");
		 return;
	  }
      if (cmdArpAddDel(1, argv[1], NULL, msg) == 0)
         fprintf(stderr, "arp: %s\n", msg);       
   }
   else if (strcasecmp(argv[0], "show") == 0)
   {  
      if(argc != 1)
         fprintf(stderr, "arp: invalid number of parameters " \
               "for option 'show'\n");
	  else
	  {
      printf("\n");
      prctl_runCommandInShellBlocking("cat /proc/net/arp");
      printf("\n");
	  }	
   }
   else if (strcasecmp(argv[0], "--help") == 0)
   {
      cmdArpHelp(argc);
   }
   else 
   {
      fprintf(stderr, "\nInvalid option '%s'\n", argv[0]);
   }
   return;
}

/** for processMacAddrCmd() */
static int boardIoctl(int board_ioctl, BOARD_IOCTL_ACTION action, char *string, int strLen, int offset, char *buf)
{
    BOARD_IOCTL_PARMS IoctlParms;
    int boardFd = 0;

    boardFd = open("/dev/brcmboard", 0x02);
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

static int sysNvRamGet(char *string, int strLen, int offset)
{
    return (boardIoctl(BOARD_IOCTL_FLASH_READ, NVRAM, string, strLen, offset, ""));
}

static int sysNvRamSet(char *string, int strLen, int offset)
{
    return (boardIoctl(BOARD_IOCTL_FLASH_WRITE, NVRAM, string, strLen, offset, ""));
}

#define CRC32_INIT_VALUE 0xffffffff

static unsigned long Crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};
static UINT32 getCrc32(unsigned char *pdata, UINT32 size, UINT32 crc) 
{
    while (size-- > 0)
        crc = (crc >> 8) ^ Crc32_table[(crc ^ *pdata++) & 0xff];

    return crc;
}

static void writeNvramData(PNVRAM_DATA pNvramData)
{
    UINT32 crc = CRC32_INIT_VALUE;
    
    pNvramData->ulCheckSum = 0;
    crc = getCrc32((unsigned char *)pNvramData, sizeof(NVRAM_DATA), crc);      
    pNvramData->ulCheckSum = crc;
    sysNvRamSet((unsigned char *)pNvramData, sizeof(NVRAM_DATA), 0);
}

// read the nvramData struct from nvram 
// return -1:  crc fail, 0 ok
static int readNvramData(PNVRAM_DATA pNvramData)
{
    UINT32 crc = CRC32_INIT_VALUE, savedCrc;
    
    sysNvRamGet((unsigned char *)pNvramData, sizeof(NVRAM_DATA), 0);
    savedCrc = pNvramData->ulCheckSum;
    pNvramData->ulCheckSum = 0;
    crc = getCrc32((unsigned char *)pNvramData, sizeof(NVRAM_DATA), crc);      
    if (savedCrc != crc)
        return -1;
    
    return 0;
}

void cmdDefaultGatewayHelp(int argc)
{
   if ( argc <= 1 )
      fprintf(stdout,
         "\nUsage: defaultgateway config [<interface(s) sperated by ',' with NO SPACE.  eg. ppp0 OR for multiple interfaces ppp0,ppp1>]\n"
         "       defaultgateway show\n"
         "       defaultgateway --help\n");
   else
      fprintf(stderr, "defaultgateway: invalid number of parameters " \
         "for option '--help'\n");
}
void processDefaultGatewayCmd(char *cmdLine)
{
   const int maxOpt=2;
   SINT32 argc = 0;
   char *argv[maxOpt];
   char *last = NULL;
   char ifaceList[BUFLEN_256];
   CmsRet ret;

   /* parse the command line and build the argument vector */
   argv[0] = strtok_r(cmdLine, " ", &last);
   if (argv[0] != NULL)
   {
      for (argc = 1; argc < maxOpt; argc++)
      {
         argv[argc] = strtok_r(NULL, " ", &last);
         if (argv[argc] == NULL)
            break;
      }
   }

   if (argv[0] == NULL)
   {
      cmdDefaultGatewayHelp(argc);
   }
   else if (strcasecmp(argv[0], "config") == 0)
   {  
      if (argc != 2)
      {
         fprintf(stderr, "defaultgateway: invalid number of parameters " \
                         "for option 'config'\n");
		   return;
	   }

      if ((ret = dalRt_setDefaultGatewayList(argv[1])) != CMSRET_SUCCESS)
      {
         fprintf(stderr, "Config defaultwatewayList failed, ret=%d flags=%s", ret, argv[1]);
      }
      else
      {
         printf("Default Gateway(s): %s\n",argv[1]);
         cliCmdSaveNeeded = TRUE;
      }		 
   }
   else if (strcasecmp(argv[0], "show") == 0)
   {  
      if(argc != 1)
      {
         fprintf(stderr, "defaultgateway: invalid number of parameters " \
                         "for option 'show'\n");
      }                         
      else
	   {
   		if ((ret = dalRt_getDefaultGatewayList(ifaceList)) != CMSRET_SUCCESS)
   		{
            fprintf(stderr, "defaultgateway: dalRt_getDefaultGatewayList failed !" \
                            "for option 'show'\n");
   		}
   		else
   		{
      		 if (strlen(ifaceList) == 0)
      		    strcpy(ifaceList, "None");
      		 printf("Default Gateway(s): %s\n",ifaceList);
         }	
      }
   }
   else if (strcasecmp(argv[0], "--help") == 0)
   {
      cmdDefaultGatewayHelp(argc);
   }
   else 
   {
      printf("\nInvalid option '%s'\n", argv[0]);
   }
   return;
}

void cmdDhcpServerHelp(int argc)
{
   if ( argc <= 1 )
      fprintf(stdout,
         "\nUsage: dhcpserver config <start IP address> <end IP address> <leased time (hour)>\n"
         "       dhcpserver show\n"
         "       dhcpserver --help\n");
   else
      fprintf(stderr, "dhcpserver: invalid number of parameters " \
         "for option '--help'\n");
}
void processDhcpServerCmd(char *cmdLine)
{
   const int maxOpt=4;
   SINT32 argc = 0;
   char *argv[maxOpt];
   char *last = NULL;
   LanHostCfgObject *lanHostCfg = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   /* parse the command line and build the argument vector */
   argv[0] = strtok_r(cmdLine, " ", &last);
   if (argv[0] != NULL)
   {
      for (argc = 1; argc < maxOpt; argc++)
      {
         argv[argc] = strtok_r(NULL, " ", &last);
         if (argv[argc] == NULL)
            break;
      }
   }

   if (argv[0] == NULL)
   {
      cmdDhcpServerHelp(argc);
   }
   else if (strcasecmp(argv[0], "config") == 0)
   {  
      SINT32 lease;
      
      if (argc != 4)
      {
         fprintf(stderr, "dhcpserver: invalid number of parameters " \
                         "for option 'config'\n");
		 return;
	  }
      if (cli_isIpAddress(argv[1]) == 0)
      {
         fprintf(stderr, "dhcpserver: invalid start IP address '%s' for option 'config'.\n", argv[1]);
         return;
      }
      if (cli_isIpAddress(argv[2]) == 0)
      {
         fprintf(stderr, "dhcpserver: invalid end IP address '%s' for option 'config'.\n", argv[2]);
         return;
      }
      lease = atoi(argv[3]) * 3600;
      if(lease <= 0)
      {
         fprintf(stderr, "dhcpserver: invalid end leased time '%s' for option 'config'.\n", argv[3]);
         return;
      }
      
      if ((ret = cmsObj_getNext(MDMOID_LAN_HOST_CFG,
                                &iidStack,
                                (void *) &lanHostCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get current lanHostCfg, ret=%d", ret);
         return;
      }

      /* update dhcp start/end ip info */
      cmsMem_free(lanHostCfg->minAddress);
      lanHostCfg->minAddress = cmsMem_strdup(argv[1]);
      cmsMem_free(lanHostCfg->maxAddress);
      lanHostCfg->maxAddress = cmsMem_strdup(argv[2]);
      lanHostCfg->DHCPLeaseTime = lease;

      if ((ret = cmsObj_set(lanHostCfg, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set Lan Host CfgMangmnt, ret = %d", ret);
      }
      
      cmsObj_free((void **) &lanHostCfg);   
   }
   else if (strcasecmp(argv[0], "show") == 0)
   {  
      if(argc != 1)
         fprintf(stderr, "dhcpserver: invalid number of parameters " \
                         "for option 'show'\n");
      else
	  {
         if ((ret = cmsObj_getNext(MDMOID_LAN_HOST_CFG,
                                   &iidStack,
                                   (void *) &lanHostCfg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get current lanHostCfg, ret=%d", ret);
            return;
         }
         printf("dhcpserver: %s\n", lanHostCfg->DHCPServerEnable ? "enable" : "disable");
         printf("start ip address: %s\n", lanHostCfg->minAddress);
         printf("end ip address: %s\n", lanHostCfg->maxAddress);
         printf("leased time: %d hours\n", lanHostCfg->DHCPLeaseTime/3600);         
         cmsObj_free((void **) &lanHostCfg);   
      }	
   }
   else if (strcasecmp(argv[0], "--help") == 0)
   {
      cmdDhcpServerHelp(argc);
   }
   else 
   {
      printf("\nInvalid option '%s'\n", argv[0]);
   }
   return;
}


void cmdDnsHelp(int argc)
{
   if ( argc <= 1 )
      fprintf(stdout,
         "\nUsage: dns config auto [<interface(s) sperated by ',' with NO SPACE.  eg. ppp0 OR for multiple interfaces ppp0,ppp1>]\n"
         "\nUsage: dns config static [<primary DNS> [<secondary DNS>]]\n"
         "       dns show\n"
         "       dns --help\n");
   else
      fprintf(stderr, "dns: invalid number of parameters " \
         "for option '--help'\n");
}
void processDnsCmd(char *cmdLine)
{
   const int maxOpt=4;
   SINT32 argc = 0;
   char *argv[maxOpt];
   char *last = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   /* parse the command line and build the argument vector */
   argv[0] = strtok_r(cmdLine, " ", &last);
   if (argv[0] != NULL)
   {
      for (argc = 1; argc < maxOpt; argc++)
      {
         argv[argc] = strtok_r(NULL, " ", &last);
         if (argv[argc] == NULL)
            break;
      }
   }

   if (ret == CMSRET_SUCCESS)
   {
      if (argv[0] == NULL)
      {
         cmdDnsHelp(argc);
      }
      else if (strcasecmp(argv[0], "config") == 0)
      {  
         if ( strcasecmp(argv[1], "static") == 0 ) 
         {  
	        char servers[BUFLEN_64];
            if (argc < 3)
            {
               fprintf(stderr, "dns: invalid number of parameters " \
                       "for option '%s'\n", argv[1]);
			      return;
            }
            else if ( cli_isIpAddress(argv[2]) == FALSE ) 
			   {
               fprintf(stderr, "dns: invalid primary dns '%s' for " \
                       "option '%s'\n", argv[2], "static");
               return;
            } 
			   else if ( argc == 4 && cli_isIpAddress(argv[3]) == FALSE ) 
			   {
               fprintf(stderr, "dns: invalid secondary dns '%s' for " \
                       "option '%s'\n", argv[3], "static");
               return;
            }
            snprintf(servers, sizeof(servers), "%s,%s", argv[2], argc == 4 ? argv[3]:"");
            dalNtwk_setSystemDns(NULL, servers);
            cliCmdSaveNeeded = TRUE;			
         } 
		   else if ( strcasecmp(argv[1], "auto") == 0 ) 
		   {
            if (argc != 3)
            {
               fprintf(stderr, "dns: invalid number of parameters " \
                       "for option '%s'\n", argv[1]);
			      return;
            }
            dalNtwk_setSystemDns(argv[2], NULL);
            cliCmdSaveNeeded = TRUE;			
         } 
   		else 
   		{
            fprintf(stderr, "dns: invalid parameter %s " \
                    "for option '%s'\n", argv[1], argv[0]);
            return;
         }
      }		 
      else if (strcasecmp(argv[0], "show") == 0)
      {  
         
         if(argc != 1)
            fprintf(stderr, "dns: invalid number of parameters " \
               "for option 'show'\n");
   		else
         {
            char dnsIfNameList[CMS_MAX_DNSIFNAME * CMS_IFNAME_LENGTH]={0};
            char dnsServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
            char dns1[CMS_IPADDR_LENGTH];
            char dns2[CMS_IPADDR_LENGTH];

            dalNtwk_getSystemDns(dnsIfNameList, dnsServers);
            if ( dnsServers[0] != '\0')
            {
               dns1[0] = dns2[0] = '\0';      
               cmsUtl_parseDNS(dnsServers, dns1, dns2, TRUE);
   			   printf("DNS: static\nPrimary DNS Server = %s\nSecondary DNS Server = %s\n", dns1, dns2);               
            }
   			else if (dnsIfNameList[0] != '\0')
   			{
   			   printf("DNS: auto %s\n", dnsIfNameList); 
   			}
         }			
      }
      else if (strcasecmp(argv[0], "--help") == 0)
      {
         cmdDnsHelp(argc);
      }
      else 
      {
         printf("\nInvalid option '%s'\n", argv[0]);
         ret = CMSRET_INVALID_ARGUMENTS;
      }
   }
   return;
}


void cmdLanHelp(int argc)
{
   if ( argc <= 1 )
      fprintf(stdout,
         "\nUsage: lan config [--ipaddr <primary|secondary> <IP address> <subnet mask>]\n"
         "                  [--dhcpserver <enable|disable>]\n"
         "                  [--dhcpclient <enable|disable>]\n"
         "       lan delete --ipaddr <secondary>\n"
         "       lan show [<primary|secondary>]\n"
         "       lan --help\n");
   else
      fprintf(stderr, "lan: invalid number of parameters " \
         "for option '--help'\n");
}
void processLanCmd(char *cmdLine)
{
   const int maxOpt=7;
   SINT32 argc = 0;
   char *argv[maxOpt];
   char *last = NULL;
   InstanceIdStack lanHostIidStatck = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack lanIptfIidStack = EMPTY_INSTANCE_ID_STACK;
   LanHostCfgObject *lanHostCfgObj = NULL;
   LanIpIntfObject *lanIpObj = NULL;
   CmsRet ret;
   UBOOL8 found = FALSE;

   /* parse the command line and build the argument vector */
   argv[0] = strtok_r(cmdLine, " ", &last);
   if (argv[0] != NULL)
   {
      for (argc = 1; argc < maxOpt; argc++)
      {
         argv[argc] = strtok_r(NULL, " ", &last);
         if (argv[argc] == NULL)
            break;
      }
   }

   if (argv[0] == NULL)
   {
      cmdLanHelp(argc);
   }
   else if (strcasecmp(argv[0], "config") == 0)
   {  
      SINT32 i;
	  char *ip=NULL, *mask=NULL;
	  SINT32 dhcp = -1, primary = -1;
      SINT32 dhcpClientMode=-1;

	  for(i = 1; i < argc; i++)
	  {
	      if (strcmp(argv[i], "--ipaddr") == 0)
	      {
	         i++;
			 if (i >= argc)
			 {
			    fprintf(stderr, "lan: invalid number of parameters for option 'config'\n");
				return;
			 }
			 if (strcmp(argv[i], "primary") && strcmp(argv[i], "secondary"))
			 {
			    fprintf(stderr, "lan: invalid parameters for option 'config'\n");
				return;
			 }
			 if (strcmp(argv[i], "secondary"))
			    primary = 1;
			 else
			 	primary = 0;
			 
			 i++;
			 if (i >= argc)
			 {
			    fprintf(stderr, "lan: invalid number of parameters for option 'config'\n");
				return;
			 }
			 if (cli_isIpAddress(argv[i]) == FALSE)
			 {
			    fprintf(stderr, "lan: invalid ip address %s for option 'config'\n",argv[i]);
				return;
			 }
			 ip = argv[i];
			 
			 i++;
			 if (i >= argc)
			 {
			    fprintf(stderr, "lan: invalid number of parameters for option 'config'\n");
				return;
			 }
			 if (cli_isIpAddress(argv[i]) == FALSE)
			 {
			    fprintf(stderr, "lan: invalid subnet mask %s for option 'config'\n",argv[i]);
				return;
			 }
			 mask = argv[i];	
	      }
		  else if (strcmp(argv[i], "--dhcpserver") == 0)
		  {
	         i++;
			 if (i >= argc)
			 {
			    fprintf(stderr, "lan: invalid number of parameters for option 'config'\n");
				return;
			 }
			 if (strcmp(argv[i], "enable") && strcmp(argv[i], "disable"))
			 {
			    fprintf(stderr, "lan: invalid parameters for option 'config'\n");
				return;
			 }
			 if (strcmp(argv[i], "disable"))
			    dhcp = 1;
			 else
			 	dhcp = 0;
		  }
         else if (!cmsUtl_strcmp(argv[i], "--dhcpclient"))
         {
            i++;
            if ((i >= argc) || (strcmp(argv[i], "enable") && strcmp(argv[i], "disable")))
            {
               fprintf(stderr, "lan: must specify enable or disable\n");
               return;
            }
            if (primary != -1 || dhcp != -1)
            {
               fprintf(stderr, "lan: cannot specify ipaddr or dhcp server with dhcpclient option\n");
               return;
            }

            dhcpClientMode = strcmp(argv[i], "disable");
         }
		  else
		  {
			 fprintf(stderr, "lan: invalid parameter %s for option 'config'\n", argv[i]);
			 return;
		  }
	  }
	  
      if (primary != -1 || dhcp != -1 || dhcpClientMode != -1)
      {
         // CLI can only configure the first LAN bridge
         if ((ret = cmsObj_getNext(MDMOID_LAN_HOST_CFG, &lanHostIidStatck, (void **) &lanHostCfgObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Lan Host CfgMangmnt, ret = %d", ret);
            return;
         }
      }

	  //configure lan or lan2 ip
      if (primary != -1)
      {
         while ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF, &lanHostIidStatck, &lanIptfIidStack, (void **) &lanIpObj)) == CMSRET_SUCCESS)
         {
            /* Assume that bridge name associated with major ip address does not have ":" */
            if (primary == 1 && cmsUtl_strstr(lanIpObj->X_BROADCOM_COM_IfName, ":") == NULL)
            {
               /* do not free lanIpObj */
               found = TRUE;
               break;
            }
			/* Assume that bridge name associated with secondary ip address have ":" */
            else if (primary == 0 && cmsUtl_strstr(lanIpObj->X_BROADCOM_COM_IfName, ":") != NULL)
            {
               /* do not free lanIpObj */
               found = TRUE;
               break;
            }
   
            cmsObj_free((void **) &lanIpObj);
         }

         if (found == FALSE && primary == 1)
         {
            cmsLog_error("Failed to get IPInterface, ret = %d", ret);
            cmsObj_free((void **) &lanHostCfgObj);
            return;
         }
		 else if( found == TRUE && primary == 1)
		 {
            /* update IpInterface with the new values */
            CMSMEM_REPLACE_STRING(lanIpObj->IPInterfaceIPAddress, ip);
            CMSMEM_REPLACE_STRING(lanIpObj->IPInterfaceSubnetMask, mask);
            if ((ret = cmsObj_set(lanIpObj, &lanIptfIidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to set IPInterface, ret = %d", ret);
               cmsObj_free((void **) &lanIpObj);
               cmsObj_free((void **) &lanHostCfgObj);
               return;
            }
            cmsObj_free((void **) &lanIpObj);
		 }
         else if (found == FALSE && primary == 0) //always create for lan2
         {
            /* could not find it, create a new one */
            if ((ret = cmsObj_addInstance(MDMOID_LAN_IP_INTF, &lanHostIidStatck)) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not create new lan ip interface instance , ret=%d", ret);
               cmsObj_free((void **) &lanHostCfgObj);
               return;
            }
      
            if ((ret = cmsObj_get(MDMOID_LAN_IP_INTF, &lanHostIidStatck, 0, (void **) &lanIpObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get LanIpIntfObject, ret=%d", ret);
               cmsObj_free((void **) &lanHostCfgObj);
               return;
            }
      
            CMSMEM_REPLACE_STRING(lanIpObj->IPInterfaceIPAddress, ip);
            CMSMEM_REPLACE_STRING(lanIpObj->IPInterfaceSubnetMask, mask);
            lanIpObj->enable = TRUE;
      
            if ((ret = cmsObj_set(lanIpObj, &lanHostIidStatck)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to set LanIpIntfObject, ret=%d", ret);
               cmsObj_free((void **) &lanIpObj);
               cmsObj_free((void **) &lanHostCfgObj);
			   return;
            }
            cmsObj_free((void **) &lanIpObj);
         }
         else if (found == TRUE && primary == 0)
         {
            /* to update secondary ip address if necessary */
            CMSMEM_REPLACE_STRING(lanIpObj->IPInterfaceIPAddress, ip);
            CMSMEM_REPLACE_STRING(lanIpObj->IPInterfaceSubnetMask, mask);
            lanIpObj->enable = TRUE;
            if((ret = cmsObj_set(lanIpObj, &lanIptfIidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to set LanIpIntfObject, ret=%d", ret);
               cmsObj_free((void **) &lanIpObj);
               cmsObj_free((void **) &lanHostCfgObj);
			   return;
            }
            cmsObj_free((void **) &lanIpObj);
         }
      }  // end of if (primary != -1)

      if (dhcp != -1)
	  {
         lanHostCfgObj->DHCPServerEnable = (dhcp == 0) ? FALSE : TRUE;

         if ((ret = cmsObj_set(lanHostCfgObj, &lanHostIidStatck)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set Lan Host CfgMangmnt, ret = %d", ret);
         }
	  }

      if (dhcpClientMode != -1)
      {
         // set dhcp client mode on the ip interface object
         if ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF,
                                            &lanHostIidStatck,
                                            &lanIptfIidStack,
                                            (void **) &lanIpObj)) == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING(lanIpObj->IPInterfaceAddressingType,
                                  (dhcpClientMode ? MDMVS_DHCP : MDMVS_STATIC));
            if((ret = cmsObj_set(lanIpObj, &lanIptfIidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to set LanIpIntfObject, ret=%d", ret);
            }
            cmsObj_free((void **) &lanIpObj);
         }
      }


      cmsObj_free((void **) &lanHostCfgObj);

   }
   else if (strcasecmp(argv[0], "delete") == 0)
   {  
      if(argc != 3)
         fprintf(stderr, "lan: invalid number of parameters for option 'delete'\n");
	  else if (strcmp(argv[1], "--ipaddr") != 0)
	     fprintf(stderr, "lan: invalid parameter %s for option 'delete'\n", argv[1]);
	  else if (strcmp(argv[2], "secondary") != 0)
	     fprintf(stderr, "lan: invalid parameter %s for option 'delete'\n", argv[2]);
      //only lan2 can be deleted
	  else if (strcmp(argv[2], "secondary") == 0)
	  {
         if ((ret = cmsObj_getNext(MDMOID_LAN_HOST_CFG, &lanHostIidStatck, (void **) &lanHostCfgObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get Lan Host CfgMangmnt, ret = %d", ret);
            return;
         }

         while ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF, &lanHostIidStatck, &lanIptfIidStack, (void **) &lanIpObj)) == CMSRET_SUCCESS)
         {
            /* Assume that bridge name associated with secondary ip address have ":" */
            if (cmsUtl_strstr(lanIpObj->X_BROADCOM_COM_IfName, ":") != NULL)
            {
               found = TRUE;
               break;
            }
            cmsObj_free((void **) &lanIpObj);
         }
		 if (found == TRUE)
		 {
            /* found it, delete secondary ip address */
            cmsObj_free((void **) &lanIpObj);
            if((ret = cmsObj_deleteInstance(MDMOID_LAN_IP_INTF, &lanIptfIidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to delete LanIpIntfObject, ret=%d", ret);
            }
		 }
		 cmsObj_free((void **) &lanHostCfgObj);	  
	  }
      else
         fprintf(stderr, "lan: invalid parameters %s for option 'delete'\n",argv[1]);
   }
   else if (strcasecmp(argv[0], "show") == 0)
   {  
      if(argc == 1)
      {
	     prctl_runCommandInShellBlocking("ifconfig br0");
	     prctl_runCommandInShellBlocking("ifconfig br0:0");		 	  	
      }
	  else if (argc == 2 && strcmp(argv[1], "primary") == 0)
	     prctl_runCommandInShellBlocking("ifconfig br0");
	  else if (argc == 2 && strcmp(argv[1], "secondary") == 0)
	     prctl_runCommandInShellBlocking("ifconfig br0:0");		 	  	
      else
         fprintf(stderr, "lan: invalid parameters %s for option 'show'\n",argv[1]);
   }
   else if (strcasecmp(argv[0], "--help") == 0)
   {
      cmdLanHelp(argc);
   }
   else 
   {
      printf("\nInvalid option '%s'\n", argv[0]);
      cmdLanHelp(0);
   }
   return;
}


static void cmdLanHostsHelp(void)
{
   printf("\nUsage: lanhosts show all\n");
   printf("       lanhosts show brx\n");
   printf("       lanhosts help\n");
}

static void printLanHostsHeader(void)
{
   printf("   MAC Addr          IP Addr     Lease Time Remaining    Hostname\n");
}

static void printHostsInLanDev(const InstanceIdStack *parentIidStack)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   LanHostEntryObject *entryObj = NULL;
   CmsRet ret;

   while ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_HOST_ENTRY, parentIidStack, &iidStack, (void **) &entryObj)) == CMSRET_SUCCESS)
   {
      printf("%s  %s        %d              %s\n", entryObj->MACAddress, entryObj->IPAddress, entryObj->leaseTimeRemaining, entryObj->hostName);
      cmsObj_free((void **) &entryObj);
   }
}

void processLanHostsCmd(char *cmdLine)
{
   LanDevObject *lanDevObj=NULL;
   LanIpIntfObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2;
   CmsRet ret;

   cmsLog_debug("cmdLine =>%s<=", cmdLine);

   if (!strcasecmp(cmdLine, "show all"))
   {
      while ((ret = cmsObj_getNext(MDMOID_LAN_DEV, &iidStack, (void **) &lanDevObj)) == CMSRET_SUCCESS)
      {
         cmsObj_free((void **) &lanDevObj);

         /* just use the first LAN IP INTF from each LAN Device */
         INIT_INSTANCE_ID_STACK(&iidStack2);
         if ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF, &iidStack, &iidStack2, (void **) &ipIntfObj)) != CMSRET_SUCCESS)
         {
            /* weird, each LANDevice should have at least 1 IP Intf. */
            cmsLog_error("could not find ip intf under LANDevice %s", cmsMdm_dumpIidStack(&iidStack));
         }
         else
         {
            printf("Bridge %s\n", ipIntfObj->X_BROADCOM_COM_IfName);
            cmsObj_free((void **) &ipIntfObj);

            printLanHostsHeader();
            printHostsInLanDev(&iidStack);
         }
      }
   }
   else if (!strncasecmp(cmdLine, "show br", 7))
   {
      printLanHostsHeader();
      ret = dalLan_getLanDevByBridgeIfName(&cmdLine[5], &iidStack, &lanDevObj);
      if (ret != CMSRET_SUCCESS)
      {
         printf("Could not find bridge ifName %s", &cmdLine[5]);
      }
      else
      {
         cmsObj_free((void **) &lanDevObj);
         printHostsInLanDev(&iidStack);
      }
   }
   else
   {
      cmdLanHostsHelp();
   }
}



void processPasswdCmd(char *cmdLine)
{
   CmsRet ret;
   char username[BUFLEN_32], password[BUFLEN_32];
   char *pc = NULL;
   NetworkAccessMode accessMode;
   UBOOL8 authSuccess;
   CmsSecurityLogData logData = { 0 };
   HttpLoginType authLevel = LOGIN_INVALID;   
   
   if (!strncasecmp(cmdLine, "help", 4) || !strncasecmp(cmdLine, "--help", 6))
   {
      printf("\nUsage: passwd \n");
      printf("       passwd --help\n");
      return;
   }

   username[0] = '\0';
   password[0] = '\0';
   printf("Username: ");

   // When the serial port is not configured, telnet sessions need
   // stdout to be flushed here.
   fflush(stdout);

   // Read username string, while checking for idle timeout
   if ((ret = cli_readString(username, sizeof(username))) != CMSRET_SUCCESS)
   {
      printf("read string failed\n");
      return;
   }

   pc = getpass("Password: ");
   if ( pc == NULL )
   {
      printf("Invalid password\n");
      return;
   }
   strcpy(password, pc);
   bzero(pc, strlen(pc));

   if ((ret = cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       return;
   }

   if ((ret = cmsDal_getCurrentLoginCfg(&glbWebVar)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get current password info, ret=%d", ret);
      return;
   }

   /* for verification we need to specify the network access
      based on the user who's password is being changed and not
      the user who is changing the password */
   if ( (0 == strcmp(username, glbWebVar.adminUserName)) ||
        (0 == strcmp(username, glbWebVar.usrUserName)) )
   {
      accessMode = NETWORK_ACCESS_LAN_SIDE;
   }
   else if (0 == strcmp(username, glbWebVar.sptUserName))
   {
      accessMode = NETWORK_ACCESS_WAN_SIDE;
   }
   else
   {
      printf("unrecognized username\n");
      return;
   }

   authSuccess = cmsDal_authenticate(&authLevel, accessMode, username, password);

   cmsLck_releaseLock();

   pc = NULL;
   pc = getpass("New Password: ");
   if ( pc == NULL )
   {
      printf("Invalid password\n");
      return;
   }
   strcpy(password, pc);
   bzero(pc, strlen(pc));

   pc = NULL;
   pc = getpass("Confirm New Password: ");
   if ( pc == NULL )
   {
      printf("Invalid password\n");
      return;
   }

   if ( 0 != strcmp(pc, password) )
   {
      cmsLog_security(LOG_SECURITY_PWD_CHANGE_FAIL, &logData, "New passwords do not match");
      printf("Passwords do not match\n");
      return;
   }
   bzero(pc, strlen(pc));

#ifdef SUPPORT_HASHED_PASSWORDS
   pc = cmsUtil_pwEncrypt(password, cmsUtil_cryptMakeSalt());
#else
   pc = &password[0];
#endif

   if (FALSE == authSuccess)
   {
      cmsLog_security(LOG_SECURITY_PWD_CHANGE_FAIL, &logData, "Invalid username or password - account %s", &username[0]);
      printf("Authentication failed\n");
      return;
   }

   CMSLOG_SEC_SET_APP_NAME(&logData, &currAppName[0]);
   CMSLOG_SEC_SET_USER(&logData, &currUser[0]);
   if (currAppPort != 0 )
   {
      CMSLOG_SEC_SET_PORT(&logData, currAppPort);
      CMSLOG_SEC_SET_SRC_IP(&logData, &currIpAddr[0]);
   }

   if (0 == strcmp(username, glbWebVar.adminUserName))
   {
      if ((currPerm & PERM_ADMIN) == 0)
      {
         cmsLog_security(LOG_SECURITY_PWD_CHANGE_FAIL, &logData, "Account %s", &username[0]);
         printf("You are not allowed to change admin password\n");
         return;
      }

      memset(glbWebVar.adminPassword, 0, sizeof(glbWebVar.adminPassword));
      strncpy(glbWebVar.adminPassword, pc, sizeof(glbWebVar.adminPassword));
   }
   else if (0 == strcmp(username, glbWebVar.sptUserName))
   {
      if ((currPerm & (PERM_ADMIN|PERM_SUPPORT)) == 0)
      {
         cmsLog_security(LOG_SECURITY_PWD_CHANGE_FAIL, &logData, "Account %s", &username[0]);
         printf("You are not allowed to change support password\n");
         return;
      }

      memset(glbWebVar.sptPassword, 0, sizeof(glbWebVar.sptPassword));
      strncpy(glbWebVar.sptPassword, pc, sizeof(glbWebVar.sptPassword));
   }
   else if (0 == strcmp(username, glbWebVar.usrUserName))
   {
      if ((currPerm & (PERM_ADMIN|PERM_USER)) == 0)
      {
         cmsLog_security(LOG_SECURITY_PWD_CHANGE_FAIL, &logData, "Account %s", &username[0]);
         printf("You are not allowed to change user password\n");
         return;
      }

      memset(glbWebVar.usrPassword, 0, sizeof(glbWebVar.usrPassword));
      strncpy(glbWebVar.usrPassword, pc, sizeof(glbWebVar.usrPassword));
   }
   else
   {
      printf("Invalid user name %s\n", username);
      cmsLog_security(LOG_SECURITY_PWD_CHANGE_FAIL, &logData, "Invalid user Account %s", &username[0]);
      return;
   }

   if ((ret = cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
       cmsLog_error("failed to get lock, ret=%d", ret);
       return;
   }

   if ((ret = cmsDal_setLoginCfg(&glbWebVar)) != CMSRET_SUCCESS)
   {
      printf("Could not set new password info, ret=%d\n", ret);
   }
   else
   {
      cmsLog_security(LOG_SECURITY_PWD_CHANGE_SUCCESS, &logData, "Account %s", &username[0]);
      printf("new password info set.\n");
   }

   cmsLck_releaseLock();

   return;
}


void cmdPppHelp(int argc)
{
   if ( argc <= 1 )
      fprintf(stdout,
         "\nUsage: ppp config <ppp interface name (eg. ppp0)> <up|down>\n"
         "       ppp --help\n"
         "       connect or disconnect ppp\n");
   else
      fprintf(stderr, "ppp: invalid number of parameters " \
         "for option '--help'\n");
}
void processPppCmd(char *cmdLine)
{
   const int maxOpt=5;
   SINT32 argc = 0;
   char *argv[maxOpt];
   char *last = NULL;

   /* parse the command line and build the argument vector */
   argv[0] = strtok_r(cmdLine, " ", &last);
   if (argv[0] != NULL)
   {
      for (argc = 1; argc < maxOpt; argc++)
      {
         argv[argc] = strtok_r(NULL, " ", &last);
         if (argv[argc] == NULL)
            break;
      }
   }

   if (argv[0] == NULL)
   {
      cmdPppHelp(argc);
   }
   else if (strcasecmp(argv[0], "config") == 0)
   {  
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      WanPppConnObject *pppConn = NULL;
   	SINT32 up;
   	UBOOL8 found = FALSE;
      
      if (argc != 3)
      {
         fprintf(stderr, "ppp: invalid number of parameters for option 'config'\n");
         return;	  
      }
      // ppp config <ppp interface name> <up|down>
      if (strcmp(argv[2], "up") == 0)
      {
	      up =USER_REQUEST_CONNECTION_UP;
      }
   	else if (strcmp(argv[2], "down") == 0)
   	{
   	   up = USER_REQUEST_CONNECTION_DOWN;
   	}
   	else
	   {
         fprintf(stderr, "ppp: invalid parameter");
		   return;	     
	   }
	  
      while (!found && 
         cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void **)&pppConn) == CMSRET_SUCCESS)
      {
         if (!strcmp(pppConn->X_BROADCOM_COM_IfName, argv[1]))
   		{
   		   found = TRUE;
         }
         cmsObj_free((void **)&pppConn);
      }

      if (found == FALSE)
      {
         fprintf(stderr, "ppp: can not find PPP %s\n", argv[1]);	     
      }      
		else
		{
         if (cli_sendMsgToSsk
            (CMS_MSG_REQUEST_FOR_PPP_CHANGE, up, argv[1], strlen(argv[1])+1) != CMSRET_SUCCESS)
         {
            fprintf(stderr, "Fail to send message to ssk");
         }		 
		}

   }
   else if (strcasecmp(argv[0], "--help") == 0)
   {
      cmdPppHelp(argc);
   }
   else 
   {
      printf("\nInvalid option '%s'\n", argv[0]);
   }
   return;
}


void processRestoreDefaultCmd(char *cmdLine __attribute__((unused)))
{
   cmsLog_notice("starting user requested restore to defaults and reboot");

   /* this invalidates the config flash, which will cause us to use
    * default values in MDM next time we reboot. */
   cmsMgm_invalidateConfigFlash();

   printf("\r\nThe system shell is being reset. Please wait...\r\n");
   fflush(stdout);

   cmsUtil_sendRequestRebootMsg(msgHandle);

   return;
}


void processRouteCmd(char *cmdLine)
{
   char *pToken = NULL, *pLast = NULL;;
   char argument[6][BUFLEN_32];
   int i = 0;
   UBOOL8 isGW = FALSE;
   UBOOL8 isDEV = FALSE;
   int isMetric = 0;
   CmsRet ret = CMSRET_SUCCESS;

   argument[0][0] = argument[1][0] = argument[2][0] = argument[3][0] = argument[4][0] = argument[5][0] = '\0';
   
   pToken = strtok_r(cmdLine, ", ", &pLast);
   while ( pToken != NULL ) 
   {
      if (!strcmp(pToken, "gw"))
      {
         isGW = TRUE;
      }
      else if (!strcmp(pToken, "dev"))
      {
         isDEV = TRUE;
      }
      else if (!strcmp(pToken, "metric"))
      {
         isMetric = 1;
      }
      else
      {
         strcpy(&argument[i][0], pToken);
         i++;
      }
	
      pToken = strtok_r(NULL, ", ", &pLast);
   }

   if (!strcasecmp(&argument[0][0], "help") || !strcasecmp(&argument[0][0], "--help") || argument[0][0] == '\0')
   {
      printf("Usage: route add <IP address> <subnet mask> |metric hops| <|<gw gtwy_IP>| |<dev interface>|>\n");
      printf("       route delete <IP address> <subnet mask>\n");
      printf("       route show\n");
      printf("       route help\n");
      return;
   }

   if(!strcasecmp(&argument[0][0], "show"))
   {
      FILE* fs = NULL;

      /* execute command with err output to rterr */
      prctl_runCommandInShellBlocking("route 2> /var/rterr");
	  
      fs = fopen("/var/rterr", "r");
      if (fs != NULL) 
      {
         prctl_runCommandInShellBlocking("cat /var/rterr");
         fclose(fs);
         prctl_runCommandInShellBlocking("rm /var/rterr");
      }

      return;
   }

   if(cmsUtl_isValidIpAddress(AF_INET, &argument[1][0]) == FALSE)
   {
      printf("Invalid destination IP address\n");
      return;
   }

   if(cmsUtl_isValidIpAddress(AF_INET, &argument[2][0]) == FALSE)
   {
      printf("Invalid destination subnet mask\n");
      return;
   }

   if (!strcasecmp(&argument[0][0], "add"))
   {
      if (isGW == FALSE && isDEV == FALSE) 
      {
         printf("Please at least enter gateway IP or interface\n");
         return;
      }
	   
      if ( isGW == TRUE) 
      {
         if (cmsUtl_isValidIpAddress(AF_INET, &argument[3+isMetric][0]) == FALSE) 
         {
            printf("\n Error: Invalid gateway IP address");
            return;
         }

         if ( isMetric )
         {
            ret = dalStaticRoute_addEntry(&argument[1][0], &argument[2][0], &argument[3+isMetric][0], &argument[4+isMetric][0], &argument[3][0]);
         }
         else
         {
            ret = dalStaticRoute_addEntry(&argument[1][0], &argument[2][0], &argument[3+isMetric][0], &argument[4+isMetric][0], NULL);
         }

         if ( ret != CMSRET_SUCCESS )
         {
            cmsLog_error("dalStaticRoute_addEntry failed, ret=%d", ret);
            printf("\nError happens when add the route.\n");
            return;
         }
      }
      else 
      {
         if ( isMetric )
         {
            ret = dalStaticRoute_addEntry(&argument[1][0], &argument[2][0], &argument[4+isMetric][0], &argument[3+isMetric][0], &argument[3][0]);
         }
         else
         {
            ret = dalStaticRoute_addEntry(&argument[1][0], &argument[2][0], &argument[4+isMetric][0], &argument[3+isMetric][0], NULL);
         }

         if ( ret != CMSRET_SUCCESS )
         {
            cmsLog_error("dalStaticRoute_addEntry failed, ret=%d", ret);
            printf("\nError happens when add the route.\n");
            return;
         }
      }
   }
   else if(!strcasecmp(&argument[0][0], "delete")) 
   {
      if ((ret = dalStaticRoute_deleteEntry(&argument[1][0], &argument[2][0])) != CMSRET_SUCCESS) 
      {
         cmsLog_error("dalStaticRoute_deleteEntry failed, ret=%d", ret);
         printf("\nError happens when delete the route.\n");
         return;
      }
   }
   else 
   {
      printf("Invalid route command.\n");
      return;
   }
		
   return;
}



void processSaveCmd(char *cmdLine __attribute__((unused)))
{
   CmsRet ret;

   ret = cmsMgm_saveConfigToFlash();
   if (ret != CMSRET_SUCCESS)
   {
      printf("Could not save config to flash, ret=%d\n", ret);
   }
   else
   {
      printf("config saved.\n");
   }

   return;
}


void processSwVersionCmd(char *cmdLine __attribute__((unused)))
{
   IGDDeviceInfoObject *obj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &obj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get device info object, ret=%d", ret);
      return;
   }

   if (!strcasecmp(cmdLine, "--help") || !strcasecmp(cmdLine, "-h"))
   {
      printf("usage: swversion \n");
      printf("       [-b | --buildtimestamp]\n");
      printf("       [-c | --cfe]\n");
#ifdef DMP_X_BROADCOM_COM_ADSLWAN_1
      printf("       [-d | --dsl]\n");
#endif
      printf(".......[-m | --model]\n");
#ifdef DMP_X_BROADCOM_COM_PSTNENDPOINT_1
      printf("       [-v | --voice]\n");
#endif
   }
   else if (!strcasecmp(cmdLine, "--buildtimestamp") || !strcasecmp(cmdLine, "-b"))
   {
      printf("%s\n", obj->X_BROADCOM_COM_SwBuildTimestamp);
   }
   else if (!strcasecmp(cmdLine, "--cfe") || !strcasecmp(cmdLine, "-c"))
   {
      char *start=NULL;
      if (obj->additionalSoftwareVersion)
      {
          start = strstr(obj->additionalSoftwareVersion, "CFE=");
      }
      if (start)
      {
         printf("%s\n", start+4);
      }
      else
      {
          printf("Could not find CFE version\n");
      }
   }
   else if (!strcasecmp(cmdLine, "--model") || !strcasecmp(cmdLine, "-m"))
   {
      printf("%s\n", obj->modelName);
   }
#ifdef DMP_X_BROADCOM_COM_ADSLWAN_1
   else if (!strcasecmp(cmdLine, "--dsl") || !strcasecmp(cmdLine, "-d"))
   {
      printf("%s\n", obj->X_BROADCOM_COM_DslPhyDrvVersion);
   }
#endif
#ifdef DMP_X_BROADCOM_COM_PSTNENDPOINT_1
   else if (!strcasecmp(cmdLine, "--voice") || !strcasecmp(cmdLine, "-v"))
   {
      printf("%s\n", obj->X_BROADCOM_COM_VoiceServiceVersion);
   }
#endif
   else
   {
      printf("%s\n", obj->softwareVersion);
   }

   cmsObj_free((void **) &obj);

   return;
}


void processUptimeCmd(char *cmdLine __attribute__((unused)))
{
   IGDDeviceInfoObject *obj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UINT32 s=0;
   char uptimeString[BUFLEN_512];

   ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &obj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get device info object, ret=%d", ret);
      return;
   }

   s = obj->upTime;

   cmsObj_free((void **) &obj);

   cmsTms_getDaysHoursMinutesSeconds(s, uptimeString, sizeof(uptimeString));

   printf("%s\n", uptimeString);

   return;
}
UINT32 hexStringToBinaryBuf(const char *hexStr, UINT8 *binaryBuf)
{
   UINT32 i = 0, j = 0;
   UINT32 len = strlen(hexStr);
   char tmpbuf[3];

   for (i = 0, j = 0; j < len; i++, j += 2)
   {
      tmpbuf[0] = hexStr[j];
      tmpbuf[1] = hexStr[j+1];
      tmpbuf[2] = 0;

      binaryBuf[i] = (UINT8) strtoul(tmpbuf, NULL, 16);
   }

   return len/2;
}

#define CLI_CONFIG_SIZE     65536
#define CLI_IMAGE_SIZE      6000000
#define DATA_BUFFER_SIZE    4001    // 4000 bytes for data + 1 byte for '\n'.
                                    // It should be double of DATA_BUFFER_SIZE that
                                    // is defined in hostTools/imgbin2hex/imgbin2hex.c

void processCfgUpdateCmd(char *cmdLine __attribute__((unused)))
{
    char line[DATA_BUFFER_SIZE], image[CLI_CONFIG_SIZE];
    const char *dslCpeConfig = "</DslCpeConfig>";
    UINT32 len = 0, imageSize = 0;
    CmsImageFormat format = CMS_IMAGE_FORMAT_INVALID;
    CmsRet cmsRet;
    CmsSecurityLogData logData = { 0 };

    cmsLck_releaseLock();

    CMSLOG_SEC_SET_APP_NAME(&logData, &currAppName[0]);
    CMSLOG_SEC_SET_USER(&logData, &currUser[0]);
    if (currAppPort != 0 )
    {
       CMSLOG_SEC_SET_PORT(&logData, currAppPort);
       CMSLOG_SEC_SET_SRC_IP(&logData, &currIpAddr[0]);
    }

    // initialize line and image
    memset(line, 0, DATA_BUFFER_SIZE);
    memset(image, 0, CLI_CONFIG_SIZE);

    while (fgets(line, DATA_BUFFER_SIZE, stdin) != NULL)
    {
        len = strlen(line);
        if (imageSize + len > CLI_CONFIG_SIZE)
        {
            cmsLog_error("transfered bytes %d is greater than allocated bytes %d", imageSize + len, CLI_CONFIG_SIZE);
            break;
        }
        strcat(image, line);
        imageSize += len;
        if (strstr(line, dslCpeConfig) != NULL)
            break;
        memset(line, 0, DATA_BUFFER_SIZE);
    }

    cmsLog_notice("Console has transfered %d bytes successfully before updating configuration file", imageSize);

    // validate download configuration file before activate it
    format = cmsImg_validateImage((char *)image, imageSize, msgHandle);
    if (format == CMS_IMAGE_FORMAT_XML_CFG)
    {
        cmsLog_notice("Calling cmsImg_writeImage now");
        cmsRet = cmsImg_writeImage((char *)image, imageSize, msgHandle);
        if (CMSRET_SUCCESS == cmsRet)
        {
            cmsLog_security(LOG_SECURITY_AUTH_RESOURCES, &logData, "Configuration update succeeded");
        }
        else
        {
            cmsLog_security(LOG_SECURITY_AUTH_RESOURCES, &logData, "Configuration update failed");
        }
    }
    else
    {
        cmsLog_security(LOG_SECURITY_AUTH_RESOURCES, &logData, "Configuration update failed");
        cmsLog_error("configuration file has wrong format %d, size = %d", format, imageSize);
    }

    cmsLck_acquireLock();
}

void processSwUpdateCmd(char *cmdLine __attribute__((unused)))
{
    int i = 0, j = 0, bufSize = (DATA_BUFFER_SIZE-1)/2;
    char hexStr[DATA_BUFFER_SIZE + 1];
    UBOOL8 endOfLine = FALSE;
    UINT8 buf[bufSize], image[CLI_IMAGE_SIZE];
    UINT32 len = 0, imageSize = 0;
    CmsImageFormat format = CMS_IMAGE_FORMAT_INVALID;
    CmsRet cmsRet;
    CmsSecurityLogData logData = { 0 };

    cmsLck_releaseLock();

    CMSLOG_SEC_SET_APP_NAME(&logData, &currAppName[0]);
    CMSLOG_SEC_SET_USER(&logData, &currUser[0]);
    if (currAppPort != 0 )
    {
       CMSLOG_SEC_SET_PORT(&logData, currAppPort);
       CMSLOG_SEC_SET_SRC_IP(&logData, &currIpAddr[0]);
    }

    // initialize hexStr
    memset(hexStr, 0, DATA_BUFFER_SIZE + 1);
    memset(image, 0, CLI_IMAGE_SIZE);

    while (fgets(hexStr, DATA_BUFFER_SIZE, stdin) != NULL)
    {
        if (hexStr[0] == '\n')
        {
            if (endOfLine == TRUE)
                break;
            else
                endOfLine = TRUE;
            memset(hexStr, 0, DATA_BUFFER_SIZE + 1);
            continue;
        }
        else
            endOfLine = FALSE;
        len = strlen(hexStr);
        if ((len % 2) != 0)
            hexStr[len-1] = '\0';
        bufSize = hexStringToBinaryBuf(hexStr, buf);
        if (imageSize + bufSize > CLI_IMAGE_SIZE)
        {
            cmsLog_error("transfered bytes %d is greater than allocated bytes %d", imageSize + bufSize, CLI_IMAGE_SIZE);
            break;
        }
        for (i = 0, j = imageSize; i < bufSize; i++, j++)
            image[j] = buf[i];
        imageSize += bufSize;
        // initialize hexStr
        memset(hexStr, 0, DATA_BUFFER_SIZE + 1);
        memset(buf, 0, (DATA_BUFFER_SIZE-1) / 2);
    }

    cmsLog_notice("Console has transfered %d bytes successfully before updating image", imageSize);

    // validate download image again before activate it
    format = cmsImg_validateImage((char *)image, imageSize, msgHandle);
    if (format == CMS_IMAGE_FORMAT_BROADCOM ||
        format == CMS_IMAGE_FORMAT_FLASH)
    {
        cmsLog_notice("Calling cmsImg_write image now");
        cmsRet = cmsImg_writeImage((char *)image, imageSize, msgHandle);
        if (CMSRET_SUCCESS == cmsRet)
        {
            cmsLog_security(LOG_SECURITY_SOFTWARE_MOD, &logData, "Software update succeeded");
        }
        else
        {
            cmsLog_security(LOG_SECURITY_SOFTWARE_MOD, &logData, "Software update failed");
        }
    }
    else
    {
        cmsLog_security(LOG_SECURITY_SOFTWARE_MOD, &logData, "Software update failed");
        cmsLog_error("image has wrong format %d, size = %d", format, imageSize);
    }

    cmsLck_acquireLock();
}

void processExitOnIdleCmd(char *cmdLine)
{
   if (!strncasecmp(cmdLine, "get", 3))
   {
      printf("current timout is %d seconds\n", exitOnIdleTimeout);
   }
   else if (!strncasecmp(cmdLine, "set", 3))
   {
      exitOnIdleTimeout = atoi(&(cmdLine[4]));
      printf("timeout is set to %d seconds (for this session only, not saved to config)\n", exitOnIdleTimeout);
   }
   else
   {
      printf("usage: exitOnIdle get\n");
      printf("       exitOnIdle set <seconds>\n\n");
   }

   return;
}


#ifdef DMP_X_ITU_ORG_GPON_1
void processShowOmciStatsCmd(char *cmdLine __attribute__((unused)))
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    GponOmciStatsObject *obj;

    ret = cmsObj_get(MDMOID_GPON_OMCI_STATS, &iidStack, 0, (void **) &obj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not get OMCI Statitics object, ret=%d", ret);
    }
    else
    {
        printf("\nGPON OMCI Statistics\n\n");
        printf("rxPackets : %d\n", obj->rxGoodPackets);
        printf("rxErrors  : %d\n",
               obj->rxLengthErrors +
               obj->rxCrcErrors +
               obj->rxOtherErrors);
        printf("   rxLengthErrors : %d\n", obj->rxLengthErrors);
        printf("   rxCrcErrors    : %d\n", obj->rxCrcErrors);
        printf("   rxOtherErrors  : %d\n\n", obj->rxOtherErrors);
        printf("txPackets : %d\n",
               obj->txAvcPackets +
               obj->txResponsePackets +
               obj->txRetransmissions);
        printf("   txAttrValChg      : %d\n", obj->txAvcPackets);
        printf("   txResponses       : %d\n", obj->txResponsePackets);
        printf("   txRetransmissions : %d\n", obj->txRetransmissions);
        printf("txErrors  : %d\n", obj->txErrors);
        printf("\n");

        cmsObj_free((void **) &obj);
    }
}

const char* omciEnetArray[] = {
"\n",                                                                                     // Line 0
"  *********************\n",                                                              // Line 1
"  * PPTP Ethernet UNI *---- Administrative State:                \n",                    // Line 2
"  * ME:               *---- Operational State:               \n",                        // Line 3
"  *********************\n"                                                               // Line 4
};


const char* omciEnetArray_ENET_PM[] = {
"            |\n",                                                                        // Line 0
"            |            ***************   *****************************\n",             // Line 1
"            |------------* Enet PM     *---* Threshold 1 * Threshold 2 *\n",             // Line 2
"            |            * ME:         *   * ME:         * ME:         *\n",             // Line 3
"            |            ***************   *****************************\n",             // Line 4
"            |\n",                                                                        // Line 5
"            |                                Statistic     Threshold\n",                 // Line 6
"            |\n",                                                                        // Line 7
"            |      FCS errors                                               \n",         // Line 8
"            |      Excessive collision                                      \n",         // Line 9
"            |      Late collision                                           \n",         // Line 10
"            |      Frames too long                                          \n",         // Line 11
"            |      Buffer overflows RX                                      \n",         // Line 12
"            |      Buffer overflows TX                                      \n",         // Line 13
"            |      Single collision                                         \n",         // Line 14
"            |      Multiple collisions                                      \n",         // Line 15
"            |      SQE counter                                              \n",         // Line 16
"            |      Deferred trans                                           \n",         // Line 17
"            |      Internal MAC TX error                                    \n",         // Line 18
"            |      Carrier sense error                                      \n",         // Line 19
"            |      Alignment error                                          \n",         // Line 20
"            |      Internal MAC RX error                                    \n",         // Line 21
"            |\n"                                                                         // Line 22
};


const char* omciEnetArray_ENET2_PM[] = {
"            |\n",                                                                        // Line 0
"            |            ***************   ***************\n",                           // Line 1
"            |------------* Enet2 PM    *---* Threshold 1 *\n",                           // Line 2
"            |            * ME:         *   * ME:         *\n",                           // Line 3
"            |            ***************   ***************\n",                           // Line 4
"            |\n",                                                                        // Line 5
"            |                                Statistic     Threshold\n",                 // Line 6
"            |\n",                                                                        // Line 7
"            |      PPPoE filtered frame                                     \n",         // Line 8
"            |\n"                                                                         // Line 9
};


const char* omciEnetArray_ENET3_PM[] = {
"            |\n",                                                                        // Line 0
"            |            ***************   ***************\n",                           // Line 1
"            |------------* Enet3 PM    *---* Threshold 1 *\n",                           // Line 2
"                         * ME:         *   * ME:         *\n",                           // Line 3
"                         ***************   ***************\n",                           // Line 4
"\n",                                                                                     // Line 5
"                                             Statistic     Threshold\n",                 // Line 6
"\n",                                                                                     // Line 7
"                   Drop events                                              \n",         // Line 8
"                   Octets                                  \n",                          // Line 9
"                   Packets                                 \n",                          // Line 10
"                   Broadcast Packets                       \n",                          // Line 11
"                   Multicast Packets                       \n",                          // Line 12
"                   Undersize packets                                        \n",         // Line 13
"                   Fragments                                                \n",         // Line 14
"                   Jabbers                                                  \n",         // Line 15
"                   Packets 64 Octets                       \n",                          // Line 16
"                   Packets 64-127 Octets                   \n",                          // Line 17
"                   Packets 128-255 Octets                  \n",                          // Line 18
"                   Packets 256-511 Octets                  \n",                          // Line 19
"                   Packets 512-1023 Octets                 \n",                          // Line 20
"                   Packets 1024-1518 Octets                \n",                          // Line 21
"\n"                                                                                      // Line 22
};


const char* omciGemArray[] = {
"\n",                                                                                     // Line 0
"  ************************\n",                                                           // Line 1
"  * GEM Port Network CTP *\n",                                                           // Line 2
"  * ME:                  *\n",                                                           // Line 3
"  ************************\n",                                                           // Line 4
"              |\n",                                                                      // Line 5
"              |            ***************   ***************\n",                         // Line 6
"              |------------* GEM Port PM *---* Threshold 1 *\n",                         // Line 7
"                           * ME:         *   * ME:         *\n",                         // Line 8
"                           ***************   ***************\n",                         // Line 9
"\n",                                                                                     // Line 10
"                                             Statistic     Threshold\n",                 // Line 11
"\n",                                                                                     // Line 12
"                   Lost packets                                                    \n",  // Line 13
"                   Misinserted packets                                             \n",  // Line 14
"                   Received packets                                                \n",  // Line 15
"                   Received blocks                                                 \n",  // Line 16
"                   Transmitted blocks                                              \n",  // Line 17
"                   Impaired block                                                  \n",  // Line 18
"\n"                                                                                      // Line 19
};


const char* omciVoiceArray[] = {
"\n",                                                                                     // Line 0
"      ***************    ***************\n",                                             // Line 1
"      * POTS UNI    *----* RTP PM      *\n",                                             // Line 2
"      * ME:         *    * ME:         *\n",                                             // Line 3
"      ***************    ***************\n",                                             // Line 4
"        |    |    |                           ******************\n",                     // Line 5
"        |    |    |---------------------------* GEM Interwork  *\n",                     // Line 6
"        |    |                                * ME:            *\n",                     // Line 7
"        |    |--------------------|           ******************\n",                     // Line 8
"        |                         |\n",                                                  // Line 9
"    ******************        *********************--Auth ME:        \n",                // Line 10
"    * Voip Voice CTP *--------* SIP User Data     *--AoR ME:        \n",                 // Line 11
"    * ME:            *        * ME:               *--Net Addr ME:        \n",            // Line 12
"    ******************        *********************\n",                                  // Line 13
"        |                         |     |    |\n",                                       // Line 14
" ******************   ****************  |  ***************\n",                           // Line 15
" * Voip Media Pro *   * VoIP Feature *  |  * VoIP Applic *--Direct ME:      \n",         // Line 16
" * ME:            *   * Access Codes *  |  * Serv Prof   *--Bridged ME:      \n",        // Line 17
" ******************   * ME:          *  |  * ME:         *--Conf ME:      \n",           // Line 18
"    |            |    ****************  |  ***************\n",                           // Line 19
"    |            |                      |\n",                                            // Line 20
"    |   ******************    ***************--SIP Proxy ME:        \n",                 // Line 21
"    |   * RTP Prof Data  *    * SIP Agent   *--SIP Outbound ME:        \n",              // Line 22
"    |   * ME:            *    * Config Data *--SIP Registrar ME:        \n",             // Line 23
"    |   ******************    * ME:         *--TCP/UDP ME:        \n",                   // Line 24
"    |                         ***************--Host Part URI ME:        \n",             // Line 25
"  ******************\n",                                                                 // Line 26
"  * Voice Serv Pro *               *********************\n",                             // Line 27
"  * ME:            *               * VoIP Config Data  *--Net Addr ME:        \n",       // Line 28
"  ******************               * ME:               *\n",                             // Line 29
"                                   *********************\n",                             // Line 30
"\n"
};

#define MAX_ADD_LINES 100
#define MAX_LINE_CHARS 80
#define MAX_LARGE_STRING_PARTS 15

static void AddOutputText(char* lineArray[], char* outputStr, int strLength, int vertOffset, int horzOffset)
{
  char* horzLinePtr = lineArray[vertOffset];

  // Copy outputStr into diagram.
  memcpy(&horzLinePtr[horzOffset], outputStr, strLength);
}


static void AddAdditionalText(char* lineArray[], char* outputStr)
{
  int loopIndex;

  // Loop thru additional line array to append this line.
  for (loopIndex = 0;loopIndex < MAX_ADD_LINES;loopIndex++)
  {
    // Test for empty line.
    if (lineArray[loopIndex] == NULL)
    {
      // Allocate temp string memory.
      lineArray[loopIndex] = (char*)malloc(MAX_LINE_CHARS);

      // Test for success
      if (lineArray[loopIndex] != NULL)
      {
        // Copy temp line into additional line array.
        strcpy(lineArray[loopIndex], outputStr);
      }

      // Done.
      break;
    }
  }
}


static int FormString(LargeStringObject* largeStrObjPtr, char* outputStr)
{
  int totalStrLen = 0;
  char** strPtrArrayPtr = &largeStrObjPtr->part1;
  int strPartCount = largeStrObjPtr->numberOfParts;
  char* tempCharPtr;

  // Loop until all parts are processed.
  while (strPartCount > 0)
  {
    // Setup part's string pointer.
    tempCharPtr = *strPtrArrayPtr++;

    // Test for valid string.
    if ((tempCharPtr != NULL) && (isascii(tempCharPtr[0]) != 0))
    {
      // Find & add part's string length.
      totalStrLen += strlen(tempCharPtr);

      // Test for valid output character array.
      if (outputStr != NULL)
      {
        // Add part's string to output character array.
        strcat(outputStr, tempCharPtr);
      }
    }
    else
    {
      // Done due to invalid pointer or characters.
      break;
    }

    // Dec object's string path count.
    strPartCount--;
  }

  // Return total string length.
  return totalStrLen;
}


static char* FormStringFromObject(LargeStringObject* largeStrObjPtr)
{
  char* strPtr = NULL;
  int totalStrLen;

  // Test for valid part.
  if ((largeStrObjPtr->numberOfParts > 0) && (largeStrObjPtr->numberOfParts < MAX_LARGE_STRING_PARTS))
  {
    // Find Large String object's total string length.
    totalStrLen = FormString(largeStrObjPtr, NULL);

    // Test for valid string length.
    if (totalStrLen > 0)
    {
      // Allocate temporary output character array.
      strPtr = (char*)malloc(totalStrLen + 10);

      // Test for valid output character array.
      if (strPtr != NULL)
      {
        // Init string
        *strPtr = 0;

        // Copy Large String object's string parts into output character array.
        FormString(largeStrObjPtr, strPtr);
      }
    }
  }

  // Return valid output character array on success, NULL on failure
  return strPtr;
}


static char* GetLargeString(UINT32 largeStrObjID)
{
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  LargeStringObject* largeStringObjPtr;
  char* resultCharPtr = NULL;
  UBOOL8 largeStringFoundFlag = FALSE;

  // Loop until object found, end-of-list, or error.
  while ((largeStringFoundFlag == FALSE) && (cmsObj_getNext(MDMOID_LARGE_STRING, &iidStack, (void**)&largeStringObjPtr) == CMSRET_SUCCESS))
  {
    // Test for requested MDMOID_LARGE_STRING object.
    if (largeStringObjPtr->managedEntityId == largeStrObjID)
    {
      // Attempt to form string from requested MDMOID_LARGE_STRING object.
      resultCharPtr = FormStringFromObject(largeStringObjPtr);

      // Signal object found.
      largeStringFoundFlag = TRUE;
    }

    // Free MDMOID_LARGE_STRING object.
    cmsObj_free((void**)&largeStringObjPtr);
  }

  // Return specified string on success, NULL on failure.
  return resultCharPtr;
}


static void AddME_RtpPM(char* lineArray[], UINT16 pptpPotsMeID)
{
  RtpPmHistoryDataObject* rtpPmPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_RTP_PM_HISTORY_DATA looking for specified voice line.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_RTP_PM_HISTORY_DATA, &iidStack, (void**)&rtpPmPtr) == CMSRET_SUCCESS))
  {
    // Test for specified line MDMOID_RTP_PM_HISTORY_DATA (implicitly linked to PPTP POTS ME).
    if (rtpPmPtr->managedEntityId == pptpPotsMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", rtpPmPtr->managedEntityId);

      // Add RTP PM ME text.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 3, 31);

      // Signal specified RTP PM ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_RTP_PM_HISTORY_DATA object.
    cmsObj_free((void**)&rtpPmPtr);
  }
}


static void AddME_RtpProfileData(char* lineArray[], UINT16 rtpProfileDataMeID)
{
  RtpProfileDataObject* rtpProfileDataPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_RTP_PROFILE_DATA looking for MDMOID_RTP_PROFILE_DATA ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_RTP_PROFILE_DATA, &iidStack, (void**)&rtpProfileDataPtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_RTP_PROFILE_DATA.
    if (rtpProfileDataPtr->managedEntityId == rtpProfileDataMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", rtpProfileDataPtr->managedEntityId);

      // Add RTP Profile Data ME text.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 23, 14);

      // Signal specified RTP PM ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_RTP_PROFILE_DATA object.
    cmsObj_free((void**)&rtpProfileDataPtr);
  }
}


static void AddME_VoiceServiceProfile(char* lineArray[], UINT16 voiceServiceProfileMeID)
{
  VoiceServiceObject* voiceServicePtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_VOICE_SERVICE looking for specified MDMOID_VOICE_SERVICE ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_VOICE_SERVICE, &iidStack, (void**)&voiceServicePtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_VOICE_SERVICE.
    if (voiceServicePtr->managedEntityId == voiceServiceProfileMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", voiceServicePtr->managedEntityId);

      // Add Voice Service Profile ME text.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 28, 8);

      // Signal specified MDMOID_VOICE_SERVICE ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_VOICE_SERVICE object.
    cmsObj_free((void**)&voiceServicePtr);
  }
}


static void AddME_VoipMediaProfile(char* lineArray[], UINT16 voipMediaProfileMeID)
{
  VoIpMediaProfileObject* voipMediaProfilePtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_VO_IP_MEDIA_PROFILE looking for MDMOID_VO_IP_MEDIA_PROFILE ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_VO_IP_MEDIA_PROFILE, &iidStack, (void**)&voipMediaProfilePtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_VO_IP_MEDIA_PROFILE.
    if (voipMediaProfilePtr->managedEntityId == voipMediaProfileMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", voipMediaProfilePtr->managedEntityId);

      // Add VOIP VOICE CTP ME text.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 17, 7);

      // Add RTP Profile Data ME text.
      AddME_RtpProfileData(lineArray, voipMediaProfilePtr->rtpProfilePointer);

      // Add Voice Service Profile ME text.
      AddME_VoiceServiceProfile(lineArray, voipMediaProfilePtr->voiceServiceProfilePointer);

      // Signal specified MDMOID_VO_IP_MEDIA_PROFILE ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_VO_IP_MEDIA_PROFILE object.
    cmsObj_free((void**)&voipMediaProfilePtr);
  }
}


static void AddME_VoipCtp(char* lineArray[], UINT16 pptpPotsMeID)
{
  VoIpVoiceCtpObject* voipVoiceCtpPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_VO_IP_VOICE_CTP looking for specified voice line.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_VO_IP_VOICE_CTP, &iidStack, (void**)&voipVoiceCtpPtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_VO_IP_VOICE_CTP that points to current PPTP POTS ME.
    if (voipVoiceCtpPtr->pptpPointer == pptpPotsMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", voipVoiceCtpPtr->managedEntityId);

      // Add VOIP VOICE CTP ME text.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 12, 10);

      // Add VOIP Media Profile ME text.
      AddME_VoipMediaProfile(lineArray, voipVoiceCtpPtr->voIpMediaProfilePointer);

      // Signal specified MDMOID_VO_IP_VOICE_CTP ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_VO_IP_VOICE_CTP object.
    cmsObj_free((void**)&voipVoiceCtpPtr);
  }
}


static void AddME_GemInterwork(char* lineArray[], PptpPotsUniObject* pptpPotsPtr)
{
  GemInterworkingTpObject* gemInterworkPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_GEM_INTERWORKING_TP looking for specified ME from PPTP POTS ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_RTP_PM_HISTORY_DATA, &iidStack, (void**)&gemInterworkPtr) == CMSRET_SUCCESS))
  {
    // Test for specified line MDMOID_GEM_INTERWORKING_TP.
    if (gemInterworkPtr->managedEntityId == pptpPotsPtr->interworkingTpPointer)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", gemInterworkPtr->managedEntityId);

      // Add ME text to PPTP POTS.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 7, 52);

      // Signal specified MDMOID_GEM_INTERWORKING_TP ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_GEM_INTERWORKING_TP object.
    cmsObj_free((void**)&gemInterworkPtr);
  }
}


static void AddME_VoipFeatureAccessCodes(char* lineArray[], UINT16 voipFeatureAccessCodesMeID)
{
  VoiceFeatureAccessCodesObject* voiceFeatureAccessCodesPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_VOICE_FEATURE_ACCESS_CODES looking for specified ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_VOICE_FEATURE_ACCESS_CODES, &iidStack, (void**)&voiceFeatureAccessCodesPtr) == CMSRET_SUCCESS))
  {
    // Test for specified line MDMOID_VOICE_FEATURE_ACCESS_CODES.
    if (voiceFeatureAccessCodesPtr->managedEntityId == voipFeatureAccessCodesMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", voiceFeatureAccessCodesPtr->managedEntityId);

      // Add ME text to MDMOID_VOICE_FEATURE_ACCESS_CODES.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 18, 28);

      // Signal specified MDMOID_VOICE_FEATURE_ACCESS_CODES ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_VOICE_FEATURE_ACCESS_CODES object.
    cmsObj_free((void**)&voiceFeatureAccessCodesPtr);
  }
}


static UBOOL8 OutputSipProxy(UINT16 largeStringMeID, char* addLineArray[])
{
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Attempt to get AoR Large String ME.
  tempCharPtr = GetLargeString(largeStringMeID);

  // Test for success finding AoR Large String ME.
  if (tempCharPtr != NULL)
  {
    // Convert ME ID to string.
    sprintf(tempCharArray, "Proxy Server Address  Large String ME: %d\n", largeStringMeID);

    // Add additional line of text for .
    AddAdditionalText(addLineArray, tempCharArray);

    // Form string
    sprintf(tempCharArray, "  Proxy Server Address: %s\n", tempCharPtr);

    // Add additional line of text for .
    AddAdditionalText(addLineArray, tempCharArray);

    // Release AoR MDMOID_LARGE_STRING ASCII-conversion string.
    free(tempCharPtr);

    // Signal specified AoR MDMOID_LARGE_STRING ME found.
    foundFlag = TRUE;
  }

  // Return TRUE if AoR MDMOID_LARGE_STRING found, FALSE if not.
  return foundFlag;
}


static UBOOL8 OutputOutboundProxy(UINT16 largeStringMeID, char* addLineArray[])
{
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Attempt to get AoR Large String ME.
  tempCharPtr = GetLargeString(largeStringMeID);

  // Test for success finding AoR Large String ME.
  if (tempCharPtr != NULL)
  {
    // Convert ME ID to string.
    sprintf(tempCharArray, "Outbound Proxy Address  Large String ME: %d\n", largeStringMeID);

    // Add additional line of text for .
    AddAdditionalText(addLineArray, tempCharArray);

    // Form string
    sprintf(tempCharArray, "  Outbound Proxy Address: %s\n", tempCharPtr);

    // Add additional line of text for .
    AddAdditionalText(addLineArray, tempCharArray);

    // Release AoR MDMOID_LARGE_STRING ASCII-conversion string.
    free(tempCharPtr);

    // Signal specified AoR MDMOID_LARGE_STRING ME found.
    foundFlag = TRUE;
  }

  // Return TRUE if AoR MDMOID_LARGE_STRING found, FALSE if not.
  return foundFlag;
}


static UBOOL8 OutputHostPartURI(UINT16 largeStringMeID, char* addLineArray[])
{
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Attempt to get AoR Large String ME.
  tempCharPtr = GetLargeString(largeStringMeID);

  // Test for success finding AoR Large String ME.
  if (tempCharPtr != NULL)
  {
    // Convert ME ID to string.
    sprintf(tempCharArray, "Host Part URI  Large String ME: %d\n", largeStringMeID);

    // Add additional line of text for .
    AddAdditionalText(addLineArray, tempCharArray);

    // Form string
    sprintf(tempCharArray, "  Host Part URI: %s\n", tempCharPtr);

    // Add additional line of text for .
    AddAdditionalText(addLineArray, tempCharArray);

    // Release AoR MDMOID_LARGE_STRING ASCII-conversion string.
    free(tempCharPtr);

    // Signal specified AoR MDMOID_LARGE_STRING ME found.
    foundFlag = TRUE;
  }

  // Return TRUE if AoR MDMOID_LARGE_STRING found, FALSE if not.
  return foundFlag;
}


static UBOOL8 OutputSipRegistrar(UINT16 netAddrMeID, char* addLineArray[])
{
  NetworkAddressObject* netAddrRecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Test for NULL pointer.
  if (netAddrMeID != 0xFFFF)
  {
    // Loop through each MDMOID_NETWORK_ADDRESS looking for specified MDMOID_NETWORK_ADDRESS ME.
    while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_NETWORK_ADDRESS, &iidStack, (void**)&netAddrRecPtr) == CMSRET_SUCCESS))
    {
      // Test for specified MDMOID_NETWORK_ADDRESS ME.
      if (netAddrRecPtr->managedEntityId == netAddrMeID)
      {
        // Attempt to get SIP Registrar Large String ME.
        tempCharPtr = GetLargeString(netAddrRecPtr->addressPointer);

        // Test for success finding SIP Registrar Large String ME.
        if (tempCharPtr != NULL)
        {
          // Convert ME ID to string.
          sprintf(tempCharArray, "SIP Registrar  Network Address ME: %d  Large String ME: %d\n", netAddrRecPtr->managedEntityId, netAddrRecPtr->addressPointer);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Form string
          sprintf(tempCharArray, "  SIP Registrar: %s\n", tempCharPtr);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Release SIP Registrar MDMOID_LARGE_STRING ASCII-conversion string.
          free(tempCharPtr);
        }

        // Signal specified MDMOID_NETWORK_ADDRESS ME found.
        foundFlag = TRUE;
      }

      // Release MDMOID_NETWORK_ADDRESS object.
      cmsObj_free((void**)&netAddrRecPtr);
    }
  }
  else
  {
    // Signal specified NULL.
    foundFlag = TRUE;
  }

  // Return TRUE if MDMOID_NETWORK_ADDRESS found (or NULL), FALSE if not.
  return foundFlag;
}


static UBOOL8 OutputTcpUdp(UINT16 tcpUdpMeID, char* addLineArray[])
{
  TcpUdpConfigDataObject* tcpUdpRecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Test for NULL pointer.
  if (tcpUdpMeID != 0xFFFF)
  {
    // Loop through each MDMOID_TCP_UDP_CONFIG_DATA looking for specified MDMOID_TCP_UDP_CONFIG_DATA ME.
    while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_TCP_UDP_CONFIG_DATA, &iidStack, (void**)&tcpUdpRecPtr) == CMSRET_SUCCESS))
    {
      // Test for specified MDMOID_TCP_UDP_CONFIG_DATA ME.
      if (tcpUdpRecPtr->managedEntityId == tcpUdpMeID)
      {
        // Convert ME ID to string.
        sprintf(tempCharArray, "TCP/UDP  ME: %d\n", tcpUdpRecPtr->managedEntityId);

        // Add additional line of text for .
        AddAdditionalText(addLineArray, tempCharArray);

        // Form string
        sprintf(tempCharArray, "  IP Host: %d\n", tcpUdpRecPtr->ipHostPointer);

        // Add additional line of text for .
        AddAdditionalText(addLineArray, tempCharArray);

        // Signal specified MDMOID_NETWORK_ADDRESS ME found.
        foundFlag = TRUE;
      }

      // Release MDMOID_TCP_UDP_CONFIG_DATA object.
      cmsObj_free((void**)&tcpUdpRecPtr);
    }
  }
  else
  {
    // Signal specified NULL.
    foundFlag = TRUE;
  }

  // Return TRUE if MDMOID_TCP_UDP_CONFIG_DATA found (or NULL), FALSE if not.
  return foundFlag;
}


static void AddME_SipAgentConfigData(char* lineArray[], UINT16 sipAgentConfigDataMeID, char* addLineArray[])
{
  SipAgentConfigDataObject* sipAgentConfigDataPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_SIP_AGENT_CONFIG_DATA looking for specified ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_SIP_AGENT_CONFIG_DATA, &iidStack, (void**)&sipAgentConfigDataPtr) == CMSRET_SUCCESS))
  {
    // Test for specified line MDMOID_SIP_AGENT_CONFIG_DATA.
    if (sipAgentConfigDataPtr->managedEntityId == sipAgentConfigDataMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", sipAgentConfigDataPtr->managedEntityId);

      // Add ME text to MDMOID_SIP_AGENT_CONFIG_DATA.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 24, 36);

      // Output additional info for SIP Proxy Address.
      if (OutputSipProxy(sipAgentConfigDataPtr->proxyServerAddressPointer, addLineArray) == TRUE)
      {
        // Convert ME ID to string.
        sprintf(tempCharArray, "%d", sipAgentConfigDataPtr->proxyServerAddressPointer);

        // Add ME text to MDMOID_SIP_AGENT_CONFIG_DATA.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 21, 61);
      }

      // Output additional info for SIP Outbound Proxy.
      if (OutputOutboundProxy(sipAgentConfigDataPtr->outboundProxyAddressPointer, addLineArray) == TRUE)
      {
        // Convert ME ID to string.
        sprintf(tempCharArray, "%d", sipAgentConfigDataPtr->outboundProxyAddressPointer);

        // Add ME text to MDMOID_SIP_AGENT_CONFIG_DATA.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 22, 64);
      }

      // Output additional info for SIP Registrar.
      if (OutputSipRegistrar(sipAgentConfigDataPtr->sipRegistrar, addLineArray) == TRUE)
      {
        // Convert ME ID to string.
        sprintf(tempCharArray, "%d", sipAgentConfigDataPtr->sipRegistrar);

        // Add ME text to MDMOID_SIP_AGENT_CONFIG_DATA.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 23, 65);
      }

      // Output additional info for TCP/UDP.
      if (OutputTcpUdp(sipAgentConfigDataPtr->tcpUdpPointer, addLineArray) == TRUE)
      {
        // Convert ME ID to string.
        sprintf(tempCharArray, "%d", sipAgentConfigDataPtr->tcpUdpPointer);

        // Add ME text to MDMOID_SIP_AGENT_CONFIG_DATA.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 24, 59);
      }

      // Output additional info for Host Part URI.
      if ((sipAgentConfigDataPtr->hostPartUri == 0xFFFF) || (OutputHostPartURI(sipAgentConfigDataPtr->hostPartUri, addLineArray) == TRUE))
      {
        // Convert ME ID to string.
        sprintf(tempCharArray, "%d", sipAgentConfigDataPtr->hostPartUri);

        // Add ME text to MDMOID_SIP_AGENT_CONFIG_DATA.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 25, 65);
      }

      // Signal specified MDMOID_SIP_AGENT_CONFIG_DATA ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_SIP_AGENT_CONFIG_DATA object.
    cmsObj_free((void**)&sipAgentConfigDataPtr);
  }
}


static UBOOL8 OutputDirectConnectURI(UINT16 netAddrMeID, char* addLineArray[])
{
  NetworkAddressObject* netAddrRecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Test for NULL pointer.
  if (netAddrMeID != 0xFFFF)
  {
    // Loop through each MDMOID_NETWORK_ADDRESS looking for specified MDMOID_NETWORK_ADDRESS ME.
    while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_NETWORK_ADDRESS, &iidStack, (void**)&netAddrRecPtr) == CMSRET_SUCCESS))
    {
      // Test for specified MDMOID_NETWORK_ADDRESS ME.
      if (netAddrRecPtr->managedEntityId == netAddrMeID)
      {
        // Attempt to get SIP Registrar Large String ME.
        tempCharPtr = GetLargeString(netAddrRecPtr->addressPointer);

        // Test for success finding Large String ME.
        if (tempCharPtr != NULL)
        {
          // Convert ME ID to string.
          sprintf(tempCharArray, "Direct Connect URI  Network Address ME: %d  Large String ME: %d\n", netAddrRecPtr->managedEntityId, netAddrRecPtr->addressPointer);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Form string
          sprintf(tempCharArray, "  Direct Connect URI: %s\n", tempCharPtr);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Release SIP Registrar MDMOID_LARGE_STRING ASCII-conversion string.
          free(tempCharPtr);
        }

        // Signal specified MDMOID_NETWORK_ADDRESS ME found.
        foundFlag = TRUE;
      }

      // Release MDMOID_NETWORK_ADDRESS object.
      cmsObj_free((void**)&netAddrRecPtr);
    }
  }
  else
  {
    // Signal specified NULL.
    foundFlag = TRUE;
  }

  // Return TRUE if MDMOID_NETWORK_ADDRESS found (or NULL), FALSE if not.
  return foundFlag;
}


static UBOOL8 OutputBridgedURI(UINT16 netAddrMeID, char* addLineArray[])
{
  NetworkAddressObject* netAddrRecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Test for NULL pointer.
  if (netAddrMeID != 0xFFFF)
  {
    // Loop through each MDMOID_NETWORK_ADDRESS looking for specified MDMOID_NETWORK_ADDRESS ME.
    while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_NETWORK_ADDRESS, &iidStack, (void**)&netAddrRecPtr) == CMSRET_SUCCESS))
    {
      // Test for specified MDMOID_NETWORK_ADDRESS ME.
      if (netAddrRecPtr->managedEntityId == netAddrMeID)
      {
        // Attempt to get SIP Registrar Large String ME.
        tempCharPtr = GetLargeString(netAddrRecPtr->addressPointer);

        // Test for success finding Large String ME.
        if (tempCharPtr != NULL)
        {
          // Convert ME ID to string.
          sprintf(tempCharArray, "Bridged Line Agent URI  Network Address ME: %d  Large String ME: %d\n", netAddrRecPtr->managedEntityId, netAddrRecPtr->addressPointer);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Form string
          sprintf(tempCharArray, "  Bridged Line Agent URI: %s\n", tempCharPtr);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Release SIP Registrar MDMOID_LARGE_STRING ASCII-conversion string.
          free(tempCharPtr);
        }

        // Signal specified MDMOID_NETWORK_ADDRESS ME found.
        foundFlag = TRUE;
      }

      // Release MDMOID_NETWORK_ADDRESS object.
      cmsObj_free((void**)&netAddrRecPtr);
    }
  }
  else
  {
    // Signal specified NULL.
    foundFlag = TRUE;
  }

  // Return TRUE if MDMOID_NETWORK_ADDRESS found (or NULL), FALSE if not.
  return foundFlag;
}


static UBOOL8 OutputConfFactoryURI(UINT16 netAddrMeID, char* addLineArray[])
{
  NetworkAddressObject* netAddrRecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Test for NULL pointer.
  if (netAddrMeID != 0xFFFF)
  {
    // Loop through each MDMOID_NETWORK_ADDRESS looking for specified MDMOID_NETWORK_ADDRESS ME.
    while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_NETWORK_ADDRESS, &iidStack, (void**)&netAddrRecPtr) == CMSRET_SUCCESS))
    {
      // Test for specified MDMOID_NETWORK_ADDRESS ME.
      if (netAddrRecPtr->managedEntityId == netAddrMeID)
      {
        // Attempt to get SIP Registrar Large String ME.
        tempCharPtr = GetLargeString(netAddrRecPtr->addressPointer);

        // Test for success finding Large String ME.
        if (tempCharPtr != NULL)
        {
          // Convert ME ID to string.
          sprintf(tempCharArray, "Conference Factory URI  Network Address ME: %d  Large String ME: %d\n", netAddrRecPtr->managedEntityId, netAddrRecPtr->addressPointer);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Form string
          sprintf(tempCharArray, "  Conference Factory URI: %s\n", tempCharPtr);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Release SIP Registrar MDMOID_LARGE_STRING ASCII-conversion string.
          free(tempCharPtr);
        }

        // Signal specified MDMOID_NETWORK_ADDRESS ME found.
        foundFlag = TRUE;
      }

      // Release MDMOID_NETWORK_ADDRESS object.
      cmsObj_free((void**)&netAddrRecPtr);
    }
  }
  else
  {
    // Signal specified NULL.
    foundFlag = TRUE;
  }

  // Return TRUE if MDMOID_NETWORK_ADDRESS found (or NULL), FALSE if not.
  return foundFlag;
}


static UBOOL8 AddME_VoipAppServiceProfile(char* lineArray[], UINT16 appServiceProfileMeID, char* addLineArray[])
{
  VoIpAppServiceProfileObject* voIpAppServiceProfilePtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_VO_IP_APP_SERVICE_PROFILE looking for specified MDMOID_VO_IP_APP_SERVICE_PROFILE ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_VO_IP_APP_SERVICE_PROFILE, &iidStack, (void**)&voIpAppServiceProfilePtr) == CMSRET_SUCCESS))
  {
    // Test for specified line MDMOID_VO_IP_APP_SERVICE_PROFILE.
    if (voIpAppServiceProfilePtr->managedEntityId == appServiceProfileMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", voIpAppServiceProfilePtr->managedEntityId);

      // Add ME text.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 18, 49);

      // Output VoipAppServiceProfile's Direct Connect URI MDMOID_NETWORK_ADDRESS.
      if (OutputDirectConnectURI(voIpAppServiceProfilePtr->directConnectUriPointer, addLineArray) == TRUE)
      {
        // Convert directConnectUriPointer to string.
        sprintf(tempCharArray, "%d", voIpAppServiceProfilePtr->directConnectUriPointer);

        // Add ME text.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 16, 71);
      }

      // Output VoipAppServiceProfile's Bridged URI MDMOID_NETWORK_ADDRESS.
      if (OutputBridgedURI(voIpAppServiceProfilePtr->bridgedLineAgentUriPointer, addLineArray) == TRUE)
      {
        // Convert bridgedLineAgentUriPointer to string.
        sprintf(tempCharArray, "%d", voIpAppServiceProfilePtr->bridgedLineAgentUriPointer);

        // Add ME text.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 17, 72);
      }

      // Output VoipAppServiceProfile's Conference Factory URI MDMOID_NETWORK_ADDRESS.
      if (OutputConfFactoryURI(voIpAppServiceProfilePtr->bridgedLineAgentUriPointer, addLineArray) == TRUE)
      {
        // Convert directConnectUriPointer to string.
        sprintf(tempCharArray, "%d", voIpAppServiceProfilePtr->conferenceFactoryUriPointer);

        // Add ME text.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 18, 69);
      }

      // Signal specified MDMOID_VO_IP_APP_SERVICE_PROFILE ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_VO_IP_APP_SERVICE_PROFILE object.
    cmsObj_free((void**)&voIpAppServiceProfilePtr);
  }

  // Return TRUE if MDMOID_VO_IP_APP_SERVICE_PROFILE found, FALSE if not.
  return foundFlag;
}


static UBOOL8 AddME_VoipConfigData(char* lineArray[], char* addLineArray[])
{
  VoIpConfigDataObject* voipConfigDataPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  NetworkAddressObject* netAddrRecPtr;
  InstanceIdStack iidNetAddrStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Attempt to find MDMOID_VO_IP_CONFIG_DATA.
  if (cmsObj_getNext(MDMOID_VO_IP_CONFIG_DATA, &iidStack, (void**)&voipConfigDataPtr) == CMSRET_SUCCESS)
  {
    // Convert ME ID to string.
    sprintf(tempCharArray, "%d", voipConfigDataPtr->managedEntityId);

    // Add ME text.
    AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 29, 41);

    // Convert Netword Address ME ID to string.
    sprintf(tempCharArray, "%d", voipConfigDataPtr->voIpConfigAddrPointer);

    // Add Netword Address ME text.
    AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 28, 71);

    // Test for non-NULL network address.
    if (voipConfigDataPtr->voIpConfigAddrPointer != 0xFFFF)
    {
      // Loop through each MDMOID_NETWORK_ADDRESS looking for specified MDMOID_NETWORK_ADDRESS ME.
      while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_NETWORK_ADDRESS, &iidNetAddrStack, (void**)&netAddrRecPtr) == CMSRET_SUCCESS))
      {
        // Test for specified MDMOID_NETWORK_ADDRESS ME.
        if (netAddrRecPtr->managedEntityId == voipConfigDataPtr->voIpConfigAddrPointer)
        {
          // Attempt to get Large String ME.
          tempCharPtr = GetLargeString(netAddrRecPtr->addressPointer);

          // Test for success finding Large String ME.
          if (tempCharPtr != NULL)
          {
            // Convert ME ID to string.
            sprintf(tempCharArray, "VoIP Configuration Address  Network Address ME: %d  Large String ME: %d\n", netAddrRecPtr->managedEntityId, netAddrRecPtr->addressPointer);

            // Add additional line of text for SIP Registrar.
            AddAdditionalText(addLineArray, tempCharArray);

            // Form string
            sprintf(tempCharArray, "  VoIP Configuration Address: %s\n", tempCharPtr);

            // Add additional line of text for SIP Registrar.
            AddAdditionalText(addLineArray, tempCharArray);

            // Release SIP Registrar MDMOID_LARGE_STRING ASCII-conversion string.
            free(tempCharPtr);
          }

          // Signal specified MDMOID_NETWORK_ADDRESS ME found.
          foundFlag = TRUE;
        }

        // Release MDMOID_NETWORK_ADDRESS object.
        cmsObj_free((void**)&netAddrRecPtr);
      }
    }

    // Release MDMOID_VO_IP_CONFIG_DATA object.
    cmsObj_free((void**)&voipConfigDataPtr);
  }

  return FALSE;
}


static UBOOL8 OutputAuthSecMethod(UINT16 authSecMethodMeID, char* addLineArray[])
{
  AuthenticationSecurityMethodObject* authSecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  UINT8* bin = NULL;
  UINT32 binSize;
  char userBuf1[BUFLEN_128], userBuf2[BUFLEN_64];
  char passBuf[BUFLEN_64];

  // Loop through each MDMOID_AUTHENTICATION_SECURITY_METHOD looking for specified MDMOID_AUTHENTICATION_SECURITY_METHOD ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_AUTHENTICATION_SECURITY_METHOD, &iidStack, (void**)&authSecPtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_AUTHENTICATION_SECURITY_METHOD ME.
    if (authSecPtr->managedEntityId == authSecMethodMeID)
    {
      // Clear string buffers.
      memset(userBuf1, 0, sizeof(userBuf1));
      memset(userBuf2, 0, sizeof(userBuf2));
      memset(passBuf, 0, sizeof(passBuf));

      // Convert ME ID to string.
      sprintf(tempCharArray, "Authentication Security Method: %d\n", authSecMethodMeID);

      // Add additional line of text for .
      AddAdditionalText(addLineArray, tempCharArray);

      // Convert MDM username1 string to normal ASCII C-string.
      cmsUtl_hexStringToBinaryBuf(authSecPtr->username1, &bin, &binSize);

      // Copy username string to temp buffer.
      memcpy(userBuf1, bin, binSize);

      // Release temporary CMS buffer & clear pointer (if needed).
      cmsMem_free(bin);
      bin = NULL;

      // Convert MDM username2 string to normal ASCII C-string.
      cmsUtl_hexStringToBinaryBuf(authSecPtr->username2, &bin, &binSize);

      // Copy username string to temp buffer.
      memcpy(userBuf2, bin, binSize);

      // Release temporary CMS buffer & clear pointer (if needed).
      cmsMem_free(bin);
      bin = NULL;

      // Concatenate to form username
      strcat(userBuf1, userBuf2);
  
      // Convert MDM password string to normal ASCII C-string.
      cmsUtl_hexStringToBinaryBuf(authSecPtr->password, &bin, &binSize);

      // Copy password string to temp buffer.
      memcpy(passBuf, bin, binSize);

      // Release temporary CMS buffer & clear pointer (if needed).
      cmsMem_free(bin);
      bin = NULL;

      // Form string
      sprintf(tempCharArray, "  Username: %s    Password: %s\n", userBuf1, passBuf);

      // Add additional line of text for .
      AddAdditionalText(addLineArray, tempCharArray);

      // Signal specified MDMOID_AUTHENTICATION_SECURITY_METHOD ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_AUTHENTICATION_SECURITY_METHOD object.
    cmsObj_free((void**)&authSecPtr);
  }

  // Return TRUE if MDMOID_AUTHENTICATION_SECURITY_METHOD found, FALSE if not.
  return foundFlag;
}


static UBOOL8 OutputAoR(UINT16 largeStringMeID, char* addLineArray[])
{
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Attempt to get AoR Large String ME.
  tempCharPtr = GetLargeString(largeStringMeID);

  // Test for success finding AoR Large String ME.
  if (tempCharPtr != NULL)
  {
    // Convert ME ID to string.
    sprintf(tempCharArray, "Address of Record  Large String ME: %d\n", largeStringMeID);

    // Add additional line of text for .
    AddAdditionalText(addLineArray, tempCharArray);

    // Form string
    sprintf(tempCharArray, "  AoR: %s\n", tempCharPtr);

    // Add additional line of text for .
    AddAdditionalText(addLineArray, tempCharArray);

    // Release AoR MDMOID_LARGE_STRING ASCII-conversion string.
    free(tempCharPtr);

    // Signal specified AoR MDMOID_LARGE_STRING ME found.
    foundFlag = TRUE;
  }

  // Return TRUE if AoR MDMOID_LARGE_STRING found, FALSE if not.
  return foundFlag;
}


static UBOOL8 OutputNetAddr(UINT16 netAddrMeID, char* addLineArray[])
{
  NetworkAddressObject* netAddrRecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  char* tempCharPtr;

  // Test for NULL pointer.
  if (netAddrMeID != 0xFFFF)
  {
    // Loop through each MDMOID_NETWORK_ADDRESS looking for specified MDMOID_NETWORK_ADDRESS ME.
    while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_NETWORK_ADDRESS, &iidStack, (void**)&netAddrRecPtr) == CMSRET_SUCCESS))
    {
      // Test for specified MDMOID_NETWORK_ADDRESS ME.
      if (netAddrRecPtr->managedEntityId == netAddrMeID)
      {
        // Attempt to get Large String ME.
        tempCharPtr = GetLargeString(netAddrRecPtr->addressPointer);

        // Test for success finding Large String ME.
        if (tempCharPtr != NULL)
        {
          // Convert ME ID to string.
          sprintf(tempCharArray, "Voicemail Server SIP URI  Network Address ME: %d  Large String ME: %d\n", netAddrMeID, netAddrRecPtr->addressPointer);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Form string
          sprintf(tempCharArray, "  Voicemail Server SIP URI: %s\n", tempCharPtr);

          // Add additional line of text for SIP Registrar.
          AddAdditionalText(addLineArray, tempCharArray);

          // Release SIP Registrar MDMOID_LARGE_STRING ASCII-conversion string.
          free(tempCharPtr);
        }

        // Signal specified MDMOID_NETWORK_ADDRESS ME found.
        foundFlag = TRUE;
      }

      // Release MDMOID_NETWORK_ADDRESS object.
      cmsObj_free((void**)&netAddrRecPtr);
    }
  }
  else
  {
    // Signal specified NULL.
    foundFlag = TRUE;
  }

  // Return TRUE if MDMOID_NETWORK_ADDRESS found (or NULL), FALSE if not.
  return foundFlag;
}


static void AddME_SipUserData(char* lineArray[], UINT16 pptpPotsMeID, char* addLineArray[])
{
  SipUserDataObject* sipUserDataPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;

  // Loop through each MDMOID_SIP_USER_DATA looking for specified PPTP POTS UNI ME.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_SIP_USER_DATA, &iidStack, (void**)&sipUserDataPtr) == CMSRET_SUCCESS))
  {
    // Test for specified PPTP POTS UNI ME.
    if (sipUserDataPtr->pptpPointer == pptpPotsMeID)
    {
      // Convert ME ID to string.
      sprintf(tempCharArray, "%d", sipUserDataPtr->managedEntityId);

      // Add ME text to SIP User Data.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 12, 36);

      // Output additional info for AUTH SEC ME.
      if (OutputAuthSecMethod(sipUserDataPtr->usernamePassword, addLineArray) == TRUE)
      {
        // Convert AUTH SEC ME ID to string.
        sprintf(tempCharArray, "%d", sipUserDataPtr->usernamePassword);

        // Add AUTH SEC ME ID text to SIP User Data.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 10, 62);
      }

      // Output additional info for AoR ME.
      if (OutputAoR(sipUserDataPtr->userPartAor, addLineArray) == TRUE)
      {
        // Convert AoR LargeString ID to string.
        sprintf(tempCharArray, "%d", sipUserDataPtr->userPartAor);

        // Add AoR ID text to SIP User Data.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 11, 61);
      }

      // Output additional info for Network Address ME.
      if (OutputNetAddr(sipUserDataPtr->voiceMailServerSipUri, addLineArray) == TRUE)
      {
        // Convert Network Address ID to string.
        sprintf(tempCharArray, "%d", sipUserDataPtr->voiceMailServerSipUri);

        // Add Network Address ID text to SIP User Data.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 12, 66);
      }

      // Add Voip Feature Access Codes ME text.
      AddME_VoipFeatureAccessCodes(lineArray, sipUserDataPtr->featureCodePointer);

      // Add SIP Agent Config Data ME text.
      AddME_SipAgentConfigData(lineArray, sipUserDataPtr->sipAgentPointer, addLineArray);

      // Add VoIP Application Service Profile ME text.
      AddME_VoipAppServiceProfile(lineArray, sipUserDataPtr->appServiceProfilePointer, addLineArray);

      // Signal specified RTP PM ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_SIP_USER_DATA object.
    cmsObj_free((void**)&sipUserDataPtr);
  }
}


static void DrawVoiceArray(PptpPotsUniObject* pptpPotsPtr, int voiceLineIndex)
{
  char** omciVoiceStrPtr = (char**)&omciVoiceArray;
  int hLineIndex;
  char* tempLineArray[sizeof(omciVoiceArray) / sizeof(char*)];
  char tempCharArray[100];
  char* addStrArray[MAX_ADD_LINES];

  printf("\nVoice Line: %d\n\n", voiceLineIndex);

  // Clear additional string array.
  memset(addStrArray, 0, sizeof(addStrArray));

  // Loop thru output lines & copy to temp array.
  for (hLineIndex = 0;hLineIndex < (sizeof(omciVoiceArray) / sizeof(char*));hLineIndex++)
  {
    tempLineArray[hLineIndex] = malloc(strlen(omciVoiceStrPtr[hLineIndex]) + 1);
    strcpy(tempLineArray[hLineIndex], omciVoiceStrPtr[hLineIndex]);
  }

  // Convert ME ID to string.
  sprintf(tempCharArray, "%d", pptpPotsPtr->managedEntityId);

  // Add ME text to PPTP POTS.
  AddOutputText(tempLineArray, tempCharArray, strlen(tempCharArray), 3, 12);

  // Add RTP PM ME (if found).
  AddME_RtpPM(tempLineArray, pptpPotsPtr->managedEntityId);

  // Add GEM Interworking TP ME (if found).
  AddME_GemInterwork(tempLineArray, pptpPotsPtr);

  // Add VoIP Voice CTP ME (if found).
  AddME_VoipCtp(tempLineArray, pptpPotsPtr->managedEntityId);

  // Add SIP User Data ME (if found).
  AddME_SipUserData(tempLineArray, pptpPotsPtr->managedEntityId, addStrArray);

  // Add Voip Config Data ME (if found).
  AddME_VoipConfigData(tempLineArray, addStrArray);

  // Loop thru output lines.
  for (hLineIndex = 0;hLineIndex < (sizeof(omciVoiceArray) / sizeof(char*));hLineIndex++)
  {
    // Output array line.
    printf("%s", tempLineArray[hLineIndex]);

    // Release temporary output array line.
    free(tempLineArray[hLineIndex]);
  }

  // Loop thru additional output lines.
  for (hLineIndex = 0;hLineIndex < (sizeof(addStrArray) / sizeof(char*));hLineIndex++)
  {
    // Test for valid output line.
    if (addStrArray[hLineIndex] != NULL)
    {
      // Output array line.
      printf("%s", addStrArray[hLineIndex]);

      // Release temporary output array line.
      free(addStrArray[hLineIndex]);
    }
    else
    {
      // Done.
      break;
    }
  }
}


void processDumpOmciVoiceCmd(char* cmdLine)
{
  int lineIndex = 0;
  int voiceLineIndex = 0;
  PptpPotsUniObject* pptpPotsPtr;
  InstanceIdStack iidPots = EMPTY_INSTANCE_ID_STACK;

  // Test for valid command line arguments.
  if (*cmdLine != 0)
  {
    // Test for first line.
    if ((*cmdLine >= '1') && (*cmdLine <= '9'))
    {
      // Convert to line number.
      lineIndex = atoi(cmdLine);
    }
    else
    {
      // Invalidate voice line index.
      lineIndex = -1;

      // Print error message.
      printf("**** dumpOmciVoice  cmdLine: %s  line index out of range <1 - 9>. ****\n", cmdLine);
    }
  }

  // Test for valid voice line(s).
  if (lineIndex >= 0)
  {
    // Loop through each MDMOID_PPTP_POTS_UNI looking for specified voice line.
    while (cmsObj_getNext(MDMOID_PPTP_POTS_UNI, &iidPots, (void**)&pptpPotsPtr) == CMSRET_SUCCESS)
    {
      // Test for specified line (or all lines).
      if ((lineIndex == 0) || ((voiceLineIndex + 1) == lineIndex))
      {
        // Attempt to draw specified voice line.
        DrawVoiceArray(pptpPotsPtr, voiceLineIndex + 1);
      }

      // Inc voice line index.
      voiceLineIndex++;

      // Release MDMOID_PPTP_POTS_UNI object.
      cmsObj_free((void**)&pptpPotsPtr);
    }
  }
}


static ThresholdData1Object* GetThreshold1(UINT16 thresholdPmId)
{
  ThresholdData1Object* thresholdRecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

  // Attempt to find MDMOID_THRESHOLD_DATA1.
  while (cmsObj_getNext(MDMOID_THRESHOLD_DATA1, &iidStack, (void**)&thresholdRecPtr) == CMSRET_SUCCESS)
  {
    // Test for specified MDMOID_THRESHOLD_DATA1 ME.
    if (thresholdRecPtr->managedEntityId == thresholdPmId)
    {
      // Return specified MDMOID_THRESHOLD_DATA1 ME.
      return thresholdRecPtr;
    }

    // Release MDMOID_THRESHOLD_DATA1 object.
    cmsObj_free((void**)&thresholdRecPtr);
  }

  // Failure to find specified MDMOID_THRESHOLD_DATA1 ME.
  return NULL;
}


static ThresholdData2Object* GetThreshold2(UINT16 thresholdPmId)
{
  ThresholdData2Object* thresholdRecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

  // Attempt to find MDMOID_THRESHOLD_DATA2.
  while (cmsObj_getNext(MDMOID_THRESHOLD_DATA2, &iidStack, (void**)&thresholdRecPtr) == CMSRET_SUCCESS)
  {
    // Test for specified MDMOID_THRESHOLD_DATA2 ME.
    if (thresholdRecPtr->managedEntityId == thresholdPmId)
    {
      // Return specified MDMOID_THRESHOLD_DATA2 ME.
      return thresholdRecPtr;
    }

    // Release MDMOID_THRESHOLD_DATA2 object.
    cmsObj_free((void**)&thresholdRecPtr);
  }

  // Failure to find specified MDMOID_THRESHOLD_DATA2 ME.
  return NULL;
}

#define VERT_LINE_OFFSET 12

static void DeleteVertLine(char* delLineArray[], int lineCount)
{
  int loopIndex;
  UBOOL8 foundFlag = FALSE;
  char* tempStrPtr;

  // Loop through input lines.
  for (loopIndex = 0;loopIndex < lineCount;loopIndex++)
  {
    // Setup temp string pointer.
    tempStrPtr = delLineArray[loopIndex];

    // Test for valid string.
    if (tempStrPtr != NULL)
    {
      // Test if horizontal line char found.
      if (foundFlag == TRUE)
      {
        // Replace vert line char to space.
        tempStrPtr[VERT_LINE_OFFSET] = ' ';
      }
      else
      {
        // Test for horizontal line char.
        if (tempStrPtr[VERT_LINE_OFFSET + 1] == '-')
        {
          // Set horizontal line char found flag.
          foundFlag = TRUE;
        }
      }
    }
    else
    {
      // Done.
      break;
    }
  }
}



#define MAX_STAT_COUNT 20
#define MAX_ENET_STAT 14
#define MAX_ENET2_STAT 1
#define MAX_ENET3_STAT 14
#define MAX_GEM_STAT 6


static UBOOL8 GetCurrentStats(UINT16 meClass, UINT16 meID, UINT32 statDestArray[])
{
  CmsRet cmsResult = CMSRET_SUCCESS;
  CmsMsgHeader cmdMsgBuffer = {0};
  char replyMsgBuffer[sizeof(CmsMsgHeader) + (sizeof(UINT32) * MAX_STAT_COUNT)] = {0};
  CmsMsgHeader* replyMsgPtr = (CmsMsgHeader*)replyMsgBuffer;
  UBOOL8 result = FALSE;
  UINT32* msgStatPtr = (UINT32*)replyMsgPtr;

  // Clear destination stats.
  memset(statDestArray, 0, (sizeof(UINT32) * MAX_STAT_COUNT));

  // Setup CMS_MSG_OMCIPMD_ALARM_SEQ_GET command.
  cmdMsgBuffer.type = CMS_MSG_OMCIPMD_GET_STATS;
  cmdMsgBuffer.src = EID_CONSOLED;
  cmdMsgBuffer.dst = EID_OMCIPMD;
  cmdMsgBuffer.flags_request = 1;
  cmdMsgBuffer.dataLength = 0;
  cmdMsgBuffer.wordData = (meClass << 16) | meID;

  // Attempt to send CMS response message & test result.
  cmsResult = cmsMsg_sendAndGetReplyBuf(msgHandle, (CmsMsgHeader*)&cmdMsgBuffer, (CmsMsgHeader**)&replyMsgPtr);
  if (cmsResult != CMSRET_SUCCESS)
  {
    // Log error.
    cmsLog_error("Send response message failure, cmsResult: %d", cmsResult);
  }
  else
  {
    // Copy stats from CMS message to caller's array.
    statDestArray[0] = msgStatPtr[7];
    statDestArray[1] = msgStatPtr[8];
    statDestArray[2] = msgStatPtr[9];
    statDestArray[3] = msgStatPtr[10];
    statDestArray[4] = msgStatPtr[11];
    statDestArray[5] = msgStatPtr[12];
    statDestArray[6] = msgStatPtr[13];
    statDestArray[7] = msgStatPtr[14];
    statDestArray[8] = msgStatPtr[15];
    statDestArray[9] = msgStatPtr[16];
    statDestArray[10] = msgStatPtr[17];
    statDestArray[11] = msgStatPtr[18];
    statDestArray[12] = msgStatPtr[19];
    statDestArray[13] = msgStatPtr[20];

    // Signal success.
    result = TRUE;
  }

  // Return TRUE if stats valid, FALSE if not.
  return result;
}


static void EndOutputLine(char* lineArray[], int vertOffset, int horzOffset, int vertCount)
{
  char* horzLinePtr;
  int lineIndex;

  // Loop through line array.
  for (lineIndex = 0;lineIndex < vertCount;lineIndex++)
  {
    // Get line pointer.
    horzLinePtr = lineArray[vertOffset + lineIndex];

    // End line with CR.
    horzLinePtr[horzOffset] = '\n';
    horzLinePtr[horzOffset + 1] = 0;
  }
}


static UBOOL8 AddME_ENETPM(UINT16 enetPmId, char* addLineArray[])
{
  EthernetPmHistoryDataObject* enetPmPtr;
  ThresholdData1Object* threshold1RecPtr;
  ThresholdData2Object* threshold2RecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char** omciEnetStrPtr = (char**)&omciEnetArray_ENET_PM;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  int hLineIndex;
  UINT32 statArray[MAX_STAT_COUNT];

  // Attempt to find MDMOID_ETHERNET_PM_HISTORY_DATA.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_ETHERNET_PM_HISTORY_DATA, &iidStack, (void**)&enetPmPtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_ETHERNET_PM_HISTORY_DATA ME.
    if (enetPmPtr->managedEntityId == enetPmId)
    {
      // Loop thru output lines & copy to temp array.
      for (hLineIndex = 0;hLineIndex < (sizeof(omciEnetArray_ENET_PM) / sizeof(char*));hLineIndex++)
      {
        addLineArray[hLineIndex] = malloc(strlen(omciEnetStrPtr[hLineIndex]) + 1);
        strcpy(addLineArray[hLineIndex], omciEnetStrPtr[hLineIndex]);
      }

      // Convert MDMOID_ETHERNET_PM_HISTORY_DATA ME ID to string.
      sprintf(tempCharArray, "%d", enetPmId);

      // Add ME text to Enet PM.
      AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 3, 31);

      // Get current stats from PM ME via CMS message.
      if (GetCurrentStats(MDMOID_ETHERNET_PM_HISTORY_DATA, enetPmId, statArray) == TRUE)
      {
        // Loop through stats.
        for (hLineIndex = 0;hLineIndex < MAX_ENET_STAT;hLineIndex++)
        {
          // Convert stat to string.
          sprintf(tempCharArray, "%d", statArray[hLineIndex]);

          // Add ME text to Enet PM.
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), (hLineIndex + 8), 45);
        }
      }

      // Attempt to get MDMOID_THRESHOLD_DATA1.
      threshold1RecPtr = GetThreshold1(enetPmPtr->thresholdDataId);

      // Test for success.
      if (threshold1RecPtr != NULL)
      {
        // Convert MDMOID_THRESHOLD_DATA1 ME ID to string.
        sprintf(tempCharArray, "%d", enetPmPtr->thresholdDataId);

        // Add ME text to Enet PM.
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 3, 49);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue1);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 8, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue2);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 9, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue3);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 10, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue4);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 11, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue5);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 12, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue6);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 13, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue7);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 14, 59);

        // Release MDMOID_THRESHOLD_DATA1 object.
        cmsObj_free((void**)&threshold1RecPtr);

        // Attempt to get MDMOID_THRESHOLD_DATA2.
        threshold2RecPtr = GetThreshold2(enetPmPtr->thresholdDataId);

        // Test for success.
        if (threshold2RecPtr != NULL)
        {
          // Convert MDMOID_THRESHOLD_DATA2 ME ID to string.
          sprintf(tempCharArray, "%d", enetPmPtr->thresholdDataId);

          // Add ME text to Enet PM.
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 3, 63);

          // Convert MDMOID_THRESHOLD_DATA2 value to string.
          sprintf(tempCharArray, "%d", threshold2RecPtr->thresholdValue8);
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 15, 59);

          // Convert MDMOID_THRESHOLD_DATA2 value to string.
          sprintf(tempCharArray, "%d", threshold2RecPtr->thresholdValue9);
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 16, 59);

          // Convert MDMOID_THRESHOLD_DATA2 value to string.
          sprintf(tempCharArray, "%d", threshold2RecPtr->thresholdValue10);
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 17, 59);

          // Convert MDMOID_THRESHOLD_DATA2 value to string.
          sprintf(tempCharArray, "%d", threshold2RecPtr->thresholdValue11);
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 18, 59);

          // Convert MDMOID_THRESHOLD_DATA2 value to string.
          sprintf(tempCharArray, "%d", threshold2RecPtr->thresholdValue12);
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 19, 59);

          // Convert MDMOID_THRESHOLD_DATA2 value to string.
          sprintf(tempCharArray, "%d", threshold2RecPtr->thresholdValue13);
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 20, 59);

          // Convert MDMOID_THRESHOLD_DATA2 value to string.
          sprintf(tempCharArray, "%d", threshold2RecPtr->thresholdValue14);
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 21, 59);

          // Release MDMOID_THRESHOLD_DATA2 object.
          cmsObj_free((void**)&threshold2RecPtr);
        }
        else
        {
          // End output line since Threshold_2 not present.
          EndOutputLine(addLineArray, 1, 58, 4);
        }
      }
      else
      {
        // End output line since Threshold_1 not present.
        EndOutputLine(addLineArray, 1, 40, 4);

        // End 'Threshold' header line since Threshold_1 not present.
        EndOutputLine(addLineArray, 6, 59, 1);
      }

      // Signal specified MDMOID_ETHERNET_PM_HISTORY_DATA ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_ETHERNET_PM_HISTORY_DATA object.
    cmsObj_free((void**)&enetPmPtr);
  }

  // Return TRUE if ENET PM found.
  return foundFlag;
}


static UBOOL8 AddME_ENET2PM(UINT16 enet2PmId, char* addLineArray[])
{
  EthernetPmHistoryData2Object* enet2PmPtr;
  ThresholdData1Object* threshold1RecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char** omciEnetStrPtr = (char**)&omciEnetArray_ENET2_PM;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  int hLineIndex;
  UINT32 statArray[MAX_STAT_COUNT];

  // Attempt to find MDMOID_ETHERNET_PM_HISTORY_DATA2.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_ETHERNET_PM_HISTORY_DATA2, &iidStack, (void**)&enet2PmPtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_ETHERNET_PM_HISTORY_DATA ME.
    if (enet2PmPtr->managedEntityId == enet2PmId)
    {
      // Loop thru output lines & copy to temp array.
      for (hLineIndex = 0;hLineIndex < (sizeof(omciEnetArray_ENET2_PM) / sizeof(char*));hLineIndex++)
      {
        addLineArray[hLineIndex] = malloc(strlen(omciEnetStrPtr[hLineIndex]) + 1);
        strcpy(addLineArray[hLineIndex], omciEnetStrPtr[hLineIndex]);
      }

      // Convert MDMOID_ETHERNET_PM_HISTORY_DATA2 ME ID to string.
      sprintf(tempCharArray, "%d", enet2PmId);

      // Add ME text to Enet PM.
      AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 3, 31);

      // Get current stats from PM ME via CMS message.
      if (GetCurrentStats(MDMOID_ETHERNET_PM_HISTORY_DATA2, enet2PmId, statArray) == TRUE)
      {
        // Loop through stats.
        for (hLineIndex = 0;hLineIndex < MAX_ENET2_STAT;hLineIndex++)
        {
          // Convert stat to string.
          sprintf(tempCharArray, "%d", statArray[hLineIndex]);

          // Add ME text to Enet PM.
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), (hLineIndex + 8), 45);
        }
      }

      // Attempt to get MDMOID_THRESHOLD_DATA1.
      threshold1RecPtr = GetThreshold1(enet2PmPtr->thresholdDataId);

      // Test for success.
      if (threshold1RecPtr != NULL)
      {
        // Convert MDMOID_THRESHOLD_DATA1 ME ID to string.
        sprintf(tempCharArray, "%d", enet2PmPtr->thresholdDataId);

        // Add ME text to Enet PM.
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 3, 49);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue1);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 8, 59);

        // Release MDMOID_THRESHOLD_DATA1 object.
        cmsObj_free((void**)&threshold1RecPtr);
      }
      else
      {
        // End output line since Threshold_1 not present.
        EndOutputLine(addLineArray, 1, 40, 4);

        // End 'Threshold' header line since Threshold_1 not present.
        EndOutputLine(addLineArray, 6, 59, 1);
      }

      // Signal specified MDMOID_ETHERNET_PM_HISTORY_DATA2 ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_ETHERNET_PM_HISTORY_DATA2 object.
    cmsObj_free((void**)&enet2PmPtr);
  }

  // Return TRUE if ENET2 PM found.
  return foundFlag;
}


static UBOOL8 AddME_ENET3PM(UINT16 enet3PmId, char* addLineArray[])
{
  EthernetPmHistoryData3Object* enet3PmPtr;
  ThresholdData1Object* threshold1RecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char** omciEnetStrPtr = (char**)&omciEnetArray_ENET3_PM;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  int hLineIndex;
  UINT32 statArray[MAX_STAT_COUNT];

  // Attempt to find MDMOID_ETHERNET_PM_HISTORY_DATA3.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_ETHERNET_PM_HISTORY_DATA3, &iidStack, (void**)&enet3PmPtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_ETHERNET_PM_HISTORY_DATA3 ME.
    if (enet3PmPtr->managedEntityId == enet3PmId)
    {
      // Loop thru output lines & copy to temp array.
      for (hLineIndex = 0;hLineIndex < (sizeof(omciEnetArray_ENET3_PM) / sizeof(char*));hLineIndex++)
      {
        addLineArray[hLineIndex] = malloc(strlen(omciEnetStrPtr[hLineIndex]) + 1);
        strcpy(addLineArray[hLineIndex], omciEnetStrPtr[hLineIndex]);
      }

      // Convert MDMOID_ETHERNET_PM_HISTORY_DATA3 ME ID to string.
      sprintf(tempCharArray, "%d", enet3PmId);

      // Add ME text to Enet PM.
      AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 3, 31);

      // Get current stats from PM ME via CMS message.
      if (GetCurrentStats(MDMOID_ETHERNET_PM_HISTORY_DATA3, enet3PmId, statArray) == TRUE)
      {
        // Loop through stats.
        for (hLineIndex = 0;hLineIndex < MAX_ENET3_STAT;hLineIndex++)
        {
          // Convert stat to string.
          sprintf(tempCharArray, "%d", statArray[hLineIndex]);

          // Add ME text to Enet PM.
          AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), (hLineIndex + 8), 45);
        }
      }

      // Attempt to get MDMOID_THRESHOLD_DATA1.
      threshold1RecPtr = GetThreshold1(enet3PmPtr->thresholdDataId);

      // Test for success.
      if (threshold1RecPtr != NULL)
      {
        // Convert MDMOID_THRESHOLD_DATA1 ME ID to string.
        sprintf(tempCharArray, "%d", enet3PmPtr->thresholdDataId);

        // Add ME text to Enet PM.
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 3, 49);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue1);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 8, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue2);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 13, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue3);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 14, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue4);
        AddOutputText(addLineArray, tempCharArray, strlen(tempCharArray), 15, 59);

        // Release MDMOID_THRESHOLD_DATA1 object.
        cmsObj_free((void**)&threshold1RecPtr);
      }
      else
      {
        // End output line since Threshold_1 not present.
        EndOutputLine(addLineArray, 1, 40, 4);

        // End 'Threshold' header line since Threshold_1 not present.
        EndOutputLine(addLineArray, 6, 59, 1);
      }

      // Signal specified MDMOID_ETHERNET_PM_HISTORY_DATA3 ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_ETHERNET_PM_HISTORY_DATA3 object.
    cmsObj_free((void**)&enet3PmPtr);
  }

  // Return TRUE if ENET3 PM found.
  return foundFlag;
}


static void DrawEnetArray(PptpEthernetUniObject* pptpEnetPtr, int enetLineIndex)
{
  char** omciEnetStrPtr = (char**)&omciEnetArray;
  int hLineIndex;
  char* tempLineArray[sizeof(omciEnetArray) / sizeof(char*)];
  char tempCharArray[100];
  char* addStrArray_ENET[MAX_ADD_LINES];
  char* addStrArray_ENET2[MAX_ADD_LINES];
  char* addStrArray_ENET3[MAX_ADD_LINES];
  UBOOL8 enetFlag;
  UBOOL8 enet2Flag;
  UBOOL8 enet3Flag;

  // Output header line.
  printf("\nEthernet Line: %d\n\n", enetLineIndex);

  // Clear additional string arrays.
  memset(addStrArray_ENET, 0, sizeof(addStrArray_ENET));
  memset(addStrArray_ENET2, 0, sizeof(addStrArray_ENET2));
  memset(addStrArray_ENET3, 0, sizeof(addStrArray_ENET3));

  // Loop thru output lines & copy to temp array.
  for (hLineIndex = 0;hLineIndex < (sizeof(omciEnetArray) / sizeof(char*));hLineIndex++)
  {
    tempLineArray[hLineIndex] = malloc(strlen(omciEnetStrPtr[hLineIndex]) + 1);
    strcpy(tempLineArray[hLineIndex], omciEnetStrPtr[hLineIndex]);
  }

  // Convert ME ID to string.
  sprintf(tempCharArray, "%d", pptpEnetPtr->managedEntityId);

  // Add ME text to PPTP Ethernet.
  AddOutputText(tempLineArray, tempCharArray, strlen(tempCharArray), 3, 8);

  // Test ME's Admin State.
  if (pptpEnetPtr->administrativeState == 0)
  {
    // Convert ME's unlocked Admin State to string.
    sprintf(tempCharArray, "Unlocked");
  }
  else
  {
    // Convert ME's locked Admin State to string.
    sprintf(tempCharArray, "Locked");
  }

  // Add ME's Admin State text to PPTP Ethernet.
  AddOutputText(tempLineArray, tempCharArray, strlen(tempCharArray), 2, 50);

  // Test ME's Operational State.
  if (pptpEnetPtr->operationalState == 0)
  {
    // Convert ME's enabled Operational State to string.
    sprintf(tempCharArray, "Enabled");
  }
  else
  {
    // Convert ME's disabled Operational State to string.
    sprintf(tempCharArray, "Disabled");
  }

  // Add ME's Operational State text to PPTP Ethernet.
  AddOutputText(tempLineArray, tempCharArray, strlen(tempCharArray), 3, 47);

  // Add ENET PM ME (if found).
  enetFlag = AddME_ENETPM(pptpEnetPtr->managedEntityId, addStrArray_ENET);

  // Add ENET2 PM ME (if found).
  enet2Flag = AddME_ENET2PM(pptpEnetPtr->managedEntityId, addStrArray_ENET2);

  // Add ENET3 PM ME (if found).
  enet3Flag = AddME_ENET3PM(pptpEnetPtr->managedEntityId, addStrArray_ENET3);

  // Test for valid ENET PM ME.
  if (enetFlag == TRUE)
  {
    // Test for last PM object.
    if ((enet2Flag == FALSE) && (enet3Flag == FALSE))
    {
      // Delete unused vert line.
      DeleteVertLine(addStrArray_ENET, (sizeof(addStrArray_ENET) / sizeof(char*)));
    }
  }

  // Test for valid ENET2 PM ME.
  if (enet2Flag == TRUE)
  {
    // Test for last PM object.
    if (enet3Flag == FALSE)
    {
      // Delete unused vert line.
      DeleteVertLine(addStrArray_ENET2, (sizeof(addStrArray_ENET2) / sizeof(char*)));
    }
  }


  // Loop thru output lines.
  for (hLineIndex = 0;hLineIndex < (sizeof(omciEnetArray) / sizeof(char*));hLineIndex++)
  {
    // Output array line.
    printf("%s", tempLineArray[hLineIndex]);

    // Release temporary output array line.
    free(tempLineArray[hLineIndex]);
  }

  // Loop thru additional output lines.
  for (hLineIndex = 0;hLineIndex < (sizeof(addStrArray_ENET) / sizeof(char*));hLineIndex++)
  {
    // Test for valid output line.
    if (addStrArray_ENET[hLineIndex] != NULL)
    {
      // Output array line.
      printf("%s", addStrArray_ENET[hLineIndex]);

      // Release temporary output array line.
      free(addStrArray_ENET[hLineIndex]);
    }
    else
    {
      // Done.
      break;
    }
  }

  // Loop thru additional output lines.
  for (hLineIndex = 0;hLineIndex < (sizeof(addStrArray_ENET2) / sizeof(char*));hLineIndex++)
  {
    // Test for valid output line.
    if (addStrArray_ENET2[hLineIndex] != NULL)
    {
      // Output array line.
      printf("%s", addStrArray_ENET2[hLineIndex]);

      // Release temporary output array line.
      free(addStrArray_ENET2[hLineIndex]);
    }
    else
    {
      // Done.
      break;
    }
  }

  // Loop thru additional output lines.
  for (hLineIndex = 0;hLineIndex < (sizeof(addStrArray_ENET3) / sizeof(char*));hLineIndex++)
  {
    // Test for valid output line.
    if (addStrArray_ENET3[hLineIndex] != NULL)
    {
      // Output array line.
      printf("%s", addStrArray_ENET3[hLineIndex]);

      // Release temporary output array line.
      free(addStrArray_ENET3[hLineIndex]);
    }
    else
    {
      // Done.
      break;
    }
  }

  // Add additional blank line.
  printf("\n");
}


void processDumpOmciEthernetCmd(char* cmdLine)
{
  int lineIndex = 0;
  int enetLineIndex = 0;
  PptpEthernetUniObject* pptpEnetPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

  // Test for valid command line arguments.
  if (*cmdLine != 0)
  {
    // Test for first line.
    if ((*cmdLine >= '1') && (*cmdLine <= '4'))
    {
      // Convert to line number.
      lineIndex = atoi(cmdLine);
    }
    else
    {
      // Invalidate voice line index.
      lineIndex = -1;

      // Print error message.
      printf("**** dumpOmciEnet  cmdLine: %s line index out of range <1 - 4>. ****\n", cmdLine);
    }
  }

  // Test for valid voice line(s).
  if (lineIndex >= 0)
  {
    // Loop through each MDMOID_PPTP_ETHERNET_UNI looking for specified Ethernet line.
    while (cmsObj_getNext(MDMOID_PPTP_ETHERNET_UNI, &iidStack, (void**)&pptpEnetPtr) == CMSRET_SUCCESS)
    {
      // Test for specified line (or all lines).
      if ((lineIndex == 0) || ((enetLineIndex + 1) == lineIndex))
      {
        // Attempt to draw specified voice line.
        DrawEnetArray(pptpEnetPtr, enetLineIndex + 1);
      }

      // Inc voice line index.
      enetLineIndex++;

      // Release MDMOID_PPTP_ETHERNET_UNI object.
      cmsObj_free((void**)&pptpEnetPtr);
    }
  }
}


static void AddME_GemPM(UINT16 gemPmId, char* lineArray[])
{
  GemPortPmHistoryDataObject* gemPmPtr;
  ThresholdData1Object* threshold1RecPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  char tempCharArray[100];
  UBOOL8 foundFlag = FALSE;
  UINT32 statArray[MAX_STAT_COUNT];
  unsigned long long tempReceivedPackets = 22;
  unsigned long long tempReceivedBlocks = 33;
  unsigned long long tempTransmittedBlocks = 44;

  // Attempt to find MDMOID_GEM_PORT_PM_HISTORY_DATA.
  while ((foundFlag == FALSE) && (cmsObj_getNext(MDMOID_GEM_PORT_PM_HISTORY_DATA, &iidStack, (void**)&gemPmPtr) == CMSRET_SUCCESS))
  {
    // Test for specified MDMOID_GEM_PORT_PM_HISTORY_DATA ME.
    if (gemPmPtr->managedEntityId == gemPmId)
    {
      // Convert MDMOID_GEM_PORT_PM_HISTORY_DATA ME ID to string.
      sprintf(tempCharArray, "%d", gemPmId);

      // Add ME text to Enet PM.
      AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 8, 33);

      // Get current stats from PM ME via CMS message.
      if (GetCurrentStats(MDMOID_GEM_PORT_PM_HISTORY_DATA, gemPmId, statArray) == TRUE)
      {
        // Convert lostPackets stat to string & output.
        sprintf(tempCharArray, "%d", statArray[6]);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 13, 45);

        // Convert misinsertedPackets stat to string & output.
        sprintf(tempCharArray, "%d", statArray[10]);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 14, 45);

        // Setup 64-bit RX packet stat.
        tempReceivedPackets = statArray[0];
        tempReceivedPackets <<= 32;
        tempReceivedPackets |= statArray[1];

        // Convert receivedPackets stat to string & output.
        sprintf(tempCharArray, "%llu", tempReceivedPackets);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 15, 45);

        // Setup 64-bit RX block stat.
        tempReceivedBlocks = statArray[2];
        tempReceivedBlocks <<= 32;
        tempReceivedBlocks |= statArray[3];

        // Convert receivedBlocks stat to string & output.
        sprintf(tempCharArray, "%llu", tempReceivedBlocks);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 16, 45);

        // Setup 64-bit TX block stat.
        tempTransmittedBlocks = statArray[4];
        tempTransmittedBlocks <<= 32;
        tempTransmittedBlocks |= statArray[5];

        // Convert transmittedBlocks stat to string & output.
        sprintf(tempCharArray, "%llu", tempTransmittedBlocks);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 17, 45);

        // Convert impairedBlocks stat to string & output.
        sprintf(tempCharArray, "%d", statArray[11]);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 18, 45);
      }

      // Attempt to get MDMOID_THRESHOLD_DATA1.
      threshold1RecPtr = GetThreshold1(gemPmPtr->thresholdDataId);

      // Test for success.
      if (threshold1RecPtr != NULL)
      {
        // Convert MDMOID_THRESHOLD_DATA1 ME ID to string.
        sprintf(tempCharArray, "%d", gemPmPtr->thresholdDataId);

        // Add ME text.
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 8, 51);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue1);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 13, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue2);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 14, 59);

        // Convert MDMOID_THRESHOLD_DATA1 value to string.
        sprintf(tempCharArray, "%d", threshold1RecPtr->thresholdValue3);
        AddOutputText(lineArray, tempCharArray, strlen(tempCharArray), 18, 59);

        // Release MDMOID_THRESHOLD_DATA1 object.
        cmsObj_free((void**)&threshold1RecPtr);
      }
      else
      {
        // End output line since Threshold_1 not present.
        EndOutputLine(lineArray, 6, 42, 4);

        // End 'Threshold' header line since Threshold_1 not present.
        EndOutputLine(lineArray, 11, 59, 1);
      }

      // Signal specified MDMOID_GEM_PORT_PM_HISTORY_DATA ME found.
      foundFlag = TRUE;
    }

    // Release MDMOID_GEM_PORT_PM_HISTORY_DATA object.
    cmsObj_free((void**)&gemPmPtr);
  }
}


static void DrawGemArray(GemPortNetworkCtpObject* gemCtpPtr, int gemIndex)
{
  char** omciGemStrPtr = (char**)&omciGemArray;
  int hLineIndex;
  char* tempLineArray[sizeof(omciGemArray) / sizeof(char*)];
  char tempCharArray[100];

  // Output header line.
  printf("\nGEM: %d\n\n", gemIndex);

  // Loop thru output lines & copy to temp array.
  for (hLineIndex = 0;hLineIndex < (sizeof(omciGemArray) / sizeof(char*));hLineIndex++)
  {
    // Alloc temp memory for display string.
    tempLineArray[hLineIndex] = (char*)malloc(strlen(omciGemStrPtr[hLineIndex]) + 1);

    // Copy permanent string into temp copy.
    strcpy(tempLineArray[hLineIndex], omciGemStrPtr[hLineIndex]);
  }

  // Convert ME ID to string.
  sprintf(tempCharArray, "%d", gemCtpPtr->managedEntityId);

  // Add ME text to PPTP Ethernet.
  AddOutputText(tempLineArray, tempCharArray, strlen(tempCharArray), 3, 8);

  // Add GEM PM ME & Threshold ME (if found).
  AddME_GemPM(gemCtpPtr->managedEntityId, tempLineArray);

  // Loop thru output lines.
  for (hLineIndex = 0;hLineIndex < (sizeof(omciGemArray) / sizeof(char*));hLineIndex++)
  {
    // Output array line.
    printf("%s", tempLineArray[hLineIndex]);

    // Release temporary output array line.
    free(tempLineArray[hLineIndex]);
  }

  // Add additional blank line.
  printf("\n");
}


void processDumpOmciGemCmd(char* cmdLine)
{
  int gemIndex = 0;
  int cliGemIndex = 0;
  UBOOL8 indexFlag = FALSE;
  GemPortNetworkCtpObject* gemCtpPtr;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

  // Test for valid command line arguments.
  if (*cmdLine != 0)
  {
    // Convert to GEM number.
    cliGemIndex = atoi(cmdLine);

    // Signal GEM specified.
    indexFlag = TRUE;
  }

  // Test for valid GEM(s).
  if (cliGemIndex >= 0)
  {
    // Loop through each MDMOID_GEM_PORT_NETWORK_CTP looking for specified Ethernet line.
    while (cmsObj_getNext(MDMOID_GEM_PORT_NETWORK_CTP, &iidStack, (void**)&gemCtpPtr) == CMSRET_SUCCESS)
    {
      // Test for specified GEM (or all GEMs).
      if ((indexFlag == FALSE) || (gemIndex == cliGemIndex))
      {
        // Attempt to draw specified GEM.
        DrawGemArray(gemCtpPtr, gemIndex);
      }

      // Inc GEM index.
      gemIndex++;

      // Release MDMOID_GEM_PORT_NETWORK_CTP object.
      cmsObj_free((void**)&gemCtpPtr);
    }
  }
}
#endif // #ifdef DMP_X_ITU_ORG_GPON_1

#endif /* SUPPORT_CLI_CMD */
