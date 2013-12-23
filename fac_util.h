#ifndef __FAC_UTIL_H__
#define __FAC_UTIL_H__

void cli_getWanConnSummaryLocked(char *varValue,int type);
#ifdef BRCM_WLAN
void cli_restartWlan(void);
int cli_getCurrentWlanBand(int wlIdx);
#endif /* BRCM_WLAN */
CmsRet cli_facAddWanConnectionLocked(UBOOL8 isAddPvc, UBOOL8 isAddRoute, char *buf);
CmsRet wlSendMsgToWlmngr(CmsEntityId sender_id, char *message_text);
#endif /* __FAC_UTIL_H__ */
