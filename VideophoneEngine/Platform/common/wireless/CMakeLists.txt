# Define the library name
set(VPE_LIB stiPlatform-common-wireless)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  stiWirelessDefs.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
