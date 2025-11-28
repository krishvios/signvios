TARGET := libXMLManager.a

OBJS := \
	XMLManager.o \
     XMLList.o \
	XMLListItem.o \
	XMLValue.o

DEFS := 

INCS := \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(HTTP_DIR)

INSTALL_FILES := \
	XMLListItem.h \
	XMLValue.h \
	XMLManager.h

include $(MAKEFILE_RULES)

