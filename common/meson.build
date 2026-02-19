libpanel_common = static_library(
  'panel-common',
  [
    'panel-debug.c',
    'panel-debug.h',
    'panel-utils.c',
    'panel-utils.h',
    'panel-xfconf.c',
    'panel-xfconf.h',
  ],
  sources: [
    libxfce4panel_h,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('libpanel-common'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gtk,
    libxfce4ui,
    libxfce4windowing,
    xfconf,
    wayland_deps,
  ],
  install: false,
)

xfce4_panel_resources_c = gnome.compile_resources(
  'xfce4-panel-resources',
  'resources' / 'xfce4-panel.gresource.xml',
  source_dir: 'resources',
)
