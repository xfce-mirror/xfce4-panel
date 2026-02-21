xdgconfdir = get_option('prefix') / get_option('sysconfdir') / 'xdg'

configure_file(
  input: 'default.xml.in',
  output: 'default.xml',
  configuration: {
    'XFCE4_PANEL_CONFIG_VERSION': xfce4_panel_config_version,
  },
  install: true,
  install_dir: xdgconfdir / 'xfce4' / 'panel',
)

migrate_sources = [
  'main.c',
  'migrate-config.c',
  'migrate-config.h',
  'migrate-default.c',
  'migrate-default.h',
]

executable(
  'migrate',
  migrate_sources,
  sources: [
    libxfce4panel_h,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-panel-migrate'),
    '-DXDGCONFIGDIR="@0@"'.format(xdgconfdir),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gtk,
    libxfce4util,
    libxfce4ui,
    xfconf,
  ],
  link_with: [
    libxfce4panel,
  ],
  install: true,
  install_dir: helper_path_prefix / 'xfce4' / 'panel',
)
