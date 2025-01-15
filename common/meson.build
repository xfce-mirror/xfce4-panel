libpanel_common = static_library(
  'libpanel-common',
  [
    'panel-debug.c',
    'panel-debug.h',
    'panel-utils.c',
    'panel-utils.h',
    'panel-xfconf.c',
    'panel-xfconf.h',
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
