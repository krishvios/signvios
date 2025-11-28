
TARGET := libntouchVPCommonMediaPipelines.a

OBJS =  \
	AudioRecordPipeline.o \
	AudioPlaybackPipeline.o	\
	TestAudioInputPipeline.o \
	AudioFileSourcePipeline.o \
	AudioPipeline.o \
	MediaPipeline.o

CHILD_DIRECTORIES = 

DEFS := \
	-DLINUX \
	-DCONFIG_STA_SUPPORT

#ifeq ($(DEVICE), DEV_NTOUCH_VP3)
#	INCS := -I$(VP_VP3_DIR)
#else
#ifeq ($(DEVICE), DEV_NTOUCH_VP2)
#	INCS := -I$(VP_VP2_DIR)
#endif
#ifeq ($(DEVICE), DEV_X86)
#	INCS := \
#		-I$(VP_X86_DIR) \
#		-I$(PLATFORM_COMMON_DIR)/audio
#endif
#endif

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
	-I$(MPEG4_DIR) \
	-I$(DBUS_DIR) \
	-I$(GSTREAMER_DIR) \
	-I$(MEDIA_PIPELINES_DIR) \
	`pkg-config --cflags glib-2.0 gthread-2.0 dbus-glib-1` \
	-I$(BLUETOOTH_DIR)
	

#ifeq ($(DEVICE), DEV_X86)
#INCS += \
#	-I$(VP_X86_NETWORK_DIR) \
#else
#INCS += \
#	-I$(VP_COMMON_NETWORK_DIR) \
#endif

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
ifeq ($(DEVICE), DEV_NTOUCH_VP4)
#TODO: add vp4 led stuff?
	INCS += `pkg-config --cflags gstreamer-1.0`
endif
ifeq ($(DEVICE), DEV_NTOUCH_VP2)
	INCS += `pkg-config --cflags gstreamer-0.10`
endif
ifeq ($(DEVICE), DEV_X86)
	INCS += `pkg-config --cflags gstreamer-1.0`
endif
endif

CPPFLAGS += \
	-Wno-unused-local-typedefs

#INSTALL_FILES := \
#	IPlatformVP.h \
#	ILightRingVP.h \
#	ISLICRinger.h

include $(MAKEFILE_RULES)
