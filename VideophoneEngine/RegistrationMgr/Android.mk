LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := registration_mgr
LOCAL_SRC_FILES := CstiRegistrationManager.cpp \
		    CstiRegistrationList.cpp \
		    CstiRegistrationListItem.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../RegistrationMgr

LOCAL_STATIC_LIBRARIES := core_services xml_mgr

include $(BUILD_STATIC_LIBRARY)
