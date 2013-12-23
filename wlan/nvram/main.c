/***********************************************************************
 *
 *  Copyright (c) 2005  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2005:proprietary:standard
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

/*
 * Frontend command-line utility for Linux NVRAM layer
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <typedefs.h>
#include <bcmnvram.h>

#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "cms_dal.h"
#include "cms_cli.h"

#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "cms_boardcmds.h"
#include "cms_boardioctl.h"
#include "cms_core.h"

#include <typedefs.h>
#include <proto/ethernet.h>



/* Send msg to wlmngr */
void nvram_save_request(void)
{
	CmsRet ret = CMSRET_INTERNAL_ERROR;
	static char buf[sizeof(CmsMsgHeader) + 32]={0};
	void *msgHandle=NULL;
	
	if ((ret = cmsMsg_initWithFlags(EID_WLNVRAM, 0, &msgHandle)) != CMSRET_SUCCESS)  {
		printf("could not initialize msg, ret=%d", ret);
		return;
	}

	sleep(1);

	CmsMsgHeader *msg=(CmsMsgHeader *) buf;
	sprintf((char *)(msg + 1), "Restart");

	msg->dataLength = 24;
	msg->type = CMS_MSG_WLAN_CHANGED;
	msg->src = EID_WLNVRAM;
	msg->dst = EID_WLMNGR;
	msg->flags_event = 1;
	msg->flags_request = 0;

	if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
		printf("could not send msg to wlmngr. nvram is not saved. Err[%d]\n", ret);

	sleep(1);

	cmsMsg_cleanup(&msgHandle);
	return;
}


static void
usage(void)
{
	fprintf(stderr, "usage: nvram [get name] [set name=value] [unset name] [show] [commit]\n");
	exit(0);
}

/* NVRAM utility */
int
main(int argc, char **argv)
{
	char *name, *value, buf[NVRAM_SPACE];
	int size;

	/* Skip program name */
	--argc;
	++argv;

	if (!*argv) 
		usage();

	/* Process the remaining arguments. */
	for (; *argv; argv++) {
		if (!strncmp(*argv, "get", 3)) {
			if (*++argv) {
				if ((value = nvram_get(*argv)))
					puts(value);
			}
		}
		else if (!strncmp(*argv, "set", 3)) {
			if (*++argv) {
				strncpy(value = buf, *argv, sizeof(buf));
				name = strsep(&value, "=");
				nvram_set(name, value);
				nvram_commit();
			}
		}
		else if (!strncmp(*argv, "unset", 5)) {
			if (*++argv){
				nvram_unset(*argv);
				nvram_commit();
			}
		}
		else if (!strncmp(*argv, "commit", 6)) {
			nvram_commit();
			nvram_save_request();
		}
		else if (!strncmp(*argv, "show", 4) ||
			   !strncmp(*argv, "getall", 6)) {
			nvram_getall(buf, sizeof(buf));
			for (name = buf; *name; name += strlen(name) + 1)
				puts(name);
			size = sizeof(struct nvram_header) + (int) name - (int) buf;
			fprintf(stderr, "size: %d bytes (%d left)\n", size, NVRAM_SPACE - size);
		}
		
		if (!*argv)
			break;
	}

	return 0;
}	
