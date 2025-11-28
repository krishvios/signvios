TARGET := libstiEndpointMonitor.a

DEFS := 

INCS := \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) \
	-I$(CONFMGR_DIR) \
	-I$(CONFMGR_DIR)/Session \
	-I$(PLATFORM_DIR) \
	-I$(SIPDIR)/include/sip \
	-I$(SIPDIR)/include/common \
	-I$(SIPDIR)/include/ads \
	-I$(PM_INCLUDE_DIR) \
	-I$(IXML_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(SERIALIZATION_DIR)

INSTALL_FILES := \

OBJS := \
	EndpointMonitor.o \

ifneq (,$(findstring clang,$(CXX)))
	CXXFLAGS += \
		-Wno-unused-lambda-capture
endif

include $(MAKEFILE_RULES)

