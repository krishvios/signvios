# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-common-network)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiNetworkBase.cpp
  NetworkDbusInterface.cpp
  NetworkManagerInterface.cpp
  ServiceInterface.cpp
  TechnologyInterface.cpp
  CstiNetworkConnmanInterface.c
)

if(NOT "${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP2")
  list(APPEND ${VPE_LIB}_SOURCES
    CstiNetworkBase2.cpp
  )
endif()

set(${VPE_LIB}_LIBRARIES
  stiPlatform-common-network
  stiPlatform-ntouch-vp-common-dbus
  stiPlatform-ntouch-vp-common-bluetooth
)

set(${VPE_LIB}_COMPILE_OPTIONS
  -Wno-deprecated-declarations
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

# Must come afer include(${VPE_COMMON_CMAKE}) because target gets created there
# Bluetooth build uses gdbus_codegen
find_program(${VPE_LIB}_GDBUS_CODEGEN_EXECUTABLE gdbus-codegen REQUIRED)

set(${VPE_LIB}_DBUS_INTERFACE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/connman.xml")
set(${VPE_LIB}_DBUS_INTERFACE_PREFIX "net.connman")
set(${VPE_LIB}_DBUS_OUTPUT_INTERFACE_NAME "CstiNetworkConnmanInterface")
set(${VPE_LIB}_DBUS_OUTPUT_HEADER "CstiNetworkConnmanInterface.h")
set(${VPE_LIB}_DBUS_OUTPUT_SOURCE "CstiNetworkConnmanInterface.c")
set(${VPE_LIB}_DBUS_NAMESPACE "CstiNetwork")

add_custom_command(
  OUTPUT ${${VPE_LIB}_DBUS_OUTPUT_HEADER} ${${VPE_LIB}_DBUS_OUTPUT_SOURCE}
  COMMAND ${${VPE_LIB}_GDBUS_CODEGEN_EXECUTABLE}
    --generate-c-code ${${VPE_LIB}_DBUS_OUTPUT_INTERFACE_NAME}
    --c-namespace ${${VPE_LIB}_DBUS_NAMESPACE}
    --interface-prefix ${${VPE_LIB}_DBUS_INTERFACE_PREFIX}
    --c-generate-object-manager
    ${${VPE_LIB}_DBUS_INTERFACE_FILE}
  DEPENDS ${${VPE_LIB}_DBUS_INTERFACE_FILE}
)

add_custom_target(${VPE_LIB}_generate_dbus_code ALL
  DEPENDS ${${VPE_LIB}_DBUS_OUTPUT_HEADER} ${${VPE_LIB}_DBUS_OUTPUT_SOURCE}
)

add_dependencies(${VPE_LIB} ${VPE_LIB}_generate_dbus_code)

target_include_directories(${VPE_LIB} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)
