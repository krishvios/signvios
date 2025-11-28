LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := message_service

LOCAL_SRC_FILES := CstiMessageRequest.cpp \
			CstiMessageResponse.cpp \
			CstiMessageService.cpp \
			CstiMessageList.cpp \
			CstiMessageListItem.cpp

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../Message

LOCAL_STATIC_LIBRARIES := vp_service core_services

include $(BUILD_STATIC_LIBRARY)
