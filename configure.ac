dnl configure.ac
dnl
dnl xfce4 - panel for the Xfce Desktop environment
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl

AC_INIT([panel/main.c])

AM_INIT_AUTOMAKE([xfce4-panel], [4.1.7])

AM_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

dnl disable static libs
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC
AC_CHECK_HEADERS([signal.h stddef.h sys/wait.h time.h netinet/in.h])
AC_CHECK_FUNCS([sigaction])

dnl Check for i18n support
BM_I18N([xfce4-panel], [ar az bg ca cs de es es_MX eu fa_IR fr fi hi hu it ja lt ms
	 nl ko no pl pt_BR pt_PT ru sv ta tr vi zh_CN zh_TW])

dnl Check for X11 installed
BM_LIBX11_REQUIRE

dnl Check for required packages
BM_DEPEND([GTHREAD], [gthread-2.0], [2.0.6])
BM_DEPEND([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.0.0])
BM_DEPEND([LIBXFCE4UTIL], [libxfcegui4-1.0], [4.1.13])
BM_DEPEND([LIBXFCEGUI4], [libxfcegui4-1.0], [4.1.19])
BM_DEPEND([LIBXML], [libxml-2.0], [2.4.0])

dnl Check for dbh installed for completion combo
dnl disable for now, doesn't work for new install, only after
dnl  combo has been used in xffm or xfrun.
dnl BM_DEPEND_CHECK([LIBDBH], [dbh-1.0], [1.0],
dnl  [dbh], [DBH library], [yes])
LIBDBH_CFLAGS=
LIBDBH_LIBS=
AC_SUBST([LIBDBH_CFLAGS])
AC_SUBST([LIBDBH_LIBS])

dnl Configure the mcs plugin
XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [4.0.0])

dnl Check for optional packages
BM_DEPEND_CHECK([LIBSTARTUP_NOTIFICATION], [libstartup-notification-1.0],
  [0.5], [startup-notification], [startup notification library], [yes])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
xfce4.spec
Makefile
doc/Makefile
doc/C/Makefile
doc/C/images/Makefile
doc/fr/Makefile
doc/fr/images/Makefile
doc/it/Makefile
doc/it/images/Makefile
po/Makefile.in
icons/Makefile
panel/Makefile
panel/xfce4-panel-1.0.pc
plugins/Makefile
plugins/mailcheck/Makefile
plugins/clock/Makefile
plugins/switcher/Makefile
plugins/systembuttons/Makefile
plugins/pager/Makefile
plugins/sample-plugin/Makefile
plugins/separator/Makefile
settings/Makefile
themes/Makefile
])

dnl themes/Crystal/Makefile
dnl themes/Crystal/48x48/Makefile
dnl themes/Crystal/48x48/emblems/Makefile
dnl themes/Crystal/22x22/Makefile
dnl themes/Crystal/22x22/emblems/Makefile
dnl themes/Curve/Makefile
dnl themes/Curve/48x48/Makefile
dnl themes/Curve/48x48/emblems/Makefile
dnl themes/Curve/22x22/Makefile
dnl themes/Curve/22x22/emblems/Makefile
dnl themes/FreeIcons/Makefile
dnl themes/FreeIcons/48x48/Makefile
dnl themes/FreeIcons/48x48/emblems/Makefile
dnl themes/FreeIcons/24x24/Makefile
dnl themes/FreeIcons/24x24/emblems/Makefile
dnl themes/FreeIcons/22x22/Makefile
dnl themes/FreeIcons/22x22/emblems/Makefile
dnl themes/Gnome/Makefile
dnl themes/Gnome/48x48/Makefile
dnl themes/Gnome/48x48/emblems/Makefile
dnl themes/Gnome/50x50/Makefile
dnl themes/Gnome/50x50/emblems/Makefile
dnl themes/Gnome/24x24/Makefile
dnl themes/Gnome/24x24/emblems/Makefile
dnl themes/Noia/Makefile
dnl themes/Noia/48x48/Makefile
dnl themes/Noia/48x48/emblems/Makefile
dnl themes/Noia/22x22/Makefile
dnl themes/Noia/22x22/emblems/Makefile
dnl themes/XFce/Makefile
dnl themes/XFce/48x48/Makefile
dnl themes/XFce/48x48/emblems/Makefile
dnl themes/XFce/19x19/Makefile
dnl themes/XFce/19x19/emblems/Makefile
dnl themes/XFce/16x16/Makefile
dnl themes/XFce/16x16/emblems/Makefile
dnl themes/XFce/24x24/Makefile
dnl themes/XFce/24x24/emblems/Makefile
dnl themes/XFce/14x14/Makefile
dnl themes/XFce/14x14/emblems/Makefile
dnl themes/XFce/50x50/Makefile
dnl themes/XFce/50x50/emblems/Makefile
dnl themes/XFce/53x53/Makefile
dnl themes/XFce/53x53/emblems/Makefile
