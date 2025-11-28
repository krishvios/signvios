# Define the library name
set(VPE_LIB stiPlatform-common-platform)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  BasePlatform.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  BasePlatform.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
