
vpe_libs += static_library('stiDataCommon',
  'CstiAesEncryptionContext.cpp',
  'CstiDataTaskCommonP.cpp',
  'CstiRtpHeaderLength.cpp',
  'CstiRtpSession.cpp',
  'CstiSyncManager.cpp',
  'RTCPFlowControl.cpp',
  'RtpPayloadAddon.cpp',
  'utf8EncodeDecode.cpp',
  include_directories : [vpe_includes],
  dependencies : [acme_dep, radvision_dep],
  cpp_args : '-DstiSILENCE_SUPPRESSION'
)
