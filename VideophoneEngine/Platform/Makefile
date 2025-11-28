#TARGET := libBSP.a

DEFS := 

export PLATFORM_VP_DIR := $(PLATFORM_DIR)/ntouch-vp
export PLATFORM_COMMON_DIR := $(PLATFORM_DIR)/common
export VP_COMMON_DIR := $(PLATFORM_VP_DIR)/common
export VP_VP2_DIR := $(PLATFORM_VP_DIR)/vp2
export VP_VP3_DIR := $(PLATFORM_VP_DIR)/vp3
export VP_VP4_DIR := $(PLATFORM_VP_DIR)/vp4
export VP_X86_DIR := $(PLATFORM_VP_DIR)/x86
export DBUS_DIR := $(VP_COMMON_DIR)/dbus
export GSTREAMER_DIR := $(VP_COMMON_DIR)/GStreamer
export BLUETOOTH_DIR := $(VP_COMMON_DIR)/bluetooth
export GSTREAMER_DIR := $(VP_COMMON_DIR)/GStreamer
export VP_COMMON_NETWORK_DIR := $(VP_COMMON_DIR)/network
export VP_X86_NETWORK_DIR := $(VP_X86_DIR)/network
export MEDIA_PIPELINES_DIR := $(VP_COMMON_DIR)/MediaPipelines

INSTALL_FILES := \
	IstiPlatform.h \
	IstiStatusLED.h \
	IstiLightRing.h \
	IstiHelios.h \
	IstiUserInput.h \
	IstiVideoInput.h \
	VideoFrame.h \
	IstiAudioInput.h \
	VideoControl.h \
	AudioControl.h \
	IstiVideoOutput.h \
	IstiVideoPlaybackFrame.h \
	IstiAudioOutput.h \
	IstiNetwork.h \
	IstiMessageViewer.h \
	IstiLogging.h

ifeq ($(APPLICATION), APP_NTOUCH_VP2)
	INSTALL_FILES += \
		IstiAudibleRinger.h \
		IstiBluetooth.h \
		IUpdate.h
endif

ifeq ($(APPLICATION), APP_NTOUCH_VP3)
	INSTALL_FILES += \
		IstiAudibleRinger.h \
		IstiBluetooth.h \
		IUpdate.h
endif

ifeq ($(APPLICATION), APP_NTOUCH_VP4)
	INSTALL_FILES += \
		IstiAudibleRinger.h \
		IstiBluetooth.h \
		IUpdate.h
endif

DOC_FILES := \
	IstiPlatform.h \
	IstiStatusLED.h \
	IstiLightRing.h \
	IstiHelios.h \
	IstiUserInput.h \
	IstiVideoInput.h \
	IstiAudioInput.h \
	VideoControl.h \
	AudioControl.h \
	IstiVideoOutput.h \
	IstiAudioOutput.h \
	IstiAudibleRinger.h \
	IstiMessageViewer.h \
	IstiLogging.h

ifeq ($(DEVICE), DEV_NTOUCH_VP2)
	CHILD_DIRECTORIES = \
		ntouch-vp/vp2 \
		common
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP3)
	CHILD_DIRECTORIES = \
		ntouch-vp/vp3 \
		common
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP4)
	CHILD_DIRECTORIES = \
		ntouch-vp/vp4 \
		common
endif

ifeq ($(DEVICE), DEV_X86)
	ifeq ($(APPLICATION), APP_REGISTRAR_LOAD_TEST_TOOL)
		CHILD_DIRECTORIES = \
			RegLoadTest \
			common
	else
		CHILD_DIRECTORIES = \
			ntouch-vp/common \
			ntouch-vp/x86 \
			common
	endif
endif

include $(MAKEFILE_RULES)
