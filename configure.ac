AC_INIT(panel/main.c)

VERSION=`date +%y%m%d`
AC_SUBST(VERSION)

AM_INIT_AUTOMAKE(xfce4, $VERSION)
AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

GETTEXT_PACKAGE=xfce4-panel
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Name of default
gettext domain])

ALL_LINGUAS="nl es es_MX fr de fi ja"
AM_GLIB_GNU_GETTEXT

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

MY_SUBDIRS="icons panel settings plugins themes" 
AC_SUBST(MY_SUBDIRS)

dnl
dnl  Debugging mode
dnl
DEBUG=0
AC_ARG_ENABLE(debug,
		[  --enable-debug          debug mode (default disabled)],
		[ AC_DEFINE(DEBUG, 1, [Define to 1 for debug.]) ],)


## Versions of dependencies
GLIB_REQUIRED_VERSION=2.0.0
GTK_REQUIRED_VERSION=2.0.0
PANGO_REQUIRED_VERSION=1.0.0
LIBXFCEGUI4_REQUIRED_VERSION=0.0.16
LIBXFCE4MCS_REQUIRED_VERSION=0.0.3
STARTUP_NOTIFICATION_VERSION=0.4

AC_DEFINE_UNQUOTED(GLIB_REQUIRED_VERSION, $GLIB_REQUIRED_VERSION, [Define glib required version.])
AC_DEFINE_UNQUOTED(GTK_REQUIRED_VERSION, $GTK_REQUIRED_VERSION, [Define gtk required version.])
AC_DEFINE_UNQUOTED(PANGO_REQUIRED_VERSION, $PANGO_REQUIRED_VERSION, [Define pango required version.])
AC_DEFINE_UNQUOTED(LIBXFCEGUI4_REQUIRED_VERSION, $LIBXFCEGUI4_REQUIRED_VERSION, [Define libxfcegui4 required version.])
AC_DEFINE_UNQUOTED(LIBXFCE4MCS_REQUIRED_VERSION, $LIBXFCE4MCS_REQUIRED_VERSION, [Define libxfce4mcs required version.])

AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)
AC_SUBST(PANGO_REQUIRED_VERSION)
AC_SUBST(LIBXFCEGUI4_REQUIRED_VERSION)
AC_SUBST(LIBXFCE4MCS_REQUIRED_VERSION)

PKG_CHECK_MODULES(BASE_DEPENDENCIES, glib-2.0 >= $GLIB_REQUIRED_VERSION gtk+-2.0 >= $GTK_REQUIRED_VERSION pango >= $PANGO_REQUIRED_VERSION gmodule-2.0 libxml-2.0 libxfcegui4-1.0 >= $LIBXFCEGUI4_REQUIRED_VERSION)

PKG_CHECK_MODULES(MCS_CLIENT,  libxfce4mcs-client-1.0 >= $LIBXFCE4MCS_REQUIRED_VERSION)
PKG_CHECK_MODULES(MCS_MANAGER, libxfce4mcs-manager-1.0 >= $LIBXFCE4MCS_REQUIRED_VERSION)
PKG_CHECK_MODULES(XFCE_MCS_MANAGER, xfce-mcs-manager >= 0.1)

if $PKG_CONFIG --atleast-version $STARTUP_NOTIFICATION_VERSION libstartup-notification-1.0; then
    have_startup_notification=yes
else
    have_startup_notification=no
fi
AC_MSG_RESULT($have_startup_notification)

if test x$have_startup_notification = xyes; then
    AC_DEFINE(HAVE_STARTUP_NOTIFICATION, 1, [Define startup notification support])
    PKG_CHECK_MODULES(STARTUP_NOTIFICATION, libstartup-notification-1.0 >= $STARTUP_NOTIFICATION_VERSION)
fi

## libtool support (for plugins)
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_OUTPUT([
xfce4.spec
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
