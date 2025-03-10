plugin_sources = [
  'launcher.c',
  'launcher.h',
  'launcher-dialog.c',
  'launcher-dialog.h',
]

plugin_sources += custom_target(
  'launcher-dialog_ui.h',
  input: 'launcher-dialog.glade',
  output: 'launcher-dialog_ui.h',
  command: [xdt_csource, '--static', '--strip-comments', '--strip-content', '--name=launcher_dialog_ui', '--output=@OUTPUT@', '@INPUT@'],
)

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

shared_module(
  'launcher',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('liblauncher'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
    garcon,
    garcon_gtk3,
    libxfce4util,
    libxfce4ui,
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
    input: 'launcher.desktop.in.in',
    output: 'launcher.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'launcher.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
