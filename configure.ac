dnl configure.ac
dnl
dnl xfce4 - panel for the Xfce Desktop environment
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl

AC_INIT([panel/main.c])

AM_INIT_AUTOMAKE([xfce4-panel], [4.2.2])

AM_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX
AM_CONDITIONAL([HAVE_CYGWIN], [test "`uname | grep \"CYGWIN\"`" != ""])

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
BM_I18N([xfce4-panel], [ar az be bg bn_IN ca cs da de el en_GB es es_MX et
                        eu fa fr fi gu he hi hu it ja lt ms nl ko nb_NO
			pa pl pt_BR pt_PT ro ru sv sk ta tr vi zh_CN zh_TW])

dnl Check for X11 installed
BM_LIBX11_REQUIRE

dnl Check for required packages
BM_DEPEND([LIBXFCE4MCS_CLIENT], [libxfce4mcs-client-1.0], [4.2.0])
BM_DEPEND([LIBXFCE4UTIL], [libxfce4util-1.0], [4.2.0])
BM_DEPEND([LIBXFCEGUI4], [libxfcegui4-1.0], [4.2.0])
BM_DEPEND([LIBXML], [libxml-2.0], [2.4.0])

dnl disable threading, too much trouble for too little gain
dnl BM_DEPEND([GTHREAD], [gthread-2.0], [2.0.6])

dnl Check for dbh installed for completion combo
dnl disable for now, doesn't work for new install, only after
dnl  combo has been used in xffm or xfrun.
dnl BM_DEPEND_CHECK([LIBDBH], [dbh-1.0], [1.0],
dnl  [dbh], [DBH library], [yes])

dnl Configure the mcs plugin
XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [4.2.0])

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
plugins/sample-plugin/Makefile
plugins/separator/Makefile
settings/Makefile
])

