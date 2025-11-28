TARGET := libstiDbus.a

OBJS := \
	DbusObject.o \
	DbusConnection.o

DEFS:= \
	-D_VERBOSE \
	-D_GNU_SOURCE \

CPPFLAGS += \
	-Wno-deprecated-declarations \
	-Wno-unused-local-typedefs

INCS := \
	-I$(DBUS_DIR) \
	-I$(VP_COMMON_DIR) \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(IXML_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(OBSERVER_PTR_DIR) \
	`pkg-config --cflags glib-2.0 gthread-2.0 gio-2.0 gio-unix-2.0`
	
include $(MAKEFILE_RULES)
