LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := dt_text_record

LOCAL_SRC_FILES := 	CstiTextRecord.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../TextRecord

LOCAL_STATIC_LIBRARIES := dt_data_common vpcommon os

include $(BUILD_STATIC_LIBRARY)
