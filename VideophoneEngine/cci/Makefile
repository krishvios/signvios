TARGET := libsticci.a

OBJS := CstiCCI.o

DEFS:=

INSTALL_FILES := \
	IstiVideophoneEngine.h

DOC_FILES := \
	IstiVideophoneEngine.h

INCS := \
	-I$(CCI_DIR) \
	-I$(COMMON_DIR) \
	-I$(CONFMGR_DIR) \
	-I$(CONFMGR_DIR)/DataTasks/DataCommon \
	-I$(CONFMGR_DIR)/Nat \
	-I$(CONFMGR_DIR)/Session \
	-I$(SERVICES_DIR) \
	-I$(CORE_SERVICE_DIR) \
	-I$(STATENOTIFY_SERVICE_DIR) \
	-I$(MESSAGE_SERVICE_DIR) \
	-I$(CONFERENCE_SERVICE_DIR) \
	-I$(LOGGER_SERVICE_DIR) \
	-I$(OUTAGE_SERVICE_DIR) \
	-I$(RVSHAREDDIR_INC) \
	-I$(RVSHAREDDIR_INC)/common \
	-I$(RVSHAREDDIR_INC)/sip \
	-I$(RVSHAREDDIR_INC)/ads \
	-I$(RVSHAREDDIR_INC)/sdp \
	-I$(RVSHAREDDIR_INC)/ice \
	-I$(RVSHAREDDIR_INC)/stun \
	-I$(RVSHAREDDIR_INC)/turn \
	-I$(IXML_DIR)/inc \
	-I$(UPNP_DIR)/threadutil/inc \
	-I$(UPNP_DIR)/upnp/inc \
	-I$(UPNP_BUILD_DIR)/upnp/inc \
	-I$(HTTP_DIR) \
	-I$(OS_DIR) \
	-I$(PM_INCLUDE_DIR) \
	-I$(VRCL_DIR) \
	-I$(XMLMGR_INCLUDE_DIR) \
	-I$(PLATFORM_DIR)/common/wireless \
	-I$(PLATFORM_DIR) \
	-I$(IMAGEMGR_DIR) \
	-I$(BLOCKLISTMGR_DIR) \
	-I$(RINGGROUPMGR_DIR) \
	-I$(MSGMGR_DIR) \
	-I$(CONTACTS_DIR) \
	-I$(ACMEDIR)/include \
	-I$(CALLHISTORYMGR_DIR) \
	-I$(FILEDOWNLOAD_DIR) \
	-I$(REGISTRATIONMGR_DIR) \
	-I$(OBSERVER_PTR_DIR) \
	-I$(USERACCOUNTMGR_DIR) \
	-I$(SERIALIZATION_DIR) \
	-I$(ENDPOINT_MONITOR_DIR)

ifneq (,$(findstring clang,$(CXX)))
	CXXFLAGS += \
		-Wno-final-dtor-non-final-class
endif

include $(MAKEFILE_RULES)

