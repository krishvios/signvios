LOCAL_PATH := $(call my-dir)
TEMP_PATH := $(LOCAL_PATH)
include $(call all-subdir-makefiles)

LOCAL_PATH := $(TEMP_PATH)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := file_play
LOCAL_SRC_FILES := CFilePlay.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Platform
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../fileplay

LOCAL_STATIC_LIBRARIES := vpcommon \
			os \
			platform_common_video \
			mpeg4

include $(BUILD_STATIC_LIBRARY)
