# Define the library name
set(VPE_LIB stiMessageManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiMessageInfo.cpp
  CstiMessageManager.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiMessageManager.h
  CstiMessageInfo.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiMessageManager.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
