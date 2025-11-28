
TARGET := libntouchVPCommon.a

OBJS =  \
	CstiBSPInterfaceBase.o \
	CstiAudioOutput.o \
	CstiAudibleRinger.o \
	CstiVideoInputBase.o \
	CstiVideoOutputBase.o \
	CstiGStreamerVideoGraphBase.o \
	CstiCameraControl.o \
	CstiUserInputBase.o \
	CstiBufferElement.o \
	CstiLogging.o \
	GstreamerBufferElement.o \
	CstiAudioInputBase.o \
    CstiMatterStubs.o \
	H263Tools.o \
	CstiFileIO.o \
	FrameCaptureBinBase.o \
	EncodePipelineBase.o \
	FanControlBase.o \
	CaptureBinBase.o

ifneq ($(DEVICE), DEV_X86)
OBJS += \
	CstiAdvancedStatusBase.o \
	CstiCECControlBase.o \
	CstiSLICRingerBase.o \
	CstiStatusLEDBase.o \
	CstiLightRingBase.o \
	CLed.o
endif

ifneq ($(DEVICE), DEV_NTOUCH_VP2)
OBJS += \
	SocketListenerBase.o \
	CstiVideoOutputBase2.o \
	CstiGStreamerVideoGraphBase2.o \
	EncodeBinBase.o \
	CaptureBinBase2.o \
	CFilePlayGS.o \
    DisplaySinkBin.o \
    DecodePipeline.o \
	AudioTest.o \
	JsonTestResponse.o \
	RecordBinBase.o \
	SelfViewBinBase.o \
	EncodePipelineBase2.o \
	SelfViewBinBase.o \
	PlaybackPipelineBase.o \
	CropScaleBinBase.o

ifneq ($(DEVICE), DEV_X86)
OBJS += \
	CstiVideoInputBase2.o \
	MonitorTaskBase2.o
endif

endif

ifeq ($(DEVICE), DEV_X86)
OBJS += \
	CstiVideoOutputBase2.o \
	CstiGStreamerVideoGraphBase2.o \
	EncodeBinBase.o \
	CaptureBinBase2.o \
	RecordBinBase.o \
	SelfViewBinBase.o \
	CFilePlayGS.o \
	EncodePipelineBase2.o \
	SelfViewBinBase.o \
	PlaybackPipelineBase.o
endif

CHILD_DIRECTORIES = \
	$(DBUS_DIR) \
	$(BLUETOOTH_DIR) \
	$(GSTREAMER_DIR) \
	$(MEDIA_PIPELINES_DIR)

ifneq ($(DEVICE), DEV_X86)
CHILD_DIRECTORIES += \
	network
endif

DEFS := \
	-DLINUX \
	-DCONFIG_STA_SUPPORT

ifeq ($(DEVICE), DEV_NTOUCH_VP3)
	INCS := -I$(VP_VP3_DIR) \
		-I$(VP_VP3_DIR)/update
else
ifeq ($(DEVICE), DEV_NTOUCH_VP2)
	CHILD_DIRECTORIES += \
		update
		
	INCS := -I$(VP_VP2_DIR) \
		-I$(VP_COMMON_DIR)/update
endif
ifeq ($(DEVICE), DEV_NTOUCH_VP4)
	INCS := -I$(VP_VP4_DIR) \
		-I$(VP_VP4_DIR)/update
endif
ifeq ($(DEVICE), DEV_X86)
	CHILD_DIRECTORIES += \
		update
		
	INCS := \
		-I$(VP_X86_DIR) \
		-I$(PLATFORM_COMMON_DIR)/audio \
		-I$(VP_COMMON_DIR)/update
endif
endif

INCS += \
	-I$(VP_COMMON_DIR) \
	-I$(OBSERVER_PTR_DIR) \
	-I$(PLATFORM_VP_DIR) \
	-I$(OS_DIR) \
	-I$(HTTP_DIR) \
	-I$(COMMON_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(IXML_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_COMMON_DIR)/audio \
	-I$(PLATFORM_COMMON_DIR)/video \
	-I$(PLATFORM_COMMON_DIR)/platform \
	-I$(PLATFORM_COMMON_DIR)/wireless \
	-I$(PLATFORM_COMMON_DIR)/network \
	-I$(PLATFORM_COMMON_DIR)/fileplay \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4 \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4/Toolbox \
	-I$(CONFMGR_DIR) \
	-I$(CONFMGR_DIR)/Session \
	-I$(DBUS_DIR) \
	-I$(GSTREAMER_DIR) \
	-I$(MEDIA_PIPELINES_DIR) \
	-I$(BLUETOOTH_DIR) \
	-I$(FILEUPLOAD_DIR) \
	`pkg-config --cflags glib-2.0 gthread-2.0 dbus-glib-1`

ifeq ($(DEVICE), DEV_X86)
INCS += \
	-I$(VP_X86_NETWORK_DIR)
else
INCS += \
	-I$(VP_COMMON_NETWORK_DIR)
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP3)
ifdef PKG_CONFIG_SYSROOT_DIR
	INCS += \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/api_lib/includes \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/api_lib/vp886_api \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/arch/x86_spi \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/lt_api/includes \
		-I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/le9653-api/lt_api/vp886_lt
endif
	INCS += `pkg-config --cflags gstreamer-1.0`
else
ifeq ($(DEVICE), DEV_NTOUCH_VP2)
	INCS += `pkg-config --cflags gstreamer-0.10`
endif
ifeq ($(DEVICE), DEV_NTOUCH_VP4)
#TODO: Add vp4 led libs?
	INCS += `pkg-config --cflags gstreamer-1.0`
endif
ifeq ($(DEVICE), DEV_X86)
	INCS += `pkg-config --cflags gstreamer-1.0`
endif
endif

CPPFLAGS += \
	-Wno-unused-local-typedefs

ifneq (,$(findstring clang,$(CXX)))
	CXXFLAGS += \
		-Wno-unused-lambda-capture
endif

INSTALL_FILES := \
	IPlatformVP.h \
	ILightRingVP.h \
	ISLICRinger.h \
	IFanControl.h \
	IUpdate.h \
    IMatter.h \
	PropertyRange.h

include $(MAKEFILE_RULES)
