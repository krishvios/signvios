
vpe_platform_deps += dependency('gstreamer-0.10')
vpe_platform_deps += dependency('gstreamer-app-0.10')
vpe_platform_deps += dependency('gio-2.0')
vpe_platform_deps += dependency('gio-unix-2.0')
vpe_platform_deps += dependency('bluez')

# Add to the platform deps
vpe_platform_deps += declare_dependency(
  link_args : [ '-ldl', '-ludev', '-lrcu_api', '-lbluealsa', '-lboost_system', '-lboost_filesystem'],
)

# HACK: vp2 platform depends on this for generated dbus sources with GLib < 2.51.3
vpe_gdbus_includes = include_directories(
  # Currently the dbus codegen stuff needs the top directory,
  # and it must be a relative path
  # (See meson bug #1387)
  # Feels odd to reach up, but really subprojects are in a static location
  '../../../..',
)

vpe_platform_includes = []
vpe_platform_includes += include_directories(
  '.',
  'network',
  'bluetooth',
  '../common/bluetooth',
)

vpe_includes += vpe_gdbus_includes
vpe_platform_includes += vpe_gdbus_includes

# some "public" headers are now in this directory
vpe_includes += include_directories(
  '.',
)

subdir('network')
subdir('bluetooth')

vpe_libs += static_library('stiDeviceController',
  'CstiBSPInterface.cpp',
  'CstiStatusLED.cpp',
  'CstiLightRing.cpp',
  'CstiAudioInput.cpp',
  'CstiAudioHandler.cpp',
  'CstiAudioOutput.cpp',
  'CstiAudibleRinger.cpp',
  'CstiUserInput.cpp',
  'CstiSLICRinger.cpp',
  'CstiBufferElement.cpp',
  'CstiGstreamerBufferElement.cpp',
  'CstiVideoInput.cpp',
  'CstiGStreamerVideoGraph.cpp',
  'CstiVideoOutput.cpp',
  'CstiAdvancedStatus.cpp',
  'CstiMonitorTask.cpp',
  'CstiISPMonitor.cpp',
  'stiOVUtils.cpp',
  'CLed.cpp',
  'CstiCECControl.cpp',
  include_directories : [vpe_includes, vpe_platform_includes],
  cpp_args : ['-DLINUX', '-DCONFIG_STA_SUPPORT'],
  dependencies : [vpe_platform_deps, ixml_dep]
)
