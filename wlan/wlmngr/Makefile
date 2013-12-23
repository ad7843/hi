#***********************************************************************
#
#  Copyright (c) 2006  Broadcom Corporation
#  All Rights Reserved
#
# 
# 
# This program is the proprietary software of Broadcom Corporation and/or its 
# licensors, and may only be used, duplicated, modified or distributed pursuant 
# to the terms and conditions of a separate, written license agreement executed 
# between you and Broadcom (an "Authorized License").  Except as set forth in 
# an Authorized License, Broadcom grants no license (express or implied), right 
# to use, or waiver of any kind with respect to the Software, and Broadcom 
# expressly reserves all rights in and to the Software and all intellectual 
# property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE 
# NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY 
# BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE. 
# 
# Except as expressly set forth in the Authorized License, 
# 
# 1. This program, including its structure, sequence and organization, 
#    constitutes the valuable trade secrets of Broadcom, and you shall use 
#    all reasonable efforts to protect the confidentiality thereof, and to 
#    use this information only in connection with your use of Broadcom 
#    integrated circuit products. 
# 
# 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS" 
#    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR 
#    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH 
#    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND 
#    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, 
#    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR 
#    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE 
#    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR 
#    PERFORMANCE OF THE SOFTWARE. 
# 
# 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR 
#    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, 
#    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY 
#    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN 
#    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; 
#    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
#    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS 
#    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY 
#    LIMITED REMEDY. 
#
#
#***********************************************************************/

#
# In most cases, you only need to modify this first section.
#

EXE  = wlmngr wlevt
OBJS = wlmngr.o wlapi.o wlmdm.o wlsyscall.o wldsltr.o wlutil.o wlmngr_enum_ifnames.o nvram_mdm.o
ifeq ($(EXT_WLCONF),y)
OBJS += wl.o wl_linux.o shutils.o wlconf.o defaults.o bcmwifi_channels.o nvparse.o
endif
LIBS = -lcms_msg $(CMS_COMMON_LIBS) -lcms_dal -l cms_cli -lcms_msg $(CMS_COMMON_LIBS) -ldl $(CMS_CORE_LIBS)
LIBS += -lwlctl -lnvram -lwlmngr

all dynamic: libwlmngr $(EXE)
install: all
	install -m 755 $(EXE) $(INSTALL_DIR)/bin
	install -m 755 libwlmngr.so $(INSTALL_DIR)/lib

clean: generic_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE)
	rm -f libwlmngr.so


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common
ifeq ($(BRCM_WAPI),y)
OBJS += wlmngr_wapi_cert_mdm.o wlmngr_wapi_cert_mgmt.o
endif

WIRELESS_DRIVER_PATH=$(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)
INC_WIRELESS_DRIVER_PATH=$(WIRELESS_DRIVER_PATH)/include

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
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core \
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core/linux 

# obviously, this app needs to access kernel headers
ALLOWED_INCLUDE_PATHS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) \
                         -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) \
                         -I$(BUILD_DIR)/shared/broadcom/include/$(BRCM_BOARD) \
                         -I$(BUILD_DIR)/shared/opensource/include/$(BRCM_BOARD) \
                         -I$(INC_WIRELESS_DRIVER_PATH) \
                         -I$(WIRELESS_DRIVER_PATH)/bcmcrypto \
                         -I$(WIRELESS_DRIVER_PATH)/shared \
                         -I$(WIRELESS_DRIVER_PATH)/router/shared \
                         -I$(WIRELESS_DRIVER_PATH)/shared/bcmwifi/include

#
# Private apps and libs are allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/lib/public:/lib/private

ifeq (exists, $(shell [ -d $(WIRELESS_DRIVER_PATH)/router/eapd ]&& echo exists))
    CFLAGS += -DBUILD_EAPD -DDSLCPE -I$(WIRELESS_DRIVER_PATH)/router/eapd
endif
CFLAGS +=-DWIRELESS -DSUPPORT_MIMO -DDSLCPE_SHLIB -DBCMWPA2
CFLAGS += -DWMF
CFLAGS += -DDUCATI
ifneq ($(strip $(BUILD_WL_IDLE_PWRSAVE)),)
   CFLAGS += -DIDLE_PWRSAVE
endif

ifeq ($(BUILD_WLHSPOT),y)
CFLAGS += -DHSPOT_SUPPORT
endif

ifeq ($(EXT_ACS),y)
CFLAGS += -DEXT_ACS
endif

ifeq ($(EXT_WPS_VER2),y)
CFLAGS += -DWPS_V2
endif

ifneq ($(strip $(BUILD_WSC)), )
CFLAGS += -DSUPPORT_WSC
ifeq ($(strip $(BUILD_WSC_CONFIG_UPNP_DISABLE)),1)
CFLAGS += -DWSC_CONFIG_UPNP_DISABLE
endif
endif

ifeq ($(EXT_WL11AC),y)
CFLAGS += -DWL11AC
endif

ifeq ($(EXT_WLCONF),y)
CFLAGS += -DWLCONF
endif

# treat all warnings as errors
# #CUSTOM_CFLAGS += -Wno-error=unused-function -Wno-error=unused-but-set-variable -Werror -Wfatal-errors  # NEW_FORBID_WARNINGS

ifeq ($(EXT_WLCONF),y)
vpath %.c $(WIRELESS_DRIVER_PATH)/router/wlconf/
vpath %.c $(WIRELESS_DRIVER_PATH)/router/shared/
vpath %.c $(WIRELESS_DRIVER_PATH)/shared/bcmwifi/src/
endif

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<


libwlmngr: $(OBJS) 
	$(LD) -shared -soname libwlmngr.so -o libwlmngr.so $(OBJS)
#-lpthread
	install -m 755 libwlmngr.so $(INSTALL_DIR)/lib
	
#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
# 
wlmngr: libwlmngr wldaemon.o
	$(CC) -o $@ wldaemon.o -s -Os -fomit-frame-pointer $(CMS_LIB_PATH) $(LIBS) -L$(LIBDIR) -L$(EXTRALIBDIR) -lc -lgcc_s 

wlevt: wlevt.o
	$(CC) -o $@ $^ -s -Os -fomit-frame-pointer $(CMS_LIB_PATH) -L$(LIBDIR) -L$(EXTRALIBDIR) -lc -lgcc_s -lcms_msg $(CMS_COMMON_LIBS)



#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

#-include $(OBJS:.o=.d)





