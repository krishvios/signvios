TARGET := libContactManager.a

OBJS := \
	ContactManager.o \
	FavoriteList.o \
	ContactListItem.o \
	CstiLDAPDirectoryContactManager.o \
	LDAPDirectoryContactListItem.o

DEFS := \
	-DLDAP_DEPRECATED=1

INCS := \
	-I$(CONTACTS_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(HTTP_DIR) \
	-I$(IMAGEMGR_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(SERIALIZATION_DIR)

INSTALL_FILES := \
	IstiContact.h \
	IstiContactManager.h \
	IstiLDAPDirectoryContactManager.h

DOC_FILES := \
	IstiContact.h \
	IstiContactManager.h 

include $(MAKEFILE_RULES)
