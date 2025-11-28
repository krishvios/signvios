# Define the library name
set(VPE_LIB stiCallHistoryManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiCallHistoryManager.cpp
  CstiCallHistoryMgrResponse.cpp
  CstiCallHistoryItem.cpp
  CstiCallHistoryList.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiCallHistoryManager.h
  IstiCallHistoryMgrResponse.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiCallHistoryManager.h
  IstiCallHistoryMgrResponse.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
