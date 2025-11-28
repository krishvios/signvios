
vpe_platform_deps += declare_dependency(
  link_args : [
    '-lrt',
  ],
)

vpe_platform_includes = include_directories(
  '.',
)

vpe_libs += static_library('stiDeviceController',
  'CstiBSPInterface.cpp',
  'CstiStatusLED.cpp',
  'CstiLightRing.cpp',
  'CstiDisplay.cpp',
  'CstiAudioInput.cpp',
  'CstiAudioOutput.cpp',
  'CstiAudibleRinger.cpp',
  'CstiUserInput.cpp',
  'CstiBufferElement.cpp',
  'CstiFileVideoInput.cpp',

  #'CstiGstreamerBufferElement.cpp',
  #'CstiGStreamerVideoInput.cpp',

  #'CstiFFMpegDecoder.cpp',
  #'CstiFFMpegEncoder.cpp',
  #'CstiFFMpegVideoInput.cpp',
  include_directories : [vpe_includes, vpe_platform_includes],
  cpp_args : ['-DLINUX', '-DCONFIG_STA_SUPPORT'],
)
