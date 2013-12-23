/*************************************************
//  Copyright (C), Dare Tech. Co., Ltd.
//  File name:          fac_cmd.c
//  Author:             Dare Oversea Team
//  Creation Date:      2012.02.08
//  Description:        Fac command support.
**************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fac_cmd.h"

#define SUPPORT_LATER NULL
TFacCmdHandler g_atFacCmdHandlerTable[] =
{
    /* parameterName,   displayName,         TFacCmdGetFunc,           TFacCmdSetFunc,           TFacCmdSetNonSaveFunc,   TFacCmdListFunc */
    {"producemode",     "producemode",       facCmdProduceModeGet,     facCmdProduceModeSet,     NULL,              				NULL},
    {"swversion",       "version",           facCmdSwVersionGet,       NULL,                     NULL,              				NULL},
    {"basemac",         "basemac",           facCmdBaseMacGet,         facCmdBaseMacSet,         NULL,              				NULL},
    {"serialnumber",    "sn",                facCmdSerialNumberGet,    facCmdSerialNumberSet,    NULL,								NULL},
    {"region",          "region",            facCmdRegionGet,          facCmdRegionSet,          NULL,  				facCmdRegionList},
    {"userpassword",    "userpassword",      facCmdUserPasswordGet,    facCmdUserPasswordSet,    NULL,								NULL},
    {"adminpassword",      "adminpassword",     facCmdAdminPasswordGet,   facCmdAdminPasswordSet,   NULL,				                NULL},
    {"clear-port-binding",  NULL,                      NULL,                                facCmdClearPortBindSet,                               NULL, NULL},
    {"pvcinfo",         NULL,                facCmdPvcInfoGet,         NULL,                     NULL,								NULL},
    {"dslrate",         NULL,                facCmdDslRateGet,         NULL,                     NULL,								NULL},
    {"restorefactory",  NULL,                NULL,                     facCmdRestorefactorySet,  NULL,								NULL},
    {"lanipaddr",       "lanipaddr",         facCmdLANIpAddrGet,       facCmdLANIpAddrSet,       facCmdLANIpAddrSetNonsave,								NULL},
    {"usbtest",         NULL,                NULL,                     facCmdUSBTest,            NULL,								NULL},
    {"signature-mode",  "signature-mode",    facCmdSignatureModeGet,   facCmdSignatureModeSet,   facCmdSignatureModeSetNonsave,								NULL},
    {"wan",             NULL,                facCmdWanGet,             facCmdWanSet,             facCmdWanSetNonsave,								NULL},
    {"wlan",            "wlan",              facCmdWlanGet,            facCmdWlanSet,            NULL,								NULL},
    {"wlan5g",            "wlan5g",              facCmdWlan5gGet,            facCmdWlan5gSet,            NULL,								NULL},
    {"curssidname",        "curssidname",          facCmdSsidNameGet,        facCmdSsidNameSet,        NULL,								NULL},
    {"cur5gssidname",        "cur5gssidname",          facCmd5gSsidNameGet,        facCmd5gSsidNameSet,        NULL,								NULL},
//    {"ssidname",        "ssidname",          facCmdSsidNameGet,        facCmdSsidNameSet,        NULL,								NULL},
//    {"ssidkey",         "ssidkey",           facCmdSsidKeyGet,         facCmdSsidKeySet,         NULL,								NULL},
    {"curssidname",     "curssidname",       facCmdCurssidnameGet,     facCmdCurssidnameSet,     NULL,								NULL},
    {"disable-wifi-auth",  "disable-wifi-auth",        NULL,           facCmdDiswifiauthSet,     NULL,								NULL},
//    {"wlan-txbw",       "wlan-txbw",         facCmdWlanTxbwGet,        facCmdWlanTxbwSet,        NULL,								NULL},
    {"voice",           "regstatus ",        facCmdVoiceGet,           facCmdVoiceSet,           NULL,								NULL},
    {"loid",            "loid",              facCmdLoidGet,            facCmdLoidSet,            NULL,								NULL},
    {"laser",        "laser",          facCmdlaserGet,            facCmdlaserSet,        NULL,								NULL},
    {"lasertxpower",    "lasertxpower",      facCmdLaserTxpowerGet,    NULL,                     NULL,								NULL},
    {"gponserialnumber","sn",                facCmdGponSNGet,          facCmdGponSNSet,          NULL,								NULL},
    {"rodc","rodc",                facCmdRogDetectGet,          facCmdRogDetectSet,          NULL,								NULL},
    {"rodccheck",    "rodccheck",      facCmdrodccheckGet,    NULL,                     NULL,								NULL},
    {"led",             NULL,                NULL,                     facCmdLedSet,             NULL,								NULL},
//    {"devicemodel",     "devicemodel",       facCmdDevicemodelGet,     NULL,                     NULL,                              NULL},                
};

static void printCliCmdResult(CliRet ret,unsigned char op)
{
    //printf("%d\n", ret);
    if(op==FAC_RET)
    {
    	    if(ret<4)
    		{
    			printf("RET=00%d\n", ret);
    		}
			else
			{
				printf("RET=%d\n", ret);
			}
    }
	else if(op==FAC_GET_RET)
	{
	    	if(ret<4)
    		{
    			printf("GET_RET=00%d\n", ret);
    		}
			else
			{
				printf("GET_RET=%d\n", ret);
			}
	}
	else if(op==FAC_SET_RET)
	{
	    	if(ret<4)
    		{
    			printf("SET_RET=00%d\n", ret);
    		}
			else
			{
				printf("SET_RET=%d\n", ret);
			}
	}

}



void processFacCmd(char *cmdLine __attribute((unused)))
{
    unsigned int dwArraySize;
    int argc = 0;
    char *argv[MAX_FAC_CMD_ARGS];
    char *pchLast = NULL;
    char *parameterName = NULL;
    char *action = NULL;
    char pValue[1024] = {0};
    CliRet ret = CLIRET_SUCCESS;

    parameterName = strtok_r(cmdLine, " ", &pchLast);
    if(parameterName == NULL)
    {
        printCliCmdResult(CLIRET_INVALIDATE_PARAM,FAC_RET);
        return;
    }
    else
    {
        if(strcmp(parameterName,"usbtest")==0)
		{
		   char tmp[1024] = {0};
		   sprintf(tmp,"set ");
		   pchLast=strncat(tmp,pchLast,strlen(pchLast));
		}
        action = strtok_r(NULL, " ", &pchLast);
        if(action == NULL)
        {
            printCliCmdResult(CLIRET_INVALIDATE_PARAM,FAC_RET);
            return;
        }
        else
        {
            if(
			   cmsUtl_strcmp(action, FAC_CMD_ACTION_GET) == 0 || 
               cmsUtl_strcmp(action, FAC_CMD_ACTION_SET) == 0  
               //cmsUtl_strcmp(action, FAC_CMD_ACTION_LIST) == 0||
               //cmsUtl_strcmp(action, FAC_CMD_ACTION_SET_NONSAVE) == 0
               )
            {
                while((argc <= MAX_FAC_CMD_ARGS) && (argv[argc] = strtok_r(NULL, " ", &pchLast)) != NULL)
                {
                    argc++;
                }
				
            }
            else
            {

                printCliCmdResult(CLIRET_INVALIDATE_PARAM,FAC_RET);
                return;
            }
        }
    }

    for (dwArraySize=0; dwArraySize < ARRAY_SIZE(g_atFacCmdHandlerTable); dwArraySize++)
    {
        if(cmsUtl_strcmp(g_atFacCmdHandlerTable[dwArraySize].parameterName, parameterName) == 0)
        {
            if(cmsUtl_strcmp(action, FAC_CMD_ACTION_GET) == 0)
            {
                if((*(g_atFacCmdHandlerTable[dwArraySize].TFacCmdGetFunc)) == NULL)
                {
                    printCliCmdResult(CLIRET_INVALIDATE_PARAM,FAC_GET_RET);
                }
                else
                {
                    ret = (*(g_atFacCmdHandlerTable[dwArraySize].TFacCmdGetFunc))(argc,argv,pValue);
                    if(ret == CLIRET_SUCCESS)
                    {
                        if(g_atFacCmdHandlerTable[dwArraySize].displayName != NULL)
                            printf("%s=%s\n",g_atFacCmdHandlerTable[dwArraySize].displayName,pValue);
                        else
                            printf(pValue);
                    }
                    else
                    {
                        printCliCmdResult(ret,FAC_GET_RET);
                    }
                }
            }
            else if(cmsUtl_strcmp(action, FAC_CMD_ACTION_SET) == 0)
            {
                if((*(g_atFacCmdHandlerTable[dwArraySize].TFacCmdSetFunc)) == NULL)
                {
                    printCliCmdResult(CLIRET_INVALIDATE_PARAM,FAC_SET_RET);
                }
                else
                {
                    ret = (*(g_atFacCmdHandlerTable[dwArraySize].TFacCmdSetFunc))(argc,argv);
                    printCliCmdResult(ret,FAC_SET_RET);
                }
            }
			else if(cmsUtl_strcmp(action, FAC_CMD_ACTION_SET_NONSAVE) == 0)
            {
                if((*(g_atFacCmdHandlerTable[dwArraySize].TFacCmdSetNonSaveFunc)) == NULL)
                {
                    printCliCmdResult(CLIRET_INVALIDATE_PARAM,FAC_SET_RET);
                }
                else
                {
                    ret = (*(g_atFacCmdHandlerTable[dwArraySize].TFacCmdSetNonSaveFunc))(argc,argv);
                    printCliCmdResult(ret,FAC_SET_RET);
                }
            }
            else if(cmsUtl_strcmp(action, FAC_CMD_ACTION_LIST) == 0)
            {
                if((*(g_atFacCmdHandlerTable[dwArraySize].TFacCmdListFunc)) == NULL)
                {
                    printCliCmdResult(CLIRET_INVALIDATE_PARAM,FAC_GET_RET);
                }
                else
                {
                    ret = (*(g_atFacCmdHandlerTable[dwArraySize].TFacCmdListFunc))(argc,argv,pValue);
                    if(ret == CLIRET_INVALIDATE_PARAM)
                    {
                        printCliCmdResult(ret,FAC_GET_RET);
                    }
                    else
                    {
                        printf("%s=%s\n",g_atFacCmdHandlerTable[dwArraySize].displayName,pValue);
                        //printf(pValue);
                    }
                }
            }
            return;
        }
		else
		{
		    if(dwArraySize==(ARRAY_SIZE(g_atFacCmdHandlerTable)-1))
		    {
		    	printCliCmdResult(CLIRET_INVALIDATE_PARAM,FAC_RET);
		    }
		}
    }
	
    return;
}
