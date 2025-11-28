# Define the library name
set(VPE_LIB stiPropertyManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  PropertyManager.cpp
  CstiPropertySender.cpp
  PropertyListItem.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiServices
  stiXMLManager
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  PropertyManager.h
  PropertyListItem.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
