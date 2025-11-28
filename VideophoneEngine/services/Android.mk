LOCAL_PATH := $(call my-dir)
MY_PATH := $(LOCAL_PATH)
include $(call all-subdir-makefiles)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := vp_service

LOCAL_SRC_FILES := CstiVPService.cpp \
			CstiVPServiceRequest.cpp \
			CstiVPServiceResponse.cpp \
			CstiListItem.cpp \
			CstiList.cpp

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../services

LOCAL_STATIC_LIBRARIES := vpcommon

include $(BUILD_STATIC_LIBRARY)
