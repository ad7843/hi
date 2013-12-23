#***********************************************************************
#
#  Copyright (c) 2006  Broadcom Corporation
#  All Rights Reserved
#
#  <:label-BRCM:2011:proprietary:standard
#  
#   This program is the proprietary software of Broadcom Corporation and/or its
#   licensors, and may only be used, duplicated, modified or distributed pursuant
#   to the terms and conditions of a separate, written license agreement executed
#   between you and Broadcom (an "Authorized License").  Except as set forth in
#   an Authorized License, Broadcom grants no license (express or implied), right
#   to use, or waiver of any kind with respect to the Software, and Broadcom
#   expressly reserves all rights in and to the Software and all intellectual
#   property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#   NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#   BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
#  
#   Except as expressly set forth in the Authorized License,
#  
#   1. This program, including its structure, sequence and organization,
#      constitutes the valuable trade secrets of Broadcom, and you shall use
#      all reasonable efforts to protect the confidentiality thereof, and to
#      use this information only in connection with your use of Broadcom
#      integrated circuit products.
#  
#   2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#      AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#      WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#      RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#      ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#      FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#      COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#      TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#      PERFORMANCE OF THE SOFTWARE.
#  
#   3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#      ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#      INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#      WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#      IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#      OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#      SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#      SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#      LIMITED REMEDY.
:>
#
#***********************************************************************/

#
# In most cases, you only need to modify this first section.
#
EXE = httpd

OBJS = main.o httpd.o basic.o ej.o cgi_cmd.o cgi_dhcpd.o cgi_diag.o cgi_main.o \
       cgi_ntwk.o cgi_route.o cgi_sec.o cgi_sts.o cgi_upload.o cgi_util.o cgi_wan.o
# cgi_lanports.o is now compiled out

WAN_OBJS = cgi_qos.o cgi_qos_class.o \
           cgi_qos_queue.o cgi_qos_policer.o cgi_ethintf.o \
           cgi_pptpacintf.o cgi_seclog.o cgi_autodetection.o cgi_qos_port_shaping.o


CUSTOM_CFLAGS += $(BRCM_APP_CFLAGS)

LIBS = -lcms_dal -lcms_msg $(CMS_COMMON_LIBS) -ldl $(CMS_CORE_LIBS)
     
all dynamic install: $(EXE) generic_exe_install install_cert cpall_html


# mwang: see the cfm Makefile, there are some more steps involving deleting
# wireless and voice html pages if they are not configured.
# Or should I just remove all the html pages from INSTALL_DIR/webs?
clean: generic_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE)



#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

BASE_HTMLS = backupsettings.html defaultsettings.html dhcpinfo.html diag.html diag8021ag.html    \
             diagbr.html diaglan.html dnscfg.html footer.html hlpethconn.html hlpusbconn.html hlpwlconn.html \
             index.html info.html lancfg2.html lockerror.html logconfig.html logintro.html       \
             logo.html main.html menu.html password.html rebootinfo.html resetrouter.html        \
             restoreinfo.html routeadd.html StaticIpAdd.html StaticIpErr.html statsifc.html      \
             statsifcreset.html updatesettings.html upload.html uploadinfo.html diagethoam.html

WAN_HTMLS =  algcfg.html diagipow.html diagmer.html diagpppoa.html diagpppoe.html                   \
             enblbridge.html enblservice.html hlpatmetoe.html hlpatmseg.html hlppngdns.html         \
             hlppnggw.html hlppppoasess.html hlppppoeauth.html hlppppoeconn.html hlppppoeip.html    \
             hlptstdns.html ifcdns.html ifcgateway.html ipoacfg.html lanvlancfg.html natcfg2.html   \
             ntwksum2.html pppoe.html pradd.html qoscls.html qospoliceradd.html qosportshaping.html \
             qosqmgmt.html qosqueueadd.html rtdefaultcfgerr.html rtdefaultcfg.html                  \
             scdmz.html scinflt.html scmacflt.html scmacpolicy.html scoutflt.html scprttrg.html     \
             scvrtsrv.html seclogintro.html statswanreset.html wanadderr.html wancfg.html

UNUSED_HTMLS = enblbridge.html enblservice.html statsatmerr.html statsatm.html statsatmreset.html

HTMLS = html/*.js html/*.gif html/*.css $(addprefix html/,$(BASE_HTMLS))

ifneq ($(strip $(BUILD_WAN_HTML)),)
OBJS += $(WAN_OBJS)
HTMLS += $(addprefix html/,$(WAN_HTMLS))
endif

ifneq ($(strip $(BUILD_VODSL)),)
include $(BUILD_DIR)/make.voice
LIBS += -lpthread
OBJS += cgi_voice.o
HTMLS += html/voice*.html
endif

ifneq ($(strip $(BUILD_OMCI)),)
LIBS +=  -lomci
OBJS += cgi_omci_cfg.o cgi_omci_create.o cgi_omci_getnext.o \
	cgi_omci_macro.o cgi_omci_download.o cgi_omci_raw.o
HTMLS += html/omci*.html
endif

ifneq ($(strip $(BUILD_GPON)),)
OBJS += cgi_gponintf.o cgi_sts_dev2.o
HTMLS += html/*gpon*.html html/statsopticifc*.html
endif

ifneq ($(strip $(BUILD_QUICKSETUP)),)
OBJS += cgi_quicksetup.o
HTMLS += html/quick*.html html/qsmain.html
endif

ifneq ($(strip $(BUILD_STORAGESERVICE)),)
OBJS += cgi_storageservice.o
HTMLS += html/storage*.html
endif

ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
OBJS += cgi_wl.o
HTMLS += html/wl*.html html/wifi*.html html/cfgwlwan.html
endif

ifneq ($(strip $(BUILD_LIBMODSW)),)
OBJS += cgi_modsw_ee.o cgi_modsw_linuxpfp.o
LIBS += -lmodsw
endif

ifneq ($(strip $(BUILD_CERT)),)
OBJS += cgi_cert.o 
HTMLS += html/cert*.html
endif

ifneq ($(strip $(BUILD_VCONFIG)),)
OBJS += cgi_pmap.o
HTMLS += html/port*.html
endif

ifneq ($(strip $(BUILD_BMU)),)
OBJS += cgi_bmu.o
HTMLS += html/bmu.html
endif

ifneq ($(strip $(BUILD_DDNSD)),)
OBJS += cgi_ddns.o
HTMLS += html/ddns*.html
endif

ifneq ($(strip $(BUILD_DLNA)),)
OBJS += cgi_dlna.o
HTMLS += html/dlna*.html
endif

ifneq ($(strip $(BUILD_DPROXY)),)
OBJS += cgi_dnsproxy.o
HTMLS += html/dnsp*.html
endif

ifneq ($(strip $(BUILD_DPROXYWITHPROBE)),)
OBJS += cgi_dnsproxy.o
HTMLS += html/dnsp*.html
endif

ifneq ($(strip $(BUILD_DSL)),)
OBJS += cgi_dslintf.o
HTMLS += html/*dsl*.html html/cfgatm.html html/cfgptm.html html/atmdelerr.html html/ber*.html html/statsxtmreset.html html/ptm*.html
endif

ifneq ($(strip $(BUILD_DEBUG_TOOLS)),)
OBJS += cgi_dump.o cgi_eng.o
HTMLS += html/eng*.html
endif

ifneq ($(strip $(BUILD_EPONWAN)),)
OBJS += cgi_eponintf.o
HTMLS += html/*epon*.html
endif

ifneq ($(strip $(BUILD_ETHWAN)),)
HTMLS += html/eth*.html html/cfgeth.html
endif

ifneq ($(strip $(BUILD_IPPD)),)
OBJS += cgi_ipp.o
HTMLS += html/ippcfg.html
endif

ifneq ($(strip $(BUILD_IPSEC_TOOLS)),)
OBJS += cgi_ipsec.o
HTMLS += html/ips*.html
endif

ifneq ($join $(strip $(DMP_X_BROADCOM_COM_IGMP_1)), $(strip $(DMP_X_BROADCOM_COM_MLD_1)), $(strip $(DMP_X_BROADCOM_COM_MCAST_1)),)
OBJS += cgi_multicast.o
endif

ifneq ($(strip $(BUILD_L2TPAC)),)
OBJS += cgi_l2tpacintf.o
HTMLS += html/*l2tp*.html
endif

ifneq ($(strip $(BUILD_IPV6)),)
OBJS += cgi_iptunnel.o cgi_lan6.o
HTMLS += html/ipv6lancfg.html html/tunnel*.html
endif

ifneq ($(strip $(BUILD_MOCAWAN)),)
OBJS += cgi_mocaintf.o
HTMLS += html/*moca*.html
endif

ifneq ($(strip $(BUILD_MCAST_PROXY)),)
HTMLS += html/multicast.html
endif

ifneq ($(strip $(BUILD_PWRCTL)),)
OBJS += cgi_pwrmngt.o
HTMLS += html/pwrmngt.html
endif

ifneq ($(strip $(BUILD_SNMP)),)
HTMLS += html/snmpconfig.html
endif

ifneq ($(strip $(BUILD_SNTP)),)
HTMLS += html/sntpcfg.html
endif

ifneq ($(strip $(BRCM_STANDBY)),)
OBJS += cgi_standby.o 
HTMLS += html/standby.html
endif

ifneq ($(strip $(BUILD_TOD)),)
OBJS += cgi_tod.o 
HTMLS += html/todadd.html
endif

ifneq ($(strip $(BUILD_TR69C)),)
HTMLS += html/tr69*.html
endif

ifneq ($(strip $(BUILD_TR69C_SSL)),)
HTMLS += html/tr69*.html
endif

ifneq ($(strip $(BUILD_TR69C_BCM_SSL)),)
HTMLS += html/tr69*.html
endif

ifneq ($(strip $(BUILD_UPNP)),)
HTMLS += html/upnpcfg.html
endif

ifneq ($(strip $(BUILD_URLFILTER)),)
HTMLS += html/url_add.html
OBJS += cgi_urlfilter.o
endif


cpall_html:		              
	mkdir -p $(INSTALL_DIR)/webs
	cp -f $(HTMLS) $(INSTALL_DIR)/webs

#
# Private apps and libs are allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR) \
                         -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) \
                         -I$(KERNEL_LINKS_DIR) \
                         -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)	\
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core	\
                         -I$(BUILD_DIR)/userspace/private/libs/dare_dns \
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core/linux

#
# Private apps and libs are allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public

ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
WIRELESS_DRIVER_PATH=$(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)
ALLOWED_INCLUDE_PATHS +=   -I$(BUILD_DIR)/userspace/private/apps/wlan/wlmngr  \
                                                 -I$(WIRELESS_DRIVER_PATH)/include
LIBS += -lwlctl -lnvram -lwlmngr 
endif


# unforunately, this custom stuff has to be put here, after the include make.common
ifneq ($(strip $(BUILD_HTTPD_SSL)),)
	LIBS += -lssl -lcrypto
	CUSTOM_CFLAGS += -I$(BUILD_DIR)/userspace/public/libs/openssl/include
endif

ifneq ($(strip $(BUILD_HTTPD_SSL)),)
install_cert:
	mkdir -p $(INSTALL_DIR)/webs/CA
	cp -f -r html/CA/* $(INSTALL_DIR)/webs/CA
else
install_cert:
endif

LIBS += -lc -lgcc_s


# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors


#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
# 
$(EXE): $(OBJS)
	$(CC) -o $@ $^ -Wl,-rpath,$(CMS_LIB_RPATH) $(CMS_LIB_PATH) $(LIBS)



#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

-include $(OBJS:.o=.d)
