# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libwindowmenu\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	libwindowmenu.la

libwindowmenu_built_sources = \
	windowmenu-dialog_glade.h

libwindowmenu_la_SOURCES = \
	$(libwindowmenu_built_sources) \
	windowmenu.c \
	windowmenu.h

libwindowmenu_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(EXO_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	$(PLATFORM_CFLAGS)

libwindowmenu_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libwindowmenu_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(top_builddir)/common/libpanel-xfconf.la \
	$(EXO_LIBS) \
	$(GTK_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBWNCK_LIBS)

libwindowmenu_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(top_builddir)/common/libpanel-xfconf.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = windowmenu.desktop.in.in
desktop_in_files =  $(desktop_in_in_files:.desktop.in.in=.desktop.in)
windowmenu.desktop.in: $(desktop_in_in_files)
	sed -e "s,\@plugindir\@,$(plugindir),g" < $< > $@
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	windowmenu-dialog.glade \
	$(desktop_in_in_files)

DISTCLEANFILES = \
	$(desktop_DATA) \
	$(desktop_in_files)

if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libwindowmenu_built_sources)

DISTCLEANFILES += \
	$(libwindowmenu_built_sources)

windowmenu-dialog_glade.h: windowmenu-dialog.glade
	exo-csource --static --strip-comments --strip-content --name=windowmenu_dialog_glade $< >$@
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
