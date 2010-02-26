# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DBINDIR=\"$(bindir)\" \
	-DG_LOG_DOMAIN=\"liblauncher\" \
	-DSN_API_NOT_YET_FROZEN \
	-DEXO_API_SUBJECT_TO_CHANGE \
	-DLOCALEDIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

plugindir = \
	$(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	liblauncher.la

liblauncher_la_SOURCES = \
	launcher.c \
	launcher.h \
	launcher-dialog.c \
	launcher-dialog.h \
	launcher-dialog_glade.h

liblauncher_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(EXO_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBSTARTUP_NOTIFICATION_CFLAGS) \
	$(PLATFORM_CFLAGS)

liblauncher_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

liblauncher_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(EXO_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBSTARTUP_NOTIFICATION_LIBS)

liblauncher_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la

if MAINTAINER_MODE
BUILT_SOURCES = \
	launcher-dialog_glade.h

launcher-dialog_glade.h: launcher-dialog.glade
	exo-csource --static --strip-comments --strip-content --name=launcher_dialog_glade $< >$@
endif

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = launcher.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
%.desktop.in: %.desktop.in.in
	sed -e "s,\@libdir\@,$(libdir),g" < $< > $@
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	launcher-dialog.glade \
	$(desktop_in_in_files)

DISTCLEANFILES = \
	$(desktop_DATA) \
	$(desktop_in_files)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
