LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := vpcommon

LOCAL_SRC_FILES :=	CstiEvent.cpp \
			CstiExtendedEvent.cpp \
			CstiHostNameResolver.cpp \
			CstiItemId.cpp \
			CstiOsTask.cpp \
			CstiOsTaskMQ.cpp \
			CstiIpAddress.cpp \
			CstiProfiler.cpp \
			CstiTask.cpp \
			CstiUniversalEvent.cpp \
			ccrc.cpp \
			cmPropertyNameDef.cpp \
			stiAssert.cpp \
			stiBase64.cpp \
			stiDebugTools.cpp \
			stiError.cpp \
			stiGUID.cpp \
			stiRemoteLogEvent.cpp \
			stiSysUtilizationInfo.cpp \
			stiTools.cpp \
			stiTrace.cpp \
			stiMessages.cpp \
			CstiPhoneNumberValidator.cpp \
			CstiRegEx.cpp \
			CstiEventQueue.cpp \
			CstiWatchDog.cpp \
			CstiTimer.cpp \
			CstiEventTimer.cpp \
			CstiString.cpp \
			OptString.cpp \
			TimeAccumulator.cpp \
			FileEncryption.cpp \
			Random.cpp \
			SelfSignedCert.cpp



LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../common \
				$(LOCAL_PATH)/../Platform

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Platform

LOCAL_STATIC_LIBRARIES := os core_services block_list_mgr conf_mgr call_history_mgr

include $(BUILD_STATIC_LIBRARY)
