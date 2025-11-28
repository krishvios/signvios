LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := platform_common_platform

LOCAL_SRC_FILES := BasePlatform.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../platform


LOCAL_STATIC_LIBRARIES := vpcommon os

include $(BUILD_STATIC_LIBRARY)
