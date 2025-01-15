sizes = [16, 24, 32, 48, 64, 96, 128]
plugins = [
  'actions',
  'applicationsmenu',
  'clock',
  'directorymenu',
  'launcher',
  'pager',
  'separator',
  'showdesktop',
  'systray',
  'tasklist',
  'windowmenu',
]

foreach size : sizes
  install_data(
    '@0@x@0@'.format(size) / 'org.xfce.panel.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
  foreach plugin : plugins
    install_data(
      '@0@x@0@'.format(size) / 'org.xfce.panel.@0@.png'.format(plugin),
      install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
    )
  endforeach
endforeach

install_data(
  'scalable' / 'org.xfce.panel.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)

foreach plugin : plugins
  install_data(
    'scalable' / 'org.xfce.panel.@0@.svg'.format(plugin),
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
  )
endforeach
