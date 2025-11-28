LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := ring_group_mgr
LOCAL_SRC_FILES := 	CstiRingGroupManager.cpp \
			CstiRingGroupMgrResponse.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../RingGroupMgr

LOCAL_STATIC_LIBRARIES := xml_mgr core_services vpcommon os

include $(BUILD_STATIC_LIBRARY)
