TARGET := libCstiCallHistoryManager.a

OBJS := \
	CstiCallHistoryManager.o \
	CstiCallHistoryMgrResponse.o \
	CstiCallHistoryItem.o \
	CstiCallHistoryList.o

DEFS :=

INCS := \
	-I$(CALLHISTORYMGR_DIR) \
	-I$(COMMON_DIR) \
	-I$(CONTACTS_DIR) \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(HTTP_DIR) \
	-I$(OS_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(IXML_DIR)/inc

INSTALL_FILES := \
	IstiCallHistoryManager.h \
	IstiCallHistoryMgrResponse.h \
	CstiCallHistoryItem.h \
	CstiCallHistoryList.h

DOC_FILES := \
	IstiCallHistoryManager.h \
	IstiCallHistoryMgrResponse.h \
	CstiCallHistoryItem.h \
	CstiCallHistoryList.h

include $(MAKEFILE_RULES)

