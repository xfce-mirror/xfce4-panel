# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libtasklist\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	libtasklist.la

libtasklist_built_sources = \
	tasklist-dialog_glade.h

libtasklist_la_SOURCES = \
	$(libtasklist_built_sources) \
	tasklist.c \
	tasklist-widget.c \
	tasklist-widget.h

libtasklist_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(EXO_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	$(PLATFORM_CFLAGS)

libtasklist_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libtasklist_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(top_builddir)/common/libpanel-xfconf.la \
	$(EXO_LIBS) \
	$(GTK_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBWNCK_LIBS)

libtasklist_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(top_builddir)/common/libpanel-xfconf.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = tasklist.desktop.in.in
desktop_in_files =  $(desktop_in_in_files:.desktop.in.in=.desktop.in)
tasklist.desktop.in: $(desktop_in_in_files)
	sed -e "s,\@plugindir\@,$(plugindir),g" < $< > $@
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	tasklist-dialog.glade \
	$(desktop_in_in_files)

DISTCLEANFILES = \
	$(desktop_DATA) \
	$(desktop_in_files)

if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libtasklist_built_sources)

DISTCLEANFILES += \
	$(libtasklist_built_sources)

tasklist-dialog_glade.h: tasklist-dialog.glade
	exo-csource --static --strip-comments --strip-content --name=tasklist_dialog_glade $< >$@
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
