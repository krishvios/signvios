# Define the library name
set(VPE_LIB stiPlatform-common-fileplay)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CFilePlay.cpp
)

set(${VPE_LIB}_LIBRARIES
  MPEG4
}

set(${VPE_LIB}_SUBDIRECTORIES
  MPEG4
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  MPEG4
  MPEG4/Toolbox
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
