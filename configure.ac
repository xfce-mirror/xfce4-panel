dnl configure.ac
dnl
dnl xfce4 - panel for the XFce Desktop environment
dnl

AC_INIT(panel/main.c)

AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(xfce4, 0.1)

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
AC_CHECK_HEADERS(signal.h stddef.h)
AC_CHECK_FUNCS(sigaction)

dnl Check for i18n support
BM_I18N([xfce4-panel], [de es es_MX fr fi nl ja])

dnl Check for X11 installed
AM_LIBX11_REQUIRE

dnl AC_CHECK_LIB(Xext, main,, AC_MSG_ERROR([The X Window System installed on this machine does not support the shape extension]), $X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)

BM_DEPEND(GLIB, glib-2.0, 2.0.0)
BM_DEPEND(GTK, gtk+-2.0, 2.0.0)
BM_DEPEND(LIBXFCE4MCS_CLIENT, libxfce4mcs-client-1.0, 0.0.3)
BM_DEPEND(LIBXFCE4MCS_MANAGER, libxfce4mcs-manager-1.0, 0.0.3)
BM_DEPEND(LIBXFCE4UTIL, libxfce4util-1.0, 0.0)
BM_DEPEND(LIBXFCEGUI4, libxfcegui4-1.0, 0.0.16)
BM_DEPEND(LIBXML, libxml-2.0, 2.4.0)
BM_DEPEND(XFCE_MCS_MANAGER, xfce-mcs-manager, 0.1)

BM_DEPEND_CHECK(LIBSTARTUP_NOTIFICATION, libstartup-notification-1.0, 0.4)

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
xfce4.spec
xfce4rc
Makefile
doc/Makefile
doc/images/Makefile
doc/html/Makefile
doc/xml/Makefile
po/Makefile.in
icons/Makefile
panel/Makefile
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
themes/Gnome/Makefile
themes/Noia/Makefile
themes/XFce/Makefile
])
