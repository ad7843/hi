/*************************************************
//  Copyright (C), Dare Tech. Co., Ltd.
//  File name:          fac_util.c
//  Author:             Dare Oversea Team
//  Creation Date:      2012.02.08
//  Description:        Fac command support.
**************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fac_cmd.h"
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "cms_cli.h"
#include "cms_msg.h"
#include "cms_seclog.h"
#include "cms_boardcmds.h"
#include "mdm_validstrings.h"
#include "cli.h"
//#include "../../../../shared/opensource/include/bcm963xx/dareinfo.h"
#include "prctl.h"
#include "cms_dal.h"
#include "dal_wan.h"
//#include "dare_params.h"
#include "fac_util.h"
#include "bcm_hwdefs.h"	
#include <sys/utsname.h>
//#include "cli_cmd.h"	
#if defined (CONFIG_BCM96816)   
#include <linux/sockios.h>
#include <linux/if.h>
#include "bcmnet.h"
#include <errno.h>
#endif
#include "wlapi.h"
#include "board.h"

extern int nvram_set(const char *name, const char *value);
extern char * nvram_get(const char *name);
extern int nvram_commit(void);
extern int BcmWl_GetMaxMbss(void);

//extern UBOOL8 dalCommon_setNvramMacaddr(char *macstr);
extern void processLanCmd(char *cmdLine);
/************************************** EXAMPLE **********************************************
CliRet facCmdSerialNumberGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
    strcpy(pValue,value);
    return ret;
}

CliRet facCmdSerialNumberSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    return ret;
}
**********************************************************************************************/

CliRet facCmdProduceModeGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_NOT_SUPPORT_FUNCTION;;
    return ret;
}

CliRet facCmdProduceModeSet(int argc, char** argv)
{
    CliRet ret = CLIRET_NOT_SUPPORT_FUNCTION;;
    return ret;
}

CliRet facCmdSwVersionGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#if 1//yrff
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
		return ret;
    }
    else if(cmsUtl_strcmp(argv[0],"CV")==0)
    {
   		InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   		IGDDeviceInfoObject *deviceInfo=NULL;
		if(cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT)!=CMSRET_SUCCESS)
		{
		   ret=CLIRET_NORMAL_GET_FAILURE;
		   return ret;
		}
   		else
   		{
   		    if((cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void *) &deviceInfo)) != CMSRET_SUCCESS)
   		    {
      			cmsLog_error("Could not get DEVICE_INFO");
				ret=CLIRET_NORMAL_GET_FAILURE;
				return ret;
   		    }
			else
			{
				strcpy(pValue,deviceInfo->softwareVersion);
			}
   		}
		
		cmsLck_releaseLock();
   		cmsObj_free((void **) &deviceInfo);

    }
    else if(cmsUtl_strcmp(argv[0],"DV")==0)
    {
    	FILE *fp;
		fp = fopen(SVN_INFO_FILE, "r");
		if (fp != NULL) 
		{
			fgets(pValue,100, fp);
			fclose(fp);
		}
    }
    else if(cmsUtl_strcmp(argv[0],"BV")==0)
    {
        strcpy(pValue,DARE_BASIC_VERSION);
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#endif
    return ret;
}

CliRet facCmdBaseMacGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#if 1//yrff
    if(argc != 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
		return ret;
    }
	NVRAM_DATA nvram;
	if(readNvramData(&nvram) != 0)
	{
		ret = CLIRET_NORMAL_GET_FAILURE;
		return ret;
	}
	else
	{
		sprintf(pValue, "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x \n", \
		nvram.ucaBaseMacAddr[0], nvram.ucaBaseMacAddr[1], nvram.ucaBaseMacAddr[2],\
		nvram.ucaBaseMacAddr[3], nvram.ucaBaseMacAddr[4], nvram.ucaBaseMacAddr[5]);
	}
#endif
    return ret;
}

CliRet facCmdBaseMacSet(int argc, char** argv)
{
    CliRet ret = CLIRET_REBOOT_RESET;
#if 1//yrff
    char macAddr[BUFLEN_16] = {0};
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(strlen(argv[0]) == 17)
    {
        if(argv[0][2] != ':' || argv[0][5] != ':' || argv[0][8] != ':' || argv[0][11] != ':' || argv[0][14] != ':')
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }
        else
        {
            sprintf(macAddr, "%c%c%c%c%c%c%c%c%c%c%c%c",
                    argv[0][0], argv[0][1], argv[0][3], argv[0][4], argv[0][6], argv[0][7],
                    argv[0][9], argv[0][10], argv[0][12], argv[0][13], argv[0][15], argv[0][16]);
			char *pStr = (char *)macAddr;
			int i;
			for(i = 0; i < strlen(pStr); i ++)
			{
				if(
					isxdigit(*(pStr+i))==0
				  )
				{
					ret = CLIRET_INVALIDATE_PARAM;
					return ret;
				}
			}
			char pMac[BUFLEN_4];
			pMac[0] = '\0';
			
			NVRAM_DATA nvram;
			if(readNvramData(&nvram) != 0)
			{
				ret = CLIRET_NORMAL_SET_FAILURE;
				return ret;
			}
			
			for (i = 0; i < 6; i ++) 
			{
				strncpy(pMac, pStr, 2);
				pMac[2] = '\0';
				
				nvram.ucaBaseMacAddr[i] = strtol(pMac, NULL, 16);
				pStr += 2;
				pMac[0] = '\0';
			}
			if(writeNvramData(&nvram) != 0)
            {
                ret = CLIRET_NORMAL_SET_FAILURE;
            }
        }
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#endif
    return ret;
}

CliRet facCmdSerialNumberGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
	if(argc != 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
		return ret;
    }
#ifdef SYS_MGMT_SERIAL_NUMBER	
	NVRAM_DATA nvram;
	if(readNvramData(&nvram)==0)
	{
		//zhan//strcpy(pValue, nvram.serialNo);
	}
	else
	{
		ret = CLIRET_NORMAL_SET_FAILURE;
	}
#else /* SYS_MGMT_SERIAL_NUMBER */
	ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* SYS_MGMT_SERIAL_NUMBER */
	return ret;
}

CliRet facCmdSerialNumberSet(int argc, char** argv)
{
    CliRet ret = CLIRET_REBOOT_RESET;
#if 1
#ifdef SYS_MGMT_SERIAL_NUMBER
    int i = 0;
    int len = 0;
    NVRAM_DATA nvram;
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else
    {
        len = strlen(argv[0]);
        if(len > 0 && len < 31)
        {
            for(i=0;i<strlen(argv[0]);i++)
   	     	{
   	         	if(isalnum(*(argv[0]+i))==0)
   	         	{
   	         		ret = CLIRET_INVALIDATE_PARAM;
					break;
   	         	}
   	     	}
            if(ret != CLIRET_INVALIDATE_PARAM)
            {
                if(readNvramData(&nvram) == 0)
                {
                    //zhan//strcpy(nvram.serialNo,argv[0]);
                    if(writeNvramData(&nvram) != 0)
                    {
                        ret = CLIRET_NORMAL_SET_FAILURE;
                    }
                }
                else
                {
                    ret = CLIRET_NORMAL_SET_FAILURE;
                }
            }
        }
        else
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }
    }
#else /* SYS_MGMT_SERIAL_NUMBER */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* SYS_MGMT_SERIAL_NUMBER */
#endif
    return ret;
}

CliRet facCmdRegionGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
	sprintf(pValue,"NULL");
    return ret;
}

CliRet facCmdRegionSet(int argc, char** argv)
{
    CliRet ret = CLIRET_NOT_SUPPORT_FUNCTION;
    return ret;
}

CliRet facCmdRegionList(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_NOT_SUPPORT_FUNCTION;
	sprintf(pValue,"NULL");
    return ret;
}

CliRet facCmdUserPasswordGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
    NVRAM_DATA nvram;

    if(argc != 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
        return ret;
    }

    if(readNvramData(&nvram)==0)
    {
        //zhan//strcpy(pValue, nvram.userPwd);
    }
    else
    {
        ret = CLIRET_NORMAL_GET_FAILURE;
    }

    return ret;
}

CliRet facCmdUserPasswordSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    NVRAM_DATA nvram;

    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(strlen(argv[0]) >= 20)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else 
    {
        if(readNvramData(&nvram) == 0)
        {
            //zhan//strcpy(nvram.userPwd,argv[0]);
            if(writeNvramData(&nvram) != 0)
            {
                ret = CLIRET_NORMAL_SET_FAILURE;
            }
        }
        else
        {
            ret = CLIRET_NORMAL_SET_FAILURE;
        }
    }
    return ret;
}

CliRet facCmdAdminPasswordGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_NORMAL_GET_FAILURE; 
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    LoginCfgObject *loginObj=NULL;

    if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) == CMSRET_SUCCESS)
    {
        if (cmsObj_get(MDMOID_LOGIN_CFG, &iidStack, 0, (void *) &loginObj) != CMSRET_SUCCESS)
        {
            cmsLog_error("Could not get MDMOID_LOGIN_CFG");
        }
        else
        {
            strcpy(pValue,loginObj->adminPassword);
            cmsObj_free((void **) &loginObj);
            ret = CLIRET_SUCCESS;
        }
		
        cmsLck_releaseLock();
    }

    return ret;
}

CliRet facUtilAdminPwdStrAvailCheck( char *passwd )
{
    CliRet ret = CLIRET_INVALIDATE_PARAM;
    int strLen = strlen(passwd);
    int i;

    if(strLen < 64)
    {
        for (i=0; i<strLen; i++)
        {
            if (((passwd[i] >=  '0') && (passwd[i] <=  '9'))
                || ((passwd[i] >=  'A') && (passwd[i] <=  'Z'))
                || ((passwd[i] >=  'a') && (passwd[i] <=  'z')))
            {
                ret = CLIRET_SUCCESS;
            }
            else
            {
                ret = CLIRET_INVALIDATE_PARAM;
                break;
            }
        }
    }

    return ret;
}

CliRet facCmdAdminPasswordSet(int argc, char** argv)
{
    CliRet ret = CLIRET_NORMAL_SET_FAILURE; 
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    LoginCfgObject *loginObj=NULL;

    if(argc == 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if ((ret = facUtilAdminPwdStrAvailCheck(argv[0])) == CLIRET_SUCCESS)
    {
        if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) == CMSRET_SUCCESS)
        {
            if (cmsObj_get(MDMOID_LOGIN_CFG, &iidStack, 0, (void *) &loginObj) != CMSRET_SUCCESS)
            {
                cmsLog_error("Could not get MDMOID_LOGIN_CFG");
            }
            else
            {
                CMSMEM_REPLACE_STRING(loginObj->adminPassword, argv[0]);
                if(cmsObj_set(loginObj, &iidStack) == CMSRET_SUCCESS)
                {
                    ret = CLIRET_REBOOT_RESET;
                }
    
                cmsObj_free((void **) &loginObj);
                cmsMgm_saveConfigToFlash(); 
            }
            cmsLck_releaseLock();
        }
    }

    return ret;
}

CliRet facCmdClearPortBindSet(int argc, char** argv)
{
    system("ebtables -F");
    return CLIRET_SUCCESS;
}

CliRet facCmdPvcInfoGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef DMP_ADSLWAN_1
    if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
    {
        ret = CLIRET_NORMAL_GET_FAILURE;
    }
    else
    {
        cli_getWanConnSummaryLocked(pValue, 0);
        cmsLck_releaseLock();
    }
#else /* DMP_ADSLWAN_1 */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* DMP_ADSLWAN_1 */
    return ret;
}

CliRet facCmdDslRateGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef DMP_ADSLWAN_1
    _WanDslIntfCfgObject *dslIntfCfg = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
    {
        ret = CLIRET_NORMAL_GET_FAILURE;
    }
    else
    {
        if(cmsObj_getNext(MDMOID_WAN_DSL_INTF_CFG, &iidStack, (void **) &dslIntfCfg) == CMSRET_SUCCESS)
        {
            if(cmsUtl_strcmp(dslIntfCfg->status, MDMVS_UP) == 0)
            {
                sprintf(pValue,"dslrateup=%d\n",(dslIntfCfg->upstreamCurrRate)/1000);
                sprintf(pValue,"%sdslratedn=%d\n",pValue,(dslIntfCfg->downstreamCurrRate)/1000);
            }
            else
            {
                strcpy(pValue,"dslrateup=\ndslratedn=\n");
            }
            cmsObj_free((void **)&dslIntfCfg);
        }
        else
        {
            ret = CLIRET_NORMAL_GET_FAILURE;
        }
        cmsLck_releaseLock();
    }
#else /* DMP_ADSLWAN_1 */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* DMP_ADSLWAN_1 */
    return ret;
}

CliRet facCmdRestorefactorySet(int argc, char** argv)
{
    CliRet ret = CLIRET_REBOOT_RESET;
	if(argc!=0)
	{
		ret = CLIRET_INVALIDATE_PARAM;
	}
	else
	{
    	if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
    	{
        	ret = CLIRET_NORMAL_SET_FAILURE;
    	}
    	else
    	{
        	oal_invalidateConfigFlash(3);
        	cmsUtil_sendRequestRebootMsg(msgHandle);
        	cmsLck_releaseLock();
    	}
	}
    return ret;
}

CliRet facCmdUSBTest(int argc, char** argv)
{
   CliRet ret = CLIRET_NOT_SUPPORT_FUNCTION;
   return ret;
}



CliRet facCmdLANIpAddrGet(int argc, char** argv, char *pValue)
{
    char buf[BUFLEN_512] = {0};
    char cmd[BUFLEN_256] = {0};
    FILE *fp = NULL;
    CliRet ret = CLIRET_SUCCESS;
	if(argc!=0)
	{
		ret = CLIRET_INVALIDATE_PARAM;
		return ret;
	}
    sprintf(cmd, "ifconfig br0>/var/ifcres");
    prctl_runCommandInShellBlocking(cmd);
    if((fp = fopen("/var/ifcres", "r")) != NULL)
    {
        while((fgets(buf, sizeof(buf), fp)) != NULL)
        {
            if(strstr(buf, "inet addr:") != NULL && strstr(buf, "Mask:") != NULL)
            {
                sscanf(strstr(buf,"inet addr:"), "inet addr:%s", pValue);
                sscanf(strstr(buf,"Mask:"), "Mask:%s", cmd);
                sprintf(pValue,"%s/%s",pValue,cmd);
                ret = CLIRET_SUCCESS;
                break;
            }
        }
        fclose(fp);
        unlink("/var/ifcres");
    }

    return ret;
}

	
CliRet facCmdLANIpAddrSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    char cmd[BUFLEN_128] = {0};

    if(argc < 2)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else
    {
        if((cli_isIpAddress(argv[0]) == TRUE) && (cli_isIpAddress(argv[1]) == TRUE))
        {
            sprintf(cmd, "config --ipaddr primary %s %s",argv[0],argv[1]);
			if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
    		{
        		ret = CLIRET_NORMAL_SET_FAILURE;
    		}
			else
			{
				processLanCmd(cmd);
				cmsLck_releaseLock();
			}
        }
        else
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }
    }
	cmsMgm_saveConfigToFlash();
    return ret;
}

CliRet facCmdLANIpAddrSetNonsave(int argc, char** argv)
{
	//ifconfig br0 192.168.1.2 netmask 255.255.0.0
	CliRet ret = CLIRET_SUCCESS;
    char cmd[BUFLEN_128] = {0};
    if(argc < 2)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else
    {
        if((cli_isIpAddress(argv[0]) == TRUE) && (cli_isIpAddress(argv[1]) == TRUE))
        {
            sprintf(cmd, "ifconfig br0 %s netmask %s",argv[0],argv[1]);
			system(cmd);
        }
        else
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }
    }

    return ret;
}

CliRet facCmdSignatureModeGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef SYS_IMAGE_SIGNATURE
    extern int pass_signature;
    strcpy(pValue,pass_signature?FAC_CMD_STATUS_ON:FAC_CMD_STATUS_OFF);
#else /* SYS_IMAGE_SIGNATURE */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* SYS_IMAGE_SIGNATURE */

    return ret;
}

CliRet facCmdSignatureModeSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef SYS_IMAGE_SIGNATURE
    if(argc == 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else
    {
        if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0)
        {
            unlink("/var/cli_signature");
        }
        else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0)
        {
            prctl_runCommandInShellBlocking("echo 1 >/var/cli_signature");
        }
        else
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }
    }
#else /* SYS_IMAGE_SIGNATURE */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* SYS_IMAGE_SIGNATURE */

    return ret;
}

CliRet facCmdSignatureModeSetNonsave(int argc, char** argv)
{
	CliRet ret = CLIRET_SUCCESS;
#ifdef SYS_IMAGE_SIGNATURE	

#else /* SYS_IMAGE_SIGNATURE */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* SYS_IMAGE_SIGNATURE */
	return ret;
}

CliRet facCmdWanGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
	if(argc!=0)
	{
		ret = CLIRET_INVALIDATE_PARAM;
	}
	else
	{
    	if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
    	{
        	ret = CLIRET_NORMAL_GET_FAILURE;
    	}
    	else
    	{
        	cli_getWanConnSummaryLocked(pValue, 1);
        	cmsLck_releaseLock();
    	}
	}
    return ret;
}

CliRet facCmdWanSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    UBOOL8 isAddPvc = FALSE;
    UBOOL8 isAddRoute = FALSE;
    UBOOL8 invalidateValue = TRUE;
    char buf[BUFLEN_256] = {0};
	char cmd[BUFLEN_128] = {0};
    if(argc !=3)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else
    {
        strcpy(buf, argv[1]);
        if(cmsUtl_strcmp(argv[0] ,"pvc") == 0)
        {
#ifndef DMP_ADSLWAN_1
            ret = CLIRET_NOT_SUPPORT_FUNCTION;
#else /* DMP_ADSLWAN_1 */
            char *pToken = NULL, *pLast = NULL;
            isAddPvc = TRUE;
            if((pToken = strtok_r(buf, "/", &pLast)) != NULL)
            {
                if (cli_isValidVpi(pToken))
                {
                    if((pToken = strtok_r(NULL, "/", &pLast)) != NULL)
                    {
                        if (cli_isValidVci(pToken))
                        {
                            invalidateValue = FALSE;
                        }
                    }
                }
            }
#endif /* DMP_ADSLWAN_1 */
        }
        else if(cmsUtl_strcmp(argv[0], "vlan") == 0)
        {
#if (!defined DMP_PTMWAN_1) && (!defined DMP_X_BROADCOM_COM_GPONWAN_1)
            ret = CLIRET_NOT_SUPPORT_FUNCTION;
#else
            if(cmsUtl_strcmp(argv[1], "NULL") == 0 || (atoi(argv[1]) >= 0 && atoi(argv[1]) <= 4094 ))
            {
                invalidateValue = FALSE;
            }
#endif
        }
        if((invalidateValue == FALSE) && (ret == CLIRET_SUCCESS))
        {
            if(
				(cmsUtl_strcmp(argv[2],"Route") == 0)||
				(cmsUtl_strcmp(argv[2],"Bridge") == 0)
			  )
            {
            	if(cmsUtl_strcmp(argv[2],"Route") == 0)
            	{
                	isAddRoute = TRUE;
            	}
            	else if(cmsUtl_strcmp(argv[2],"Bridge") != 0)
            	{
            		isAddRoute = FALSE;
                	
            	}
            }
			else
			{
				ret = CLIRET_INVALIDATE_PARAM;
			}

        }
        else
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }

        if(ret == CLIRET_SUCCESS)
        {
            if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
            {
                ret = CLIRET_NORMAL_SET_FAILURE;
            }
            else
            {
				ret=cli_facAddWanConnectionLocked(isAddPvc,isAddRoute,argv[1]);
				if(ret==CLIRET_SUCCESS)
				{
					cmsMgm_saveConfigToFlash();
				}
				cmsLck_releaseLock();
            }
        }
    }

    return ret;
}

CliRet facCmdWanSetNonsave(int argc, char** argv)
{
	CliRet ret = CLIRET_NOT_SUPPORT_FUNCTION;
	
	return ret;
}


CliRet facCmdWlanGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
    char *p = NULL;
    char buffer[32];
#ifdef BRCM_WLAN
    p=nvram_get("wl0_radio");
     if(atoi(p)==1)
    {
        strcpy(pValue, FAC_CMD_STATUS_ON);
    }
    else
    {
        strcpy(pValue, FAC_CMD_STATUS_OFF);
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif
    return ret;
}

CliRet facCmdWlanSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    char * p=NULL;
    int i;
    char tmp_s[32];
    UBOOL8 needRestartWlan = FALSE;    
#ifdef BRCM_WLAN
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0 || cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0)
    {
            p=nvram_get("wl0_radio");
            if((atoi(p) == 1) && (cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0))
            {
                nvram_set("wl0_radio","0");
                needRestartWlan = TRUE;
            }
            else if((atoi(p) == 0) && (cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0))
            {
                nvram_set("wl0_radio","1");
                needRestartWlan = TRUE;
            }
            if(needRestartWlan == TRUE)
            {
                  wlSendMsgToWlmngr(EID_HTTPD, "Fac:1"); 
            }
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif    
    return ret;
}

CliRet facCmdWlan5gGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
    char *p = NULL;
    char buffer[32];
#ifdef BRCM_WLAN
    p=nvram_get("wl1_radio");
     if(atoi(p))
    {
        strcpy(pValue, FAC_CMD_STATUS_ON);
    }
    else
    {
        strcpy(pValue, FAC_CMD_STATUS_OFF);
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif
    return ret;
}

CliRet facCmdWlan5gSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    char * p=NULL;
    int i;
    char tmp_s[32];
    UBOOL8 needRestartWlan = FALSE;    
#ifdef BRCM_WLAN
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0 || cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0)
    {
            p=nvram_get("wl1_radio");
            if((atoi(p) == 1) && (cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0))
            {
                nvram_set("wl1_radio","0");
                needRestartWlan = TRUE;
            }
            else if((atoi(p) == 0) && (cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0))
            {
                nvram_set("wl1_radio","1");
                needRestartWlan = TRUE;
            }
            if(needRestartWlan == TRUE)
            {
                wlSendMsgToWlmngr(EID_HTTPD, "Fac:2"); 
            }
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif    
    return ret;
}

CliRet facCmdSsidNameGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
    char *p = NULL;
    char buffer[32];
#ifdef BRCM_WLAN
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
            p=nvram_get("wl0_ssid");
            if(p)
                sprintf(pValue,"%s",p);
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif
    return ret;
}

CliRet facCmdSsidNameSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    char *p = NULL;
    char buffer[32];
#ifdef BRCM_WLAN
    if(argc != 2)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
           nvram_set("wl0_ssid",argv[1]);
           wlSendMsgToWlmngr(EID_HTTPD, "Fac:1");    
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif
    return ret;
}

CmsRet wlSendMsgToWlmngr(CmsEntityId sender_id, char *message_text)
{
   static void *msgHandle = NULL;
   static char buf[sizeof(CmsMsgHeader) + 32] = {0};
   CmsMsgHeader *msg = (CmsMsgHeader *)buf;

   int unit;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   if ((ret = cmsMsg_init(sender_id, &msgHandle)) != CMSRET_SUCCESS)
   {
      printf("could not initialize msg, ret=%d\n", ret);
      fflush(stdout);
      return ret;
   }

   sprintf((char *)(msg + 1), "%s", message_text);

   msg->dataLength = 32;
   msg->type = CMS_MSG_WLAN_CHANGED;
   msg->src = sender_id;
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

   cmsMsg_cleanup(&msgHandle);
   return ret;
 }


CliRet facCmd5gSsidNameGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
    char *p = NULL;
    char buffer[32];
#ifdef BRCM_WLAN
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
            p=nvram_get("wl1_ssid");
            if(p)
                sprintf(pValue,"%s",p);
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif
    return ret;
}

CliRet facCmd5gSsidNameSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    char *p = NULL;
    char buffer[32];
#ifdef BRCM_WLAN
    if(argc != 2)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
           nvram_set("wl1_ssid",argv[1]);
           wlSendMsgToWlmngr(EID_HTTPD, "Fac:2");    
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif
    return ret;
}



CliRet facCmdSsidKeyGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
    char *p = NULL;
#ifdef BRCM_WLAN
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
        NVRAM_DATA nvram;
        if(readNvramData(&nvram) == 0)
        {
	     if((p = strchr(nvram.wlanParams,',')) != NULL)
	     {
	         strcpy(pValue, ++p);
	     }
	     else
	     {
	         ret = CLIRET_NORMAL_GET_FAILURE;
	     }
        }
        else
        {
            ret = CLIRET_NORMAL_GET_FAILURE;
        }
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif

    return ret;
}

CliRet facCmdSsidKeySet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    char *p = NULL;
    char wlSsid[BUFLEN_64] = {0};
#ifdef BRCM_WLAN
    if(argc != 2)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
        NVRAM_DATA nvram;
        int len = 0;
        len = strlen(argv[1]);
        if(len > 0 && len < 32)
        {
            if(readNvramData(&nvram) == 0)
            {
                strcpy(wlSsid, nvram.wlanParams);
		            if((p = strchr(wlSsid,',')) != NULL)
	     	        {
	                  *p = '\0';
		                snprintf(nvram.wlanParams,sizeof(nvram.wlanParams),"%s,%s",wlSsid,argv[1]);
                    if(writeNvramData(&nvram) != 0)
	                  {
		                    ret = CLIRET_NORMAL_SET_FAILURE;
		              }		      	
            	  }
                else
                {
                    ret = CLIRET_NORMAL_SET_FAILURE;
                }
            }
            else
            {
                ret = CLIRET_NORMAL_SET_FAILURE;
            }
        }
        else
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif

    return ret;
}

CliRet facCmdCurssidnameGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef BRCM_WLAN
    _LanWlanObject *lanWlanObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
        if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
        {
        ret = CLIRET_NORMAL_GET_FAILURE;
        }
        else
        {
            if(cmsObj_getNext(MDMOID_LAN_WLAN, &iidStack, (void **)&lanWlanObj) == CMSRET_SUCCESS)
            {
                strcpy(pValue,lanWlanObj->SSID);
                cmsObj_free((void **)&lanWlanObj);
            }
	          else
            {
                ret = CLIRET_NORMAL_GET_FAILURE;
            }
            cmsLck_releaseLock();
        }
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }	
#else /* BRCM_WLAN */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* BRCM_WLAN */

    return ret;
}

CliRet facCmdCurssidnameSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef BRCM_WLAN
    UBOOL8 needRestartWlan = FALSE;
    _LanWlanObject  *lanWlanObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;     
	
    if(argc != 2)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
        int len = 0;
        len = strlen(argv[1]);
        if(len > 0 && len < 32)
        {
            if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
            {
                ret = CLIRET_NORMAL_SET_FAILURE;
            }
            else
            {
                if(cmsObj_getNext(MDMOID_LAN_WLAN, &iidStack, (void **)&lanWlanObj) == CMSRET_SUCCESS)
                {
                    REPLACE_STRING_IF_NOT_EQUAL(lanWlanObj->SSID, argv[1]);
                    cmsObj_set(lanWlanObj, &iidStack);
                    needRestartWlan = TRUE;  
		                cmsObj_free((void **)&lanWlanObj);
                }                
		            else
                {
                    ret = CLIRET_NORMAL_SET_FAILURE;
                }
		            cmsLck_releaseLock();
            }           
        }
	      else
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }        
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }

    if(needRestartWlan == TRUE)
    {
        cli_restartWlan();
    }
#else /* BRCM_WLAN */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* BRCM_WLAN */
    return ret;
}

CliRet facCmdDiswifiauthSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef BRCM_WLAN
    UBOOL8 needRestartWlan = FALSE;
    _LanWlanObject  *lanWlanObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;     
	
    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(atoi(argv[0]) == 1)
    {
    
        if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
        {
            ret = CLIRET_NORMAL_SET_FAILURE;
        }
        else
        {
            if(cmsObj_getNext(MDMOID_LAN_WLAN, &iidStack, (void **)&lanWlanObj) == CMSRET_SUCCESS)
            {
                cmsMem_free(lanWlanObj->beaconType);
                lanWlanObj->beaconType = cmsMem_strdup(MDMVS_BASIC);

                cmsMem_free(lanWlanObj->basicAuthenticationMode);
                lanWlanObj->basicAuthenticationMode = cmsMem_strdup(MDMVS_NONE);
                    
                cmsMem_free(lanWlanObj->basicEncryptionModes);
                lanWlanObj->basicEncryptionModes = cmsMem_strdup(MDMVS_NONE);
                                       
                cmsObj_set(lanWlanObj, &iidStack);
                needRestartWlan = TRUE;  
	             cmsObj_free((void **)&lanWlanObj);
            }                
	         else
            {
                ret = CLIRET_NORMAL_SET_FAILURE;
            }
	         cmsLck_releaseLock();
        }                 
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }

    if(needRestartWlan == TRUE)
    {
        cli_restartWlan();
    }
#else /* BRCM_WLAN */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* BRCM_WLAN */
    return ret;
}

CliRet facCmdWlanTxbwGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef BRCM_WLAN
    int currbw = 0;
    currbw = cli_getCurrentWlanBand(0);
    if(currbw == 10 || currbw == 20 || currbw == 40)
    {
        sprintf(pValue, "%d", currbw);
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#else /* BRCM_WLAN */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* BRCM_WLAN */

    return ret;
}

CliRet facCmdWlanTxbwSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef BRCM_WLAN
    UBOOL8 needRestartWlan = FALSE;
    _WlMimoCfgObject *WlMimoCfgObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    int currbw = 0;

    if(argc != 1)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(cmsUtl_strcmp(argv[0], "20") == 0 || cmsUtl_strcmp(argv[0], "40") == 0)
    {
        currbw = cli_getCurrentWlanBand(0);
        if ((cmsUtl_strcmp(argv[0], "20") == 0 && currbw != 20) ||
            (cmsUtl_strcmp(argv[0], "40") == 0 && currbw != 40))
        {
            if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) != CMSRET_SUCCESS)
            {
                ret = CLIRET_NORMAL_SET_FAILURE;
            }
            else
            {/*zhan
                if(cmsObj_getNext(MDMOID_WL_MIMO_CFG, &iidStack, (void **)&WlMimoCfgObj) == CMSRET_SUCCESS)
                {
                    if(cmsUtl_strcmp(argv[0], "20") == 0 && WlMimoCfgObj->X_BROADCOM_COM_NBwCap == 1)
                    {
                        WlMimoCfgObj->X_BROADCOM_COM_NBwCap = 0;
                        cmsObj_set(WlMimoCfgObj, &iidStack);
                    }
                    else if(cmsUtl_strcmp(argv[0], "40") == 0 && WlMimoCfgObj->X_BROADCOM_COM_NBwCap == 0)
                    {
                        WlMimoCfgObj->X_BROADCOM_COM_NBwCap = 1;
                        cmsObj_set(WlMimoCfgObj, &iidStack);
                    }
                    cmsObj_free((void **)&WlMimoCfgObj);
                }
	              else
                {
                    ret = CLIRET_NORMAL_SET_FAILURE;
                }
                cmsLck_releaseLock();*/
            }
            /* we make sure that the config is right just now, here we try to restart the wlan. */
            needRestartWlan = TRUE;
        }
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }

    if(needRestartWlan == TRUE)
    {
        cli_restartWlan();
    }
#else /* BRCM_WLAN */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* BRCM_WLAN */
    return ret;
}

CliRet facCmdVoiceSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef DMP_ENDPOINT_1
    char cmdLine[BUFLEN_1024] = {0};
    int i = 0;
    strcpy(cmdLine,"voice set");
    for(i = 0; i < argc; i++)
    {
        sprintf(cmdLine, "%s %s", cmdLine, argv[i]);
    }
    processFacVoiceCmd(cmdLine);
#else /* DMP_ENDPOINT_1 */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* DMP_ENDPOINT_1 */

    return ret;
}

CliRet facCmdVoiceGet(int argc, char** argv,char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef DMP_ENDPOINT_1
    char cmdLine[BUFLEN_1024] = {0};
    int i = 0;
    strcpy(cmdLine,"voice get");
    for(i = 0; i < argc; i++)
    {
        sprintf(cmdLine, "%s %s", cmdLine, argv[i]);
    }
    //zhan//processFacVoiceGetCmd(cmdLine,pValue);
#else /* DMP_ENDPOINT_1 */
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif /* DMP_ENDPOINT_1 */

    return ret;
}


void stringToHex(char *s, char *v)
{
    char *t=NULL;
    t= s;
    char Tmp[BUFLEN_4]={0};
    for(;*t!='\0';t++){
        sprintf(Tmp, "%x",*t);
        strcat(v,Tmp);
    }

    return;
}

void hexToString(char *s, char *v)
{
	char tpv[100] = {0};
	char *tp = NULL;
	char tmp[1] = {0};
	int cc = 0;
	int i = 0, j = 0;
	int slen = 0;
	tp = s;
	slen = strlen(tp);
	for(i = 1, j = 0 ; i <= slen; i++){
		tmp[0] = *tp;
		if(i%2 == 0){
			cc = cc + atoi(tmp);
			if(cc != 0){
				tpv[j] = toascii(cc);
				j++;
			}
			cc = 0;
		}else{
			cc = atoi(tmp)*16 ;
		}
		tp++;
	}

	strcpy(v,tpv);
    return;
}

CliRet facCmdLoidGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#if defined(DARE_CT_GPON)
	char *p[100] = {0};

	LoidObject *ctLoid;
   	InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
	if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) == CMSRET_SUCCESS){
		if(( cmsObj_get(MDMOID_LOID, &iidStack, 0, (void **) &ctLoid)) != CMSRET_SUCCESS)
		{
			cmsLog_error("get of MDMOID_LOID failed\n");
			ret = CLIRET_NORMAL_GET_FAILURE;
		}else{
		
			strcpy(p,ctLoid->LOID);
			hexToString(p,pValue);
		}	

	
		cmsObj_free((void **) &ctLoid);	
        	cmsLck_releaseLock();

	}else{
	
			ret = CLIRET_NORMAL_GET_FAILURE;
	}


#endif
#if defined(DARE_CT_EPON)

    CtUserInfoObject *ctUserinfo;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

	
	if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) == CMSRET_SUCCESS){
		if((cmsObj_get(MDMOID_CT_USER_INFO, &iidStack, 0, (void **) &ctUserinfo)) != CMSRET_SUCCESS)
	  	{
	        	cmsLog_error("get of MDMOID_CT_USER_INFO failed\n");
			ret = CLIRET_NORMAL_GET_FAILURE;
	    	}else{
	    		if(ctUserinfo->LOID != NULL){
			strcpy(pValue,ctUserinfo->LOID);
	    		}else{
	    		pValue = NULL;
	    		}
	    	}

    		cmsObj_free((void **) &ctUserinfo);
        	cmsLck_releaseLock();
	}else{
		ret = CLIRET_NORMAL_GET_FAILURE;
	}


#endif
    return ret;
}


CliRet facCmdLoidSet(int argc, char** argv)
{
    CliRet ret = CLIRET_REBOOT_RESET;

    int i = 0;
    int len = 0;
   
    if(argc == 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else
    {
        len = strlen(argv[0]);
        if(len > 0 && len < 32)
        {
#if defined(DARE_CT_GPON)
	LoidObject *ctLoid;
   	InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
	char TmpLoidValueHex[BUFLEN_64]={0};
	char TmpPasswdValueHex[BUFLEN_64]={0};
	
	if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) == CMSRET_SUCCESS){
		if((cmsObj_get(MDMOID_LOID, &iidStack, 0, (void **) &ctLoid)) != CMSRET_SUCCESS)
		{
			cmsLog_error("get of MDMOID_LOID failed\n");
			ret = CLIRET_NORMAL_SET_FAILURE;
		}
	
		if(strcmp(argv[0] ,"") != 0){
			stringToHex(argv[0], TmpLoidValueHex);
		}else{
			strcpy(TmpLoidValueHex, "000000000000000000000000");
		}

		CMSMEM_REPLACE_STRING(ctLoid->LOID, TmpLoidValueHex);

		if((cmsObj_set(ctLoid, &iidStack))!=CMSRET_SUCCESS)
		{
			cmsLog_error("set of MDMOID_LOID failed\n");
			ret = CLIRET_NORMAL_SET_FAILURE;
		}

		cmsObj_free((void **) &ctLoid);
		cmsMgm_saveConfigToFlash(); 

	       cmsLck_releaseLock();

		
	}else{
		ret = CLIRET_NORMAL_SET_FAILURE;
	}




#endif
#if defined(DARE_CT_EPON)
    CtUserInfoObject *ctUserinfo;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

    
    if (cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT) == CMSRET_SUCCESS){
	    if((cmsObj_get(MDMOID_CT_USER_INFO, &iidStack, 0, (void **) &ctUserinfo)) != CMSRET_SUCCESS)
	    {
	        cmsLog_error("get of MDMOID_CT_USER_INFO failed\n");
			ret = CLIRET_NORMAL_SET_FAILURE;
	    }

	    CMSMEM_REPLACE_STRING(ctUserinfo->LOID, argv[0]);
	   // CMSMEM_REPLACE_STRING(ctUserinfo->password, webVar->LOIDPass);

	    if((cmsObj_set(ctUserinfo, &iidStack))!=CMSRET_SUCCESS)
	    {
	        cmsLog_error("set of MDMOID_CT_USER_INFO failed\n");
		ret = CLIRET_NORMAL_SET_FAILURE;
	    }
	    
	    cmsObj_free((void **) &ctUserinfo);
	    cmsMgm_saveConfigToFlash(); 
	    cmsLck_releaseLock();

    }else{
		ret = CLIRET_NORMAL_SET_FAILURE;
	}

	
#endif
            
        }
        else
        {
          //printf("The loid length is wrong\n");
            ret = CLIRET_INVALIDATE_PARAM;
        }
    }


    return ret;
}




CliRet facCmdlaserSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
//#ifdef DARE_CT_GPON
    if(argc == 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0)
    {
        rut_doSystemAction("fac_util", "gponctl testMode --oper 1");
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0)
    {
        rut_doSystemAction("fac_util", "gponctl testMode --oper 0");
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
//#endif

#ifdef DARE_CT_EPON

	CmsRet cmsret;  
	unsigned char *precv;
   	unsigned long recvlen;

    if(argc == 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0)
    {
       cmsret=cmsUtil_sendMsgToOamd(msgHandle, TK_OAMOP_LASER_SET,1, NULL, 0, &precv, &recvlen, 5*1000);
   	CMSMEM_FREE_BUF_AND_NULL_PTR(precv);
	if(cmsret !=CMSRET_SUCCESS){
		ret = CLIRET_NORMAL_SET_FAILURE;
	}
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0)
    {
      cmsret=cmsUtil_sendMsgToOamd(msgHandle, TK_OAMOP_LASER_SET,0, NULL, 0, &precv, &recvlen, 5*1000);
   	CMSMEM_FREE_BUF_AND_NULL_PTR(precv);
	if(cmsret !=CMSRET_SUCCESS){
		ret = CLIRET_NORMAL_SET_FAILURE;
	}
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }

#endif
    return ret;
}

CliRet facCmdlaserGet(int argc, char** argv,char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef DARE_CT_GPON
 ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif

#ifdef DARE_CT_EPON
	   unsigned char *precv;
	   unsigned long recvlen;
	   CmsRet cmsret;
         cmsret = cmsUtil_sendMsgToOamd(msgHandle, TK_OAMOP_LASER_GET, 0, NULL, 0, &precv, &recvlen, 5*1000);
         if(cmsret !=CMSRET_SUCCESS || precv==NULL)
         {
            cmsLog_error("get laser error\n");
	    	ret = CLIRET_NORMAL_GET_FAILURE;
         }
         //printf("laser is %s.\n", *precv ? "on" : "off");
	 if(*precv != NULL){
			strcpy(pValue,"on");
	 }else{
			strcpy(pValue,"off");
	 }
         CMSMEM_FREE_BUF_AND_NULL_PTR(precv);

#endif
    return ret;
}

CliRet facCmdLaserTxpowerGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;
#ifdef DARE_CT_GPON
    UINT8 reg[2];
    UINT32 MSB;
    UINT32 LSB;
    int retv;
    float tmp= 0;
    float tmpTxpower = -40;

    retv = rut_GponPhyRead(1, 102, 2, reg);
    if(retv < 0)
    {
        ret = CLIRET_NORMAL_GET_FAILURE;
    }
    else
    { 
        MSB=(reg[0]<<8)/10000;
        LSB=(reg[0]<<8)%10000;

        LSB+=reg[1];
        //LSB/=100; /*keep 2 digits*/ 
        if(MSB == 0 && LSB==0)
        {		
            sprintf(pValue,"%0.1lf",tmpTxpower);
        }
        else
        {
            tmp = MSB + LSB/10000.0;
            sprintf(pValue,"%0.1lf",((log10(tmp))*10));
        }
    }
#endif
#ifdef DARE_CT_EPON
    unsigned char *precv;
    unsigned long recvlen;
    CmsRet cmsret=CMSRET_SUCCESS;
    
    	cmsret = cmsUtil_sendMsgToOamd(msgHandle, TK_OAMOP_SEND_POW_GET, 0, NULL, 0, 
										(void**)&precv, (UINT32 *)&recvlen, 5*1000);
        if(cmsret == CMSRET_SUCCESS)
        {
			short Txpower=0;
			memcpy(&Txpower, precv, 2);
			if(Txpower == 0){
				float tmpTxpower = -40;
				sprintf(pValue, "%.0f", tmpTxpower);
			}else{
				 sprintf(pValue, "%.2f", ((log10(Txpower)-4)*10));
				//sprintf(pValue, "%.0f", tt());;
			}	
        	CMSMEM_FREE_BUF_AND_NULL_PTR(precv);
       	}
        else
        {
	    	ret = CLIRET_NORMAL_GET_FAILURE;
        }

		
#endif 
    return ret;
}

CliRet facCmdGponSNGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;

#ifdef DARE_CT_GPON
    NVRAM_DATA nvram;
	if(readNvramData(&nvram)!=0)
	{
		ret = CLIRET_NORMAL_GET_FAILURE;
	}
	else
	{
		strncpy(pValue, &(nvram.gponSerialNumber),sizeof(nvram.gponSerialNumber));
	}
#else 
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif

    return ret;
}

CliRet facCmdGponSNSet(int argc, char** argv)
{
    CliRet ret = CLIRET_REBOOT_RESET;

#ifdef DARE_CT_GPON
    int len = 0;
    int i = 0;
    NVRAM_DATA nvram;
    if(argc == 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else
    {
        len = strlen(argv[0]);
        if(len > 0 && len < 17)
        {

            if(ret != CLIRET_INVALIDATE_PARAM)
            {
				if(readNvramData(&nvram)!=0)
				{
					ret = CLIRET_NORMAL_SET_FAILURE;
					return ret;
				}
				else
				{
                	//printf("argv[0] = %s\n",argv[0] );
		  			if (argv[0] != '\0') 
					{
			   			snprintf(nvram.gponSerialNumber, sizeof(nvram.gponSerialNumber), "%s", argv[0]);
						if(writeNvramData(&nvram)!=0)
						{
							ret = CLIRET_NORMAL_SET_FAILURE;
							return ret;
						}
		  			}
					else
					{
						ret = CLIRET_INVALIDATE_PARAM;
						return ret;
					}
				}
            }
        }
        else
        {
            ret = CLIRET_INVALIDATE_PARAM;
        }
    }
#else 
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif 

    return ret;
}


static char rogueOnuDetect = 0;
CliRet facCmdRogDetectGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;

#ifdef DARE_CT_EPON
	 if(rogueOnuDetect == 1){
		strcpy(pValue,"on");
	 }else{
		strcpy(pValue,"off");
	 }
#else 
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif

    return ret;
}

CliRet facCmdRogDetectSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;

#ifdef DARE_CT_EPON


    char cmd[BUFLEN_64];
    struct utsname kernel;

    if(argc == 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0)
    {
    uname(&kernel);
    sprintf(cmd, "insmod /lib/modules/%s/extra/laseronmonitor.ko", kernel.release);
    system(cmd);
    rogueOnuDetect = 1;
   
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0)
    {
    system("rmmod laseronmonitor.ko");
    rogueOnuDetect = 0;
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }

#else 
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif

    return ret;
}

CliRet facCmdrodccheckGet(int argc, char** argv, char *pValue)
{
    CliRet ret = CLIRET_SUCCESS;

#ifdef DARE_CT_EPON
	 char rodccheck = 0;
        FILE *fp;
        if((fp=fopen("/proc/laseronmonitor","r"))==NULL) 
        {
            printf("not open");
            exit(0);
        }
        rodccheck=fgetc(fp);
        fclose(fp);
	 if(rodccheck == '2'){
		strcpy(pValue,"good");
	 }else{
		strcpy(pValue,"bad");
	 }
#else 
    ret = CLIRET_NOT_SUPPORT_FUNCTION;
#endif

    return ret;
}


CliRet facCmdLedSet(int argc, char** argv)
{
    CliRet ret = CLIRET_SUCCESS;
    int i = 0;
    char cmd[BUFLEN_32] = {0};
    
#if defined (CONFIG_BCM96816)    
    struct ifreq ifr;
    int skfd;
    int *data;
    
    memset(&ifr, 0, sizeof(struct ifreq));
   
    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
    {
       fprintf(stderr, "socket open error\n");
       return 0 ;
    }    
    strcpy(ifr.ifr_name, "eth0"); 
    if ( ioctl(skfd, SIOCGIFINDEX, &ifr) < 0 ) 
    {
        fprintf(stderr, "invalid interface name! %s\n", strerror(errno));
        close(skfd);
        return 0 ;
    }  
    data = (int *)(&ifr.ifr_data);
#endif 

#ifdef DARE_CT_GPON
      /* Richard, we should stop gponctl to control the GPON LED and Los Led simultaneously. */
      prctl_runCommandInShellBlocking("gponctl stop 1>/dev/null 2>/dev/null;");
      usleep(5000);
#endif

    if(argc == 0)
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_ON) == 0)
    {
        /* turn on gpio led first. */
        for(i = 0; i < 35; i++)
        {
            sprintf(cmd, "echo \"%02x 01\" >/proc/led", i);
            prctl_runCommandInShellBlocking(cmd);
        }
#if defined (CONFIG_BCM96816)          
        data[0] = 2;
        
        if (ioctl(skfd, SIOCSINETSTATUS, &ifr) < 0) 
        {
            fprintf(stderr, "interface %s ioctl SIOCSINETSTATUS led on error! %s\n", ifr.ifr_name, strerror(errno));
            close(skfd);
            return 0 ;
        }
#endif
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_OFF) == 0)
    {
        /* turn off gpio led first. */
        for(i = 0; i < 35; i++)
        {
            sprintf(cmd, "echo \"%02x 00\" >/proc/led", i);
            prctl_runCommandInShellBlocking(cmd);
        }
#if defined (CONFIG_BCM96816)         
        data[0] = 3;
        
        if (ioctl(skfd, SIOCSINETSTATUS, &ifr) < 0) 
        {
            fprintf(stderr, "interface %s ioctl SIOCSINETSTATUS led off error! %s\n", ifr.ifr_name, strerror(errno));
            close(skfd);
            return 0 ;
        }
#endif       
    }
    else if(cmsUtl_strcmp(argv[0], FAC_CMD_STATUS_RED) == 0)
    {
        /* set gpio led red only. */
        for(i = 0; i < 35; i++)
        {
            sprintf(cmd, "echo \"%02x 02\" >/proc/led", i);
            prctl_runCommandInShellBlocking(cmd);
        }
    }
    else
    {
        ret = CLIRET_INVALIDATE_PARAM;
    }
#if defined (CONFIG_BCM96816)    
    close(skfd);
#endif    
    return ret;
}

#ifdef DMP_ADSLWAN_1
static CmsRet cli_getDestinationAddressLocked(char *varValue, const InstanceIdStack *parentiidStack, int type)
{
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
    int i = 0;
    char destinationAddr[BUFLEN_32] = {0};
    _WanDslLinkCfgObject *wanDslLinkCfgObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    while(cmsObj_getNextInSubTree(MDMOID_WAN_DSL_LINK_CFG, parentiidStack, &iidStack, (void **) &wanDslLinkCfgObj) == CMSRET_SUCCESS)
    {
        if(wanDslLinkCfgObj->destinationAddress != NULL)
        {
            strcpy(destinationAddr,wanDslLinkCfgObj->destinationAddress);
            ret = CMSRET_SUCCESS;
            cmsObj_free((void **) &wanDslLinkCfgObj);
            break;
        }
        cmsObj_free((void **) &wanDslLinkCfgObj);
    }
    if(ret == CMSRET_SUCCESS)
    {
        if(type == 0)
        {
            for(i=0;i<strlen(destinationAddr);i++)
            {
                if(destinationAddr[i] == '/')
                {
                    sprintf(varValue,"%s%c",varValue, '|');
                }
                else if(destinationAddr[i] >= 48/*0*/ && destinationAddr[i] <= 57 /*9*/)
                {
                    sprintf(varValue,"%s%c",varValue, destinationAddr[i]);
                }
            }
        }
        else
        {
            for(i=0;i<strlen(destinationAddr);i++)
            {
                if((destinationAddr[i] == '/') ||
                   (destinationAddr[i] >= 48/*0*/ && destinationAddr[i] <= 57 /*9*/))
                {
                    sprintf(varValue,"%s%c",varValue, destinationAddr[i]);
                }
            }
        }
    }
    return ret;
}
#endif /* DMP_ADSLWAN_1 */

static void cli_getWanDislayInfo(char *varValue, int wanIndex, const InstanceIdStack *parentiidStack, SINT32 vlanId, int type, UBOOL8 isRoute)
{
#ifdef DMP_ADSLWAN_1
            char destinationAddress[BUFLEN_16] = {0};
            if(cli_getDestinationAddressLocked(destinationAddress, parentiidStack,  type) == CMSRET_SUCCESS)
            {
                if(type == 0)
                {
                    sprintf(varValue,"%spvc%d=%s|%s\n",varValue,wanIndex,destinationAddress,isRoute?"R":"B");
                }
                else
                {
                    sprintf(varValue,"%swan%d=%s:%s\n",varValue,wanIndex,isRoute?"Route":"Bridge",destinationAddress);
                }
                return;
            }
#else /* DMP_ADSLWAN_1 */
            sprintf(varValue,"%swan%d=%s:%d\n",varValue,wanIndex,isRoute?"Route":"Bridge",vlanId);
            return;
#endif /* DMP_ADSLWAN_1 */
}

/*
INPUT: type 0 means get pvcinfo(pvc1=0|35|B),1 means get wan conninfo(wan1=Route:0/35).
*/
void cli_getWanConnSummaryLocked(char *varValue,int type)
{
    _WanConnDeviceObject *wanConnDeviceObj = NULL;
    _WanPppConnObject *wanPppConnObj = NULL;
    _WanIpConnObject *wanIpConnObj = NULL;
    InstanceIdStack iidStack_1 = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidStack_2 = EMPTY_INSTANCE_ID_STACK;
    int wanIndex = 0;

    while (cmsObj_getNext(MDMOID_WAN_CONN_DEVICE, &iidStack_1, (void **) &wanConnDeviceObj) == CMSRET_SUCCESS)
    {
        cmsObj_free((void **) &wanConnDeviceObj);
        INIT_INSTANCE_ID_STACK(&iidStack_2);
        while(cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN, &iidStack_1, &iidStack_2, (void **) &wanPppConnObj) == CMSRET_SUCCESS)
        {
            wanIndex++;
			if(strcmp(wanPppConnObj->connectionType,"PPPoE_Bridged")==0)
			{
				cli_getWanDislayInfo(varValue, wanIndex, &iidStack_1, wanPppConnObj->X_BROADCOM_COM_VlanMuxID, type, FALSE);
			}
			if(strcmp(wanPppConnObj->connectionType,"IP_Routed")==0)
			{
				cli_getWanDislayInfo(varValue, wanIndex, &iidStack_1, wanPppConnObj->X_BROADCOM_COM_VlanMuxID, type, TRUE);
			}
            cmsObj_free((void **) &wanPppConnObj);
        }

        INIT_INSTANCE_ID_STACK(&iidStack_2);
        while(cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN, &iidStack_1, &iidStack_2, (void **) &wanIpConnObj) == CMSRET_SUCCESS)
        {
            wanIndex++;
            if(cmsUtl_strcmp(wanIpConnObj->connectionType, MDMVS_IP_ROUTED) == 0)
            {
                cli_getWanDislayInfo(varValue, wanIndex, &iidStack_1, wanIpConnObj->X_BROADCOM_COM_VlanMuxID, type, TRUE);
            }
            else
            {
                cli_getWanDislayInfo(varValue, wanIndex, &iidStack_1, wanIpConnObj->X_BROADCOM_COM_VlanMuxID, type, FALSE);
            }
            cmsObj_free((void **) &wanIpConnObj);
        }
    }
}

#ifdef BRCM_WLAN
void cli_restartWlan(void)
{
    static char buf[sizeof(CmsMsgHeader) + 32]={0};
    CmsMsgHeader *msg=(CmsMsgHeader *) buf;
    CmsRet ret = CMSRET_INTERNAL_ERROR;
    sprintf((char *)(msg + 1), "Modify:%d", 1);

    msg->dataLength = 32;
    msg->type = CMS_MSG_WLAN_CHANGED;
    msg->src = EID_HTTPD; /* do not change the EID to others,tks. */
    msg->dst = EID_WLMNGR;
    msg->flags_event = 1;
    msg->flags_request = 0;

    if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
        cmsLog_error("could not send CMS_MSG_WLAN_CHANGED msg to wlmngr, ret=%d", ret);
    else
        cmsLog_debug("\nmessage CMS_MSG_WLAN_CHANGED sent successfully\n");
    return;
}

#define ID_CURSPEC_SEP '('
int cli_getCurrentWlanBand(int wlIdx)
{
    char cmd[BUFLEN_512] = {0};
    char chanspecfile[80] = {0};
    FILE *fp = NULL;
    char *cp;
    int chanspec = 0;
    int curr_bw = 0;

    sprintf(cmd, "wlctl -i wl%d chanspec > /var/curchaspec%d", wlIdx, wlIdx );
    prctl_runCommandInShellBlocking(cmd);    
    sprintf(chanspecfile, "/var/curchaspec%d", wlIdx);
    fp = fopen(chanspecfile, "r");
    if ( fp != NULL )
    {
        while ( fgets( cmd, sizeof(cmd), fp ) )
        {
            cp = strchr(cmd, ID_CURSPEC_SEP);
            cp++;
            if (cp)
            {
                chanspec = strtol(cp, 0, 16);
            }              
        }
        fclose(fp);
        unlink(chanspecfile);      
    }
    else
    {
        printf("%s: error open file\n",__FUNCTION__);
    }

    curr_bw = (chanspec & 0xc00 ) >> 10;
    if(curr_bw == 2)
        curr_bw = 20;
    else if (curr_bw == 3)
        curr_bw = 40;
    else if (curr_bw == 1)
        curr_bw = 10;

    return curr_bw;
}
#endif /* BRCM_WLAN */

static CmsRet cli_facAddWanLayer2InterfaceLocked(UBOOL8 isAddPvc, char *buf, InstanceIdStack *iidStack_wanDev, InstanceIdStack *iidStack_WanConnDev, char *layer2IfName)
{
    CmsRet ret = CMSRET_SUCCESS;
    if(isAddPvc == TRUE)
    {
#ifdef DMP_ADSLWAN_1
        _WanDslLinkCfgObject *wanDslLinkCfgObj = NULL;
        char destinationAddress[BUFLEN_16] = {0};
        if((ret = cmsObj_get(MDMOID_WAN_DSL_LINK_CFG, iidStack_WanConnDev, 0, (void **) &wanDslLinkCfgObj)) == CMSRET_SUCCESS)
        {
            sprintf(destinationAddress,"PVC: %s",buf);
            wanDslLinkCfgObj->enable = TRUE;
            CMSMEM_REPLACE_STRING(wanDslLinkCfgObj->X_BROADCOM_COM_ConnectionMode, MDMVS_VLANMUXMODE);
            CMSMEM_REPLACE_STRING(wanDslLinkCfgObj->destinationAddress, destinationAddress);
            strcpy(layer2IfName,"atm0");
            CMSMEM_REPLACE_STRING(wanDslLinkCfgObj->X_BROADCOM_COM_IfName, layer2IfName);
            ret = cmsObj_set(wanDslLinkCfgObj, iidStack_WanConnDev);
            cmsObj_free((void **) &wanDslLinkCfgObj);
        }
#else /* DMP_ADSLWAN_1 */
        ret = CMSRET_INVALID_ARGUMENTS;
#endif /* DMP_ADSLWAN_1 */
    }
    else
    {
#if defined DMP_PTMWAN_1
        _WanPtmLinkCfgObject *wanPtmLinkCfgObj = NULL;
        if((ret = cmsObj_get(MDMOID_WAN_PTM_LINK_CFG, iidStack_WanConnDev, 0, (void **) &wanPtmLinkCfgObj)) == CMSRET_SUCCESS)
        {
            wanPtmLinkCfgObj->enable = TRUE;
            strcpy(layer2IfName,"ptm0");
            ret = cmsObj_set(wanPtmLinkCfgObj, iidStack_WanConnDev);
            cmsObj_free((void **) &wanPtmLinkCfgObj);
        }
#elif defined DMP_X_BROADCOM_COM_GPONWAN_1
        _WanGponLinkCfgObject *wanGponLinkCfgObj = NULL;
        if((ret = cmsObj_get(MDMOID_WAN_GPON_LINK_CFG, iidStack_WanConnDev, 0, (void **) &wanGponLinkCfgObj)) == CMSRET_SUCCESS)
        {
            wanGponLinkCfgObj->enable = TRUE;
            CMSMEM_REPLACE_STRING(wanGponLinkCfgObj->connectionMode, MDMVS_VLANMUXMODE);
            strcpy(layer2IfName,"veip0");
            CMSMEM_REPLACE_STRING(wanGponLinkCfgObj->ifName, layer2IfName);
            ret = cmsObj_set(wanGponLinkCfgObj, iidStack_WanConnDev);
            cmsObj_free((void **) &wanGponLinkCfgObj);
        }
#else
        ret = CMSRET_INTERNAL_ERROR;
#endif
    }

    return ret;
}


CmsRet cli_facAddWanConnectionLocked(UBOOL8 isAddPvc, UBOOL8 isAddRoute, char *buf)
{
    CliRet ret = CLIRET_SUCCESS;
    #if 0 //zhan
	//if(cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT)==CMSRET_SUCCESS)
	{
   		InstanceIdStack wanDevIid; 
		WanDevObject    *wanDev  = NULL;   
		char currIfc[BUFLEN_64];   
		char serviceName[BUFLEN_32];  
		char defaultGwStr[BUFLEN_32];   
		char tempStr[BUFLEN_64];   
		char ifcListStr[BUFLEN_1024]={0};
		UBOOL8 usedForDns = FALSE;   
		UBOOL8 needEoAPvc = FALSE;   
		UBOOL8 needPortMirror = FALSE;   
		UBOOL8 firewallEnabledInfo = FALSE;  
		UBOOL8 bridgeIfcInfo = FALSE;   
		UBOOL8 needIPv6 = FALSE;   
		
		unsigned char vlan_in=0;
		char cmd[BUFLEN_128] = {0};

		INIT_INSTANCE_ID_STACK(&wanDevIid);
		while (cmsObj_getNext(MDMOID_WAN_DEV, &wanDevIid, (void **)&wanDev) == CMSRET_SUCCESS)
		{
			WanConnDeviceObject *connObj = NULL;
			InstanceIdStack      conDevIid;
			cmsObj_free((void **)&wanDev);  /* no longer needed */
			CtWANGponLinkConfigObject *linkCfg = NULL;

			INIT_INSTANCE_ID_STACK(&conDevIid);
      		while (cmsObj_getNextInSubTree(MDMOID_WAN_CONN_DEVICE, &wanDevIid, &conDevIid, (void **)&connObj) == CMSRET_SUCCESS) 
			{

				
				if (cmsObj_get(MDMOID_WAN_GPON_LINK_CONFIG, &conDevIid, 0, (void **)&linkCfg) == CMSRET_SUCCESS)
				{
					if(linkCfg->VLANIDMark==atoi(buf))
					{
						vlan_in=1;	
					}
					else
					{
						vlan_in=0;
					}
					ret=CLIRET_SUCCESS;
					cmsObj_free((void **)&linkCfg);
				}
				else
				{
					ret=CLIRET_NORMAL_SET_FAILURE;
					return ret;
				}

				
         		InstanceIdStack  iidStack; 
				WanPppConnObject *pppCon = NULL; 
				WanIpConnObject  *ipCon = NULL;
				INIT_INSTANCE_ID_STACK(&iidStack);
				unsigned char ppp_in=0;
				unsigned char ip_in=0;
				
				while (cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN, &conDevIid, &iidStack, (void **)&ipCon) == CMSRET_SUCCESS)
				{
					if(vlan_in==1)
					{
					    if(ppp_in==0)
				    	{
				    		cmsObj_deleteInstance(MDMOID_WAN_IP_CONN, &iidStack);
							break;
				    	}
					    if(isAddRoute==1)
					    {
					    		CMSMEM_REPLACE_STRING(ipCon->connectionType, "IP_Routed");
					    }
						if(isAddRoute==0)
					    {
					    		CMSMEM_REPLACE_STRING(ipCon->connectionType, "IP_Bridged");
					    }
						
						if(cmsObj_set(ipCon, &iidStack)==CMSRET_SUCCESS)
						{
							ret=CLIRET_SUCCESS;
						}
						else
						{
							ret=CLIRET_NORMAL_SET_FAILURE;
							return ret;
						}
						break;
					}
					ip_in++;
				}
				cmsObj_free((void **)&ipCon);

				INIT_INSTANCE_ID_STACK(&iidStack);
				while (cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN, &conDevIid, &iidStack, (void **)&pppCon) == CMSRET_SUCCESS) 
				{

					if(vlan_in==1)
					{
					    if(ip_in==0)
				    	{
				    		cmsObj_deleteInstance(MDMOID_WAN_PPP_CONN, &iidStack);
							break;
				    	}
					    if(isAddRoute==1)
					    {
								//CMSMEM_REPLACE_STRING(pppCon->connectionType, "IP_Routed");
					    }
						if(isAddRoute==0)
					    {
								CMSMEM_REPLACE_STRING(pppCon->connectionType, "PPPoE_Bridged");	
					    }
						if(cmsObj_set(pppCon, &iidStack)==CMSRET_SUCCESS)
						{
						    	ret=CLIRET_SUCCESS;
						}
						else
						{
								ret=CLIRET_NORMAL_SET_FAILURE;
								return ret;
						}
						break;
					}
					ppp_in++;
				}
				cmsObj_free((void **)&pppCon);
				

				if(
					(ppp_in==0)&&(vlan_in==1)
				  )
				{
					InstanceIdStack  iidStack;
			   		iidStack=conDevIid;
        	   		if((cmsObj_addInstance(MDMOID_WAN_IP_CONN, &iidStack)) == CMSRET_SUCCESS)
        	   		{
        				WanIpConnObject  *ipCon = NULL;
            			if((cmsObj_get(MDMOID_WAN_IP_CONN, &iidStack, 0, (void **) &ipCon)) == CMSRET_SUCCESS)
            			{
							if(isAddRoute==1)
					    	{
					    		CMSMEM_REPLACE_STRING(ipCon->connectionType, "IP_Routed");
					    	}
							if(isAddRoute==0)
					    	{
					    		CMSMEM_REPLACE_STRING(ipCon->connectionType, "IP_Bridged");
					    	}
							CMSMEM_REPLACE_STRING(ipCon->X_CT_COM_ServiceList, "INTERNET");
							if(cmsObj_set(ipCon, &iidStack)==CMSRET_SUCCESS)
							{
								ret=CLIRET_SUCCESS;
							}
							else
							{
								ret=CLIRET_NORMAL_SET_FAILURE;
								return ret;
							}
                			cmsObj_free((void **) &ipCon);
            			}
						else
						{
							ret=CLIRET_NORMAL_SET_FAILURE;
							return ret;
						}
        			}
					else
					{
						ret=CLIRET_NORMAL_SET_FAILURE;
						return ret;
					}
				}

				cmsObj_free((void **)&connObj);
				if(vlan_in==1)
				{
				    break;
				}

      	}
			
      	if(vlan_in==0)
		{ 
			   conDevIid=wanDevIid;
			   if((cmsObj_addInstance(MDMOID_WAN_CONN_DEVICE, &conDevIid)) == CMSRET_SUCCESS)
        	   {
        	   		if (cmsObj_get(MDMOID_WAN_GPON_LINK_CONFIG, &conDevIid, 0, (void **)&linkCfg) == CMSRET_SUCCESS)
					{
					    linkCfg->VLANIDMark=atoi(buf);
						if(cmsObj_set(linkCfg, &conDevIid)==CMSRET_SUCCESS)
						{
							ret=CLIRET_SUCCESS;
						}
						else
						{
								ret=CLIRET_NORMAL_SET_FAILURE;
								return ret;
						}
						cmsObj_free((void **)&linkCfg);
					}
					else
					{
						ret=CLIRET_NORMAL_SET_FAILURE;
						return ret;
					}
        	   }
			   else
			   {
			   		ret=CLIRET_NORMAL_SET_FAILURE;
					return ret;
			   }

			   if(isAddRoute==1)
			   {
			   		InstanceIdStack  iidStack;
			   		iidStack=conDevIid;
        	   		if((cmsObj_addInstance(MDMOID_WAN_IP_CONN, &iidStack)) == CMSRET_SUCCESS)
        	   		{
        				WanIpConnObject  *ipCon = NULL;
            			if((cmsObj_get(MDMOID_WAN_IP_CONN, &iidStack, 0, (void **) &ipCon)) == CMSRET_SUCCESS)
            			{
							if(isAddRoute==1)
					    	{
					    		CMSMEM_REPLACE_STRING(ipCon->connectionType, "IP_Routed");
					    	}
							if(isAddRoute==0)
					    	{
					    		CMSMEM_REPLACE_STRING(ipCon->connectionType, "IP_Bridged");
					    	}
							CMSMEM_REPLACE_STRING(ipCon->X_CT_COM_ServiceList, "INTERNET");
							if(cmsObj_set(ipCon, &iidStack)==CMSRET_SUCCESS)
							{
								ret=CLIRET_SUCCESS;
							}
							else
							{
								ret=CLIRET_NORMAL_SET_FAILURE;
								return ret;
							}
                			cmsObj_free((void **) &ipCon);
            			}
						else
						{
							ret=CLIRET_NORMAL_SET_FAILURE;
							return ret;
						}
        			}
					else
					{
						ret=CLIRET_NORMAL_SET_FAILURE;
						return ret;
					}
			   	}

			    //
			    if(isAddRoute==0)
			    {
			   		InstanceIdStack  iidStack;
			   		iidStack=conDevIid;
        	   		if((cmsObj_addInstance(MDMOID_WAN_PPP_CONN, &iidStack)) == CMSRET_SUCCESS)
        	   		{
        				WanPppConnObject *pppCon = NULL; 
            			if((cmsObj_get(MDMOID_WAN_PPP_CONN, &iidStack, 0, (void **) &pppCon)) == CMSRET_SUCCESS)
            			{
							if(isAddRoute==1)
					    	{
					    		CMSMEM_REPLACE_STRING(pppCon->connectionType, "IP_Routed");
					    	}
							if(isAddRoute==0)
					    	{
					    		CMSMEM_REPLACE_STRING(pppCon->connectionType, "PPPoE_Bridged");
					    	}
							CMSMEM_REPLACE_STRING(pppCon->X_CT_COM_ServiceList, "INTERNET");
							if(cmsObj_set(pppCon, &iidStack)==CMSRET_SUCCESS)
							{
								ret=CLIRET_SUCCESS;
							}
							else
							{
								ret=CLIRET_NORMAL_SET_FAILURE;
								return ret;
							}
                			cmsObj_free((void **) &pppCon);
            		   }
					   else
					   {
					   		ret=CLIRET_NORMAL_SET_FAILURE;
							return ret;
					   }
        		   }
				   else
				   {
				   	   ret=CLIRET_NORMAL_SET_FAILURE;
					   return ret;
				   }
			   	}
		}
		cmsObj_free((void **)&wanDev);
		}
	}
	#endif
    return ret;
}


CliRet facCmdDevicemodelGet(int argc, char** argv, char *pValue)
{
    CmsRet ret = CLIRET_SUCCESS;
    if(pValue)
        sprintf(pValue, "%s", DEVICE_MODEL);
    else
    {
        ret = CLIRET_NORMAL_GET_FAILURE;
        printf("pValue is NULL\n");
    }
    return ret;
    
}

