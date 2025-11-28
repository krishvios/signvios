# Define the library name
set(VPE_LIB stiRemoteLogger)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiRemoteLoggerRequest.cpp
  CstiRemoteLoggerService.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  CstiRemoteLoggerRequest.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
