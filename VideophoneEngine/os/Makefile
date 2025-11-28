TARGET := libos.a

OBJS := \
	stiOSNet.o \
	stiOSMsg.o \
	stiOS.o \
	stiOSLinux.o \
	stiOSLinuxMsg.o \
	stiOSLinuxMutex.o \
	stiOSLinuxTask.o \
	stiOSLinuxWd.o \
	stiOSLinuxSignal.o

ifeq ($(PLATFORM), LINUX)
	OBJS += stiOSLinuxNet.o
endif
ifeq ($(PLATFORM), APPLE)
	OBJS += stiOSMacOSXNet.o
endif

DEFS := \
	-D_GNU_SOURCE

INCS := \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_DIR)/common/wireless

INSTALL_FILES := \
	stiOS.h \
	stiOSMsg.h \
	stiOSNet.h \
	stiOSMutex.h \
	stiOSTask.h \
	stiOSSignal.h \
	stiOSWd.h \
	stiOSLinux.h \
	stiOSLinuxMsg.h \
	stiOSLinuxMutex.h \
	stiOSLinuxTask.h \
	stiOSLinuxNet.h \
	stiOSLinuxWd.h \
	stiOSLinuxSignal.h

DOC_FILES := \
	stiOS.h \
	stiOSMsg.h \
	stiOSNet.h \
	stiOSMutex.h \
	stiOSTask.h \
	stiOSSignal.h \
	stiOSWd.h

CPPFLAGS += -fno-strict-aliasing

include $(MAKEFILE_RULES)

