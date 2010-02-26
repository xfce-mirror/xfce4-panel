# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libpanel-xfconf\" \
	-DLIBXFCE4PANEL_COMPILATION \
	$(PLATFORM_CPPFLAGS)

noinst_LTLIBRARIES = \
	libpanel-xfconf.la

libpanel_xfconf_la_SOURCES = \
	panel-builder.c \
	panel-builder.h \
	panel-xfconf.c \
	panel-xfconf.h

libpanel_xfconf_la_CFLAGS = \
	$(XFCONF_CFLAGS) \
	$(GTK_CFLAGS)

libpanel_xfconf_la_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libpanel_xfconf_la_LIBADD = \
	$(XFCONF_LIBS) \
	$(GTK_LIBS)

EXTRA_DIST = \
	panel-dbus.h \
	panel-private.h

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
