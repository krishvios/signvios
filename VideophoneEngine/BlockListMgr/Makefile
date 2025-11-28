TARGET := libCstiBlockListManager.a

OBJS := \
	CstiBlockListManager.o \
	CstiBlockListMgrResponse.o \
	CstiBlockListItem.o

DEFS :=

INCS := \
	-I$(BLOCKLISTMGR_DIR) \
	-I$(COMMON_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(HTTP_DIR) \
	-I$(OS_DIR)

INSTALL_FILES := \
	IstiBlockListManager.h \
	IstiBlockListMgrResponse.h

DOC_FILES := \
	IstiBlockListManager.h \
	IstiBlockListMgrResponse.h

include $(MAKEFILE_RULES)

