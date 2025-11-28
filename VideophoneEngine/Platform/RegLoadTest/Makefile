TARGET := libstiDeviceController.a

#LIBS :=  \
#	`pkg-config --libs gstreamer-0.10` \
#	`pkg-config --libs gstreamer-app-0.10`
LIBS :=

OBJS := \
	CstiBSPInterface.o \
	CstiStatusLED.o \
	CstiLightRing.o \
	CstiDisplay.o \
	CstiAudioInput.o \
	CstiAudioOutput.o \
	CstiAudibleRinger.o \
	CstiUserInput.o \
	CstiBufferElement.o \
	CstiNetwork.o \
	CstiFileVideoInput.o

DEFS := \
	-DLINUX \
	-DCONFIG_STA_SUPPORT

INCS := \
	-I$(SMI_CONFIG_PATH) \
	-I. \
	-I../../os \
	-I../../common \
	-I../common/platform \
	-I../common/audio \
	-I../common/network \
	-I../common/video \
	-I../ \
	-I$(COMMON_DIR)/callback \
	-I../../MPEG4/Toolbox \
	-I/usr/local/include \
	`pkg-config --cflags glib-2.0 gthread-2.0 dbus-glib-1`
	
#INCS += `pkg-config --cflags gstreamer-0.10`


LIBS += -ludev `pkg-config -libs gio-2.0`

include $(MAKEFILE_RULES)
