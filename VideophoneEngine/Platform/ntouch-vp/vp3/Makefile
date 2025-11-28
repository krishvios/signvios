TARGET := libstiDeviceController.a

OBJS := \
	CstiBSPInterface.o \
	CstiStatusLED.o \
	CstiLightRing.o \
	CstiAudioHandler.o \
	CstiSLICRinger.o \
	CstiMonitorTask.o \
	CstiAdvancedStatus.o \
	CstiVideoOutput.o \
	CstiVideoInput.o \
	CstiGStreamerVideoGraph.o \
	CstiCECControl.o \
	CstiUserInput.o \
	CaptureBin.o \
	EncodeBin.o \
	EncodePipeline.o \
	ToFSensor.o \
	AudioRecordPipelineVP3.o \
	AudioPlaybackPipelineVP3.o \
	FrameCaptureBin.o \
	BluetoothAudio.o \
	FanControl.o \
	CstiNetwork.o \
	CropScaleBin.o


CHILD_DIRECTORIES := \
	../common \
	update

DEFS := \
	-DLINUX \
	-DCONFIG_STA_SUPPORT

$(echo $DEFS)

INCS := \
	-I$(VP_VP3_DIR) \
	-I$(OBSERVER_PTR_DIR) \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(IXML_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_VP_DIR) \
	-I$(PLATFORM_VP_DIR)/common \
	-I$(PLATFORM_COMMON_DIR)/audio \
	-I$(PLATFORM_COMMON_DIR)/video \
	-I$(PLATFORM_COMMON_DIR)/platform \
	-I$(PLATFORM_COMMON_DIR)/network \
	-I$(PLATFORM_COMMON_DIR)/wireless \
	-I$(CONFMGR_DIR) \
	-I$(CONFMGR_DIR)/Session \
	-I$(VP_COMMON_NETWORK_DIR) \
	-I$(BLUETOOTH_DIR) \
	-I$(DBUS_DIR) \
	-I$(GSTREAMER_DIR) \
	-I$(VP_COMMON_DIR) \
	-I$(MEDIA_PIPELINES_DIR) \
	-I$(NETWORK_DIR) \
	`pkg-config --cflags glib-2.0 gthread-2.0 dbus-glib-1 gstreamer-1.0 libcamhal`

ifdef PKG_CONFIG_SYSROOT_DIR
	INCS += \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/api_lib/includes \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/api_lib/vp886_api \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/arch/x86_spi \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/lt_api/includes \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/lt_api/vp886_lt
endif

CPPFLAGS += \
	-Wno-unused-local-typedefs

INSTALL_FILES := \
	IAudioInputVP2.h \
	../IVideoInputVP2.h \
	../IVideoOutputVP.h

include $(MAKEFILE_RULES)
