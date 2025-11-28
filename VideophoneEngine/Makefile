#
# The variables used for compilation flags are thus:
# CPPFLAGS: C Pre Processor flags.  Use this variable to set compilation flags for both .c and .cpp files
# CFLAGS: C flags.  Use this variable to set compilation flags that are only used with .c files.
# CXXFLAGS: C++ flags. Use this variable to set compilation flags that are only used with .cpp files.
#

SHELL := /bin/bash

RULE_HEADER := Executing rule for target:
RULE_FOOTER := Successfully executed rule for target:

export VPE_ROOT := $(shell pwd)
export MAKEFILE_RULES := $(VPE_ROOT)/Rules.mk

# make sure the important variables are set
ifndef APPLICATION
	export APPLICATION := APP_NTOUCH_VP3
endif

ifndef DEVICE
	export DEVICE := DEV_X86
endif

ifndef SUBDEVICE
	export SUBDEVICE := SUBDEV_MPU
endif

ifndef MODE
	export MODE := release
endif

ifndef BUILD_DIR
ifeq ($(DEVICE), DEV_NTOUCH_VP2)
ifeq ($(APPLICATION), APP_NTOUCH_VP2)
	export BUILD_DIR = Build/ntouchvp2/$(MODE)
endif
ifeq ($(APPLICATION), APP_NTOUCH_VP3)
	export BUILD_DIR = Build/ntouchvp3/$(MODE)
endif
ifeq ($(APPLICATION), APP_NTOUCH_VP4)
	export BUILD_DIR = Build/ntouchvp4/$(MODE)
endif
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP3)
	export BUILD_DIR = Build/ntouchvp3/$(MODE)
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP4)
	export BUILD_DIR = Build/ntouchvp4/$(MODE)
endif

ifeq ($(DEVICE), DEV_X86)
	export RANLIB := ranlib

ifeq ($(APPLICATION), APP_NTOUCH_VP2)
	export BUILD_DIR = Build/ntouchvp2_x86/$(MODE)
endif
ifeq ($(APPLICATION), APP_NTOUCH_VP3)
	export BUILD_DIR = Build/ntouchvp3_x86/$(MODE)
endif
ifeq ($(APPLICATION), APP_NTOUCH_VP4)
	export BUILD_DIR = Build/ntouchvp4_x86/$(MODE)
endif
endif
endif

ifeq ($(DEVICE), DEV_X86)
	export RANLIB := ranlib
endif 

ifndef PLATFORM
	export PLATFORM := LINUX
endif

ifndef USE_CLANG_TIDY
	export USE_CLANG_TIDY := 0
endif

# Determine the architecture (32 or 64 bit)
ARCH :=  $(shell getconf LONG_BIT)
CPP_FLAGS_32 := -m32
CPP_FLAGS_64 := -m64


# These are the Radvision makefile variables that enable/disable logging.  Comment out the lines
# to enable logging in Radvision.  These declarations have to remain here (or higher) in the source hiearchy
# in order to properly include Radvision headers into application source.
#
# Make sure radvision logging is enabled if output is enabled
ifneq ($(RV_LOGGING), true)
ifneq ($(RV_LOGGING_OUTPUT), true)
	export NOLOG := On
	CPPFLAGS += -DRV_CFLAG_NOLOG
endif
endif

ifeq ($(RV_LOGGING_OUTPUT), true)
	CPPFLAGS += -DVPE_RV_LOGGING
endif

# TODO: what's the correct way to do this?
CPPFLAGS += -DSRTP_ADD_ON

#Fix this later when we get Radvision building into MODE dependent folders
#ifeq ($(MODE), debug)
#	CPPFLAGS += -DRV_DEBUG
#else
#CPPFLAGS += -DRV_RELEASE
#endif

ifeq ($(RCU_CAPTURE_APP), true)
	CPPFLAGS += -DRCU_CAPTURE_APP
endif

DOC := \
	pm \
	os \
	cci \
	MsgMgr

DIRS := \
	common \
	ConfMgr \
	http \
	os \
	EndpointMonitor
	
ifneq ($(APPLICATION), APP_REGISTRAR_LOAD_TEST_TOOL)
	DIRS += cci \
	BlockListMgr \
	CallHistoryMgr \
	contacts \
	services \
	FileDownload \
	FileUpload \
	ImageMgr \
	MsgMgr \
	vrcl \
	xmlmgr \
	RingGroupMgr \
	RegistrationMgr \
	pm \
	UserAccountMgr \
	Serialization
endif
	
# Don't have support for Apple yet. Remove when there is Platform for Apple platform.
ifeq ($(PLATFORM), LINUX)
	DIRS += Platform
endif

export OBSERVER_PTR_DIR := $(VPE_ROOT)/Libraries/observer_ptr
export PM_INCLUDE_DIR := $(VPE_ROOT)/pm/
export IXML_INCLUDE_DIR := $(VPE_ROOT)/Libraries/Linux/libupnp-1.6.2/ixml/inc
export XMLMGR_INCLUDE_DIR := $(VPE_ROOT)/xmlmgr
export OS_DIR := $(VPE_ROOT)/os
export COMMON_DIR := $(VPE_ROOT)/common
export HTTP_DIR := $(VPE_ROOT)/http
export PLATFORM_DIR := $(VPE_ROOT)/Platform
export SERVICES_DIR := $(VPE_ROOT)/services
export CORE_SERVICE_DIR := $(VPE_ROOT)/services/Core
export STATENOTIFY_SERVICE_DIR := $(VPE_ROOT)/services/StateNotify
export CONFERENCE_SERVICE_DIR := $(VPE_ROOT)/services/Conference
export MESSAGE_SERVICE_DIR := $(VPE_ROOT)/services/Message
export LOGGER_SERVICE_DIR := $(VPE_ROOT)/services/RemoteLogger
export OUTAGE_SERVICE_DIR := $(VPE_ROOT)/services/ServiceOutage
export BLOCKLISTMGR_DIR := $(VPE_ROOT)/BlockListMgr
export CONTACTS_DIR := $(VPE_ROOT)/contacts
export CCI_DIR := $(VPE_ROOT)/cci
export MSGMGR_DIR := $(VPE_ROOT)/MsgMgr
export REGISTRATIONMGR_DIR := $(VPE_ROOT)/RegistrationMgr
export FILEDOWNLOAD_DIR := $(VPE_ROOT)/FileDownload
export FILEUPLOAD_DIR = $(VPE_ROOT)/FileUpload
export CALLHISTORYMGR_DIR := $(VPE_ROOT)/CallHistoryMgr
export RINGGROUPMGR_DIR := $(VPE_ROOT)/RingGroupMgr
export IMAGEMGR_DIR := $(VPE_ROOT)/ImageMgr
export VRCL_DIR := $(VPE_ROOT)/vrcl
export CONFMGR_DIR := $(VPE_ROOT)/ConfMgr
export USERACCOUNTMGR_DIR = $(VPE_ROOT)/UserAccountMgr
export SERIALIZATION_DIR = $(VPE_ROOT)/Serialization
export ENDPOINT_MONITOR_DIR = $(VPE_ROOT)/EndpointMonitor

# Radvision locations
export SIPDIR := $(VPE_ROOT)/Libraries/Radvision
export RVSHAREDDIR := $(VPE_ROOT)/Libraries/Radvision
export RVSHAREDDIR_INC := $(RVSHAREDDIR)/$(BUILD_DIR)/include
export ACMEDIR := $(VPE_ROOT)/Libraries/Acme/tscf
export ACMEDIR_INC := $(ACMEDIR)/include
export TSCLIB := $(VPE_ROOT)/Libraries/Acme/tscf
export RVUSRCONFIG_HEADER := $(RVSHAREDDIR)/common/config/rvusrconfig.h 
export RVUSRCONFIG_BACKUP := $(RVSHAREDDIR)/common/config/rvusrconfig_h

# upnp settings
export UPNP_DIR := $(VPE_ROOT)/Libraries/Linux/libupnp-1.6.2
export UPNP_BUILD_DIR := $(UPNP_DIR)/sci-build/$(BUILD_DIR)
ifeq ($(DEVICE), $(filter $(DEVICE), DEV_NTOUCH_VP2))
	UPNPCFG := \
		CPPFLAGS=-D_GNU_SOURCE \
		--host=arm-linux \
		--target=arm-linux \
		--build=i686-pc-linux-gnu \
		--host=i686
else ifeq ($(DEVICE), $(filter $(DEVICE), DEV_NTOUCH_VP3))
	UPNPCFG := \
		CPPFLAGS=-D_GNU_SOURCE \
		--host=x86_64-linux \
		$(CONFIGURE_FLAGS)
else ifeq ($(DEVICE), $(filter $(DEVICE), DEV_NTOUCH_VP4))
	UPNPCFG := \
		CPPFLAGS=-D_GNU_SOURCE \
		--host=arm-linux \
		--target=arm-linux \
		--build=i686-pc-linux-gnu \
		--host=i686
else
	UPNPCFG := \
		CPPFLAGS=-D_GNU_SOURCE
endif

export IXML_DIR := $(UPNP_DIR)/ixml

# ipv6 settings
#ifeq ($(APPLICATION), APP_NTOUCH_VP2)
#	CPPFLAGS += -DIPV6_ENABLED=1 -DRV_CFLAG_IPV6
#	export IPV6_ENABLED := true
#endif

# translate the debug mode into a yes/no flag
ifeq ($(MODE), release)
	export DEBUG = no

else
	export DEBUG = yes
	CPPFLAGS += -g

ifeq ($(USE_ASAN), true)
# 	Note ASAN enabled for C++ only. If you enable for C code you will get too many false positive leaks from Radvision, etc.
	CXXFLAGS += -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer
	LDFLAGS += -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer
	export LDFLAGS
endif

endif

CPPFLAGS += -fexceptions

ifneq ($(PLATFORM), APPLE)
ifeq ($(DEVICE), DEV_X86)
	CPPFLAGS += \
		$(CPP_FLAGS_$(ARCH))
endif

ifeq ($(COVERAGE), true)
	CPPFLAGS += \
	-fprofile-arcs \
	-ftest-coverage
endif
else
	CPPFLAGS += $(STDCFLAGS)
endif

# gcc 4.8 seems to have issues with aggregate initializations, disable this warning
CPPFLAGS += \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-Wno-type-limits \
	-Wno-missing-field-initializers \
	-DDEVICE=$(DEVICE) \
	-DSUBDEVICE=$(SUBDEVICE) \
	-DAPPLICATION=$(APPLICATION) \
	-include $(SMI_CONFIG_PATH)/smiConfig.h \
	-I$(VPE_ROOT)/Libraries/observer_ptr

# We need this to handle deprecations in the new 2025 radvision release, 
# until we upgrade our client code.
CPPFLAGS += \
	-Wno-deprecated-declarations

# Beethoven needs these flags for dumping backtraces
ifeq ($(DEVICE), DEV_NTOUCH_VP2)
CPPFLAGS += \
	-rdynamic -funwind-tables
endif

ifeq ($(DEVICE), DEV_X86)
CPPFLAGS += \
    -Wno-deprecated-declarations \
    -Wno-ignored-qualifiers \
	-Wno-overloaded-virtual \
	-Wno-ignored-attributes
endif

ifneq (,$(findstring clang,$(CXX)))
# Lumina2: suppress deprecated declarations for now, specifically RSA_new() in SelfSignedCert.cpp
	CXXFLAGS += \
		-Wno-overloaded-virtual \
		-Wno-deprecated-declarations

# Yocto SDK includes this flag - even though it's not implmeneted in clang
	CFLAGS := $(filter-out -feliminate-unused-debug-types,$(CFLAGS))
	CXXFLAGS := $(filter-out -feliminate-unused-debug-types,$(CXXFLAGS))
endif

ifneq ($(PLATFORM), APPLE)
	CPPFLAGS += -MD \
		-Werror
endif

CPPFLAGS += \
	-DsmiSW_VERSION_NUMBER=\"$(smiSW_VERSION_NUMBER)\"

CXXFLAGS += \
	-fcheck-new

ifeq ($(APPLICATION), APP_NTOUCH_VP2)
	CPP11 = yes
endif
# Note: VP3 & above use flags defined by Yocto environment
ifeq ($(APPLICATION), APP_REGISTRAR_LOAD_TEST_TOOL)
	CPP11 = yes
endif
ifdef CPP11
	CFLAGS += -std=c11
	CFLAGS += -rdynamic
	CXXFLAGS += -std=c++11
	CXXFLAGS += -rdynamic
endif

# The -g flag still gets added somewhere, so we need to filter it out.
ifeq ($(MODE), release)
CFLAGS := $(filter-out -g,$(CFLAGS))
CXXFLAGS := $(filter-out -g,$(CXXFLAGS))
endif

export CPPFLAGS
export CFLAGS
export CXXFLAGS

export DISTRIBUTION_FOLDER = $(VPE_ROOT)/Distribution/$(BUILD_DIR)
export DOC_FOLDER = $(VPE_ROOT)/Documentation/$(BUILD_DIR)

#------------------ TARGETS -------------------

# ------------------------------
# all (default) Rule
# ------------------------------
.PHONY: all
all: validate_variables

ifneq ($(APPLICATION), APP_REGISTRAR_LOAD_TEST_TOOL)
	@if [ ! -e $(UPNP_BUILD_DIR)/Makefile ]; \
	then { \
		mkdir -p $(UPNP_BUILD_DIR); \
		pushd $(UPNP_BUILD_DIR); \
		$(UPNP_DIR)/configure --disable-webserver --disable-largefile --disable-samples --enable-debug=$(DEBUG) $(UPNPCFG); \
		popd; \
	} fi

	$(MAKE) -C $(UPNP_BUILD_DIR)
endif

ifneq ($(SUBDEVICE), SUBDEV_RCU)
ifneq ($(APPLICATION), APP_REGISTRAR_LOAD_TEST_TOOL)
	$(MAKE) -C $(ACMEDIR)
endif
endif

	mkdir -p $(RVSHAREDDIR)/$(BUILD_DIR)

	@if [ -e $(RVUSRCONFIG_HEADER) ]; then \
		mv -f $(RVUSRCONFIG_HEADER)  $(RVUSRCONFIG_BACKUP) ; \
	fi

	
	pushd $(RVSHAREDDIR)/$(BUILD_DIR) && \
	TUNNELING_INCLUDE_DIR=$(ACMEDIR_INC) \
	APPLICATION=$(APPLICATION) \
	cmake $(RVSHAREDDIR)/build -DCMAKE_BUILD_TYPE=Release \
	-DSP_CCFG_UDP_RBUFSIZE=-1 \
	-DSP_CCFG_LDAP=1 \
	-DSP_CCFG_WEBSOCKET=0 \
	-DSP_CCFG_LOGLEVEL=EXCEP
	
	$(MAKE) -C $(RVSHAREDDIR)/$(BUILD_DIR)

	@$(foreach TARGET, $(DIRS), $(MAKE) -C $(TARGET) || exit 1;)

	$(MAKE) install

	@if [ -e $(RVUSRCONFIG_BACKUP) ]; then \
		mv -f $(RVUSRCONFIG_BACKUP)  $(RVUSRCONFIG_HEADER) ; \
	fi
	
#	$(MAKE) document

ifeq ($(APPLICATION), APP_NTOUCH_VP2)
	$(MAKE) -C Tests
endif

ifeq ($(APPLICATION), APP_NTOUCH_VP3)
ifeq ($(DEVICE), DEV_X86)
	$(MAKE) -C Tests
endif
endif

ifeq ($(APPLICATION), APP_NTOUCH_VP4)
ifeq ($(DEVICE), DEV_X86)
	$(MAKE) -C Tests
endif
endif

#ifeq ($(DEVICE), DEV_NTOUCH_VP2)
#	$(MAKE) -C Platform/ntouch-vp2/Tests
#endif

install:
	@$(foreach TARGET, $(DIRS), $(MAKE) -C $(TARGET) install || exit 1;)
	$(MAKE) -C Distribution
	
document:
	@$(foreach TARGET, $(DIRS), $(MAKE) -C $(TARGET) document || exit 1;)
	$(MAKE) -C Documentation
	
clean:
	@$(foreach TARGET, $(DIRS), $(MAKE) -C $(TARGET) clean || exit 1;)

	$(MAKE) -C Tests clean

	$(MAKE) -C Distribution clean

ifneq ("$(wildcard $(RVSHAREDDIR)/$(BUILD_DIR))","")
	$(MAKE) -C $(RVSHAREDDIR)/$(BUILD_DIR) clean
endif
	rm -rf $(RVSHAREDDIR)/$(BUILD_DIR)

	$(MAKE) -C $(ACMEDIR) clean

	rm -rf $(UPNP_BUILD_DIR)
	
#ifeq ($(DEVICE), DEV_NTOUCH_VP2)
#	$(MAKE) -C Platform/ntouch-vp2/Tests clean
#endif

build-mac:
	@$(foreach TARGET, $(DIRS), $(MAKE) -C $(TARGET) || exit 1;)

	$(MAKE) install

clean-mac:
	@$(foreach TARGET, $(DIRS), $(MAKE) -C $(TARGET) clean || exit 1;)

	$(MAKE) -C Distribution clean

# ------------------------------
#  - validate_variables (default, must be first)
# ------------------------------
.PHONY: validate_variables
validate_variables:
	@echo "    ----- $(RULE_HEADER) $@"

	@echo "    ----- Validating variables: $@"
ifndef APPLICATION
	$(error APPLICATION is not defined)
else
	@echo "          APPLICATION is $(APPLICATION)"
endif

ifndef DEVICE
	$(error DEVICE is not defined)
else
	@echo "          DEVICE is $(DEVICE)"
endif

ifndef SUBDEVICE
	$(error SUBDEVICE is not defined)
else
	@echo "          SUBDEVICE is $(SUBDEVICE)"
endif

	@echo "          Successfully validated variables: $@"

	@echo "          $(RULE_FOOTER) $@"

clean-docs:
	rm -rf doc/cci
	rm -rf doc/os
	rm -rf doc/pm

docs:
	mkdir -p Distribution/$(BUILD_DIR)/doc
	@$(foreach TARGET, $(DOC), $(MAKE) -C $(TARGET) docs;)
	@pushd doc/ui/html; \
	./installdox -q -l pm.doxytag@../../pm/html -l cci.doxytag@../../cci/html \
	-l os.doxytag@../../os/html
