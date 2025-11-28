LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := contacts

LOCAL_SRC_FILES := 	ContactListItem.cpp \
			ContactManager.cpp \
			CstiLDAPDirectoryContactManager.cpp \
			LDAPDirectoryContactListItem.cpp \
			FavoriteList.cpp \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../contacts

LOCAL_STATIC_LIBRARIES := xml_mgr os vpcommon image_mgr libldap_static liblber_static

include $(BUILD_STATIC_LIBRARY)


