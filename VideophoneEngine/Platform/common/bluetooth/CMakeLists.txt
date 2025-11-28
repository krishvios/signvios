# Define the library name
set(VPE_LIB stiPlatform-common-bluetooth)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  BleSendProgress.cpp
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
