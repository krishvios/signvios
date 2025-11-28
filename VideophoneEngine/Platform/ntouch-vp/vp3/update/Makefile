TARGET := libstiUpdate.a

CHILD_DIRECTORIES = 

OBJS :=  \
	CstiUpdate.o

DEFS := -DBUILD_OSTREE

INCS := \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_DIR)/common/wireless \
	-I$(VP_COMMON_DIR) \
	-I$(PLATFORM_COMMON_DIR)/platform \
	`pkg-config --cflags ostree-1` \
	
include $(MAKEFILE_RULES)

