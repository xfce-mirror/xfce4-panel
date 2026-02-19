panel_sources = [
  'main.c',
  'panel-application.c',
  'panel-application.h',
  'panel-base-window.c',
  'panel-base-window.h',
  'panel-dbus-service.c',
  'panel-dbus-service.h',
  'panel-dbus-client.c',
  'panel-dbus-client.h',
  'panel-dialogs.c',
  'panel-dialogs.h',
  'panel-item-dialog.c',
  'panel-item-dialog.h',
  'panel-itembar.c',
  'panel-itembar.h',
  'panel-module.c',
  'panel-module.h',
  'panel-module-factory.c',
  'panel-module-factory.h',
  'panel-plugin-external.c',
  'panel-plugin-external.h',
  'panel-plugin-external-wrapper.c',
  'panel-plugin-external-wrapper.h',
  'panel-preferences-dialog.c',
  'panel-preferences-dialog.h',
  'panel-tic-tac-toe.c',
  'panel-tic-tac-toe.h',
  'panel-window.c',
  'panel-window.h',
]

if enable_x11
  panel_sources += [
    'panel-plugin-external-wrapper-x11.c',
    'panel-plugin-external-wrapper-x11.h',
  ]
endif

if gtk_layer_shell.found()
  panel_sources += [
    'panel-plugin-external-wrapper-wayland.c',
    'panel-plugin-external-wrapper-wayland.h',
  ]
endif

panel_sources += gnome.genmarshal(
  'panel-marshal',
  sources: 'panel-marshal.list',
  prefix: '_panel_marshal',
  internal: true,
  install_header: false,
)

panel_sources += gnome.gdbus_codegen(
  'panel-gdbus-exported-service',
  sources: 'panel-dbus-service-infos.xml',
  interface_prefix: 'org.xfce.Panel',
  namespace: 'XfcePanel',
  install_header: false,
)
panel_sources += gnome.gdbus_codegen(
  'panel-plugin-external-wrapper-exported',
  sources: 'panel-plugin-external-wrapper-infos.xml',
  interface_prefix: 'org.xfce.Panel.Wrapper',
  namespace: 'XfcePanelPluginWrapper',
  install_header: false,
)

executable(
  'xfce4-panel',
  panel_sources,
  sources: [
    xfce_revision_h,
    xfce4_panel_resources_c,
    libxfce4panel_h,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-panel'),
    '-DHELPERDIR="@0@"'.format(helper_path_prefix / 'xfce4' / 'panel'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gmodule,
    gtk,
    libxfce4util,
    libxfce4ui,
    libxfce4windowing,
    xfconf,
    x11_deps,
    wayland_deps,
    gtk_layer_shell,
    libm,
  ],
  link_with: [
    libpanel_common,
    libxfce4panel,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)
