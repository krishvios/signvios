# Define the library name
set(VPE_LIB stiCCI)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiCCI.cpp
)

set(${VPE_LIB}_LIBRARIES
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
  stiLibraries
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiVideophoneEngine.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiVideophoneEngine.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
