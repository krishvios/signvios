# Set the package name
set(VideophoneEngine_NAME "VideophoneEngine")

# Set the version of the package
set(VideophoneEngine_VERSION "1.0.0")

# Set the include directories
set(VideophoneEngine_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/Distribution")

# Set the library files
set(VideophoneEngine_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/libVideophoneEngine.a")

# Set compile definitions
set(VideophoneEngine_COMPILE_DEFINITIONS "MYLIB_ENABLE_FEATURE1")

# Set compile options
#set(VideophoneEngine_COMPILE_OPTIONS "-Wall;-Wextra")
set(VideophoneEngine_COMPILE_OPTIONS "")

# Handle required components (optional)
if (VideophoneEngine_FIND_COMPONENTS)
  foreach(component ${VideophoneEngine_FIND_COMPONENTS})
    if (component STREQUAL "ComponentA")
      # Set component-specific variables
      set(VideophoneEngine_COMPONENT_A_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include/component_a")
      list(APPEND VideophoneEngine_INCLUDE_DIRS "${VideophoneEngine_COMPONENT_A_INCLUDE_DIRS}")
    elseif(component STREQUAL "ComponentB")
      # Set component-specific variables
       set(VideophoneEngine_COMPONENT_B_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/component_b/libcomponentb.so")
       list(APPEND VideophoneEngine_LIBRARIES "${VideophoneEngine_COMPONENT_B_LIBRARIES}")
    else()
      message(WARNING "VideophoneEngine: Unknown component requested: ${component}")
    endif()
  endforeach()
endif()

# Provide the include directories and libraries to the user
set(VideophoneEngine_INCLUDE_DIRS ${VideophoneEngine_INCLUDE_DIRS} CACHE STRING "Include directories for VideophoneEngine" FORCE)
set(VideophoneEngine_LIBRARIES ${VideophoneEngine_LIBRARIES} CACHE STRING "Libraries for VideophoneEngine" FORCE)
set(VideophoneEngine_COMPILE_DEFINITIONS ${VideophoneEngine_COMPILE_DEFINITIONS} CACHE STRING "Compile definitions for VideophoneEngine" FORCE)
set(VideophoneEngine_COMPILE_OPTIONS ${VideophoneEngine_COMPILE_OPTIONS} CACHE STRING "Compile options for VideophoneEngine" FORCE)

# Create imported targets (modern approach)
if(NOT TARGET VideophoneEngine::VideophoneEngine)
    add_library(VideophoneEngine::VideophoneEngine UNKNOWN IMPORTED)
    set_target_properties(VideophoneEngine::VideophoneEngine PROPERTIES
        IMPORTED_LOCATION ${VideophoneEngine_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES "${VideophoneEngine_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${VideophoneEngine_LIBRARIES}"
        INTERFACE_COMPILE_DEFINITIONS "${VideophoneEngine_COMPILE_DEFINITIONS}"
        INTERFACE_COMPILE_OPTIONS "${VideophoneEngine_COMPILE_OPTIONS}"
    )
endif()

# Provide version information
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_LIST_DIR}/VideophoneEngineConfigVersion.cmake"
    VERSION ${VideophoneEngine_VERSION}
    COMPATIBILITY SameMajorVersion
)

#install the config file and version file
install(FILES
    "${CMAKE_CURRENT_LIST_DIR}/VideophoneEngineConfigVersion.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/VideophoneEngineConfig.cmake"
    DESTINATION "lib/cmake/VideophoneEngine"
)

#install headers
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include/"
    DESTINATION include
)

#install libraries
install(FILES "${CMAKE_CURRENT_LIST_DIR}/lib/libVideophoneEngine.a"
    DESTINATION lib
)
