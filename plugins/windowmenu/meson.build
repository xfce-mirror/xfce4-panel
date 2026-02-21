plugin_sources = [
  'windowmenu.c',
  'windowmenu.h',
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

shared_module(
  'windowmenu',
  plugin_sources,
  sources: [
    xfce4_panel_resources_c,
    libxfce4panel_h,
  ],
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libwindowmenu'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    libxfce4windowingui,
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
  input: 'xfce4-popup-windowmenu.sh',
  output: 'xfce4-popup-windowmenu',
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
    input: 'windowmenu.desktop.in.in',
    output: 'windowmenu.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'windowmenu.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
