TARGET := libstiVrcl.a

OBJS := \
    CstiCircularBuffer.o \
    CstiVRCLClient.o \
    CstiVRCLServer.o \
    CstiXML.o \
    CstiXMLCircularBuffer.o \
    ExternalConferenceData.o \
    stiClientValidate.o \
    stiVRCLCommon.o \
    ExternalConferenceData.o

DEFS := 

INSTALL_FILES := \
    ExternalConferenceData.h

INCS := \
	-I$(VRCL_DIR) \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(IXML_DIR)/inc \
	-I$(CCI_DIR) \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_DIR)/common/wireless \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(HTTP_DIR) \
	-I$(CONFMGR_DIR) \
	-I$(CONFMGR_DIR)/Session \
	-I$(CONFMGR_DIR)/DataTasks/DataCommon \
	-I$(RVSHAREDDIR_INC) \
	-I$(RVSHAREDDIR_INC)/common \
	-I$(RVSHAREDDIR_INC)/common/config \
	-I$(RVSHAREDDIR_INC)/sip \
	-I$(RVSHAREDDIR_INC)/ads \
	-I$(ACMEDIR)/include \
	-I$(OBSERVER_PTR_DIR) \
	-I$(SERIALIZATION_DIR)

ifneq (,$(findstring clang,$(CXX)))
	CXXFLAGS += \
		-Wno-overloaded-virtual
endif

include $(MAKEFILE_RULES)
