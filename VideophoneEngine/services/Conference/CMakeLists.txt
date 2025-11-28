# Define the library name
set(VPE_LIB stiConference)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiConferenceRequest.cpp
  CstiConferenceResponse.cpp
  CstiConferenceService.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  CstiConferenceRequest.h
  CstiConferenceResponse.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  CstiConferenceRequest.h
  CstiConferenceResponse.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
