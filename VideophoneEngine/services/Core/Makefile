TARGET := libstiCoreService.a

DEFS := 

INCS := \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(CONFERENCE_SERVICE_DIR) \
	-I$(LOGGER_SERVICE_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(HTTP_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(CONFMGR_DIR)
	
INSTALL_FILES := \
	CstiCoreResponse.h \
	CstiCoreRequest.h \
	CstiCallList.h \
	CstiCallListItem.h \
	CstiContactList.h \
	CstiContactListItem.h \
	CstiPhoneNumberList.h \
	CstiPhoneNumberListItem.h \
	CstiUserAccountInfo.h \
	CstiDirectoryResolveResult.h \
	SstiSettingItem.h \
	CstiFavorite.h \
	ClientAuthenticateResult.h \
	Address.h

DOC_FILES := \
	CstiCallListItem.h \
	CstiCallList.h \
	CstiCoreResponse.h \
	CstiContactList.h \
	CstiContactListItem.h \
	CstiPhoneNumberList.h \
	CstiPhoneNumberListItem.h \
	CstiUserAccountInfo.h \
	CstiDirectoryResolveResult.h \
	SstiSettingItem.h \
	CstiCoreRequest.h \
	CstiCoreRequest.h \
	CstiFavorite.h \
	Address.h
	
OBJS := \
	CstiCoreRequest.o \
	CstiCoreResponse.o \
	CstiCoreService.o \
	CstiCallList.o \
	CstiPhoneNumberList.o \
	CstiPhoneNumberListItem.o \
	CstiCallListItem.o \
	CstiDirectoryResolveResult.o \
	CstiContactList.o \
	CstiContactListItem.o \
	CstiFavorite.o \
	OfflineCore.o

include $(MAKEFILE_RULES)

