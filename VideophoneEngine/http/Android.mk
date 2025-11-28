LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := http
LOCAL_SRC_FILES := 	CstiHTTPService.cpp \
			CstiURLResolve.cpp \
			stiSSLTools.cpp \
			CstiHTTPTask.cpp \
			stiHTTPServiceTools.cpp \
			SslCertificateHandler.cpp

LOCAL_C_INCLUDES += $(OPENSSL_DIR)/include \

LOCAL_EXPORT_C_INCLUDES := 	$(LOCAL_PATH)/../http \
				$(OPENSSL_DIR)/include \

LOCAL_STATIC_LIBRARIES := os vpcommon

include $(BUILD_STATIC_LIBRARY)
