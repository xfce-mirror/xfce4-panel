wrapper_sources = [
  'main.c',
  'wrapper-module.c',
  'wrapper-module.h',
  'wrapper-plug.c',
  'wrapper-plug.h',
]

if enable_x11
  wrapper_sources += [
    'wrapper-plug-x11.c',
    'wrapper-plug-x11.h',
  ]
endif

if gtk_layer_shell.found()
  wrapper_sources += [
    'wrapper-plug-wayland.c',
    'wrapper-plug-wayland.h',
  ]
endif

wrapper_sources += gnome.gdbus_codegen(
  'wrapper-external-exported',
  sources: 'wrapper-external-infos.xml',
  interface_prefix: 'org.xfce.Panel.External',
  namespace: 'WrapperExternal',
  install_header: false,
)

executable(
  'wrapper-2.0',
  wrapper_sources,
  sources: [
    libxfce4panel_h,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-panel-wrapper'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gmodule,
    gtk,
    libxfce4util,
    libxfce4windowing,
    gtk_layer_shell,
  ],
  link_with: [
    libpanel_common,
    libxfce4panel,
  ],
  install: true,
  install_dir: helper_path_prefix / 'xfce4' / 'panel',
)
