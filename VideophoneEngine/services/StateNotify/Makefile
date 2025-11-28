TARGET := libstiStateNotifyService.a

DEFS := 

INCS := \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(HTTP_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(CONFMGR_DIR)
	
INSTALL_FILES := \
	CstiStateNotifyResponse.h

DOC_FILES := \
	CstiStateNotifyResponse.h
	
OBJS := \
	CstiStateNotifyRequest.o \
	CstiStateNotifyResponse.o \
	CstiStateNotifyService.o

include $(MAKEFILE_RULES)

