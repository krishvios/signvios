
gnome = import('gnome')

generated_sources = gnome.gdbus_codegen(
  'CstiNetworkConnmanInterface',
  'connman.xml',
  interface_prefix : 'net.connman',
  namespace : 'CstiNetwork',
  object_manager : true,
)

# Compile this separately to allow different compiler flags
vpe_libs += static_library('stiNetworkConnmanInterface',
  generated_sources,
  include_directories : vpe_gdbus_includes,
  override_options : vpe_suppress_warnings_overrides,
  dependencies : [vpe_platform_deps],
)

vpe_libs += static_library('stiNetwork',
  'CstiNetwork.cpp',
  include_directories : [vpe_includes],
  cpp_args : ['-D_VERSION'],
  dependencies : [vpe_platform_deps],
)
