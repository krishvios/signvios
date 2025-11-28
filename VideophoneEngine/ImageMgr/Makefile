TARGET := libCstiImageManager.a

OBJS := \
	CstiImageDownloadService.o \
	CstiImageManager.o \
	CstiNvmImageCache.o

DEFS := 

INCS := \
	-I$(IMAGEMGR_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(HTTP_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(XMLMGR_INCLUDE_DIR)

INSTALL_FILES := \
	IstiImageManager.h

DOC_FILES := \
	IstiImageManager.h

include $(MAKEFILE_RULES)

