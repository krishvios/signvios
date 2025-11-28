LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := image_mgr
LOCAL_SRC_FILES := CstiImageManager.cpp \
		    CstiImageDownloadService.cpp \
		    CstiNvmImageCache.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../ImageMgr

LOCAL_STATIC_LIBRARIES := vpcommon os core_services pm xml_mgr

include $(BUILD_STATIC_LIBRARY)
