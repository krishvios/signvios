
# Try finding this via pkg-config
jpeg_dependency = dependency('libjpeg',
  static : true,
  required : false,
)

if jpeg_dependency.found()
  vpe_deps += jpeg_dependency
else
  message('Unable to locate jpeg via pkg-config (not an error), linking manually')
  vpe_deps += declare_dependency(
    link_args : [ '-ljpeg'],
    )
endif

vpe_libs += static_library('CstiImageManager',
  'CstiImageDownloadService.cpp',
  'CstiImageManager.cpp',
  'CstiNvmImageCache.cpp',
  include_directories : [vpe_includes],
  dependencies : [ixml_dep, jpeg_dependency],
)
