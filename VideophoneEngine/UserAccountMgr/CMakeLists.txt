# Define the library name
set(VPE_LIB stiUserAccountManager)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  UserAccountListItem.cpp
  UserAccountManager.cpp
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IUserAccountManager.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
