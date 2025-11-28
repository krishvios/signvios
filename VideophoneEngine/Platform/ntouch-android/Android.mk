LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := platform_ntouch

LOCAL_SRC_FILES := 	CstiAndroidDisplay.cpp \
			CstiAndroidEncoder.cpp \
			CstiAudio.cpp \
			CstiBSPInterface.cpp \
			CstiDisplay.cpp \
			CstiVideoInput.cpp \
			VideophoneEngineCallback.cpp \
			CstiNetwork.cpp


LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common/video
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common/platform
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common/network
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common/fileplay
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../ntouch-android

LOCAL_EXPORT_C_INCLUDES += $(LOCAL_PATH)/../common/video
LOCAL_EXPORT_C_INCLUDES += $(LOCAL_PATH)/../common/platform
LOCAL_EXPORT_C_INCLUDES += $(LOCAL_PATH)/../common/network
LOCAL_EXPORT_C_INCLUDES += $(LOCAL_PATH)/../ntouch-android

LOCAL_CFLAGS    := -D__STDC_CONSTANT_MACROS

LOCAL_SHARED_LIBRARIES := ffmpeg_avcodec

LOCAL_STATIC_LIBRARIES := os vpcommon vpvrcl platform_common_video mpeg4_toolbox mpeg4 platform_common_platform platform_common_network service_outage file_play

LOCAL_EXPORT_CFLAGS := $(LOCAL_CFLAGS)

include $(BUILD_STATIC_LIBRARY)
