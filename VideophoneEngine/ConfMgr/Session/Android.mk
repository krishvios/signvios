LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := conf_mgr_session

LOCAL_SRC_FILES := 	CstiSdp.cpp \
			stiPayloadTypes.cpp
			
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../Session

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../

LOCAL_STATIC_LIBRARIES := vpcommon os

include $(BUILD_STATIC_LIBRARY)

include $(call all-subdir-makefiles)

