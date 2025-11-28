LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := vp_tests
LOCAL_SRC_FILES := IstiVideophoneEngineTest.cpp \
	PropertyManagerTest.cpp \
	stdafx.cpp \
	stiToolsTest.cpp \
	Test.cpp \
	StringsTest.cpp

LOCAL_EXPORT_C_INCLUDES += $(LOCAL_PATH)/

LOCAL_C_INCLUDES += $(STANDALONE)/sysroot/usr/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../Libraries/Acme/tscf/include

LOCAL_STATIC_LIBRARIES := os pm cci cppunit

include $(BUILD_STATIC_LIBRARY)
