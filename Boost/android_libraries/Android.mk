LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_date_time
LOCAL_SRC_FILES := libboost_date_time.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_filesystem
LOCAL_SRC_FILES := libboost_filesystem.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_system
LOCAL_SRC_FILES := libboost_system.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_iostreams
LOCAL_SRC_FILES := libboost_iostreams.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_serialization
LOCAL_SRC_FILES := libboost_serialization.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_log
LOCAL_SRC_FILES := libboost_log.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_program_options
LOCAL_SRC_FILES := libboost_program_options.a
include $(PREBUILT_STATIC_LIBRARY)