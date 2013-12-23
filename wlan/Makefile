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
# To add an app, add it to APP_DIRS, and write the target rule in the
# last section of this Makefile
#
WLAN_USERSPACE_CFG = 1
-include $(BUILD_DIR)/bcmdrivers/broadcom/net/wl/bcm9$(BRCM_CHIP)/wl/config/wlconfig_lx_wl_dslcpe
-include $(BUILD_DIR)/bcmdrivers/broadcom/net/wl/bcm9$(BRCM_CHIP)/dslcpe_wlan_minor_version

export DSLCPE_MK=1
ifeq ($(shell if [ $(SVN_IMPL) -ge 20 ]; then echo 1; else echo 0; fi),1)
WLCTL_MAKEFILE := GNUmakefile
else
WLCTL_MAKEFILE := Makefile
endif

DSLCPE_WLAN_VERSION=cpe$(BRCM_VERSION).$(BRCM_VERSION)$(BRCM_RELEASE).$(DSLCPE_WLAN_MINOR_VERSION)
CFLAGS += -DDSLCPE_WLAN_VERSION=\"$(DSLCPE_WLAN_VERSION)\"
CFLAGS += $(BRCM_WERROR_CFLAGS)

WLLIB_APPS = nvram wlctl
WLLIBS = bcmwlcrypto bcmwlshared $(WLLIB_APPS)

WLANAPP_DIRS = usbdev util acsd wlmngr nas epittcp lltd libupnp wps ses eapd wapi ias hspot_ap

#
# We need to declare these directory names as phony targets,
# otherwise make will think the directories themselves are
# the targets and will never execute the cmds.
# See GNU Make, 4.6 
#
.PHONY: $(WLLIBS) $(WLANAPP_DIRS)

default: all

$(WLANAPP_DIRS): $(WLLIBS)

all: $(WLANAPP_DIRS)

#
# Cleaning rules
# See example in GNU Make, 4.6
# (what a pain, upnp does not have a top level Makefile, so
# we have to take upnp out of APP_DIRS for the clean.)
#
clean:
	$(foreach dir, $(WLANAPP_DIRS) $(WLLIB_APPS), \
	  if [ -e $(dir) ]; then $(MAKE) -C $(dir) clean; fi;\
	  if [ -L $(dir) ]; then rm -f $(dir); fi; )


#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

include $(KERNEL_DIR)/.config
ifneq ($(WL),)
IMPL := $(patsubst IMPL%,%,$(WL))
else
IMPL := $(CONFIG_BCM_WLAN_IMPL)
endif


ifneq ($(findstring $(IMPL),$(NIGHTLY_IMPLS)),)
BUILD_NAS=
BUILD_WSC=
endif

ifeq ($(EXT_ACS),y)
CFLAGS += -DEXT_ACS
export EXT_ACS
export CONFIG_BCMDCS=y
endif

ifeq ($(EXT_WPS_VER2),y)
export EXT_WPS_VER2
endif

ifeq ($(EXT_WL11AC),y)
export EXT_WL11AC
ifeq ($(EXT_WLCONF),y)
export EXT_WLCONF
endif
endif

EXT_CPU_ARCH_NAME := $(subst ",,$(CONFIG_BCM_CPU_ARCH_NAME))
#"#
export EXT_CPU_ARCH_NAME

bcmwlcrypto:
	$(MAKE) -C $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/bcmcrypto dynamic install

bcmwlshared:
	if [ -f $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/shared/Makefile ]; then \
		$(MAKE) -C $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/shared dynamic install; \
	elif [ -f $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/shared/Makefile ]; then \
		$(MAKE) -C $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/shared dynamic install; \
	fi

##############################################################
# 
# Rules for inidividual apps
# apps can be compiled or not depending on the profile (which
# is modified via make menuconfig)
#
##############################################################

#
#wireless wl command, a soft link directory
#
ifneq ($(strip $(BUILD_WLCTL)),)
export BUILD_WLCTL_SHLIB=1
wlctl:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ $@ ; \
	elif [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/wl/exe ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/wl/exe $@ ; \
	fi; \
	if [ -e $(@)/$(WLCTL_MAKEFILE) ]; then \
	$(MAKE) -f $(WLCTL_MAKEFILE) -C $@ $(BUILD_WLCTL) ;\
	fi
else
wlctl:
	@echo "skipping $@ (not configured)"
endif

#	
#wireless nas command, a soft link directory		
#
ifneq ($(strip $(BUILD_NAS)),)
nas:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ $@ ; \
	elif [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ $@ ; \
	fi; \
	if [ -e $(@)/Makefile ]; then \
	$(MAKE) -C $@ $(BUILD_NAS) install ;\
	fi
else
nas:
	@echo "skipping $@ (not configured)"
endif

#
#eapd, a soft link directory
#
ifneq ($(strip $(BUILD_NAS)),)
eapd:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ $@ ; \
	elif [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ $@ ; \
	fi; \
	if [ -e $@/Makefile ]; then \
	$(MAKE) -C $@ $(BUILD_NAS) install ;\
	fi
else
eapd:
	@echo "skipping $@ (not configured)"
endif

#
#acsd, a soft link directory
#
ifneq ($(strip $(EXT_ACS)),)
acsd:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ $@ ; \
	   $(MAKE) -C $@ dynamic ;\
	fi
else
acsd:
	@echo "skipping $@ (not configured)"
endif


#
#epi_ttcp
#
ifneq ($(strip $(BUILD_EPITTCP)),)
epittcp:
	$(MAKE) -C $@ $(BUILD_EPITTCP)
else
epittcp:
	@echo "skipping $@ (not configured)"
endif


#
#nvram emulation
#
ifneq ($(strip $(BUILD_NVRAM)),)
nvram:
	$(MAKE) -C $@ $(BUILD_NVRAM)
else
nvram:
	@echo "skipping $@ (not configured)"
endif

wlmngr:
	$(MAKE) -C $@ all install

util:
	$(MAKE) -C $@ all install
#
#lltd, a soft link directory
#
ifneq ($(strip $(BUILD_LLTD)),)
lltd:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ $@ ; \
	elif [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ $@ ; \
	fi; \
	if [ -e $(@)/Makefile ]; then \
	$(MAKE) -C $@ $(BUILD_LLTD) install ;\
	fi
else
lltd:
	@echo "skipping $@ (not configured)"
endif

ifeq ($(BUILD_WLHSPOT),y)
hspot_ap:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ $@ ; \
	fi; \
	if [ -e $@/Makefile ]; then \
	$(MAKE) -C $@ install ;\
	fi
else
hspot_ap:
	@echo "skipping $@ (not configured)"
endif
#wps, a soft link directory
#
ifneq ($(strip $(BUILD_WSC)),)
export BUILD_WSC_CONFIG_UPNP_DISABLE=0
# BUILD_WSC_CONFIG_UPNP_DISABLE=1 to disable UPNP w/ AP config mode
# BUILD_WSC_CONFIG_UPNP_DISABLE=0 should be 0 for WiFi test
wps: libupnp
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ $@ ; \
	fi; \
	if [ -e $@/Makefile ]; then \
	$(MAKE) -C $@ install ;\
	fi
else
wps:
	@echo "skipping $@ (not configured)"
endif

ifneq ($(strip $(BUILD_WSC)),)
libupnp:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ $@ ; \
	elif [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ $@ ; \
	fi; \
	if [ -e $@/Makefile ]; then \
	$(MAKE) -C $@ $(BUILD_WSC) install; \
	fi
else
libupnp:
	@echo "skipping $@ (not configured)"
endif

#
#ses, a soft link directory
#
ifneq ($(strip $(BUILD_SES)),)
ses:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ $@ ; \
	fi; \
	if [ -e $(@)/Makefile ]; then \
	$(MAKE) -C $@ $(BUILD_SES) install;\
	fi
else
ses:
	@echo "skipping $@ (not configured)"
endif

#
#usbdev apps for Wlan USB driver
#
ifneq (,$(findstring usb, $(strip $(WLBUS))))
#ifeq ($(strip $(BRCM_DRIVER_WIRELESS_USBAP)),y)
usbdev:
	@echo "copy bcmdl utilities to " $(INSTALL_DIR)/sbin
	rm -f $@
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/$@ $@ ; \
	fi

	cd usbdev; tar xzkf libusb-0.1.12/libusb-0.1.12.tgz 2>/dev/null; cd ..
	make -C usbdev/libusb-0.1.12 clean
	make TARGETENV=linuxmips -C usbdev/libusb-0.1.12
	install -m 755 usbdev/libusb-0.1.12/.libs/libusb-0.1.so.4 $(INSTALL_DIR)/lib

#	make -C usbdev/usbdl clean
#	make -C usbdev/usbdl
	install -m 755 usbdev/usbdl/bcmdl $(INSTALL_DIR)/bin
	cp -f $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/dongle/rte/wl/builds/4322-bmac/rom-ag/rtecdc.trx $(INSTALL_DIR)/etc/wlan
else
usbdev:
	@echo "skipping $@ (not configured)"
endif

#
#wapi, a soft link directory
#
ifeq ($(BRCM_WAPI),y)
wapi:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/router/$@ $@ ; \
	fi; \
	if [ -e $@/Makefile ]; then \
	$(MAKE) -C $@ $(BUILD_WAPI) install ;\
	fi
else
wapi:
	@echo "skipping $@ (not configured)"
endif

#
#ias, a soft link directory
#
ifeq ($(BRCM_WAPI),y)
ias:
	rm -f $@; \
	if [ -e $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/wapi ]; then \
	   ln -s $(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/wapi ias ; \
	fi; \
	if [ -e ias/Makefile ]; then \
	$(MAKE) -C $@ $(BUILD_WAPI) install ;\
	fi
else
ias:
	@echo "skipping $@ (not configured)"
endif
