TARGET := libWireless.a

OBJS :=

#OBJS += \
#   CstiWireless.o

DEFS := \
	-DLINUX \
	-DCONFIG_STA_SUPPORT

INSTALL_FILES := \
	stiWirelessDefs.h

INCS := \
	-I$(PLATFORM_COMMON_DIR)/wireless \
	-I$(PLATFORM_DIR) \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) \
	-I$(PLATFORM_COMMON_DIR)/wireless/include

include $(MAKEFILE_RULES)

