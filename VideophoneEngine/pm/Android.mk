LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := pm
LOCAL_SRC_FILES := CstiPropertySender.cpp \
	PropertyListItem.cpp \
	PropertyManager.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../pm

LOCAL_STATIC_LIBRARIES := os xml_mgr

include $(BUILD_STATIC_LIBRARY)
