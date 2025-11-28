
gnome = import('gnome')

vpe_libs += static_library('stiDbus',
  include_directories : [vpe_includes, vpe_platform_includes],
  DbusInterfaceBase.cpp
  DbusObject.cpp
  PropertiesInterface.cpp
  dependencies : [vpe_platform_deps, ixml_dep],
)
