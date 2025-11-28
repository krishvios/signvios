
TARGET := libstiBluetooth.a

OBJS := \
	CstiBluetooth.o \
	CstiBluetoothInterface.o \
	BluetoothDbusInterface.o \
	AdapterInterface.o \
	AgentManagerInterface.o \
	DeviceInterface.o \
	PropertiesInterface.o \
	GattCharacteristicInterface.o \
	GattDescriptorInterface.o \
	GattManagerInterface.o \
	GattServiceInterface.o \
	BluetoothDefs.o \
	CstiHelios.o \
	BleSendProgress.o \
	NordicDfu.o \
	BluetoothAudioBase.o \
	$(EXTRA_OBJ)

ifeq ($(DEVICE), DEV_NTOUCH_VP2)
OBJS += \
	BluezAlsaClient.o
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP3)
OBJS += \
	BluezAlsaClient.o
endif

ifeq ($(DEVICE), DEV_NTOUCH_VP4)
OBJS += \
	BluezAlsaClient.o
endif

DEFS:= \
	-D_VERBOSE \
	-D_GNU_SOURCE \

CPPFLAGS += \
	-Wno-deprecated-declarations \
	-Wno-unused-local-typedefs

INCS := \
	-I./ \
	-I$(VP_COMMON_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(VP_COMMON_DIR)/wireless \
	-I$(DBUS_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(IXML_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(OBSERVER_PTR_DIR) \
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
	CstiBluetoothInterface.c \
	CstiBluetoothInterface.h

include $(MAKEFILE_RULES)

CstiBluetoothInterface.o : CstiBluetoothInterface.c

CstiBluetoothInterface.c : CstiBluetoothInterface.h

CstiBluetoothInterface.h : bluez.xml
	gdbus-codegen --interface-prefix org.bluez --c-namespace=CstiBluetooth --generate-c-code $* --c-generate-object-manager $<
