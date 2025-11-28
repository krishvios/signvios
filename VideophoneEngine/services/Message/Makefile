TARGET := libstiMessageService.a

DEFS := 

INCS := \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(HTTP_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(CONFMGR_DIR)
	
INSTALL_FILES := \
	CstiMessageRequest.h \
	CstiMessageResponse.h \
	CstiMessageList.h \
	CstiMessageListItem.h

DOC_FILES := \
	CstiMessageRequest.h \
	CstiMessageResponse.h
		
OBJS := \
	CstiMessageRequest.o \
	CstiMessageResponse.o \
	CstiMessageService.o \
	CstiMessageList.o \
	CstiMessageListItem.o \

include $(MAKEFILE_RULES)

