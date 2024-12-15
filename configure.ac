dnl
dnl Copyright (c) 2004-2024 The Xfce development team
dnl
dnl 2005-2007 Jasper Huijsmans <jasper@xfce.org>
dnl 2006-2008 Nick Schermer <nick@xfce.org>
dnl

dnl ***************************
dnl *** Version information ***
dnl ***************************
m4_define([libxfce4panel_verinfo], [4:0:0]) dnl current:revision:age
m4_define([libxfce4panel_version_api], [2.0])
m4_define([xfce4_panel_config_version], [2])
XDT_VERSION_INIT([4.20.0])

dnl minimum required versions
m4_define([glib_minimum_version], [2.72.0])
m4_define([gtk_minimum_version], [3.24.0])
m4_define([cairo_minimum_version], [1.16])
m4_define([libxfce4util_minimum_version], [4.17.2])
m4_define([libxfce4ui_minimum_version], [4.17.1])
m4_define([libxfce4windowing_minimum_version], [4.19.6])
m4_define([garcon_minimum_version], [4.17.0])
m4_define([exo_minimum_version], [0.11.2])
m4_define([xfconf_minimum_version], [4.13.2])

m4_define([libx11_minimum_version], [1.6.7])
m4_define([libxext_minimum_version], [1.0.0])
m4_define([libwnck_minimum_version], [3.0])

m4_define([wayland_client_minimum_version], [1.20])
m4_define([gtk_layer_shell_minimum_version], [0.7.0])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2004-2024 The Xfce development team. All rights reserved.
  Written for Xfce by Jasper Huijsmans <jasper@xfce.org> and Nick Schermer <nick@xfce.org>.])
AC_INIT([xfce4-panel],[xdt_version],[https://gitlab.xfce.org/xfce/xfce4-panel/-/issues],[xfce4-panel])
AC_PREREQ([2.69])
AC_REVISION([xdt_version_build])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.8 no-dist-gzip dist-bzip2 tar-ustar foreign])
AM_MAINTAINER_MODE()
AC_CONFIG_MACRO_DIRS([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl ******************************
dnl *** Set helper path prefix ***
dnl ******************************
AC_ARG_WITH([helper-path-prefix],
            [AS_HELP_STRING([--with-helper-path-prefix=PATH],
                            [Path prefix under which helper executables will be installed (default: $libdir)])],
            [HELPER_PATH_PREFIX="$withval"],
            [HELPER_PATH_PREFIX="$libdir"])
AC_SUBST([HELPER_PATH_PREFIX])

dnl *******************************
dnl *** Check for UNIX variants ***
dnl *******************************
AC_USE_SYSTEM_EXTENSIONS

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
AM_PROG_CC_C_O()
LT_PATH_LD([])
AC_PROG_INSTALL()
AC_CHECK_PROGS([PERL], [perl5 perl])
m4_ifdef([AC_PROG_SED], [AC_PROG_SED], [AC_CHECK_PROG(SED, sed, sed)])

dnl **************************
dnl *** Initialize libtool ***
dnl **************************
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl **************************************
dnl *** Substitute version information ***
dnl **************************************
LIBXFCE4PANEL_VERINFO=libxfce4panel_verinfo()
AC_SUBST([LIBXFCE4PANEL_VERINFO])

dnl **************************
dnl *** Substitute version ***
dnl **************************
LIBXFCE4PANEL_VERSION=xdt_version()
LIBXFCE4PANEL_VERSION_API=libxfce4panel_version_api()
LIBXFCE4PANEL_VERSION_MAJOR=xdt_version_major()
LIBXFCE4PANEL_VERSION_MINOR=xdt_version_minor()
LIBXFCE4PANEL_VERSION_MICRO=xdt_version_micro()
AC_DEFINE([LIBXFCE4PANEL_VERSION_API], ["libxfce4panel_version_api()"], [libxfce4panel api version])
AC_SUBST([LIBXFCE4PANEL_VERSION])
AC_SUBST([LIBXFCE4PANEL_VERSION_API])
AC_SUBST([LIBXFCE4PANEL_VERSION_MAJOR])
AC_SUBST([LIBXFCE4PANEL_VERSION_MINOR])
AC_SUBST([LIBXFCE4PANEL_VERSION_MICRO])

dnl *********************************
dnl *** Substitute config version ***
dnl *********************************
XFCE4_PANEL_CONFIG_VERSION=xfce4_panel_config_version()
AC_DEFINE([XFCE4_PANEL_CONFIG_VERSION], [xfce4_panel_config_version()], [config migration version])
AC_SUBST([XFCE4_PANEL_CONFIG_VERSION])

dnl **********************************
dnl *** Check for standard headers ***
dnl **********************************
AC_CHECK_HEADERS([stdlib.h unistd.h locale.h stdio.h errno.h time.h string.h \
                  math.h sys/types.h sys/wait.h memory.h signal.h sys/prctl.h \
                  libintl.h])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])
AC_SEARCH_LIBS([bind_textdomain_codeset], [intl],
  [AC_DEFINE([HAVE_BIND_TEXTDOMAIN_CODESET], [1], [Define to 1 if you have the 'bind_textdomain_codeset' function.])],
  [])


dnl ***********************************
dnl *** Check for gdbus-codegen ***
dnl ***********************************
XDT_CHECK_PACKAGE_BINARY([GDBUS_CODEGEN], [gio-2.0], [gdbus_codegen], [gdbus-codegen])

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GIO_UNIX], [gio-unix-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GMODULE], [gmodule-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [gtk_minimum_version])
XDT_CHECK_PACKAGE([CAIRO], [cairo], [cairo_minimum_version])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [libxfce4util_minimum_version])
XDT_CHECK_PACKAGE([GARCON], [garcon-1], [garcon_minimum_version])
XDT_CHECK_PACKAGE([GARCON_GTK3], [garcon-gtk3-1], [garcon_minimum_version])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [libxfce4util_minimum_version])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [xfconf_minimum_version])
XDT_CHECK_PACKAGE([EXO], [exo-2], [exo_minimum_version])
XDT_CHECK_PACKAGE([LIBXFCE4WINDOWING], [libxfce4windowing-0], [libxfce4windowing_minimum_version])
XDT_CHECK_PACKAGE([LIBXFCE4WINDOWINGUI], [libxfce4windowingui-0], [libxfce4windowing_minimum_version])

dnl **********************************************
dnl *** Optional X11/Wayland-specific packages ***
dnl **********************************************
XDT_CHECK_OPTIONAL_FEATURE([X11],
                           [x11],
                           [
                             XDT_FEATURE_DEPENDENCY([LIBX11], [x11], [libx11_minimum_version])
                             XDT_FEATURE_DEPENDENCY([LIBXEXT], [xext], [libxext_minimum_version])
                             XDT_FEATURE_DEPENDENCY([GDK_X11], [gdk-x11-3.0], [gtk_minimum_version])
                             XDT_FEATURE_DEPENDENCY([GTK_X11], [gtk+-x11-3.0], [gtk_minimum_version])
                             XDT_FEATURE_DEPENDENCY([LIBWNCK], [libwnck-3.0], [libwnck_minimum_version])
                             XDT_FEATURE_DEPENDENCY([LIBXFCE4WINDOWING_X11], [libxfce4windowing-x11-0], [libxfce4windowing_minimum_version])
                           ],
                           [the X11 windowing system])
XDT_CHECK_OPTIONAL_FEATURE([WAYLAND],
                           [wayland],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_WAYLAND], [gdk-wayland-3.0], [gtk_minimum_version])
                             XDT_FEATURE_DEPENDENCY([WAYLAND_CLIENT], [wayland-client], [wayland_client_minimum_version])
                           ],
                           [the Wayland windowing system])
if test x"$ENABLE_X11" != x"yes" -a x"$ENABLE_WAYLAND" != x"yes"; then
  AC_MSG_ERROR([Either both X11 and Wayland support was disabled, or required dependencies are missing. One of the two must be enabled.])
fi
if test x"$ENABLE_WAYLAND" != x"yes"; then
  dnl Do not check for optional Wayland packages if Wayland is disabled
  enable_gtk_layer_shell=no
fi
XDT_CHECK_OPTIONAL_PACKAGE([GTK_LAYER_SHELL], [gtk-layer-shell-0], [gtk_layer_shell_minimum_version], [gtk-layer-shell], [GTK Layer Shell support])

dnl *********************************************
dnl *** Optional DBUSMENU for StatusNotifiers ***
dnl *********************************************
XDT_CHECK_OPTIONAL_PACKAGE([DBUSMENU], [dbusmenu-gtk3-0.4],
                           [16.04.0], [dbusmenu-gtk3], [dbusmenu features])

dnl ***************************************
dnl *** Check for gobject-introspection ***
dnl ***************************************
GOBJECT_INTROSPECTION_CHECK([1.72.0])

dnl *************************
dnl *** Check for vapigen ***
dnl *************************
VAPIGEN_CHECK()

dnl ***********************************************************
dnl *** Check for gtk-doc and compensate for broken feature ***
dnl ***********************************************************
GTK_DOC_CHECK([1.9])
XDT_SUPPORTED_FLAGS([GTK_DOC_EXTRA_CFLAGS], [-Wno-sign-compare])
AC_SUBST([GTK_DOC_EXTRA_CFLAGS])

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl **************************************
dnl *** Check for linker optimizations ***
dnl **************************************
XDT_FEATURE_LINKER_OPTS()

dnl ****************************************
dnl *** Check for ELF visibility support ***
dnl ****************************************
AC_ARG_ENABLE([visibility], AS_HELP_STRING([--disable-visibility], [Do not use ELF visibility attributes]), [], [enable_visibility=yes])
have_gnuc_visibility=no
if test x"$enable_visibility" != x"no"; then
  dnl Check whether the compiler supports the visibility attribute
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Wall -Werror"
  AC_MSG_CHECKING([whether $CC supports the GNUC visibility attribute])
  AC_COMPILE_IFELSE([AC_LANG_SOURCE(
  [
    void test_default (void);
    void test_hidden (void);

    void __attribute__ ((visibility("default"))) test_default (void) {}
    void __attribute__ ((visibility("hidden"))) test_hidden (void) {}

    int main (int argc, char **argv) { test_default (); test_hidden (); return 0; }
  ])],
  [
    have_gnuc_visibility=yes
    AC_MSG_RESULT([yes])
  ],
  [
    AC_MSG_RESULT([no])
  ])
  CFLAGS="$save_CFLAGS"
fi
if test x"$have_gnuc_visibility" = x"yes"; then
  CPPFLAGS="$CPPFLAGS -DHAVE_GNUC_VISIBILITY"
fi
AM_CONDITIONAL([HAVE_GNUC_VISIBILITY], [test x"$have_gnuc_visibility" = x"yes"])

dnl *********************************
dnl *** Substitute platform flags ***
dnl *********************************
AC_MSG_CHECKING([PLATFORM_CPPFLAGS])
AC_MSG_RESULT([$PLATFORM_CPPFLAGS])
AC_SUBST([PLATFORM_CPPFLAGS])
AC_MSG_CHECKING([PLATFORM_CFLAGS])
AC_MSG_RESULT([$PLATFORM_CFLAGS])
AC_SUBST([PLATFORM_CFLAGS])
AC_MSG_CHECKING([PLATFORM_LDFLAGS])
AC_MSG_RESULT([$PLATFORM_LDFLAGS])
AC_SUBST([PLATFORM_LDFLAGS])

AC_CONFIG_FILES([
Makefile
common/Makefile
docs/Makefile
docs/reference/Makefile
icons/Makefile
icons/16x16/Makefile
icons/24x24/Makefile
icons/32x32/Makefile
icons/48x48/Makefile
icons/64x64/Makefile
icons/96x96/Makefile
icons/128x128/Makefile
icons/scalable/Makefile
libxfce4panel/Makefile
libxfce4panel/libxfce4panel-2.0.pc
libxfce4panel/libxfce4panel-config.h
migrate/Makefile
migrate/default.xml
panel/Makefile
wrapper/Makefile
plugins/Makefile
plugins/actions/Makefile
plugins/actions/actions.desktop.in
plugins/applicationsmenu/Makefile
plugins/applicationsmenu/applicationsmenu.desktop.in
plugins/clock/Makefile
plugins/clock/clock.desktop.in
plugins/directorymenu/Makefile
plugins/directorymenu/directorymenu.desktop.in
plugins/launcher/Makefile
plugins/launcher/launcher.desktop.in
plugins/pager/Makefile
plugins/pager/pager.desktop.in
plugins/separator/Makefile
plugins/separator/separator.desktop.in
plugins/showdesktop/Makefile
plugins/showdesktop/showdesktop.desktop.in
plugins/systray/Makefile
plugins/systray/systray.desktop.in
plugins/tasklist/Makefile
plugins/tasklist/tasklist.desktop.in
plugins/windowmenu/Makefile
plugins/windowmenu/windowmenu.desktop.in
po/Makefile.in
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "* Debug support:                 $enable_debug"
echo "* GNU Visibility support:        $have_gnuc_visibility"
echo "* GObject Introspection support: $enable_introspection"
echo "* Documentation support:         $enable_gtk_doc"
echo "* D-Bus menu support:            ${DBUSMENU_FOUND:-no}"
echo "* X11 support:                   ${ENABLE_X11:-no}"
echo "* Wayland support:               ${ENABLE_WAYLAND:-no}"
if test x"$ENABLE_WAYLAND" = x"yes"; then
echo "  * GTK Layer Shell support:     ${GTK_LAYER_SHELL_FOUND:-no}"
fi
echo
