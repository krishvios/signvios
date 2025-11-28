LOCAL_PATH := $(call my-dir)
MY_PATH := $(LOCAL_PATH)
include $(call all-subdir-makefiles)

include $(CLEAR_VARS)

STANDALONE ?= ~/mvrs_ndk

LOCAL_PATH := $(MY_PATH)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := conf_mgr
LOCAL_SRC_FILES := 	CstiCall.cpp \
			CstiCallInfo.cpp \
			CstiSipCallLeg.cpp \
			CstiCallStorage.cpp \
			CstiConferenceManager.cpp \
			CstiProtocolCall.cpp \
			CstiProtocolManager.cpp \
			CstiRoutingAddress.cpp \
			CstiSipCall.cpp \
			CstiSipManager.cpp \
			CstiSipRegistration.cpp \
			CstiSipResolver.cpp \
			CstiTunnelManager.cpp \
			stiSipTools.cpp \
			stiSystemInfo.cpp \
			stiSipStatistics.cpp \
			DhvClient.cpp \

#			CstiDialString.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../ConfMgr

LOCAL_STATIC_LIBRARIES := os pm vpcommon block_list_mgr conf_mgr_nat conf_mgr_session conf_mgr_sip dt_audio_playback dt_text_record dt_text_playback dt_audio_record dt_video_record dt_video_playback radvision_includes tsm platform_common_audio rtpRtcp sip sdp ads rvcommon ice turn stun ssltsc cryptotsc pocofoundation poconet poconetssl pocojson pocoutil serialization endpoint_monitor

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../Libraries/android/ffmpeg/include \
			$(LOCAL_PATH)/DataTasks/TextPlayback \
			$(LOCAL_PATH)/DataTasks/TextRecord \

# Note: We include sys root before to make sure that we get our android
#       stdint.h instead of the incomplete version included with tsm.
LOCAL_C_INCLUDES += $(STANDALONE)/sysroot/usr/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Libraries/Acme/tscf/include

include $(BUILD_STATIC_LIBRARY)
