# Define the library name
set(VPE_LIB stiXMLManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  XMLManager.cpp
  XMLList.cpp
  XMLListItem.cpp
  XMLValue.cpp
  XMLListItem.h
)

set(${VPE_LIB}_LIBRARIES
  stiCommon
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  XMLListItem.h
  XMLValue.h
  XMLManager.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
