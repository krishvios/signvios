# Define the library name
set(VPE_LIB stiFileDownload)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiFileDownloadHandler.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiFileDownloadHandler.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiFileDownloadHandler.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
