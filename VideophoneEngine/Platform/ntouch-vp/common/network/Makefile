TARGET := libstiNetwork.a

OBJS := \
	CstiNetworkBase.o \
	NetworkDbusInterface.o \
	NetworkManagerInterface.o \
	ServiceInterface.o \
	TechnologyInterface.o \
	CstiNetworkConnmanInterface.o

ifneq ($(DEVICE), DEV_NTOUCH_VP2)
OBJS += \
	CstiNetworkBase2.o
endif

DEFS:= \
	-D_VERBOSE \
	-D_GNU_SOURCE \

CPPFLAGS += \
	-Wno-deprecated-declarations \
	-Wno-unused-local-typedefs

INCS := \
	-I$(VP_COMMON_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_COMMON_DIR)/wireless \
	-I$(PLATFORM_COMMON_DIR)/network \
	-I$(DBUS_DIR) \
	-I$(BLUETOOTH_DIR) \
	`pkg-config --cflags glib-2.0 gthread-2.0 gio-2.0 gio-unix-2.0`

ifeq ($(DEVICE), DEV_NTOUCH_VP2)
	INCS += \
	-I$(PLATFORM_VP_DIR)/vp2
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP3)
	INCS += \
	-I$(PLATFORM_VP_DIR)/vp3
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP4)
	INCS += \
	-I$(PLATFORM_VP_DIR)/vp4
endif

ifeq ($(DEVICE), DEV_X86)
	INCS += \
		-I$(VP_X86_DIR)
endif

PRIORITY_TARGETS := \
	CstiNetworkConnmanInterface.c \
	CstiNetworkConnmanInterface.h

include $(MAKEFILE_RULES)

CstiNetworkBase.cpp: CstiNetworkConnmanInterface.h

CstiNetworkConnmanInterface.o : CstiNetworkConnmanInterface.c

CstiNetworkConnmanInterface.c : CstiNetworkConnmanInterface.h

CstiNetworkConnmanInterface.h : connman.xml
	 gdbus-codegen --interface-prefix net.connman --c-namespace=CstiNetwork --generate-c-code $* --c-generate-object-manager $<


