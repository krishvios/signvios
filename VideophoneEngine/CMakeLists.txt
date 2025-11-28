cmake_minimum_required(VERSION 3.20)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Find a few vpe required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
if(Threads_FOUND)
  message(STATUS "Threads found:")
  message(STATUS "  Version: ${Threads_VERSION}")
  message(STATUS "  Include directories: ${Threads_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${Threads_LIBRARIES}")
  message(STATUS "  Definitions: ${Threads_DEFINITIONS}")
  message(STATUS "  Compile options: ${Threads_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "Threads not found")
endif()

#set(POCO_VERSION_MIN 1.11.2)
find_package(Poco ${POCO_VERSION_MIN} REQUIRED COMPONENTS Foundation Util Crypto JSON Net NetSSL XML Zip Data)
if(Poco_FOUND)
  message(STATUS "Poco found:")
  message(STATUS "  Version: ${Poco_VERSION}")
  message(STATUS "  Include directories: ${Poco_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${Poco_LIBRARIES}")
  message(STATUS "  Definitions: ${Poco_DEFINITIONS}")
  message(STATUS "  Compile options: ${Poco_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "Poco not found")
endif()

#set(BOOST_VERSION_MIN 1.78.0)
find_package(Boost ${BOOST_VERSION_MIN} REQUIRED COMPONENTS system filesystem program_options)
if(Boost_FOUND)
  message(STATUS "Boost found:")
  message(STATUS "  Version: ${Boost_VERSION}")
  message(STATUS "  Include directories: ${Boost_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${Boost_LIBRARIES}")
  message(STATUS "  Definitions: ${Boost_DEFINITIONS}")
  message(STATUS "  Compile options: ${Boost_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "Boost not found")
endif()

find_package(Libudev)
if(Libudev_FOUND)
  message(STATUS "Libudev found:")
  message(STATUS "  Version: ${Libudev_VERSION}")
  message(STATUS "  Include directories: ${Libudev_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${Libudev_LIBRARIES}")
  message(STATUS "  Definitions: ${Libudev_DEFINITIONS}")
  message(STATUS "  Compile options: ${Libudev_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "Libudev not found")
endif()

find_package(SQLite3)
if(SQLite3_FOUND)
  message(STATUS "SQLite3 found:")
  message(STATUS "  Version: ${SQLite3_VERSION}")
  message(STATUS "  Include directories: ${SQLite3_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${SQLite3_LIBRARIES}")
  message(STATUS "  Definitions: ${SQLite3_DEFINITIONS}")
  message(STATUS "  Compile options: ${SQLite3_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "SQLite3 not found")
endif()

# Radvision no longer pulls ldap in for us, so we need to do it here
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux"  OR  ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    find_path(OPENLDAP_INCLUDE_DIR ldap.h PATHS /usr/include /opt/local/include /usr/local/include)
    find_library(OPENLDAP_LIBRARY ldap)
    find_library(OPENLBER_LIBRARY lber)
    if (EXISTS "${OPENLDAP_INCLUDE_DIR}" AND EXISTS "${OPENLDAP_LIBRARY}" AND EXISTS "${OPENLBER_LIBRARY}")
      set(Sor_OpenldapIncPath "${OPENLDAP_INCLUDE_DIR}" CACHE PATH "path to OpenLDAP include folder")
      set(Sor_OpenldapLibs "${OPENLDAP_LIBRARY};${OPENLBER_LIBRARY}" CACHE FILEPATH "OpenLDAP ldap and lber library names")
    endif()
endif()
if (NOT Sor_OpenldapIncPath OR NOT Sor_OpenldapLibs)
  set(Sor_OpenldapIncPath "ldap.h-NOTFOUND" CACHE PATH "path to openldap include folder")
  set(Sor_OpenldapLibs    "libldap-liblber-LIB-NOTFOUND" CACHE FILEPATH "openldap library names")
endif()

find_package(JPEG REQUIRED COMPONENTS JPEG)
if(JPEG_FOUND)
  message(STATUS "JPEG found:")
  message(STATUS "  Version: ${JPEG_VERSION}")
  message(STATUS "  Include directories: ${JPEG_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${JPEG_LIBRARIES}")
  message(STATUS "  Definitions: ${JPEG_DEFINITIONS}")
  message(STATUS "  Compile options: ${JPEG_COMPILE_OPTIONS}")
else()
  message(FATAL_ERROR "JPEG not found")
endif()

# Define the library name
set(VPE_LIB stiVideophoneEngine)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  "smiConfig.h"
)

set(${VPE_LIB}_LIBRARIES
  stiFileDownload
  stiFileUpload
#  Tests
  stiCCI
  stiMessageManager
  stiSerialization
  stiVrcl
  stiContacts
  stiRingGroupManager
  stiBlockListManager
  stiImageManager
  stiCallHistoryManager
  stiEndpointMonitor
  stiUserAccountManager
  stiRegistrationManager
  stiServices
  stiHttp
  stiConferenceManager
  stiCommon
  stiOS
  stiPropertyManager
  stiXMLManager
  stiPlatform
  stiLibraries
)

set(${VPE_LIB}_SUBDIRECTORIES
  BlockListMgr
  CallHistoryMgr
  ConfMgr
  EndpointMonitor
  FileDownload
  FileUpload
  ImageMgr
  MsgMgr
  Platform
  RegistrationMgr
  RingGroupMgr
  Serialization
#  Tests
  UserAccountMgr
  cci
  contacts
  common
  http
  os
  pm
  services
  vrcl
  xmlmgr
  Libraries
)

set(VPE_COMMON_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/BlockListMgr
  ${CMAKE_CURRENT_LIST_DIR}/CallHistoryMgr
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/DataTasks/AudioPlayback
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/DataTasks/AudioRecord
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/DataTasks/DataCommon
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/DataTasks/TextPlayback
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/DataTasks/TextRecord
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/DataTasks/VideoPlayback
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/DataTasks/VideoRecord
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/Session
  ${CMAKE_CURRENT_LIST_DIR}/ConfMgr/Nat
  ${CMAKE_CURRENT_LIST_DIR}/EndpointMonitor
  ${CMAKE_CURRENT_LIST_DIR}/FileDownload
  ${CMAKE_CURRENT_LIST_DIR}/FileUpload
  ${CMAKE_CURRENT_LIST_DIR}/ImageMgr
  ${CMAKE_CURRENT_LIST_DIR}/MsgMgr
  ${CMAKE_CURRENT_LIST_DIR}/Platform
  ${CMAKE_CURRENT_LIST_DIR}/Platform/common/CameraControl
  ${CMAKE_CURRENT_LIST_DIR}/Platform/common/audio
  ${CMAKE_CURRENT_LIST_DIR}/Platform/common/callback
  ${CMAKE_CURRENT_LIST_DIR}/Platform/common/wireless
  ${CMAKE_CURRENT_LIST_DIR}/RegistrationMgr
  ${CMAKE_CURRENT_LIST_DIR}/RingGroupMgr
  ${CMAKE_CURRENT_LIST_DIR}/Serialization
  ${CMAKE_CURRENT_LIST_DIR}/UserAccountMgr
  ${CMAKE_CURRENT_LIST_DIR}/cci
  ${CMAKE_CURRENT_LIST_DIR}/common
  ${CMAKE_CURRENT_LIST_DIR}/contacts
  ${CMAKE_CURRENT_LIST_DIR}/http
  ${CMAKE_CURRENT_LIST_DIR}/os
  ${CMAKE_CURRENT_LIST_DIR}/pm
  ${CMAKE_CURRENT_LIST_DIR}/services
  ${CMAKE_CURRENT_LIST_DIR}/services/Conference
  ${CMAKE_CURRENT_LIST_DIR}/services/Core
  ${CMAKE_CURRENT_LIST_DIR}/services/Message
  ${CMAKE_CURRENT_LIST_DIR}/services/RemoteLogger
  ${CMAKE_CURRENT_LIST_DIR}/services/ServiceOutage
  ${CMAKE_CURRENT_LIST_DIR}/services/StateNotify
  ${CMAKE_CURRENT_LIST_DIR}/vrcl
  ${CMAKE_CURRENT_LIST_DIR}/xmlmgr
  ${CMAKE_CURRENT_LIST_DIR}/Libraries/observer_ptr
  ${CMAKE_CURRENT_LIST_DIR}/Libraries/Linux/libupnp-1.6.2/ixml/inc
  ${CMAKE_CURRENT_LIST_DIR}/Libraries/Acme/tscf/include
  $<TARGET_PROPERTY:rvads,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:rvartp,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:rvcommon,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:rvice,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:rvsdp,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:rvstun,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:rvturn,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:rvsip,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:rvofferanswer,INTERFACE_INCLUDE_DIRECTORIES>
)

set(VPE_COMMON_CMAKE
  ${CMAKE_CURRENT_LIST_DIR}/VideophoneEngine_common.cmake
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
