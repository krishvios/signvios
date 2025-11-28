LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := core_services

LOCAL_SRC_FILES := CstiCoreRequest.cpp \
			CstiCoreResponse.cpp \
			CstiCoreService.cpp \
			CstiCallList.cpp \
			CstiCallListItem.cpp \
			CstiFavorite.cpp \
			CstiContactList.cpp \
			CstiContactListItem.cpp \
			CstiDirectoryResolveResult.cpp \
			CstiPhoneNumberList.cpp \
			CstiPhoneNumberListItem.cpp \
			OfflineCore.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../Core

LOCAL_STATIC_LIBRARIES := vp_service os http libupnp message_service state_notify_service conference_service remote_logger_service

include $(BUILD_STATIC_LIBRARY)
