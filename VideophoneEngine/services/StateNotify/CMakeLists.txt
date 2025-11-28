# Define the library name
set(VPE_LIB stiStateNotify)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiStateNotifyRequest.cpp
  CstiStateNotifyResponse.cpp
  CstiStateNotifyService.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  CstiStateNotifyResponse.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  CstiStateNotifyResponse.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
