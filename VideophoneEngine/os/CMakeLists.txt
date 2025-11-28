# Define the library name
set(VPE_LIB stiOS)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  stiOSNet.c
  stiOSMsg.c
  stiOS.cpp
  stiOSLinux.cpp
  stiOSLinuxMsg.cpp
  stiOSLinuxMutex.cpp
  stiOSLinuxTask.cpp
  stiOSLinuxWd.cpp
  stiOSLinuxSignal.cpp
  stiOSLinuxNet.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiCommon
  stiPlatform-ntouch-vp-common
)  

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  stiOS.h
  stiOSMsg.h
  stiOSNet.h
  stiOSMutex.h
  stiOSTask.h
  stiOSSignal.h
  stiOSWd.h
  stiOSLinux.h
  stiOSLinuxMsg.h
  stiOSLinuxMutex.h
  stiOSLinuxTask.h
  stiOSLinuxNet.h
  stiOSLinuxWd.h
  stiOSLinuxSignal.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  stiOS.h
  stiOSMsg.h
  stiOSNet.h
  stiOSMutex.h
  stiOSTask.h
  stiOSSignal.h
  stiOSWd.h
)

#CPPFLAGS += -fno-strict-aliasing

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

