LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := network
LOCAL_SRC_FILES := 	CstiDHCP.cpp \
			CstiNetwork.cpp
#			CstiRouter.cpp \
#			IstiRouter.cpp \


LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../network

LOCAL_STATIC_LIBRARIES := os vpcommon platform_common_wireless pm

include $(BUILD_STATIC_LIBRARY)
