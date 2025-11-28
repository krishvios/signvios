LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := block_list_mgr
LOCAL_SRC_FILES := 	CstiBlockListItem.cpp \
			CstiBlockListMgrResponse.cpp \
			CstiBlockListManager.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Platform
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Platform/ntouch/codecEngine/lib

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../BlockListMgr

LOCAL_STATIC_LIBRARIES := radvision_includes vpcommon core_services http libupnp os xml_mgr

include $(BUILD_STATIC_LIBRARY)
