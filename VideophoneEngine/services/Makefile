TARGET := libstiServices.a

DEFS := 

INCS := \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(HTTP_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(CONFMGR_DIR)

CHILD_DIRECTORIES := \
	Conference \
	Core \
	Message \
	RemoteLogger \
	StateNotify \
	ServiceOutage

INSTALL_FILES := \
	VPServiceRequestContext.h \
	CstiVPServiceRequest.h \
	CstiVPServiceResponse.h \
	CstiVPService.h \
	ServiceCallbackContext.h \
	ServiceResponse.h \
	CstiList.h \
	CstiListItem.h

DOC_FILES := \
	VPServiceRequestContext.h \
	CstiList.h \
	CstiListItem.h \
	CstiString.h \
	CstiVPServiceResponse.h
	
OBJS := \
	CstiVPService.o \
	CstiVPServiceRequest.o \
	CstiVPServiceResponse.o \
	CstiList.o \
	CstiListItem.o

include $(MAKEFILE_RULES)

