LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := dt_video_playback

LOCAL_SRC_FILES := 	CstiVideoRecord.cpp RecordFlowControl.cpp RateHistoryManager.cpp Packetization.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../openssl-android/include/ \
		    $(LOCAL_PATH)/../../../Platform/common/CameraControl \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../VideoPlayback

LOCAL_STATIC_LIBRARIES := radvision_includes vpcommon os dt_data_common

include $(BUILD_STATIC_LIBRARY)
