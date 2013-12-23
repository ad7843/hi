#ifndef __FAC_CMD_H__
#define __FAC_CMD_H__

#ifdef ARRAY_SIZE
#undef ARRAY_SIZE
#endif
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define MAX_FAC_CMD_ARGS        6
#define FAC_CMD_ACTION_GET      "get"
#define FAC_CMD_ACTION_SET      "set"
#define FAC_CMD_ACTION_SET_NONSAVE      "set-nonsave"
#define FAC_CMD_ACTION_LIST     "list"
#define FAC_CMD_STATUS_ON       "on"
#define FAC_CMD_STATUS_OFF      "off"
#define FAC_CMD_STATUS_RED      "red"

#define FAC_RET 0
#define FAC_GET_RET 1
#define FAC_SET_RET 2


typedef enum
{
#if 1
    CLIRET_SUCCESS=001,     
    CLIRET_REBOOT_RESET=002,    
    CLIRET_REFACTORY_RESET=003, 
    
    CLIRET_NORMAL_SET_FAILURE=100, 
    CLIRET_NORMAL_GET_FAILURE=100,
    CLIRET_NOT_SUPPORT_FUNCTION=101, 
    CLIRET_NOT_SUPPORT_OPERATION=102, 
    CLIRET_INVALIDATE_PARAM=103,
	CLIRET_NOT_SUPPORT_CMD=107
#endif
    
}CliRet;

typedef CliRet (*TFacCmdSetFunc) (int,char**);
typedef CliRet (*TFacCmdSetNonSaveFunc) (int,char**);
typedef CliRet (*TFacCmdGetListFunc) (int,char**,char*);

typedef struct
{
   char*                    parameterName;
   char*                    displayName;
   TFacCmdGetListFunc       TFacCmdGetFunc;
   TFacCmdSetFunc           TFacCmdSetFunc;
   TFacCmdSetNonSaveFunc           TFacCmdSetNonSaveFunc;
   TFacCmdGetListFunc       TFacCmdListFunc;
}TFacCmdHandler;

CliRet facCmdSwVersionGet(int argc, char** argv, char *pValue);
CliRet facCmdBaseMacGet(int argc, char** argv, char *pValue);
CliRet facCmdBaseMacSet(int argc, char** argv);
CliRet facCmdSerialNumberGet(int argc, char** argv, char *pValue);
CliRet facCmdSerialNumberSet(int argc, char** argv);
CliRet facCmdRegionGet(int argc, char** argv, char *pValue);
CliRet facCmdRegionSet(int argc, char** argv);
CliRet facCmdRegionList(int argc, char** argv, char *pValue);
CliRet facCmdUserPasswordGet(int argc, char** argv, char *pValue);
CliRet facCmdUserPasswordSet(int argc, char** argv);
CliRet facCmdAdminPasswordGet(int argc, char** argv, char *pValue);
CliRet facCmdAdminPasswordSet(int argc, char** argv);
CliRet facCmdClearPortBindSet(int argc, char** argv);
CliRet facCmdPvcInfoGet(int argc, char** argv, char *pValue);
CliRet facCmdDslRateGet(int argc, char** argv, char *pValue);
CliRet facCmdRestorefactorySet(int argc, char** argv);
CliRet facCmdLANIpAddrGet(int argc, char** argv, char *pValue);
CliRet facCmdLANIpAddrSet(int argc, char** argv);
CliRet facCmdSignatureModeGet(int argc, char** argv, char *pValue);
CliRet facCmdSignatureModeSet(int argc, char** argv);
CliRet facCmdWanGet(int argc, char** argv, char *pValue);
CliRet facCmdWanSet(int argc, char** argv);
CliRet facCmdWlanGet(int argc, char** argv, char *pValue);
CliRet facCmdWlanSet(int argc, char** argv);
CliRet facCmdWlan5gGet(int argc, char** argv, char *pValue);
CliRet facCmdWlan5gSet(int argc, char** argv);
CliRet facCmd5gSsidNameSet(int argc, char** argv);
CliRet facCmd5gSsidNameGet(int argc, char** argv, char *pValue);
CliRet facCmdSsidNameGet(int argc, char** argv, char *pValue);
CliRet facCmdSsidNameSet(int argc, char** argv);
CliRet facCmdSsidKeyGet(int argc, char** argv, char *pValue);
CliRet facCmdSsidKeySet(int argc, char** argv);
CliRet facCmdCurssidnameGet(int argc, char** argv, char *pValue);
CliRet facCmdCurssidnameSet(int argc, char** argv);
CliRet facCmdDiswifiauthSet(int argc, char** argv);
CliRet facCmdWlanTxbwGet(int argc, char** argv, char *pValue);
CliRet facCmdWlanTxbwSet(int argc, char** argv);
CliRet facCmdVoiceSet(int argc, char** argv);
CliRet facCmdVoiceGet(int argc, char** argv, char *pValue);
CliRet facCmdlaserSet(int argc, char** argv);
CliRet facCmdlaserGet(int argc, char** argv,char *pValue);
CliRet facCmdLaserTxpowerGet(int argc, char** argv, char *pValue);
CliRet facCmdGponSNGet(int argc, char** argv, char *pValue);
CliRet facCmdGponSNSet(int argc, char** argv);
CliRet facCmdLedSet(int argc, char** argv);
CliRet facCmdLANIpAddrSetNonsave(int argc, char** argv);
CliRet facCmdSignatureModeSetNonsave(int argc, char** argv);
CliRet facCmdWanSetNonsave(int argc, char** argv);
CliRet facCmdUSBTest(int argc, char** argv);
CliRet facCmdProduceModeGet(int argc, char** argv, char *pValue);
CliRet facCmdProduceModeSet(int argc, char** argv);
CliRet facCmdLoidGet(int argc, char** argv, char *pValue);
CliRet facCmdLoidSet(int argc, char** argv);
CliRet facCmdRogDetectGet(int argc, char** argv, char *pValue);
CliRet facCmdRogDetectSet(int argc, char** argv);
CliRet facCmdrodccheckGet(int argc, char** argv, char *pValue);
CliRet facCmdDevicemodelGet(int argc, char** argv, char *pValue);
#define SYS_MGMT_SERIAL_NUMBER
#define SYS_USER_USER_NVRAM
#endif /* __FAC_CMD_H__ */
