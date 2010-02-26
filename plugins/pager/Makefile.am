# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libpager\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(PLATFORM_CPPFLAGS)

plugindir = \
	$(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	libpager.la

libpager_la_SOURCES = \
	pager.c \
	pager.h

libpager_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(PLATFORM_CFLAGS)

libpager_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libpager_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBWNCK_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBXFCE4UI_LIBS)

libpager_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = pager.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
pager.desktop.in: $(desktop_in_in_files)
	sed -e "s,\@libdir\@,$(libdir),g" < $< > $@
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_in_files)

DISTCLEANFILES = \
	$(desktop_DATA) \
	$(desktop_in_files)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
