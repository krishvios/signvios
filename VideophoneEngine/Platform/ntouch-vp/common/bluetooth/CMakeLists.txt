# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp-common-bluetooth)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiBluetooth.cpp
  CstiBluetoothInterface.c
  BluetoothDbusInterface.cpp
  AdapterInterface.cpp
  AgentManagerInterface.cpp
  DeviceInterface.cpp
  PropertiesInterface.cpp
  GattCharacteristicInterface.cpp
  GattDescriptorInterface.cpp
  GattManagerInterface.cpp
  GattServiceInterface.cpp
  BluetoothDefs.cpp
  CstiHelios.cpp
  BleSendProgress.cpp
  NordicDfu.cpp
  BluetoothAudioBase.cpp
)

set(${VPE_LIB}_LIBRARIES
  stiCommon
  ${Boost_LIBRARIES}
  Boost::dynamic_linking
  ${PKG_CFG_GLIB_LIBRARIES}
  ${PKG_CFG_GTHREAD_LIBRARIES}
  ${PKG_CFG_GIO_LIBRARIES}
  ${PKG_CFG_GIO_UNIX_LIBRARIES}
  ${PKG_CFG_DBUS_GLIB_LIBRARIES}
)

set(${VPE_LIB}_INCLUDE_DIRECTORIES
)

if("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP2"
  OR
  "${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP3"
  OR
  "${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP4"
  )
  list(APPEND ${VPE_LIB}_SOURCES
    BluezAlsaClient.cpp
  )

  find_package(ALSA REQUIRED)
  if(ALSA_FOUND)
    message(STATUS "ALSA found:")
    message(STATUS "  Version: ${ALSA_VERSION}")
    message(STATUS "  Include directories: ${ALSA_INCLUDE_DIRS}")
    message(STATUS "  Libraries: ${ALSA_LIBRARIES}")
    message(STATUS "  Definitions: ${ALSA_DEFINITIONS}")
    message(STATUS "  Compile options: ${ALSA_COMPILE_OPTIONS}")
  else()
    message(FATAL_ERROR "ALSA not found")
  endif()
  
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(PKG_CFG_BLUEZ REQUIRED bluez)
  
  if(PKG_CFG_BLUEZ_FOUND)
    message(STATUS "bluez found:")
    message(STATUS "  Version: ${PKG_CFG_BLUEZ_VERSION}")
    message(STATUS "  Include directories: ${PKG_CFG_BLUEZ_INCLUDE_DIRS}")
    message(STATUS "  Libraries: ${PKG_CFG_BLUEZ_LIBRARIES}")
    message(STATUS "  Definitions: ${PKG_CFG_BLUEZ_DEFINITIONS}")
    message(STATUS "  Compile options: ${PKG_CFG_BLUEZ_COMPILE_OPTIONS}")
  else()
    message(FATAL_ERROR "bluez not found")
  endif()
  
  list(APPEND ${VPE_LIB}_LIBRARIES
    bluealsa
    ${ALSA_LIBRARIES}
    ${PKG_CFG_BLUEZ_LIBRARIES}
  )
  
  list(APPEND ${VPE_LIB}_INCLUDE_DIRECTORIES
    ${ALSA_INCLUDE_DIRS}
    ${PKG_CFG_BLUEZ_INCLUDE_DIRS}
  )
endif()

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ..
  ../dbus
)

set(${VPE_LIB}_COMPILE_OPTIONS
  -Wno-deprecated-declarations
)


if("${LUMINA_DEVICE}" STREQUAL "DEV_X86")
  list(APPEND ${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
    ../../x86
  )
endif()

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

# Must come afer include(${VPE_COMMON_CMAKE}) because target get created there
# Bluetooth build uses gdbus_codegen
find_program(${VPE_LIB}_GDBUS_CODEGEN_EXECUTABLE gdbus-codegen REQUIRED)

set(${VPE_LIB}_DBUS_INTERFACE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/bluez.xml")
set(${VPE_LIB}_DBUS_INTERFACE_PREFIX "org.bluez")
set(${VPE_LIB}_DBUS_OUTPUT_INTERFACE_NAME "CstiBluetoothInterface")
set(${VPE_LIB}_DBUS_OUTPUT_HEADER "CstiBluetoothInterface.h")
set(${VPE_LIB}_DBUS_OUTPUT_SOURCE "CstiBluetoothInterface.c")
set(${VPE_LIB}_DBUS_NAMESPACE "CstiBluetooth")

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

