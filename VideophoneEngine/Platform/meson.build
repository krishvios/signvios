
subdir('common')

if device == 'x86'
  subdir('x86')
elif device == 'ntouch_vp2'
  subdir('ntouch-vp2')
elif device == 'regloadtest'
  subdir('RegLoadTest')
else
  error('Unsupported device: ' + device)
endif
# TODO: handle rest of the platforms
