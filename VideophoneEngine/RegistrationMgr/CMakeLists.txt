# Define the library name
set(VPE_LIB stiRegistrationManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiRegistrationManager.cpp
  CstiRegistrationList.cpp
  CstiRegistrationListItem.cpp
)

set(${VPE_LIB}_COMPILE_OPTIONS
  -Wno-deprecated-declarations
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
