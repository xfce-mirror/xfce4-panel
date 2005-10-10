dnl configure.ac
dnl
dnl xfce4 - panel for the Xfce Desktop environment
dnl
dnl 2005 Jasper Huijsmans <jasper@xfce.org>
dnl

dnl version info
m4_define([xfce4_panel_version],[4.3.23])
m4_define([libxfce4panel_verinfo], [1:1:0])

dnl init autoconf
AC_COPYRIGHT([Copyright (c) 2002-2005
        The Xfce development team. All rights reserved.

Written for Xfce by Jasper Huijsmans <jasper@xfce.org>.])
AC_INIT([xfce4-panel], [xfce4_panel_version()], [xfce4-dev@xfce.org])

dnl init automake
XFCE4_PANEL_VERSION=xfce4_panel_version()
AM_INIT_AUTOMAKE([xfce4-panel], [$XFCE4_PANEL_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX
AM_CONDITIONAL([HAVE_CYGWIN], [test "`uname | grep \"CYGWIN\"`" != ""])

dnl disable static libs
AC_DISABLE_STATIC

dnl Check for basic programs
AC_PROG_CC()
AC_PROG_INSTALL()
AC_PROG_LN_S()
AC_PROG_LIBTOOL()
AC_PROG_INTLTOOL()

LIBXFCE4PANEL_VERINFO=libxfce4panel_verinfo()
AC_SUBST([LIBXFCE4PANEL_VERINFO])

dnl Check for standard header files
AC_HEADER_STDC
AC_CHECK_HEADERS([signal.h stddef.h sys/wait.h time.h])
AC_CHECK_FUNCS([sigaction])

dnl Check for i18n support
XDT_I18N([ar az be bg bn_IN ca cs de el en_GB eo es_MX es et eu fa fi fr
	  gu he hi hu hy it ja ko lt mr ms nb_NO nl pa pl pt_BR pt_PT ro
	  ru sk sv ta tr uk vi zh_CN zh_TW ])

dnl Check for X11 installed
XDT_CHECK_LIBX11_REQUIRE
XDT_CHECK_LIBSM

dnl Check for required packages
dnl XDT_CHECK_PACKAGE([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.2.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.3.0])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.3.5])
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.4.0])

dnl Check for optional packages
XDT_CHECK_OPTIONAL_PACKAGE([LIBSTARTUP_NOTIFICATION],
			   [libstartup-notification-1.0], [0.5],
			   [startup-notification], 
			   [startup notification library], [yes])

dnl gtk-doc
GTK_DOC_CHECK([1.0])

dnl Configure the mcs plugin
dnl XDT_XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [4.2.0])

dnl Check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
xfce4-panel.spec
Makefile
config/Makefile
icons/Makefile
libxfce4panel/Makefile
libxfce4panel/libxfce4panel-1.0.pc
panel/Makefile
plugins/Makefile
plugins/launcher/Makefile
plugins/separator/Makefile
plugins/actions/Makefile
plugins/tasklist/Makefile
plugins/pager/Makefile
plugins/systray/Makefile
plugins/iconbox/Makefile
plugins/clock/Makefile
plugins/testplugin/Makefile
po/Makefile.in
docs/Makefile
docs/API/Makefile
docs/API/version.xml
])

