LOCAL_PATH := $(call my-dir)
MY_PATH := $(LOCAL_PATH)
include $(call all-subdir-makefiles)

include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := mpeg4
LOCAL_SRC_FILES := 	CDataTransferHandler.cpp \
			CFileDataHandler.cpp \
			CServerDataHandler.cpp \
			CVMCircularBuffer.cpp \
			DataHandler.cpp \
			MP4Common.cpp \
			MP4Handle.cpp \
			MP4Media.cpp \
			MP4Movie.cpp \
			MP4Optimize.cpp \
			MP4Parser.cpp \
			MP4Signals.cpp \
			MP4Track.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../MPEG4

LOCAL_STATIC_LIBRARIES := http mpeg4_atoms mpeg4_toolbox os vpcommon

include $(BUILD_STATIC_LIBRARY)
