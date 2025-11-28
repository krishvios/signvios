# Define the library name
set(VPE_LIB stiSerialization)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  JsonFileSerializer.cpp
)

set(${VPE_LIB}_LIBRARIES
  ${Poco_LIBRARIES}
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  ISerializable.h
  SerializedProperty.h
  JsonSerializer.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
