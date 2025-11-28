# Define the library name
set(VPE_LIB stiBlockListManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiBlockListManager.cpp
  CstiBlockListMgrResponse.cpp
  CstiBlockListItem.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiBlockListManager.h
  IstiBlockListMgrResponse.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiBlockListManager.h
  IstiBlockListMgrResponse.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})


