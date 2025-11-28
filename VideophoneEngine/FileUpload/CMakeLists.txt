# Define the library name
set(VPE_LIB stiFileUpload)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiFileUploadHandler.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiFileUploadHandler.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiFileUploadHandler.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
