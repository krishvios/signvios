LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := os

LOCAL_SRC_FILES := 	stiOS.cpp \
			stiOSNet.c \
			stiOSMsg.c \
			stiOSLinux.cpp \
			stiOSLinuxMsg.cpp \
			stiOSLinuxMutex.cpp \
			stiOSLinuxTask.cpp \
			stiOSLinuxWd.cpp \
			stiOSLinuxSignal.cpp \
			stiOSLinuxNet.cpp \
			../common/stiSysUtilizationInfo.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../os

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../common \
			$(LOCAL_PATH)/../Platform \
			$(LOCAL_PATH)/../common/wireless \


LOCAL_CFLAGS    := 	

LOCAL_EXPORT_CFLAGS := $(LOCAL_CFLAGS)

include $(BUILD_STATIC_LIBRARY)
