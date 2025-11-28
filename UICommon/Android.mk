LOCAL_PATH := $(call my-dir)
MY_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := uicommon 
LOCAL_SRC_FILES := ErrorCodeLookup.cpp propertydef.cpp SecretCodeParser.cpp
LOCAL_LDLIBS := -llog 

LOCAL_STATIC_LIBRARIES := os

include $(BUILD_STATIC_LIBRARY)
