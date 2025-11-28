# Define the library name
set(VPE_LIB stiImageManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiImageDownloadService.cpp
  CstiImageManager.cpp
  CstiNvmImageCache.cpp
)

set(${VPE_LIB}_LIBRARIES
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
  ${JPEG_LIBRARIES}
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiImageManager.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiImageManager.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
