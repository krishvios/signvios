TARGET := libstiGStreamer.a

OBJS := \
	GStreamerBin.o \
	GStreamerPad.o \
	GStreamerElement.o \
	GStreamerProperty.o \
	GStreamerPad.o \
	GStreamerBin.o \
	GStreamerEvent.o

DEFS:= 

CPPFLAGS += 

INCS := \
	-I$(COMMON_DIR) \
	-I$(OS_DIR) \
	-I$(GSTREAMER_DIR)
	
ifeq ($(DEVICE), DEV_NTOUCH_VP3)
	INCS += `pkg-config --cflags gstreamer-1.0`
else
ifeq ($(DEVICE), DEV_NTOUCH_VP4)
	INCS += `pkg-config --cflags gstreamer-1.0`
endif
ifeq ($(DEVICE), DEV_NTOUCH_VP2)
	INCS += `pkg-config --cflags gstreamer-0.10`
endif
ifeq ($(DEVICE), DEV_X86)
	INCS += `pkg-config --cflags gstreamer-1.0`
endif
endif

include $(MAKEFILE_RULES)
