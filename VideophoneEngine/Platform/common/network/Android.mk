LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := platform_common_network

LOCAL_SRC_FILES := BaseNetwork.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../network


LOCAL_STATIC_LIBRARIES := vpcommon os

include $(BUILD_STATIC_LIBRARY)
