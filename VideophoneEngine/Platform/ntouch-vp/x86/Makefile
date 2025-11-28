TARGET := libstiDeviceController.a

LIBS :=

CHILD_DIRECTORIES := \
	network \


OBJS := \
	CstiBSPInterface.o \
	CstiStatusLED.o \
	CstiLightRing.o \
	CstiAudioHandler.o \
	CstiVideoOutput.o \
	CstiSLICRinger.o \
	CstiVideoInput.o \
	CstiMonitorTask.o \
	CstiAdvancedStatus.o \
	CstiGStreamerVideoGraph.o \
	CaptureBin.o \
	EncodeBin.o \
	EncodePipeline.o \
	FrameCaptureBin.o \
	CropScaleBin.o

DEFS := \
	-DLINUX \
	-DCONFIG_STA_SUPPORT

INCS := \
	-I$(VP_X86_DIR) \
	-I$(VP_X86_NETWORK_DIR) \
	-I$(BLUETOOTH_DIR) \
	-I$(DBUS_DIR) \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(IXML_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_VP_DIR) \
	-I$(PLATFORM_VP_DIR)/common \
	-I$(PLATFORM_COMMON_DIR)/video \
	-I$(PLATFORM_COMMON_DIR)/audio \
	-I$(PLATFORM_COMMON_DIR)/wireless \
	-I$(PLATFORM_COMMON_DIR)/platform \
	-I$(PLATFORM_COMMON_DIR)/fileplay \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4 \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4/Toolbox \
	-I$(CONFMGR_DIR) \
	-I$(CONFMGR_DIR)/Session \
	-I$(VP_COMMON_DIR) \
	-I$(GSTREAMER_DIR) \
	-I$(OBSERVER_PTR_DIR) \
	-I$(MEDIA_PIPELINES_DIR) \
	`pkg-config --cflags glib-2.0 gthread-2.0 dbus-glib-1 gstreamer-1.0`

INSTALL_FILES := \
	IAudioInputVP2.h \
	../IVideoInputVP2.h \
	../IVideoOutputVP.h

include $(MAKEFILE_RULES)
