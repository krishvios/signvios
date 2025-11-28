LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := platform_common_audio

LOCAL_SRC_FILES := CstiAudioBridge.cpp \
                    BaseAudioInput.cpp \
                    BaseAudioOutput.cpp \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../audio

LOCAL_SHARED_LIBRARIES := ffmpeg_swresample

LOCAL_STATIC_LIBRARIES := vpcommon os

include $(BUILD_STATIC_LIBRARY)
