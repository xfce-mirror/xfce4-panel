plugin_sources = [
  'actions.c',
  'actions.h',
]

plugin_sources += custom_target(
  'actions-dialog_ui.h',
  input: 'actions-dialog.glade',
  output: 'actions-dialog_ui.h',
  command: [xdt_csource, '--static', '--strip-comments', '--strip-content', '--name=actions_dialog_ui', '--output=@OUTPUT@', '@INPUT@'],
)

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

shared_module(
  'actions',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libactions'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
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
    input: 'actions.desktop.in.in',
    output: 'actions.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'actions.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
