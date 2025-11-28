# Define the library name
set(VPE_LIB stiEndpointMonitor)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  EndpointMonitor.cpp
)

set(${VPE_LIB}_LIBRARIES
  ${Poco_LIBRARIES}
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

#ifneq (,$(findstring clang,$(CXX)))
#	CXXFLAGS += \
#		-Wno-unused-lambda-capture
#endif
