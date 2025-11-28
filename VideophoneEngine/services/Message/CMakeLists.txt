# Define the library name
set(VPE_LIB stiMessage)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiMessageRequest.cpp
  CstiMessageResponse.cpp
  CstiMessageService.cpp
  CstiMessageList.cpp
  CstiMessageListItem.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  CstiMessageRequest.h
  CstiMessageResponse.h
  CstiMessageList.h
  CstiMessageListItem.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  CstiMessageRequest.h
  CstiMessageResponse.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
