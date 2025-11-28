TARGET := libCstiRingGroupManager.a

OBJS := \
	CstiRingGroupManager.o \
 	CstiRingGroupMgrResponse.o
# 	CstiRingGroupItem.o

DEFS :=

INCS := \
	-I$(RINGGROUPMGR_DIR) \
	-I$(COMMON_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(HTTP_DIR) \
	-I$(OS_DIR)

INSTALL_FILES := \
	IstiRingGroupManager.h \
	IstiRingGroupMgrResponse.h

DOC_FILES := \
	IstiRingGroupManager.h \
	IstiRingGroupMgrResponse.h

include $(MAKEFILE_RULES)

