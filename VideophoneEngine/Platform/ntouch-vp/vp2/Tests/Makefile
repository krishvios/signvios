VIDEOPHONE_DIR = ../../..

GSTREAMER_VERSION ?= 1

ifeq ($(GSTREAMER_VERSION),10)
	GSTREAMER_LIB = 1.0
else
	GSTREAMER_LIB = 0.10
endif

EXECUTABLE := test

EXECUTABLE_DEPENDENTS := $(VIDEOPHONE_DIR)/Distribution/$(BUILD_DIR)/libVideophoneEngine.a

LIBS := \
	-lVideophoneEngine \
	-lpthread \
	-lssl \
	-lrt \
	-lcrypto \
	-lresolv \
	-ljpeg \
	-ldl
	
ifeq ($(DEVICE), DEV_X86)
LIBS += \
	`pkg-config --libs gstreamer-0.10` \
	`pkg-config --libs gstreamer-app-0.10`
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP2)
LIBS += \
	`pkg-config --libs gstreamer-0.10` \
	`pkg-config --libs gstreamer-app-0.10` \
	`pkg-config --libs glib-2.0` \
	`pkg-config --libs gthread-2.0` \
	`pkg-config --libs dbus-glib-1`	
endif


	
LIBDIRS := \
	-L$(VIDEOPHONE_DIR)/Distribution/$(BUILD_DIR) \
	-L$(VIDEOPHONE_DIR)/pm/$(BUILD_DIR) \
	
	
OBJS := \
       test.o
		
DEFS:= \
	-D_VERBOSE \
	-D_GNU_SOURCE \


INCS := \
	-I.. \
	-I$(VIDEOPHONE_DIR)/BlockListMgr \
	-I$(VIDEOPHONE_DIR)/MsgMgr \
	-I$(VIDEOPHONE_DIR)/Platform \
	-I$(VIDEOPHONE_DIR)/Platform/common \
	-I$(VIDEOPHONE_DIR)/Platform/common/wireless \
	-I$(VIDEOPHONE_DIR)/CallHistoryMgr \
	-I$(VIDEOPHONE_DIR)/cci \
	-I$(VIDEOPHONE_DIR)/common \
	-I$(VIDEOPHONE_DIR)/contacts \
	-I$(VIDEOPHONE_DIR)/ConfMgr \
	-I$(VIDEOPHONE_DIR)/ConfMgr/DataTasks/AudioPlayback \
	-I$(VIDEOPHONE_DIR)/ConfMgr/DataTasks/AudioRecord \
	-I$(VIDEOPHONE_DIR)/ConfMgr/DataTasks/DataCommon \
	-I$(VIDEOPHONE_DIR)/ConfMgr/DataTasks/VideoPlayback \
	-I$(VIDEOPHONE_DIR)/ConfMgr/DataTasks/VideoRecord \
	-I$(VIDEOPHONE_DIR)/CoreServices \
	-I$(VIDEOPHONE_DIR)/FilePlay \
	-I$(RVSHAREDDIR)/rtpRtcp/rtp \
	-I$(IXML_DIR)/inc \
	-I$(UPNP_DIR)/threadutil/inc \
	-I$(UPNP_DIR)/upnp/inc \
	-I$(UPNP_BUILD_DIR)/upnp/inc \
	-I$(ACMEDIR)/include \
	-I$(VIDEOPHONE_DIR)/http \
	-I$(VIDEOPHONE_DIR)/ImageMgr \
	-I$(VIDEOPHONE_DIR)/MPEG4 \
	-I$(VIDEOPHONE_DIR)/MPEG4/Toolbox \
	-I$(VIDEOPHONE_DIR)/MPEG4/Atoms \
	-I$(VIDEOPHONE_DIR)/os \
	-I$(VIDEOPHONE_DIR)/pm \
	-I$(VIDEOPHONE_DIR)/update \
	-I$(VIDEOPHONE_DIR)/router \
	-I$(VIDEOPHONE_DIR)/stun \
	-I$(VIDEOPHONE_DIR)/vrcl \
	-I$(VIDEOPHONE_DIR)/xmlmgr \
	-I$(VIDEOPHONE_DIR)/RingGroupMgr \
	-I$(VIDEOPHONE_DIR)/FileDownload
	
INCS += `pkg-config --cflags gstreamer-$(GSTREAMER_LIB)`
	

include $(MAKEFILE_RULES)



