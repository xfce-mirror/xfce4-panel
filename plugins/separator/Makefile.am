# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libseparator\" \
	-DLOCALEDIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

plugindir = \
	$(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	libseparator.la

libseparator_built_sources = \
	separator-dialog_glade.h

libseparator_la_SOURCES = \
	$(libseparator_built_sources) \
	separator.c \
	separator.h

libseparator_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(PLATFORM_CFLAGS) \
	$(EXO_CFLAGS) \
	$(XFCONF_CFLAGS)

libseparator_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libseparator_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(GTK_LIBS) \
	$(CAIRO_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(EXO_LIBS) \
	$(XFCONF_LIBS)

libseparator_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = separator.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
separator.desktop.in: $(desktop_in_in_files)
	sed -e "s,\@libdir\@,$(libdir),g" < $< > $@
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	separator-dialog.glade \
	$(desktop_in_in_files)

DISTCLEANFILES = \
	$(desktop_DATA) \
	$(desktop_in_files)

if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libseparator_built_sources)

DISTCLEANFILES += \
	$(libseparator_built_sources)

separator-dialog_glade.h: separator-dialog.glade
	exo-csource --static --strip-comments --strip-content --name=separator_dialog_glade $< >$@
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
