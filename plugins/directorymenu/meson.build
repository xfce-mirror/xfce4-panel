plugin_sources = [
  'directorymenu.c',
  'directorymenu.h',
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

shared_module(
  'directorymenu',
  plugin_sources,
  sources: [
    xfce4_panel_resources_c,
    libxfce4panel_h,
  ],
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libdirectorymenu'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gio_unix,
    gtk,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    xfconf,
  ],
  link_with: [
    libpanel_common,
    libxfce4panel,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

configure_file(
  input: 'xfce4-popup-directorymenu.sh',
  output: 'xfce4-popup-directorymenu',
  configuration: {
    'bindir': get_option('prefix') / get_option('bindir'),
    'localedir': get_option('prefix') / get_option('localedir'),
  },
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)

i18n.merge_file(
  input: configure_file(
    input: 'directorymenu.desktop.in.in',
    output: 'directorymenu.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'directorymenu.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
