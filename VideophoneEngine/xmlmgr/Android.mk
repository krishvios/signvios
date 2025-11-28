LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := xml_mgr
LOCAL_SRC_FILES := 	XMLList.cpp \
			XMLListItem.cpp \
			XMLManager.cpp \
			XMLValue.cpp

LOCAL_STATIC_LIBRARIES := os vpcommon core_services http libupnp

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../xmlmgr

include $(BUILD_STATIC_LIBRARY)
