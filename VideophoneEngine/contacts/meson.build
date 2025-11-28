
vpe_deps += declare_dependency(
  link_args : [
    '-lldap',
    '-lldap_r',
    '-llber',
  ],
)

vpe_libs += static_library('CstiContactManager',
  'ContactManager.cpp',
  'FavoriteList.cpp',
  'ContactListItem.cpp',
  'CstiLDAPDirectoryContactManager.cpp',
  'LDAPDirectoryContactListItem.cpp',
  include_directories : [vpe_includes],
  cpp_args : '-DLDAP_DEPRECATED=1',
  dependencies : ixml_dep
)
