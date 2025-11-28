LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := user_account_mgr
LOCAL_SRC_FILES := UserAccountListItem.cpp \
                    UserAccountManager.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../UserAccountMgr

LOCAL_STATIC_LIBRARIES := core_services xml_mgr

include $(BUILD_STATIC_LIBRARY)
