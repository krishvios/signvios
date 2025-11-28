# Define the library name
set(VPE_LIB stiCore)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiCoreRequest.cpp
  CstiCoreResponse.cpp
  CstiCoreService.cpp
  CstiCallList.cpp
  CstiPhoneNumberList.cpp
  CstiPhoneNumberListItem.cpp
  CstiCallListItem.cpp
  CstiDirectoryResolveResult.cpp
  CstiContactList.cpp
  CstiContactListItem.cpp
  CstiFavorite.cpp
  OfflineCore.cpp
)

set(${VPE_LIB}_LIBRARIES
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  CstiCoreResponse.h
  CstiCoreRequest.h
  CstiCallList.h
  CstiCallListItem.h
  CstiContactList.h
  CstiContactListItem.h
  CstiPhoneNumberList.h
  CstiPhoneNumberListItem.h
  CstiUserAccountInfo.h
  CstiDirectoryResolveResult.h
  SstiSettingItem.h
  CstiFavorite.h
  ClientAuthenticateResult.h
  Address.h
)

# Define the documentation files for the library
set(${VPE_LIB}_DOC_FILES
  CstiCallListItem.h
  CstiCallList.h
  CstiCoreResponse.h
  CstiContactList.h
  CstiContactListItem.h
  CstiPhoneNumberList.h
  CstiPhoneNumberListItem.h
  CstiUserAccountInfo.h
  CstiDirectoryResolveResult.h
  SstiSettingItem.h
  CstiCoreRequest.h
  CstiCoreRequest.h
  CstiFavorite.h
  Address.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
