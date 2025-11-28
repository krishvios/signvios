TARGET := libstiDataCommon.a

OBJS := \
	CstiRtpHeaderLength.o \
	CstiDataTaskCommonP.o \
	CstiSyncManager.o \
	RTCPFlowControl.o \
	RtpPayloadAddon.o \
	utf8EncodeDecode.o \
	CstiRtpSession.o \
	CstiAesEncryptionContext.o \
	SDESKey.o \
	DtlsContext.o \
	RtxSender.o \
	RtpSender.o

DEFS := 

INCS := \
	-I$(OBSERVER_PTR_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(CCI_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(PM_INCLUDE_DIR) \
	-I$(CONFMGR_DIR)/DataTasks/AudioPlayback \
	-I$(CONFMGR_DIR)/DataTasks/AudioRecord \
	-I$(CONFMGR_DIR)/DataTasks/TextPlayback \
	-I$(CONFMGR_DIR)/DataTasks/TextRecord \
	-I$(CONFMGR_DIR)/DataTasks/DataCommon \
	-I$(CONFMGR_DIR)/DataTasks/VideoPlayback \
	-I$(CONFMGR_DIR)/DataTasks/VideoRecord \
	-I$(CONFMGR_DIR)/Session \
	-I$(RVSHAREDDIR_INC) \
	-I$(RVSHAREDDIR_INC)/common \
	-I$(RVSHAREDDIR_INC)/artp \
	-I$(RVSHAREDDIR_INC)/sdp \
	-I$(RVSHAREDDIR_INC)/sip \
	-I$(RVSHAREDDIR_INC)/ads \
	-I$(ACMEDIR_INC) \
	-I$(ACMEDIR)/include \
	-I$(CONFMGR_DIR)

include $(MAKEFILE_RULES)

