LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := call_history_mgr
LOCAL_SRC_FILES := 	CstiCallHistoryItem.cpp \
			CstiCallHistoryList.cpp \
			CstiCallHistoryManager.cpp \
			CstiCallHistoryMgrResponse.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Platform
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Platform/ntouch/codecEngine/lib

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../CallHistoryMgr

LOCAL_STATIC_LIBRARIES := radvision_includes vpcommon core_services http libupnp os xml_mgr contacts

include $(BUILD_STATIC_LIBRARY)
