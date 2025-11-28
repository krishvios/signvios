LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := remote_logger_service

LOCAL_SRC_FILES := CstiRemoteLoggerRequest.cpp \
			CstiRemoteLoggerService.cpp

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../RemoteLogger

LOCAL_STATIC_LIBRARIES := vp_service

include $(BUILD_STATIC_LIBRARY)
