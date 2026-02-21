libpanel_libname = 'libxfce4panel'
libpanel_pkgname = '@0@-@1@'.format(libpanel_libname, libxfce4panel_version_api)

libpanel_install_subdir = 'xfce4' / libpanel_pkgname / libpanel_libname
libpanel_install_dir = get_option('prefix') / get_option('includedir') / libpanel_install_subdir

libpanel_headers = [
  'libxfce4panel-enums.h',
  'xfce-arrow-button.h',
  'xfce-panel-convenience.h',
  'xfce-panel-image.h',
  'xfce-panel-macros.h',
  'xfce-panel-plugin.h',
]

# do not add this header to GI sources, it creates unsolvable conflicts with other
# libs whose identifier prefix is Xfce, e.g. libxfce4ui and libxfce4util
libpanel_config_h = configure_file(
  configuration: configuration_data({
    'LIBXFCE4PANEL_VERSION': meson.project_version(),
    'LIBXFCE4PANEL_VERSION_MAJOR': version_parts[0],
    'LIBXFCE4PANEL_VERSION_MINOR': version_parts[1],
    'LIBXFCE4PANEL_VERSION_MICRO': version_parts[2],
  }),
  input: 'libxfce4panel-config.h.in',
  output: 'libxfce4panel-config.h',
  install: true,
  install_dir: libpanel_install_dir,
)

libpanel_sources_gir = [
  'libxfce4panel-config.c',
  'xfce-arrow-button.c',
  'xfce-panel-convenience.c',
  'xfce-panel-image.c',
  'xfce-panel-plugin.c',
]

libpanel_sources = libpanel_sources_gir + [
  'xfce-panel-plugin-provider.c',
  'xfce-panel-plugin-provider.h',
]

libpanel_sources += gnome.genmarshal(
  'libxfce4panel-marshal',
  sources: 'libxfce4panel-marshal.list',
  prefix: '_libxfce4panel_marshal',
  internal: true,
  install_header: false,
)

libpanel_sources += configure_file(
  input: 'libxfce4panel.symbols',
  output: 'libxfce4panel-visibility.h',
  command: [xdt_gen_visibility, '--kind=header', '@INPUT@', '@OUTPUT@'],
)
libpanel_sources += configure_file(
  input: 'libxfce4panel.symbols',
  output: 'libxfce4panel-visibility.c',
  command: [xdt_gen_visibility, '--kind=source', '@INPUT@', '@OUTPUT@'],
)

libpanel_enums = []
libpanel_enums += gnome.mkenums(
  'libxfce4panel-enum-types.h',
  install_header: true,
  install_dir: libpanel_install_dir,
  sources: libpanel_headers,
  fhead: '#if !defined(_LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H) && !defined(LIBXFCE4PANEL_COMPILATION)\n#error "Only <libxfce4panel/libxfce4panel.h> can be included directly, this file may disappear or change contents"\n#endif\n\n#ifndef __LIBXFCE4PANEL_ENUM_TYPES_H__\n#define __LIBXFCE4PANEL_ENUM_TYPES_H__\n#include <glib-object.h>\nG_BEGIN_DECLS\n',
  fprod: '/* enumerations from "@filename@" */\n',
  vhead: 'GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define XFCE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n',
  ftail: 'G_END_DECLS\n\n#endif /* !__LIBXFCE4PANEL_ENUM_TYPES_H__ */',
)
libpanel_enums += gnome.mkenums(
  'libxfce4panel-enum-types.c',
  install_header: false,
  sources: libpanel_headers,
  fhead: '#include "libxfce4panel-enums.h"\n#include "libxfce4panel-enum-types.h"\n\n#include "libxfce4panel-visibility.h"',
  fprod: '\n/* enumerations from "@filename@" */',
  vhead: 'GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {',
  vprod: '\t{ @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
  vtail: '\t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static ("@EnumName@", values);\n  }\n\treturn type;\n}\n',
  ftail: '\n#define __LIBXFCE4PANEL_ENUM_TYPES_C__\n#include "libxfce4panel-visibility.c"\n',
)

libxfce4panel_h = custom_target(
  'libxfce4panel.h',
  input: 'libxfce4panel.h.in',
  output: 'libxfce4panel.h',
  command: ['cp', '@INPUT@', '@OUTPUT@'],
  depends: [
    libpanel_enums,
  ],
  depend_files: [
    libpanel_config_h,
  ],
)
libpanel_sources += libxfce4panel_h

libpanel_deps = [
  glib,
  gmodule,
  gtk,
  libxfce4ui,
  libxfce4util,
  libm,
  gtk_layer_shell,
]
if need_libintl
  libpanel_deps += libintl
endif

libpanel_so_version = '4'
libpanel_version = '@0@.0.0'.format(libpanel_so_version)

libxfce4panel = library(
  libpanel_pkgname.substring(3),
  sources: libpanel_sources + libpanel_enums,
  version: libpanel_version,
  soversion: libpanel_so_version,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format(libpanel_libname),
    '-DLIBXFCE4PANEL_COMPILATION=1',
  ],
  include_directories: [
    include_directories('..'),
  ],
  gnu_symbol_visibility: gnu_symbol_visibility,
  dependencies: libpanel_deps,
  install: true,
)

install_headers(
  libpanel_headers,
  subdir: libpanel_install_subdir,
)

pkgconfig.generate(
  libraries: [
    libxfce4panel,
  ],
  version: meson.project_version(),
  name: libpanel_libname,
  filebase: libpanel_pkgname,
  description: 'Library for the Xfce Panel',
  subdirs: [
    'xfce4' / libpanel_pkgname,
  ],
  variables: [
    'datarootdir=@0@'.format('${prefix}' / get_option('datadir')),
    'localedir=@0@'.format('${datarootdir}' / 'locale'),
    'api=@0@'.format(libxfce4panel_version_api),
  ],
  install_dir: get_option('prefix') / get_option('libdir') / 'pkgconfig',
)

if get_option('introspection')
  libxfce4panel_gir = gnome.generate_gir(
    libxfce4panel,
    sources: libpanel_sources_gir + libpanel_headers + libpanel_enums,
    namespace: 'Libxfce4panel',
    nsversion: libxfce4panel_version_api,
    identifier_prefix: 'Xfce',
    symbol_prefix: [
      'xfce',
    ],
    export_packages: libpanel_pkgname,
    includes: [
      'GLib-2.0',
      'GObject-2.0',
      'Gio-2.0',
      'Gtk-3.0',
      'Libxfce4util-1.0',
    ],
    header: 'libxfce4panel' / 'libxfce4panel.h',
    install: true,
    extra_args: [
      '-DLIBXFCE4PANEL_COMPILATION=1',
    ],
  )

  if generate_vala
    gnome.generate_vapi(
      libpanel_pkgname,
      install: true,
      packages: [
        'glib-2.0',
        'gobject-2.0',
        'gio-2.0',
        'gtk+-3.0',
        'libxfce4util-1.0',
      ],
      sources: libxfce4panel_gir[0],
    )
  endif
endif

libxfce4panel_dep = declare_dependency(
  dependencies: libpanel_deps,
  link_with: libxfce4panel,
)
