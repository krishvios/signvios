cmake_minimum_required(VERSION 3.20)

message("\n${VPE_LIB}: Creating VideophoneEngine target: ${VPE_LIB}")

if(${VPE_LIB}_SOURCES)
  message("${VPE_LIB}: adding these sources: ${${VPE_LIB}_SOURCES}")
  add_library(${VPE_LIB} STATIC ${${VPE_LIB}_SOURCES})
  if(${VPE_LIB}_API_INCLUDE_FILES)
    # Define the directory containing the header files to be copied
    set(BUILD_API_INCLUDE_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")

    set(BUILD_API_INCLUDE_INPUT_FILES ${${VPE_LIB}_API_INCLUDE_FILES})
    list(TRANSFORM BUILD_API_INCLUDE_INPUT_FILES PREPEND ${BUILD_API_INCLUDE_INPUT_DIR})

    # Define the destination directory for the copied header files within the build directory
    set(BUILD_API_INCLUDE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/VideophoneEngine/include/")

    set(BUILD_API_INCLUDE_OUTPUT_FILES ${${VPE_LIB}_API_INCLUDE_FILES})
    list(TRANSFORM BUILD_API_INCLUDE_OUTPUT_FILES PREPEND ${BUILD_API_INCLUDE_OUTPUT_DIR})
    #message("${VPE_LIB}_API_INCLUDE_FILES: ${${VPE_LIB}_API_INCLUDE_FILES}")
    #message("BUILD_API_INCLUDE_INPUT_DIR: ${BUILD_API_INCLUDE_INPUT_DIR}")
    #message("BUILD_API_INCLUDE_INPUT_FILES: ${BUILD_API_INCLUDE_INPUT_FILES}")

    #message("BUILD_API_INCLUDE_OUTPUT_DIR: ${BUILD_API_INCLUDE_OUTPUT_DIR}")
    #message("BUILD_API_INCLUDE_OUTPUT_FILES: ${BUILD_API_INCLUDE_OUTPUT_FILES}")

    # Add other API files
    set(BUILD_API_DIST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Distribution/")

    add_custom_command(
      PRE_BUILD
      OUTPUT ${BUILD_API_INCLUDE_OUTPUT_FILES}
      COMMAND_EXPAND_LISTS
      COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_API_INCLUDE_OUTPUT_DIR}
      # Copy each file in the list
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${BUILD_API_INCLUDE_INPUT_FILES}
        ${BUILD_API_INCLUDE_OUTPUT_DIR}
      COMMENT "Copying ${VPE_LIB} include files to ${BUILD_API_INCLUDE_OUTPUT_DIR}"
      DEPENDS ${BUILD_API_INCLUDE_INPUT_FILES}
    )

    add_custom_target(${VPE_LIB}_copy_files
      ALL
      DEPENDS ${BUILD_API_INCLUDE_OUTPUT_FILES}
    )

    add_dependencies(${VPE_LIB}
      ${VPE_LIB}_copy_files
    )

    #target_include_directories(${VPE_LIB}
    #  PUBLIC "${BUILD_API_INCLUDE_OUTPUT_DIR}"
    #)

    install(FILES ${${VPE_LIB}_API_INCLUDE_FILES} DESTINATION include/VideophoneEngine)
  endif()

  if(${VPE_LIB}_LIBRARIES)
    message("${VPE_LIB}: linking these libraries: ${${VPE_LIB}_LIBRARIES}")

    foreach(lib ${${VPE_LIB}_LIBRARIES})

      #message(STATUS "Adding dependency: ${lib}")
      #add_dependencies(${VPE_LIB}
      #  ${lib}
      #)

      target_link_libraries(${VPE_LIB}
        PUBLIC
        ${lib}
      )

      #target_include_directories(${VPE_LIB}
      #  PUBLIC
      #  $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>
      #)
    endforeach()

  endif()

  if(${VPE_LIB}_DEPENDENCIES)
    message("${VPE_LIB}: adding these dependencies to target: ${${VPE_LIB}_DEPENDENCIES}")

    foreach(lib ${${VPE_LIB}_DEPENDENCIES})
      add_dependencies(${VPE_LIB}
        ${lib}
      )

      #message("${VPE_LIB}: because of this dependency: ${lib}")
      #message("${VPE_LIB}: adding these include directories: $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>")
      #target_include_directories(${VPE_LIB}
      #  PUBLIC
      #  $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>
      #)

    endforeach()

  endif()

  # Specify videophone engine include directories for the target
  if(${VPE_LIB}_INCLUDE_DIRECTORIES)
    message("${VPE_LIB}: adding these include directories to target: ${${VPE_LIB}_INCLUDE_DIRECTORIES}")
    target_include_directories(${VPE_LIB}
      PUBLIC
      ${${VPE_LIB}_INCLUDE_DIRECTORIES}
    )
  endif()

  target_include_directories(${VPE_LIB}
    PUBLIC
    .
  )

#[[
  if(VPE_ALL_LIBRARIES)
    message("${VPE_LIB}: linking these libraries: ${VPE_ALL_LIBRARIES}")

    foreach(lib ${VPE_ALL_LIBRARIES})

      #message(STATUS "Adding dependency: ${lib}")
      #add_dependencies(${VPE_LIB}
      #  ${lib}
      #)

      if(NOT (${lib} STREQUAL ${VPE_LIB}))
      target_link_libraries(${VPE_LIB}
        PUBLIC
#       PRIVATE
        ${lib}
      )
      endif()

      #target_include_directories(${VPE_LIB}
      #  PUBLIC
      #  $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>
      #)
    endforeach()

  endif()
]]

  if(${VPE_LIB}_COMPILE_DEFINITIONS)
    message("${VPE_LIB}: adding these compile definitions to target: ${${VPE_LIB}_COMPILE_DEFINITIONS}")
    target_compile_definitions(${VPE_LIB}
      PUBLIC
      ${${VPE_LIB}_COMPILE_DEFINITIONS}
    )
  endif()

  target_compile_options(${VPE_LIB}
    PUBLIC
    -Werror
  )

  if(${VPE_LIB}_COMPILE_OPTIONS)
    message("${VPE_LIB}: adding these compile options to target: ${${VPE_LIB}_COMPILE_OPTIONS}")
    target_compile_options(${VPE_LIB}
      PUBLIC
      ${${VPE_LIB}_COMPILE_OPTIONS}
    )
  endif()

  if(${VPE_LIB}_VPE_EXCLUDE_COMPILE_DEFINES_AND_OPTIONS)
    message("${VPE_LIB}: Not adding all videophone engine defines and options to target")
  else()
    message("${VPE_LIB}: Adding all common videophone engine defines and options to target")
    target_compile_definitions(${VPE_LIB}
      PUBLIC
      DEVICE=${LUMINA_DEVICE}
      SUBDEVICE=SUBDEV_MPU
      APPLICATION=${LUMINA_APP}
      smiSW_VERSION_NUMBER=\"${LUMINA_SW_VERSION_NUMBER}\"
    )

    target_compile_options(${VPE_LIB}
      PUBLIC
      -include smiConfig.h
    )
  endif()

  # Specify videophone engine include directories for the target
  if(${VPE_LIB}_VPE_EXCLUDE_COMMON_INCLUDE_DIRECTORIES)
    message("${VPE_LIB}: Not adding all common videophone engine include directories to target")
  else()
    message("${VPE_LIB}: Adding all common videophone engine include directories to target")
#   message("${VPE_LIB} adding these videophone engine include directories to target: ${VPE_COMMON_INCLUDE_DIRECTORIES}")
    target_include_directories(${VPE_LIB}
      PUBLIC
      ${VPE_COMMON_INCLUDE_DIRECTORIES}
    )
  endif()

else()
  # Create an INTERACE library
  message("${VPE_LIB}: has no sources, creating an INTERFACE Library")
#  add_library(${VPE_LIB} STATIC /dev/null)
  add_library(${VPE_LIB} INTERFACE)

  if(${VPE_LIB}_LIBRARIES)
    message("${VPE_LIB}: linking these libraries: ${${VPE_LIB}_LIBRARIES}")
    foreach(lib ${${VPE_LIB}_LIBRARIES})

      #message(STATUS "Adding dependency: ${lib}")
      #add_dependencies(${VPE_LIB}
      #  ${lib}
      #)

      target_link_libraries(${VPE_LIB}
        INTERFACE
        ${lib}
      )

      #target_include_directories(${VPE_LIB}
      #  PUBLIC
      #  $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>
      #)
    endforeach()

  endif()

  if(${VPE_LIB}_DEPENDENCIES)
    message("${VPE_LIB}: adding these dependencies to target: ${${VPE_LIB}_DEPENDENCIES}")

    foreach(lib ${${VPE_LIB}_DEPENDENCIES})
      add_dependencies(${VPE_LIB}
        ${lib}
      )
    endforeach()
  endif()

  # Specify videophone engine include directories for the target
  if(${VPE_LIB}_INCLUDE_DIRECTORIES)
    message("${VPE_LIB}: adding these include directories to target: ${${VPE_LIB}_INCLUDE_DIRECTORIES}")
    target_include_directories(${VPE_LIB}
      INTERFACE
      ${${VPE_LIB}_INCLUDE_DIRECTORIES}
    )
  endif()

  target_include_directories(${VPE_LIB}
    INTERFACE
    .
  )

endif()

# Do the rest for both STATIC and INERFACE libraries
if(${VPE_LIB}_SUBDIRECTORIES)
  message("${VPE_LIB}: adding these subdirectories: ${${VPE_LIB}_SUBDIRECTORIES}")
  foreach(SUBDIR ${${VPE_LIB}_SUBDIRECTORIES})
    add_subdirectory(${SUBDIR})
  endforeach()
endif()

# Specify videophone engine private include directories for the target
if(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES)
  message("${VPE_LIB}: adding these private include directories to target: ${${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES}")
  target_include_directories(${VPE_LIB}
    PRIVATE
    ${${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES}
  )
endif()


