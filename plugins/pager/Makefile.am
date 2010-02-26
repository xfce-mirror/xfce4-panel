# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libpager\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	libpager.la

libpager_built_sources = \
	pager-dialog_glade.h

libpager_la_SOURCES = \
	$(libpager_built_sources) \
	pager.c \
	pager.h

libpager_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(EXO_CFLAGS) \
	$(PLATFORM_CFLAGS)

libpager_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libpager_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(top_builddir)/common/libpanel-xfconf.la \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBWNCK_LIBS) \
	$(XFCONF_LIBS) \
	$(EXO_LIBS) \
	$(LIBXFCE4UI_LIBS)

libpager_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(top_builddir)/common/libpanel-xfconf.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_files = pager.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	pager-dialog.glade \
	$(desktop_in_files)

DISTCLEANFILES = \
	$(desktop_DATA)

if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libpager_built_sources)

DISTCLEANFILES += \
	$(libpager_built_sources)

pager-dialog_glade.h: pager-dialog.glade
	exo-csource --static --strip-comments --strip-content --name=pager_dialog_glade $< >$@
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
