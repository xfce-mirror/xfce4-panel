AC_INIT(panel/xfce.c)

VERSION=`date +%y%m%d`
AC_SUBST(VERSION)

AM_INIT_AUTOMAKE(xfce4, $VERSION)
AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AC_PROG_CC
AC_PROG_INSTALL

AC_C_INLINE
AC_C_CONST

AC_PATH_XTRA
AC_HEADER_STDC
AC_CHECK_HEADERS(stdlib.h string.h unistd.h)

AC_CHECK_LIB(X11, main,, AC_MSG_ERROR([libX11 is missing from your system: Make sure you have both the  X Window System libraries and development headers correctly installed]), $X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)
AC_CHECK_LIB(Xext, main,, AC_MSG_ERROR([The X Window System installed on this machine does not support the shape extension]), $X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)
AC_CHECK_LIB(Xpm, main,, AC_MSG_ERROR([libXpm is missing from your system: Make sure you have both the Xpm libraries and development headers correctly installed]), $X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)

AC_CHECK_FUNCS(setlocale strchr strrchr strstr)

AC_ARG_ENABLE(plugins,
	      [  --disable-plugins  	Don't build plugins],
	      [ case "${enableval}" in
	          no) MY_SUBDIRS="icons panel themes" ;;
		  *)  MY_SUBDIRS="icons panel plugins themes" ;;
	    	esac],
	      [ MY_SUBDIRS="icons panel plugins themes" ])

dnl
dnl  Debugging mode
dnl
DEBUG=0
AC_ARG_ENABLE(debug,
		[  --enable-debug          debug mode (default disabled)],
		[ AC_DEFINE(DEBUG, 1, "Mode debug") ],)


AC_SUBST(MY_SUBDIRS)

## Versions of dependencies
GLIB_REQUIRED_VERSION=2.0.0
GTK_REQUIRED_VERSION=2.0.0
PANGO_REQUIRED_VERSION=1.0.0

PKG_CHECK_MODULES(BASE_DEPENDENCIES, glib-2.0 >= $GLIB_REQUIRED_VERSION gtk+-2.0 >= $GTK_REQUIRED_VERSION pango >= $PANGO_REQUIRED_VERSION gmodule-2.0 libxml-2.0)

## libtool support (for plugins)
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_OUTPUT([
xfce4.spec
Makefile
icons/Makefile
panel/Makefile
plugins/Makefile
plugins/mailcheck/Makefile
plugins/clock/Makefile
themes/Makefile
themes/Curve/Makefile
themes/Gnome/Makefile
])
