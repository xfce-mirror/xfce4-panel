project(
  'xfce4-panel',
  'c',
version : '4.20.0-dev',
  license : 'GPL-2.0-or-later',
  meson_version : '>= 0.56.0',
  default_options : ['c_std=gnu11', 'buildtype=debugoptimized', 'warning_level=2']
)

project_namespace = 'xfce'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
copyright_year = '2025'
libxfce4panel_version_api = '2.0'
xfce4_panel_config_version = '2'

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'cairo': '>= 1.16.0',
  'garcon': '>= 4.17.0',
  'glib': '>= 2.74.0',
  'gtk': '>= 3.24.0',
  'libxfce4ui': '>= 4.21.0',
  'libxfce4util': '>= 4.17.2',
  'libxfce4windowing': '>= 4.20.1',
  'xfconf': '>= 4.13.2',

  'libwnck': '>= 3.0',
  'libx11': '>= 1.6.7',
  'libxext': '>= 1.0.0',

  'gtk-layer-shell': '>= 0.7.0',
  'wayland': '>= 1.20',

  'gir': '>= 1.72.0',
  'dbusmenu': '>= 16.04.0',
}

cairo = dependency('cairo', version: dependency_versions['cairo'])
garcon = dependency('garcon-1', version: dependency_versions['garcon'])
garcon_gtk3 = dependency('garcon-gtk3-1', version: dependency_versions['garcon'])
glib = dependency('glib-2.0', version: dependency_versions['glib'])
gio_unix = dependency('gio-unix-2.0', version: dependency_versions['glib'])
gmodule = dependency('gmodule-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
libxfce4ui = dependency('libxfce4ui-2', version: dependency_versions['libxfce4ui'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['libxfce4util'])
libxfce4windowing = dependency('libxfce4windowing-0', version: dependency_versions['libxfce4windowing'])
libxfce4windowingui = dependency('libxfce4windowingui-0', version: dependency_versions['libxfce4windowing'])
xfconf = dependency('libxfconf-0', version: dependency_versions['xfconf'])

xdt_gen_visibility = find_program('xdt-gen-visibility', required: true)
xdt_csource = find_program('xdt-csource', required: true)

# Feature: 'x11'
x11_deps = []
x11_deps += dependency('gdk-x11-3.0', version: dependency_versions['gtk'], required: get_option('x11'))
x11_deps += dependency('gtk+-x11-3.0', version: dependency_versions['gtk'], required: get_option('x11'))
x11_deps += dependency('libwnck-3.0', version: dependency_versions['libwnck'], required: get_option('x11'))
libwnck = x11_deps[-1]
x11_deps += dependency('x11', version: dependency_versions['libx11'], required: get_option('x11'))
x11_deps += dependency('xext', version: dependency_versions['libxext'], required: get_option('x11'))
x11_deps += dependency('libxfce4windowing-x11-0', version: dependency_versions['libxfce4windowing'], required: get_option('x11'))

# Feature: 'wayland'
wayland_deps = []
wayland_deps += dependency('gdk-wayland-3.0', version: dependency_versions['gtk'], required: get_option('wayland'))
wayland_deps += dependency('wayland-client', version: dependency_versions['wayland'], required: get_option('wayland'))

feature_cflags = []

enable_x11 = not get_option('x11').disabled()
foreach dep : x11_deps
  enable_x11 = enable_x11 and dep.found()
endforeach
if enable_x11
  feature_cflags += '-DENABLE_X11=1'
endif

enable_wayland = not get_option('wayland').disabled()
foreach dep : wayland_deps
  enable_wayland = enable_wayland and dep.found()
endforeach
if enable_wayland
  feature_cflags += '-DENABLE_WAYLAND=1'
  gtk_layer_shell = dependency('gtk-layer-shell-0', version: dependency_versions['gtk-layer-shell'], required: get_option('gtk-layer-shell'))
  if gtk_layer_shell.found()
    feature_cflags += '-DHAVE_GTK_LAYER_SHELL=1'
  endif
else
  gtk_layer_shell = dependency('', required: false)
endif

if not enable_x11 and not enable_wayland
  error('At least one of the X11 and Wayland backends must be enabled')
endif

dbusmenu = dependency('dbusmenu-gtk3-0.4', version: dependency_versions['dbusmenu'], required: get_option('dbusmenu'))
if dbusmenu.found()
  feature_cflags += '-DHAVE_DBUSMENU=1'
endif

gir = dependency('gobject-introspection-1.0', version: dependency_versions['gir'], required: get_option('introspection'))

vapigen = find_program('vapigen', required: get_option('vala'))
if get_option('vala').enabled()
  assert(get_option('introspection'), 'Building Vala bindings requires building GObject introspection data')
endif
generate_vala = not get_option('vala').disabled() and vapigen.found()

gnu_symbol_visibility = 'default'
if get_option('visibility')
  gnu_symbol_visibility = 'hidden'
  feature_cflags += '-DENABLE_SYMBOL_VISIBILITY=1'
endif

helper_path_prefix = get_option('helper-path-prefix')
if helper_path_prefix == ''
  helper_path_prefix = get_option('prefix') / get_option('libdir')
endif

need_libintl = false
has_bind_textdomain_codeset = cc.has_function('bind_textdomain_codeset')
if not has_bind_textdomain_codeset
  libintl = cc.find_library('intl', required: false)
  if libintl.found()
    has_bind_textdomain_codeset = cc.has_function(
      'bind_textdomain_codeset',
      dependencies: [
        libintl, 
      ],
    )
    if has_bind_textdomain_codeset
      need_libintl = true 
    endif
  endif
endif
if has_bind_textdomain_codeset
  feature_cflags += '-DHAVE_BIND_TEXTDOMAIN_CODESET=1'
endif
  
headers = [
  'sys/prctl.h',
  'sys/wait.h',
  'math.h',
]
foreach header : headers
  if cc.check_header(header)
    feature_cflags += '-DHAVE_@0@=1'.format(header.underscorify().to_upper())
  endif
endforeach

libm = cc.find_library('m', required: true)

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DCOPYRIGHT_YEAR="@0@"'.format(copyright_year),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
  '-DLIBXFCE4PANEL_VERSION_API="@0@"'.format(libxfce4panel_version_api),
  '-DXFCE4_PANEL_CONFIG_VERSION=@0@'.format(xfce4_panel_config_version),
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(extra_cflags, language: 'c')
add_project_arguments(feature_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

desktop_files = [
  'panel-desktop-handler.desktop.in',
  'panel-preferences.desktop.in',
]
foreach file : desktop_files
  i18n.merge_file(
    input: file,
    output: file.substring(0, -3),
    po_dir: 'po',
    type: 'desktop',
    install: true,
    install_dir: get_option('prefix') / get_option('datadir') / 'applications',
  )
endforeach

subdir('icons')
subdir('libxfce4panel')
subdir('common')
subdir('docs' / 'reference')
subdir('migrate')
subdir('panel')
subdir('plugins' / 'actions')
subdir('plugins' / 'applicationsmenu')
subdir('plugins' / 'clock')
subdir('plugins' / 'directorymenu')
subdir('plugins' / 'launcher')
subdir('plugins' / 'pager')
subdir('plugins' / 'separator')
subdir('plugins' / 'showdesktop')
subdir('plugins' / 'systray')
subdir('plugins' / 'tasklist')
subdir('plugins' / 'windowmenu')
subdir('po')
subdir('wrapper')
