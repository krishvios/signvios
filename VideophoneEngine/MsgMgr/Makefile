TARGET := libCstiMessageManager.a

OBJS := \
	CstiMessageInfo.o \
	CstiMessageManager.o 

DEFS :=

INCS := \
	-I$(MSGMGR_DIR) \
	-I$(COMMON_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(HTTP_DIR) \
	-I$(OS_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(SERIALIZATION_DIR)

INSTALL_FILES := \
	IstiMessageManager.h \
	CstiMessageInfo.h

DOC_FILES := \
	IstiMessageManager.h 

include $(MAKEFILE_RULES)

