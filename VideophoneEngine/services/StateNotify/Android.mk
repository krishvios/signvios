LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := state_notify_service

LOCAL_SRC_FILES := CstiStateNotifyRequest.cpp \
			CstiStateNotifyResponse.cpp \
			CstiStateNotifyService.cpp

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../StateNotify

LOCAL_STATIC_LIBRARIES := vp_service core_services

include $(BUILD_STATIC_LIBRARY)
