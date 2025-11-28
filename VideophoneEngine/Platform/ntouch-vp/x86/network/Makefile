TARGET := libstiNetwork.a

OBJS := \
	CstiDHCP.o \
	CstiNetwork.o

DEFS:= \
	-D_VERBOSE \
	-D_GNU_SOURCE \

INCS := \
	-I$(VP_X86_DIR) \
	-I$(PLATFORM_VP_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_COMMON_DIR)/wireless \
	-I$(PLATFORM_COMMON_DIR)/network

include $(MAKEFILE_RULES)


