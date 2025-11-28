# Define the library name
set(VPE_LIB stiPlatform-common-fileplay-MPEG4)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  MP4Movie.cpp
  MP4Track.cpp
  MP4Media.cpp
  MP4Common.cpp
  MP4Parser.cpp
  MP4Optimize.cpp
  MP4Signals.cpp
  CVMCircularBuffer.cpp
  CFileDataHandler.cpp
  CServerDataHandler.cpp
  CDataTransferHandler.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiPlatform-common-fileplay-MPEG4-Atoms
  stiPlatform-common-fileplay-MPEG4-Toolbox
  ${Poco_LIBRARIES}
)

set(${VPE_LIB}_SUBDIRECTORIES
  Atoms
  Toolbox
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  Atoms
  Toolbox
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
