TARGET := libstiVideoPlayback.a

OBJS := \
	CstiVideoPlayback.o \
	CstiVideoPlaybackRead.o \
	PlaybackFlowControl.o

DEFS := 

INCS := \
	-I$(OBSERVER_PTR_DIR) \
	-I$(CONFMGR_DIR)/DataTasks/VideoPlayback \
	-I$(CONFMGR_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(RVSHAREDDIR_INC) \
	-I$(RVSHAREDDIR_INC)/common \
	-I$(RVSHAREDDIR_INC)/artp \
	-I$(RVSHAREDDIR_INC)/ads \
	-I$(RVSHAREDDIR_INC)/sip \
	-I$(RVSHAREDDIR_INC)/sdp \
	-I$(RVSHAREDDIR_INC)/turn \
	-I$(RVSHAREDDIR_INC)/stun \
	-I$(ACMEDIR_INC) \
	-I$(CONFMGR_DIR)/DataTasks/DataCommon \
	-I$(CONFMGR_DIR)/Session

include $(MAKEFILE_RULES)

