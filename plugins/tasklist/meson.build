plugin_sources = [
  'tasklist.c',
  'tasklist.h',
  'tasklist-widget.c',
  'tasklist-widget.h',
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

shared_module(
  'tasklist',
  plugin_sources,
  sources: [
    xfce4_panel_resources_c,
    libxfce4panel_h,
  ],
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libtasklist'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
    x11_deps,
    gtk_layer_shell,
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

i18n.merge_file(
  input: configure_file(
    input: 'tasklist.desktop.in.in',
    output: 'tasklist.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'tasklist.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
