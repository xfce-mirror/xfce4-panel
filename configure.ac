dnl configure.ac
dnl
dnl xfce4 - panel for the XFce Desktop environment
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl

AC_INIT([panel/main.c])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfce4-panel], [3.90])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC
AC_CHECK_HEADERS([signal.h stddef.h sys/wait.h time.h])
AC_CHECK_FUNCS([sigaction])

dnl Check for i18n support
BM_I18N([xfce4-panel], [ca de es es_MX fr fi nl ja pl ta zh_CN])

dnl Check for X11 installed
BM_LIBX11_REQUIRE

dnl Check for required packages
BM_DEPEND([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [0.0.3])
BM_DEPEND([LIBXFCEGUI4], [libxfcegui4-1.0], [0.0.17])
BM_DEPEND([LIBXML], [libxml-2.0], [2.4.0])

dnl Configure the mcs plugin
XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [0.2.0])

dnl Check for optional packages
BM_DEPEND_CHECK([LIBSTARTUP_NOTIFICATION], [libstartup-notification-1.0],
  [0.4], [startup-notification], [startup notification library], [yes])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
xfce4.spec
Makefile
doc/Makefile
doc/C/Makefile
doc/C/images/Makefile
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
settings/Makefile
themes/Makefile
themes/Crystal/Makefile
themes/Curve/Makefile
themes/FreeIcons/Makefile
themes/Gnome/Makefile
themes/Noia/Makefile
themes/XFce/Makefile
])
