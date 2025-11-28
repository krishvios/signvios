# Define the library name
set(VPE_LIB stiPlatform-ntouch-vp)

if("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP2")
  pkg_check_modules(PKG_CFG_GSTREAMER REQUIRED gstreamer-0.10)
else()
  pkg_check_modules(PKG_CFG_GSTREAMER REQUIRED gstreamer-1.0)
  pkg_check_modules(PKG_CFG_GSTREAMER_APP REQUIRED gstreamer-app-1.0)
  pkg_check_modules(PKG_CFG_GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
endif()
include_directories(${PKG_CFG_GSTREAMER_INCLUDE_DIRS})
add_compile_options(${PKG_CFG_GSTREAMER_CFLAGS})
include_directories(${PKG_CFG_GSTREAMER_APP_INCLUDE_DIRS})
add_compile_options(${PKG_CFG_GSTREAMER_APP_CFLAGS})
include_directories(${PKG_CFG_GSTREAMER_VIDEO_INCLUDE_DIRS})
add_compile_options(${PKG_CFG_GSTREAMER_VIDEO_CFLAGS})

pkg_check_modules(PKG_CFG_GLIB REQUIRED glib-2.0)
include_directories(${PKG_CFG_GLIB_INCLUDE_DIRS})
add_compile_options(${PKG_CFG_GLIB_CFLAGS})

pkg_check_modules(PKG_CFG_GTHREAD REQUIRED gthread-2.0)
include_directories(${PKG_CFG_GTHREAD_INCLUDE_DIRS})
add_compile_options(${PKG_CFG_GTHREAD_CFLAGS})

pkg_check_modules(PKG_CFG_GIO REQUIRED gio-2.0)
include_directories(${PKG_CFG_GIO_INCLUDE_DIRS})
add_compile_options(${PKG_CFG_GIO_CFLAGS})

pkg_check_modules(PKG_CFG_GIO_UNIX REQUIRED gio-unix-2.0)
include_directories(${PKG_CFG_GIO_UNIX_INCLUDE_DIRS})
add_compile_options(${PKG_CFG_GIO_UNIX_CFLAGS})

pkg_check_modules(PKG_CFG_DBUS_GLIB REQUIRED dbus-glib-1)
include_directories(${PKG_CFG_DBUS_GLIB_INCLUDE_DIRS})
add_compile_options(${PKG_CFG_DBUS_GLIB_CFLAGS})

set(${VPE_LIB}_LIBRARIES
  stiPlatform-ntouch-vp-common
)

set(${VPE_LIB}_SUBDIRECTORIES
  common
)

if("${LUMINA_DEVICE}" STREQUAL "DEV_X86")
  list(APPEND ${VPE_LIB}_LIBRARIES
    stiPlatform-ntouch-vp-x86
  )
  list(APPEND ${VPE_LIB}_SUBDIRECTORIES
    x86
  )
#elseif("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP2")
#  list(APPEND ${VPE_LIB}_LIBRARIES
#    stiPlatform-ntouch-vp-vp2
#  )
#  list(APPEND ${VPE_LIB}_SUBDIRECTORIES
#    vp2
#  )
elseif("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP3")
  list(APPEND ${VPE_LIB}_LIBRARIES
    stiPlatform-ntouch-vp-vp3
  )
  list(APPEND ${VPE_LIB}_SUBDIRECTORIES
    vp3
  )
elseif("${LUMINA_DEVICE}" STREQUAL "DEV_NTOUCH_VP4")
  list(APPEND ${VPE_LIB}_LIBRARIES
    stiPlatform-ntouch-vp-vp4
  )
  list(APPEND ${VPE_LIB}_SUBDIRECTORIES
    vp4
  )
endif()

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
