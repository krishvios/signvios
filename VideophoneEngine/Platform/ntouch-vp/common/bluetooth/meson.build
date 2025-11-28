
gnome = import('gnome')

generated_sources = gnome.gdbus_codegen(
  'CstiBluetoothInterface',
  'bluez.xml',
  interface_prefix : 'org.bluez',
  namespace : 'CstiBluetooth',
  object_manager : true,
)

# Compile this separately to allow different compiler flags
vpe_libs += static_library('stiBluetoothInterface',
  generated_sources,
  include_directories : vpe_gdbus_includes,
  override_options : vpe_suppress_warnings_overrides,
  dependencies : [vpe_platform_deps],
)

vpe_libs += static_library('stiBluetooth',
  'CstiBluetooth.cpp',
  'AdapterInterface.cpp',
  'AgentManagerInterface.cpp',
  'DeviceInterface.cpp',
  'GattCharacteristicInterface.cpp',
  'GattDescriptorInterface.cpp',
  'GattManagerInterface.cpp',
  'GattServiceInterface.cpp',
  'BluetoothDefs.cpp',
  'CstiHelios.cpp',
  'NordicDfu.cpp',
  'BluezAlsaClient.cpp',
  'BluetoothAudio.cpp',
  include_directories : [vpe_includes, vpe_platform_includes],
  dependencies : [vpe_platform_deps, ixml_dep],
)
