TARGET := libMPEG4.a

CHILD_DIRECTORIES := \
	Atoms \
	Toolbox

OBJS := \
	MP4Movie.o \
	MP4Track.o \
	MP4Media.o \
	MP4Common.o \
	MP4Parser.o \
	MP4Optimize.o \
	MP4Signals.o \
	CVMCircularBuffer.o \
	CFileDataHandler.o \
	CServerDataHandler.o \
    CDataTransferHandler.o \

DEFS := 

INCS := \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_DIR)/common/wireless \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4/Toolbox \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4/Atoms \
	-I$(PLATFORM_COMMON_DIR)/fileplay/MPEG4 \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(HTTP_DIR)

include $(MAKEFILE_RULES)

