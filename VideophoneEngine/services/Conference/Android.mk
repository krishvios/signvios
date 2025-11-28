LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := conference_service

LOCAL_SRC_FILES := CstiConferenceRequest.cpp \
			CstiConferenceResponse.cpp \
			CstiConferenceService.cpp

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../Conference

LOCAL_STATIC_LIBRARIES := vp_service core_services

include $(BUILD_STATIC_LIBRARY)
