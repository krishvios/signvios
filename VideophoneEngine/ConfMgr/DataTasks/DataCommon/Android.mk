LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

RADVISION	:= $(LOCAL_PATH)/../../../Libraries/Radvision

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := dt_data_common

LOCAL_SRC_FILES := 	\
			CstiDataTaskCommonP.cpp \
			CstiRtpHeaderLength.cpp \
			CstiSyncManager.cpp \
			RTCPFlowControl.cpp \
			RtpPayloadAddon.cpp \
			utf8EncodeDecode.cpp \
			CstiRtpSession.cpp \
			CstiAesEncryptionContext.cpp \
			SDESKey.cpp \
			DtlsContext.cpp \
			RtpSender.cpp \
			RtxSender.cpp \

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../AudioPlayback
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../TextPlayback
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../Libraries/Acme/tscf/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../DataCommon

LOCAL_STATIC_LIBRARIES := radvision_includes vpcommon os

include $(BUILD_STATIC_LIBRARY)
