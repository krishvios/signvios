LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := conf_mgr_sip

LOCAL_SRC_FILES := 	ContactHeader.cpp \
			Message.cpp \
			Transaction.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../ConfMgr/Sip

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../

LOCAL_STATIC_LIBRARIES := vpcommon os

include $(BUILD_STATIC_LIBRARY)

include $(call all-subdir-makefiles)