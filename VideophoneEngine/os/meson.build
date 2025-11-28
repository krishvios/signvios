
vpe_libs += static_library('os',
  'stiOSNet.c',
  'stiOSMsg.c',
  'stiOS.cpp',
  'stiOSLinux.cpp',
  'stiOSLinuxMsg.cpp',
  'stiOSLinuxMutex.cpp',
  'stiOSLinuxTask.cpp',
  'stiOSLinuxWd.cpp',
  'stiOSLinuxPipe.cpp',
  'stiOSLinuxCond.cpp',
  'stiOSLinuxSignal.cpp',
  'stiOSLinuxNet.cpp', # TODO: pick Mac?
  include_directories : [vpe_includes],
  cpp_args : '-fno-strict-aliasing'
)

# add thread dependency
vpe_deps += dependency('Threads')
