# Define the library name
set(VPE_LIB stiCommon)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  "CstiOsTask.cpp"
  "CstiOsTaskMQ.cpp"
  "CstiExtendedEvent.cpp"
  "CstiTask.cpp"
  "CstiUniversalEvent.cpp"
  "CstiHostNameResolver.cpp"
  "cmPropertyNameDef.cpp"
  "stiAssert.cpp"
  "CstiEvent.cpp"
  "stiError.cpp"
  "stiDebugTools.cpp"
  "stiSysUtilizationInfo.cpp"
  "stiTools.cpp"
  "stiTrace.cpp"
  "stiGUID.cpp"
  "ccrc.cpp"
  "CstiItemId.cpp"
  "CstiProfiler.cpp"
  "stiBase64.cpp"
  "CstiIpAddress.cpp"
  "stiRemoteLogEvent.cpp"
  "stiMessages.cpp"
  "CstiPhoneNumberValidator.cpp"
  "CstiRegEx.cpp"
  "CstiEventTimer.cpp"
  "CstiEventQueue.cpp"
  "CstiWatchDog.cpp"
  "CstiTimer.cpp"
  "CstiString.cpp"
  "OptString.cpp"
  "TimeAccumulator.cpp"
  "Random.cpp"
  "FileEncryption.cpp"
  "SelfSignedCert.cpp"
)

set(${VPE_LIB}_LIBRARIES
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  "stiDefs.h"
  "stiSVX.h"
  "stiMessages.h"
  "stiMem.h"
  "stiTrace.h"
  "stiAssert.h"
  "stiError.h"
  "stiDebugTools.h"
  "stiTools.h"
  "stiSysUtilizationInfo.h"
  "stiRemoteLogEvent.h"
  "CstiItemId.h"
  "CstiProfiler.h"
  "CstiIpAddress.h"
  "CstiPhoneNumberValidator.h"
  "CstiRegEx.h"
  "CstiString.h"
  "CstiSignal.h"
  "OptString.h"
  "AgentHistoryItem.h"
  "ISignal.h"
)

set(${VPE_LIB}_LIBRARIES
  stiOS
)

#set(${VPE_LIB}_COMPILE_DEFINITIONS
#  _REENTRANT
#)

#ifeq ($(DEVICE), DEV_X86)
#CPPFLAGS += -fno-strict-aliasing
#endif

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
