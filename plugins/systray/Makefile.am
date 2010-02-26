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
	systray-dialog_glade.h \
	systray-marshal.c \
	systray-marshal.h

libsystray_la_SOURCES = \
	$(libsystray_built_sources) \
	systray.c \
	systray.h \
	systray-box.c \
	systray-box.h \
	systray-manager.c \
	systray-manager.h

libsystray_la_CFLAGS = \
	$(LIBX11_CFLAGS) \
	$(GTK_CFLAGS) \
	$(EXO_CFLAGS) \
	$(XFCONF_CFLAGS) \
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
	$(EXO_LIBS) \
	$(XFCONF_LIBS) \
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
	systray-dialog.glade \
	systray-marshal.list

DISTCLEANFILES = \
	$(desktop_DATA) \
	$(desktop_in_files)

#
# Rules to auto-generate built sources
#
if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libsystray_built_sources)

DISTCLEANFILES +=	\
	$(libsystray_built_sources)

systray-marshal.h: systray-marshal.list Makefile
	glib-genmarshal --header --internal --prefix=_systray_marshal $< > $@

systray-marshal.c: systray-marshal.list Makefile
	echo "#include \"systray-marshal.h\"" > $@ \
	&& glib-genmarshal --prefix=_systray_marshal --body $< >> $@

systray-dialog_glade.h: systray-dialog.glade
	exo-csource --static --strip-comments --strip-content --name=systray_dialog_glade $< >$@
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
