LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := endpoint_monitor

LOCAL_SRC_FILES := 	EndpointMonitor.cpp

LOCAL_STATIC_LIBRARIES := vpcommon

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../EndpointMonitor

include $(BUILD_STATIC_LIBRARY)


