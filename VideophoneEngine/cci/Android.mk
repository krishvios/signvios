LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

STANDALONE ?= ~/mvrs_ndk

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := cci
LOCAL_SRC_FILES := 	AstiVideophoneEngine.cpp \
			CstiCCI.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../cci

# Common is needed for propertydef.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../UICommon
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Platform

# VRCL is needed for ExternalConferenceData.h
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../vrcl

# Note: We include sys root before to make sure that we get our android
#       stdint.h instead of the incomplete version included with tsm.
LOCAL_C_INCLUDES += $(STANDALONE)/sysroot/usr/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Libraries/Acme/tscf/include

LOCAL_STATIC_LIBRARIES := os vpcommon contacts block_list_mgr conf_mgr core_services file_play http image_mgr libupnp mpeg4 msg_mgr platform_ntouch pm vpvrcl vpvrcl xml_mgr ring_group_mgr uicommon call_history_mgr registration_mgr user_account_mgr remote_logger_service service_outage endpoint_monitor

include $(BUILD_STATIC_LIBRARY)
