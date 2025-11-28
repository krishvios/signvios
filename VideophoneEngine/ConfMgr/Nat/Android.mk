LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := conf_mgr_nat


LOCAL_SRC_FILES := 	CstiIceManager.cpp CstiIceSession.cpp
			
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../Nat

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../

LOCAL_STATIC_LIBRARIES := radvision_includes vpcommon os

include $(BUILD_STATIC_LIBRARY)

include $(call all-subdir-makefiles)
