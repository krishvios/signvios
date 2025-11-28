LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := serialization

LOCAL_SRC_FILES :=	JsonFileSerializer.cpp

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../Serialization \

LOCAL_STATIC_LIBRARIES := vpcommon

include $(BUILD_STATIC_LIBRARY)
