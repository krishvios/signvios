TARGET := libstiCommon.a

DEFS := \
	-D_REENTRANT \
	-D_GNU_SOURCE

INCS := \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(BLOCKLISTMGR_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(CALLHISTORYMGR_DIR) \
	-I$(CONFMGR_DIR) \
	-I$(RVSHAREDDIR_INC) \
	-I$(RVSHAREDDIR_INC)/common
	
INSTALL_FILES := \
 	stiDefs.h \
	stiSVX.h \
	stiMessages.h \
	stiMem.h \
	stiTrace.h \
	stiAssert.h \
	stiError.h \
	stiDebugTools.h \
	stiTools.h \
	stiSysUtilizationInfo.h \
	stiRemoteLogEvent.h \
	CstiItemId.h \
	CstiProfiler.h \
	CstiIpAddress.h \
	CstiPhoneNumberValidator.h \
	CstiRegEx.h \
	CstiString.h \
	CstiSignal.h \
	OptString.h \
	AgentHistoryItem.h \
	ISignal.h

DOC_FILES := $(INSTALL_FILES)

OBJS := \
	CstiOsTask.o \
	CstiOsTaskMQ.o \
	CstiExtendedEvent.o \
	CstiTask.o \
	CstiUniversalEvent.o \
	CstiHostNameResolver.o \
	cmPropertyNameDef.o \
	stiAssert.o \
	CstiEvent.o \
	stiError.o \
	stiDebugTools.o \
	stiSysUtilizationInfo.o \
	stiTools.o \
	stiTrace.o \
	stiGUID.o \
	ccrc.o \
	CstiItemId.o \
	CstiProfiler.o \
	stiBase64.o \
	CstiIpAddress.o \
	stiRemoteLogEvent.o \
	stiMessages.o \
	CstiPhoneNumberValidator.o \
	CstiRegEx.o\
	CstiEventTimer.o \
	CstiEventQueue.o \
	CstiWatchDog.o \
	CstiTimer.o \
	CstiString.o \
	OptString.o \
	TimeAccumulator.o \
	Random.o \
	FileEncryption.o \
	SelfSignedCert.o

ifeq ($(DEVICE), DEV_X86)
CPPFLAGS += -fno-strict-aliasing
endif

include $(MAKEFILE_RULES)

