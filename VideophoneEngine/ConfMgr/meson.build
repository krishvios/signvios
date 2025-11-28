
conf_mgr_sources = [
  'CstiCall.cpp',
  'CstiCallInfo.cpp',
  'CstiCallStorage.cpp',
  'CstiConferenceManager.cpp',
  'CstiProtocolCall.cpp',
  'CstiProtocolManager.cpp',
  'stiPayloadTypes.cpp',
  'stiSystemInfo.cpp',
  'CstiRoutingAddress.cpp',
  'stiSipTools.cpp',
  'CstiSipRegistration.cpp',
  'CstiSipManager.cpp',
  'CstiSipCall.cpp',
  'CstiSipCallLeg.cpp',
  'CstiSipResolver.cpp',
  'stiSipStatistics.cpp',
  'CstiSdp.cpp',
]

if application != 'registrarloadtest'
  conf_mgr_sources += [
    'CstiTunnelManager.cpp',
  ]
endif

vpe_libs += static_library('stiConfMgr',
  conf_mgr_sources,
  include_directories : [vpe_includes],
  dependencies : [radvision_dep, acme_dep, ixml_dep]
)

subdir('DataTasks')
subdir('Nat')
