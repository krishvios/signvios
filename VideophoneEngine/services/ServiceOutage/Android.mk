LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := service_outage

LOCAL_SRC_FILES := ServiceOutageClient.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../ServiceOutage

LOCAL_STATIC_LIBRARIES := vpcommon

include $(BUILD_STATIC_LIBRARY)
