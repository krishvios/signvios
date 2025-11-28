
GSTREAMER_VERSION ?= 1

TARGET := libstiDeviceController.a

OBJS := \
	CstiBSPInterface.o \
	CstiStatusLED.o \
	CstiLightRing.o \
	CstiAudioInput.o \
	CstiAudioHandler.o \
	CstiSLICRinger.o \
	CstiVideoInput.o \
	CstiGStreamerVideoGraph.o \
	EncodePipeline.o \
	CaptureBin.o \
	CstiVideoOutput.o \
	CstiAdvancedStatus.o \
	CstiMonitorTask.o \
	CstiISPMonitor.o \
	stiOVUtils.o \
	CstiCECControl.o \
	CstiUserInput.o \
	AudioRecordPipelineVP2.o \
	AudioPlaybackPipelineVP2.o \
	FrameCaptureBin.o \
	BluetoothAudio.o \
	CstiNetwork.o

CHILD_DIRECTORIES := \
	../common

DEFS := \
	-DLINUX \
	-DCONFIG_STA_SUPPORT \
	-DGSTREAMER_VERSION=$(GSTREAMER_VERSION)

INCS := \
	-I$(VP_VP2_DIR) \
	-I$(OBSERVER_PTR_DIR) \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(IXML_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_VP_DIR) \
	-I$(PLATFORM_COMMON_DIR)/audio \
	-I$(PLATFORM_COMMON_DIR)/video \
	-I$(PLATFORM_COMMON_DIR)/platform \
	-I$(PLATFORM_COMMON_DIR)/network \
	-I$(PLATFORM_COMMON_DIR)/wireless \
	-I$(PLATFORM_COMMON_DIR)/fileplay \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4 \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4/Toolbox \
	-I$(CONFMGR_DIR) \
	-I$(CONFMGR_DIR)/Session \
	-I$(VP_COMMON_NETWORK_DIR) \
	-I$(BLUETOOTH_DIR) \
	-I$(DBUS_DIR) \
	-I$(GSTREAMER_DIR) \
	-I$(VP_COMMON_DIR) \
	-I$(MEDIA_PIPELINES_DIR) \
	-I$(NETWORK_DIR) \
	`pkg-config --cflags glib-2.0 gthread-2.0 dbus-glib-1 gstreamer-0.10`

CPPFLAGS += \
	-Wno-unused-local-typedefs

INSTALL_FILES := \
	../common/IAudioInputVP2.h \
	../IVideoInputVP2.h \
	../IVideoOutputVP.h

include $(MAKEFILE_RULES)
