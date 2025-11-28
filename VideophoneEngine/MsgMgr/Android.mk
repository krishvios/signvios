LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := msg_mgr
LOCAL_SRC_FILES := CstiMessageInfo.cpp \
	CstiMessageManager.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../MsgMgr

LOCAL_STATIC_LIBRARIES := core_services http libupnp os vpcommon xml_mgr serialization

include $(BUILD_STATIC_LIBRARY)
