LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := platform_common_video

LOCAL_SRC_FILES := 	CstiFFMpegContext.cpp \
			CstiFFMpegDecoder.cpp \
			CstiSoftwareDisplay.cpp \
			CstiSoftwareInput.cpp \
			CstiVideoEncoder.cpp \
			CstiX264Encoder.cpp \
			stiRgb2yuv.cpp \
			BaseVideoInput.cpp \
			BaseVideoOutput.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../video

LOCAL_CFLAGS    := -D__STDC_CONSTANT_MACROS

LOCAL_SHARED_LIBRARIES := ffmpeg_avcodec x264

LOCAL_STATIC_LIBRARIES := vpcommon os  mpeg4 gd_static

include $(BUILD_STATIC_LIBRARY)
