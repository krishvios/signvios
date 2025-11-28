# Define the library name
set(VPE_LIB stiRingGroupManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiRingGroupManager.cpp
  CstiRingGroupMgrResponse.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiRingGroupManager.h
  IstiRingGroupMgrResponse.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiRingGroupManager.h
  IstiRingGroupMgrResponse.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
