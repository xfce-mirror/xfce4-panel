
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libshowdesktop\" \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = \
	libshowdesktop.la

libshowdesktop_built_sources = \
	showdesktop-dialog_ui.h

libshowdesktop_la_SOURCES = \
	$(libshowdesktop_built_sources) \
	showdesktop.c \
	showdesktop.h

libshowdesktop_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4WINDOWING_CFLAGS) \
	$(PLATFORM_CFLAGS)

libshowdesktop_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

libshowdesktop_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la \
	$(top_builddir)/common/libpanel-common.la \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBXFCE4WINDOWING_LIBS)

libshowdesktop_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la \
	$(top_builddir)/common/libpanel-common.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = showdesktop.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
	showdesktop-dialog.glade \
	$(desktop_in_files)

DISTCLEANFILES = \
	$(desktop_DATA)

if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libshowdesktop_built_sources)

DISTCLEANFILES += \
	$(libshowdesktop_built_sources)

showdesktop-dialog_ui.h: showdesktop-dialog.glade
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=showdesktop_dialog_ui $< >$@
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
