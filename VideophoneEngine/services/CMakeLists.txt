# Define the library name
set(VPE_LIB stiServices)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiVPService.cpp
  CstiVPServiceRequest.cpp
  CstiVPServiceResponse.cpp
  CstiList.cpp
  CstiListItem.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiConference
  stiCore
  stiMessage
  stiRemoteLogger
  stiServiceOutage
  stiStateNotify
)

set(${VPE_LIB}_SUBDIRECTORIES
  Conference
  Core
  Message
  RemoteLogger
  ServiceOutage
  StateNotify
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  VPServiceRequestContext.h
  CstiVPServiceRequest.h
  CstiVPServiceResponse.h
  CstiVPService.h
  ServiceCallbackContext.h
  ServiceResponse.h
  CstiList.h
  CstiListItem.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  VPServiceRequestContext.h
  CstiList.h
  CstiListItem.h
  CstiString.h
  CstiVPServiceResponse.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
