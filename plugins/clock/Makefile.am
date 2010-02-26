# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libclock\" \
	-DLOCALEDIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	libclock.la

libclock_built_sources = \
	clock-dialog_ui.h

libclock_la_SOURCES = \
	$(libclock_built_sources) \
	clock.c \
	clock.h \
	clock-analog.c \
	clock-analog.h \
	clock-binary.c \
	clock-binary.h \
	clock-digital.c \
	clock-digital.h \
	clock-fuzzy.c \
	clock-fuzzy.h \
	clock-lcd.c \
	clock-lcd.h

libclock_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(PLATFORM_CFLAGS) \
	$(XFCONF_CFLAGS)

libclock_la_LDFLAGS =	 \
	-avoid-version \
	-module \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libclock_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(top_builddir)/common/libpanel-xfconf.la \
	$(GTK_LIBS) \
	$(CAIRO_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(XFCONF_LIBS)

libclock_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(top_builddir)/common/libpanel-xfconf.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_files =	clock.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	clock-dialog.ui \
	$(desktop_in_files)

DISTCLEANFILES = \
	$(desktop_DATA)

if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libclock_built_sources)

DISTCLEANFILES += \
	$(libclock_built_sources)

clock-dialog_ui.h: clock-dialog.ui
	exo-csource --static --strip-comments --strip-content --name=clock_dialog_ui $< >$@
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
