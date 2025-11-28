LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := platform_common_wireless

LOCAL_SRC_FILES :=

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../Platform/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../wireless/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../wireless

LOCAL_STATIC_LIBRARIES := vpcommon

include $(BUILD_STATIC_LIBRARY)
