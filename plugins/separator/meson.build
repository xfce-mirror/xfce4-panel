plugin_sources = [
  'separator.c',
  'separator.h',
]

plugin_sources += custom_target(
  'separator-dialog_ui.h',
  input: 'separator-dialog.glade',
  output: 'separator-dialog_ui.h',
  command: [xdt_csource, '--static', '--strip-comments', '--strip-content', '--name=separator_dialog_ui', '--output=@OUTPUT@', '@INPUT@'],
)

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

shared_module(
  'separator',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libseparator'),
  ],
  include_directories: [
    include_directories('..' / '..'),
  ],
  dependencies: [
    gtk,
    cairo,
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
    input: 'separator.desktop.in.in',
    output: 'separator.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel_version_api,
    },
    install: false,
  ),
  output: 'separator.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
