dnl configure.ac
dnl
dnl xfce4 - panel for the XFce Desktop environment
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl

AC_INIT([panel/main.c])

AM_INIT_AUTOMAKE([xfce4-panel], [4.1.3])

AM_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL

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
BM_DEPEND([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.0.0])
BM_DEPEND([LIBXFCEGUI4], [libxfcegui4-1.0], [4.1.8])
BM_DEPEND([LIBXML], [libxml-2.0], [2.4.0])

dnl Check for dbh installed
BM_DEPEND_CHECK([LIBDBH], [dbh-1.0], [1.0],
  [dbh], [DBH library], [yes])

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
plugins/separator/Makefile
settings/Makefile
themes/Makefile
themes/Crystal/Makefile
themes/Crystal/48x48/Makefile
themes/Crystal/48x48/emblems/Makefile
themes/Crystal/22x22/Makefile
themes/Crystal/22x22/emblems/Makefile
themes/Curve/Makefile
themes/Curve/48x48/Makefile
themes/Curve/48x48/emblems/Makefile
themes/Curve/22x22/Makefile
themes/Curve/22x22/emblems/Makefile
themes/FreeIcons/Makefile
themes/FreeIcons/48x48/Makefile
themes/FreeIcons/48x48/emblems/Makefile
themes/FreeIcons/24x24/Makefile
themes/FreeIcons/24x24/emblems/Makefile
themes/FreeIcons/22x22/Makefile
themes/FreeIcons/22x22/emblems/Makefile
themes/Gnome/Makefile
themes/Gnome/48x48/Makefile
themes/Gnome/48x48/emblems/Makefile
themes/Gnome/50x50/Makefile
themes/Gnome/50x50/emblems/Makefile
themes/Gnome/24x24/Makefile
themes/Gnome/24x24/emblems/Makefile
themes/Noia/Makefile
themes/Noia/48x48/Makefile
themes/Noia/48x48/emblems/Makefile
themes/Noia/22x22/Makefile
themes/Noia/22x22/emblems/Makefile
themes/XFce/Makefile
themes/XFce/48x48/Makefile
themes/XFce/48x48/emblems/Makefile
themes/XFce/19x19/Makefile
themes/XFce/19x19/emblems/Makefile
themes/XFce/16x16/Makefile
themes/XFce/16x16/emblems/Makefile
themes/XFce/24x24/Makefile
themes/XFce/24x24/emblems/Makefile
themes/XFce/14x14/Makefile
themes/XFce/14x14/emblems/Makefile
themes/XFce/50x50/Makefile
themes/XFce/50x50/emblems/Makefile
themes/XFce/53x53/Makefile
themes/XFce/53x53/emblems/Makefile
])
