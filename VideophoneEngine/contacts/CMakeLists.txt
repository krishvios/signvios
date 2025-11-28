# Define the library name
set(VPE_LIB stiContacts)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  ContactManager.cpp
  FavoriteList.cpp
  ContactListItem.cpp
  CstiLDAPDirectoryContactManager.cpp
  LDAPDirectoryContactListItem.cpp
)

message("Sor_OpenldapLibs=${Sor_OpenldapLibs}")
set(${VPE_LIB}_LIBRARIES
  ${Sor_OpenldapLibs}
)

message("Sor_OpenldapIncPath=${Sor_OpenldapIncPath}")
set(${VPE_LIB}_INCLUDE_DIRECTORIES
  ${Sor_OpenldapIncPath}
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  IstiContact.h
  IstiContactManager.h
  IstiLDAPDirectoryContactManager.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  IstiContact.h
  IstiContactManager.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
