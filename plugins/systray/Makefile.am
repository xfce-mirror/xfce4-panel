# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libsystray\" \
	-DLOCALEDIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

plugindir = \
	$(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	libsystray.la
	
libsystray_built_sources = \
	xfce-tray-marshal.c \
	xfce-tray-marshal.h

libsystray_la_SOURCES = \
	$(libsystray_built_sources) \
	xfce-tray-dialogs.c \
	xfce-tray-dialogs.h \
	xfce-tray-manager.c \
	xfce-tray-manager.h \
	xfce-tray-plugin.c \
	xfce-tray-plugin.h \
	xfce-tray-widget.c \
	xfce-tray-widget.h

libsystray_la_CFLAGS = \
	$(LIBX11_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(PLATFORM_CFLAGS)

libsystray_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libsystray_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(LIBX11_LIBS) \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS)

libsystray_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = systray.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
systray.desktop.in: $(desktop_in_in_files)
	sed -e "s,\@libdir\@,$(libdir),g" < $< > $@
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_in_files) \
	xfce-tray-marshal.list
	
DISTCLEANFILES = \
	$(desktop_DATA) \
	$(desktop_in_files)
	
#
# Rules to auto-generate built sources
#
if MAINTAINER_MODE
CLEANFILES = \
	xgen-xtmc \
	xgen-xtmh

BUILT_SOURCES = \
	$(libsystray_built_sources)

DISTCLEANFILES +=	\
	$(libsystray_built_sources) \
	stamp-xfce-tray-marshal.h

xfce-tray-marshal.h: stamp-xfce-tray-marshal.h
	@true
stamp-xfce-tray-marshal.h: $(srcdir)/xfce-tray-marshal.list Makefile
	( cd $(srcdir) && glib-genmarshal \
		--prefix=_xfce_tray_marshal \
		--header xfce-tray-marshal.list \
		| sed -e 's/marshal_data);$$/marshal_data);/' ) >> xgen-xtmh \
	&& ( cmp -s xgen-xtmh xfce-tray-marshal.h || cp xgen-xtmh xfce-tray-marshal.h ) \
	&& rm -f xgen-xtmh \
	&& echo timestamp > $(@F)

xfce-tray-marshal.c: xfce-tray-marshal.h Makefile
	( cd $(srcdir) && glib-genmarshal \
		--prefix=_xfce_tray_marshal \
		--body xfce-tray-marshal.list ) >> xgen-xtmc \
	&& cp xgen-xtmc xfce-tray-marshal.c \
	&& rm -f xgen-xtmc
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
