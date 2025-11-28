# Enable this to include support for standards-compliant H.263
# RFC2190 := 1


TARGET := libstiConfMgr.a

CHILD_DIRECTORIES := \
	DataTasks \
	Nat \
	Session \
	Sip

OBJS := \
	CstiCall.o \
	CstiCallInfo.o \
	CstiCallStorage.o \
	CstiConferenceManager.o \
	CstiProtocolCall.o \
	CstiProtocolManager.o \
	stiSystemInfo.o \
	CstiRoutingAddress.o \
	stiSipTools.o \
	CstiSipRegistration.o \
	CstiSipManager.o \
	CstiSipCall.o \
	CstiSipCallLeg.o \
	CstiSipResolver.o \
	stiSipStatistics.o \
	DhvClient.o

ifneq ($(APPLICATION), APP_REGISTRAR_LOAD_TEST_TOOL)
	OBJS += CstiTunnelManager.o
endif

DEFS :=

INSTALL_FILES := \
	ICallInfo.h \
	IstiCall.h \
	IVRCLCall.h \
	IstiTunnelManager.h \
	stiConfDefs.h \
	CstiServerInfo.h \
	CORP_CA.cer.pem

DOC_FILES := \
	ICallInfo.h \
	IstiCall.h

INCS := \
	-I$(OBSERVER_PTR_DIR) \
	-I$(CONFMGR_DIR) \
	-I$(COMMON_DIR) \
	-I$(CONFMGR_DIR)/DataTasks/AudioPlayback \
	-I$(CONFMGR_DIR)/DataTasks/AudioRecord \
	-I$(CONFMGR_DIR)/DataTasks/TextPlayback \
	-I$(CONFMGR_DIR)/DataTasks/TextRecord \
	-I$(CONFMGR_DIR)/DataTasks/DataCommon \
	-I$(CONFMGR_DIR)/DataTasks/VideoPlayback \
	-I$(CONFMGR_DIR)/DataTasks/VideoRecord \
	-I$(CONFMGR_DIR)/Nat \
	-I$(CONFMGR_DIR)/Session \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_DIR)/common/audio \
	-I$(PLATFORM_DIR)/common/wireless \
	-I$(PM_INCLUDE_DIR) \
	-I$(OS_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(BLOCKLISTMGR_DIR) \
	-I$(RVSHAREDDIR_INC) \
	-I$(RVSHAREDDIR_INC)/common \
	-I$(RVSHAREDDIR_INC)/ads \
	-I$(RVSHAREDDIR_INC)/common \
	-I$(RVSHAREDDIR_INC)/offerAnswer \
	-I$(RVSHAREDDIR_INC)/artp \
	-I$(RVSHAREDDIR_INC)/sdp \
	-I$(RVSHAREDDIR_INC)/sip \
	-I$(RVSHAREDDIR_INC)/ice \
	-I$(RVSHAREDDIR_INC)/stun \
	-I$(RVSHAREDDIR_INC)/turn \
	-I$(ACMEDIR)/include \
	-I$(HTTP_DIR) \
	-I$(ENDPOINT_MONITOR_DIR) \
	-I$(SERIALIZATION_DIR)

ifneq (,$(findstring clang,$(CXX)))
	CXXFLAGS += \
		-Wno-unused-lambda-capture \
		-Wno-undefined-bool-conversion
endif

include $(MAKEFILE_RULES)

