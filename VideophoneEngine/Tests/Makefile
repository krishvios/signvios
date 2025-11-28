EXECUTABLE := Test

EXECUTABLE_DEPENDENTS := ../Distribution/$(BUILD_DIR)/libVideophoneEngine.a

LIBS := \
	-lVideophoneEngine \
	-lcppunit \
	-lpthread \
	-lssl \
	-lrt \
	-lcrypto \
	-lresolv \
	-ljpeg \
	-ldl \
	-lz \
	-lboost_program_options \
	-lboost_filesystem \
	-lboost_system \
	-lPocoFoundation \
	-lPocoUtil \
	-lPocoCrypto \
	-lPocoJSON \
	-lPocoNet \
	-lPocoNetSSL \
	-lPocoZip \
	`PKG_CONFIG_PATH=$(VPE_ROOT) PKG_CONFIG_SYSROOT_DIR=$(VPE_ROOT) pkg-config --define-variable=BUILDDIR=$(BUILD_DIR) --libs vpe`

ifeq ($(DEVICE), DEV_X86)
LIBS += \
	`pkg-config --libs gstreamer-1.0` \
	`pkg-config --libs gstreamer-video-1.0` \
	`pkg-config --libs gstreamer-app-1.0` \
	`pkg-config --libs gstreamer-pbutils-1.0` \
	`pkg-config --libs glib-2.0` \
	`pkg-config --libs gio-2.0` \
	`pkg-config --libs bluez` \
	
#	`pkg-config --libs gstreamer-0.10` \
#	`pkg-config --libs gstreamer-app-0.10`
#	ifeq (($APPLICATION), APP_NTOUCH_VP2)
#	LIBS += \
#		-ludev
#	endif
endif

ifeq ($(APPLICATION), APP_NTOUCH_VP2)
CXXFLAGS += \
	-fexceptions \
	-frtti

LIBS += \
	-llber \
	-lldap \
	-lldap_r
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP2)
LIBS += \
	-lrcu_api \
	`pkg-config --libs gstreamer-0.10` \
	`pkg-config --libs gstreamer-app-0.10` \
	`pkg-config --libs glib-2.0` \
	`pkg-config --libs gthread-2.0` \
	`pkg-config --libs dbus-glib-1`	 \
	`pkg-config --libs gio-2.0` \
	`pkg-config --libs bluez` \
	-ludev \
	-lbluealsa \
	-lboost_system \
	-lboost_filesystem
endif

ifeq ($(APPLICATION), APP_NTOUCH_VP3)
CXXFLAGS += \
	-fexceptions \
	-frtti \
	-fPIC

LIBS += \
	-llber \
	-lldap \
	-lldap_r
endif

ifeq ($(APPLICATION), APP_NTOUCH_VP4)
CXXFLAGS += \
	-fexceptions \
	-frtti \
	-fPIC

LIBS += \
	-llber \
	-lldap \
	-lldap_r
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP3)
LIBS += \
	`pkg-config --libs gstreamer-1.0` \
	`pkg-config --libs gstreamer-video-1.0` \
	`pkg-config --libs gstreamer-app-1.0` \
	`pkg-config --libs gstreamer-pbutils-1.0` \
	`pkg-config --libs glib-2.0` \
	`pkg-config --libs gthread-2.0` \
	`pkg-config --libs dbus-glib-1`	 \
	`pkg-config --libs gio-2.0` \
	`pkg-config --libs bluez` \
	-ludev
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP4)
LIBS += \
	`pkg-config --libs gstreamer-1.0` \
	`pkg-config --libs gstreamer-video-1.0` \
	`pkg-config --libs gstreamer-app-1.0` \
	`pkg-config --libs gstreamer-pbutils-1.0` \
	`pkg-config --libs glib-2.0` \
	`pkg-config --libs gthread-2.0` \
	`pkg-config --libs dbus-glib-1`	 \
	`pkg-config --libs gio-2.0` \
	`pkg-config --libs bluez` \
	-ludev
endif

LIBDIRS := \
	-L../Distribution/$(BUILD_DIR) \
	-L../pm/$(BUILD_DIR)

OBJS := \
	Test.o \
	PropertyManagerTest.o \
	PropertyManagerReadTest.o \
	stiToolsTest.o \
	CstiIpAddressTest.o \
	IstiVideophoneEngineTest.o \
	StringsTest.o \
	SignalTest.o \
	CstiListTest.o \

DEFS:= \
	-D_VERBOSE \
	-D_GNU_SOURCE

INCS := \
	-I$(BLOCKLISTMGR_DIR) \
	-I$(MSGMGR_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_DIR)/common \
	-I$(PLATFORM_DIR)/common/wireless \
	-I$(PLATFORM_DIR)/common/audio \
	-I$(PLATFORM_DIR)/common/video \
	-I$(PLATFORM_DIR)/ntouch-vp \
	-I$(PLATFORM_DIR)/ntouch-vp/common \
	-I$(CALLHISTORYMGR_DIR) \
	-I$(CCI_DIR) \
	-I$(COMMON_DIR) \
	-I$(CONTACTS_DIR) \
	-I$(CONFMGR_DIR) \
	-I$(CONFMGR_DIR)/DataTasks/AudioPlayback \
	-I$(CONFMGR_DIR)/DataTasks/AudioRecord \
	-I$(CONFMGR_DIR)/DataTasks/DataCommon \
	-I$(CONFMGR_DIR)/DataTasks/VideoPlayback \
	-I$(CONFMGR_DIR)/DataTasks/VideoRecord \
	-I$(CONFMGR_DIR)/Session \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(CONFERENCE_SERVICE_DIR) \
	-I$(LOGGER_SERVICE_DIR) \
	-I$(OUTAGE_SERVICE_DIR) \
	-I$(FILEPLAY_DIR) \
	-I$(RVSHAREDDIR)/common/config \
	-I$(RVSHAREDDIR)/common/config/arch \
	-I$(RVSHAREDDIR)/common/config/os \
	-I$(RVSHAREDDIR)/common/config/tool \
	-I$(RVSHAREDDIR)/common/cutils \
	-I$(RVSHAREDDIR)/include/common \
	-I$(IXML_DIR)/inc \
	-I$(UPNP_DIR)/threadutil/inc \
	-I$(UPNP_DIR)/upnp/inc \
	-I$(UPNP_BUILD_DIR)/upnp/inc \
	-I$(ACMEDIR)/include \
	-I$(HTTP_DIR) \
	-I$(IMAGEMGR_DIR) \
	-I$(MPEG4_DIR) \
	-I$(MPEG4_DIR)/Toolbox \
	-I$(MPEG4_DIR)/Atoms \
	-I$(OS_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(VRCL_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(RINGGROUPMGR_DIR) \
	-I$(REGISTRATIONMGR_DIR) \
	-I$(USERACCOUNTMGR_DIR) \
	-I$(SERIALIZATION_DIR) \
	-I$(ENDPOINT_MONITOR_DIR)
	
include $(MAKEFILE_RULES)


ifeq ($(DEVICE), DEV_X86)
# This looks legitimate, but not worth investigating in the softphone
CPPFLAGS += \
    -Wno-infinite-recursion
endif
