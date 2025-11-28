
# standalone project
project('VideophoneEngine', ['cpp', 'c'],
  version : 'TODO',
  default_options : [
    'cpp_std=c++11',
    'werror=true',
    'warning_level=2', # -Wall
  ],
  meson_version: '>=0.41.0',
)

vpe_libs = []
vpe_deps = []
vpe_platform_deps = []
vpe_compile_args = []

###############################################
# Convert meson options to defines
###############################################

application = get_option('application')
device = get_option('device')
subdevice = get_option('subdevice')

message('vpe application: ' + application)
message('vpe device: ' + device)
message('vpe subdevice: ' + subdevice)
message('Radvision logging output: @0@'.format(get_option('rv_logging_output')))

if application == 'ntouchvp2'
  vpe_compile_args += '-DAPPLICATION=APP_NTOUCH_VP2'
elif application == 'registrarloadtest'
  vpe_compile_args += '-DAPPLICATION=APP_REGISTRAR_LOAD_TEST_TOOL'
else
  error('Must specify application')
endif

if device == 'ntouch_vp2'
  vpe_compile_args += ['-DDEVICE=DEV_NTOUCH_VP2']
elif device == 'x86' or device == 'regloadtest' # TODO: for regloadtest?
  vpe_compile_args += ['-DDEVICE=DEV_X86']
else
  error('Must specify device')
endif

if subdevice == 'mpu'
  vpe_compile_args += ['-DSUBDEVICE=SUBDEV_MPU']
elif device == 'rcu'
  vpe_compile_args += ['-DSUBDEVICE=SUBDEV_RCU']
else
  error('Must specify subdevice')
endif

# Define some settings to ignore warnings, both on a meson level

# It may have been nice if meson had a 0 level, but maybe some warnings shouldn't ever be ignored.
# NOTE: only use these on 3rd party sources

vpe_suppress_warnings_overrides = ['werror=false', 'warning_level=1']

vpe_compile_args += [
  '-include' + meson.current_source_dir() + '/smiConfig.h', # This seems really odd?
  # TODO: Until these warnings can get fixed
  '-Wno-non-virtual-dtor',
  '-Wno-unused-parameter',
  '-Wno-type-limits',
  # VP2 compiler has issue with this warnings
  # also, the x86 compiler is throwing a warning with "= { 0 }" but not "= {}" ??
  '-Wno-missing-field-initializers',
  #'-Woverloaded-virtual', # help catch during refactoring (also, some apps have this enabled)
  # TODO: per platform
  '-D_REENTRANT',
  '-D_GNU_SOURCE',
  '-fexceptions',
  '-fcheck-new',
]

if get_option('rv_logging_output')
  vpe_compile_args += '-DVPE_RV_LOGGING'
endif

# add to current project (but will also get added to the vpe dependency)
add_project_arguments([
  vpe_compile_args,
  ],
  language : 'cpp',
)

subdir('Libraries')

vpe_includes = []
vpe_includes += include_directories(
  '.',
  'BlockListMgr',
  'CallHistoryMgr',
  'ConfMgr',
  'ConfMgr/DataTasks/AudioPlayback',
  'ConfMgr/DataTasks/AudioRecord',
  'ConfMgr/DataTasks/DataCommon',
  'ConfMgr/DataTasks/TextPlayback',
  'ConfMgr/DataTasks/TextRecord',
  'ConfMgr/DataTasks/VideoPlayback',
  'ConfMgr/DataTasks/VideoRecord',
  'ConfMgr/Nat',
  'CoreServices',
  'FileDownload',
  'FilePlay',
  'ImageMgr',
  'Libraries/observer_ptr',
  'MPEG4',
  'MPEG4/Atoms',
  'MPEG4/Toolbox',
  'MsgMgr',
  'Platform',
  'Platform/common/CameraControl',
  'Platform/common/audio',
  'Platform/common/callback',
  'Platform/common/wireless',
  'RegistrationMgr',
  'RingGroupMgr',
  'cci',
  'common',
  'contacts',
  'http',
  'os',
  'pm',
  'update',
  'vrcl',
  'xmlmgr',
)

subdirs = [
  'common',
  'ConfMgr',
  'http',
  'os',
  'Platform', # TODO: dependent on platform?
]

if application != 'registrarloadtest'
  subdirs += [
    'BlockListMgr',
    'CallHistoryMgr',
    'CoreServices',
    'FileDownload',
    'FilePlay',
    'ImageMgr',
    'MPEG4',
    'MsgMgr',
    'RegistrationMgr',
    'RingGroupMgr',
    'cci',
    'contacts',
    'pm',
    'update',
    'vrcl',
    'xmlmgr',
  ]
endif

buildinfo_name = 'vpe'

vpebuildinfo_target = custom_target(
  buildinfo_name + 'buildinfo',
  output : [buildinfo_name + 'buildinfo.cpp', buildinfo_name + 'buildinfo.h'],
  command : [
    meson.current_source_dir() + '/meson/buildinfo/buildinfo.py',
    '--namespace=' + buildinfo_name,
    '--source=@OUTPUT0@',
    '--header=@OUTPUT1@',
    '--scm=git',
    '--scm-directory=' + meson.current_source_dir(),
    '--lang=cpp',
    '--version-date-env=SCI_VERSION',
  ],
  build_always : true,
)

vpebuildinfo_dep = declare_dependency(
  sources: vpebuildinfo_target,
  compile_args : '-DHAVE_VPE_BUILDINFO',
)

foreach dir : subdirs
  subdir(dir)
endforeach

vpe_dep = declare_dependency(
  dependencies : [radvision_dep, acme_dep, ixml_dep, vpe_deps, vpe_platform_deps],
  link_with : vpe_libs,
  include_directories : vpe_includes,
  compile_args : vpe_compile_args,
)

# Do after the vpe_dep is created
if application != 'registrarloadtest'
  subdir('Tests')
endif
