dnl configure.ac
dnl
dnl xfce4 - panel for the XFce Desktop environment
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl

AC_INIT([panel/main.c])

AM_INIT_AUTOMAKE([xfce4-panel], [4.0.6])

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
AC_CHECK_HEADERS([signal.h stddef.h sys/wait.h time.h])
AC_CHECK_FUNCS([sigaction])

dnl Check for i18n support
BM_I18N([xfce4-panel], [ar az bg ca cs de es es_MX et eu fa_IR fr fi hi hu it lt ms nl
	 ja ko no pl pt_BR pt_PT ru sv ta tr uk vi zh_CN zh_TW])

dnl Check for X11 installed
BM_LIBX11_REQUIRE

dnl Check for required packages
BM_DEPEND([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.0.0])
BM_DEPEND([LIBXFCEGUI4], [libxfcegui4-1.0], [4.0.0])
BM_DEPEND([LIBXML], [libxml-2.0], [2.4.0])

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
themes/Curve/Makefile
themes/FreeIcons/Makefile
themes/Gnome/Makefile
themes/Noia/Makefile
themes/XFce/Makefile
])
