TARGET := libstiVideoRecord.a

OBJS := \
	CstiVideoRecord.o \
	RecordFlowControl.o \
	RateHistoryManager.o \
	Packetization.o

DEFS := 

INCS := \
	-I$(OBSERVER_PTR_DIR) \
	-I$(CONFMGR_DIR)/DataTasks/VideoRecord \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_DIR)/common/CameraControl \
	-I$(RVSHAREDDIR_INC) \
	-I$(RVSHAREDDIR_INC)/common \
	-I$(RVSHAREDDIR_INC)/artp \
	-I$(RVSHAREDDIR_INC)/ads \
	-I$(RVSHAREDDIR_INC)/sip \
	-I$(RVSHAREDDIR_INC)/sdp \
	-I$(ACMEDIR_INC) \
	-I$(CONFMGR_DIR)/DataTasks/DataCommon \
	-I$(CONFMGR_DIR)/Session \
	-I$(CONFMGR_DIR)

include $(MAKEFILE_RULES)

