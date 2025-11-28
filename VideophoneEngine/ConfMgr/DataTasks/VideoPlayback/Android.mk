LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

RADVISION	:= $(LOCAL_PATH)/../../../Libraries/Radvision

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := dt_video_record

LOCAL_SRC_FILES := 	CstiVideoPlayback.cpp \
			CstiVideoPlaybackRead.cpp \
			PlaybackFlowControl.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../VideoRecord

LOCAL_STATIC_LIBRARIES := radvision_includes vpcommon os dt_data_common

include $(BUILD_STATIC_LIBRARY)
