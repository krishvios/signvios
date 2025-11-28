LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := vpvrcl
LOCAL_SRC_FILES := 	CstiCircularBuffer.cpp \
			CstiVRCLClient.cpp \
			CstiVRCLServer.cpp \
			CstiXML.cpp \
			CstiXMLCircularBuffer.cpp \
			stiClientValidate.cpp \
			stiVRCLCommon.cpp \
			ExternalConferenceData.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../vrcl

LOCAL_STATIC_LIBRARIES := vpcommon os conf_mgr pm xml_mgr

include $(BUILD_STATIC_LIBRARY)
