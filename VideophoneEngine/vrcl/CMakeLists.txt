# Define the library name
set(VPE_LIB stiVrcl)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiCircularBuffer.cpp
  CstiVRCLClient.cpp
  CstiVRCLServer.cpp
  CstiXML.cpp
  CstiXMLCircularBuffer.cpp
  ExternalConferenceData.cpp
  stiClientValidate.cpp
  stiVRCLCommon.cpp
  ExternalConferenceData.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiLibraries
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  ExternalConferenceData.h
)

#ifneq (,$(findstring clang,$(CXX)))
#	CXXFLAGS += \
#		-Wno-overloaded-virtual
#endif

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
