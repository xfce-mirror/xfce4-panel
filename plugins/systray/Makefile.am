
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libsystray\" \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel/plugins

# Keep the legacy "systray" name to ease the migration for users
plugin_LTLIBRARIES = \
	libsystray.la

libsystray_built_sources = \
	sn-watcher.h \
	sn-watcher.c \
	systray-marshal.c \
	systray-marshal.h

libsystray_la_SOURCES = \
	$(libsystray_built_sources) \
	$(top_srcdir)/common/xfce4-panel-resources.c \
	sn-config.c \
	sn-config.h \
	sn-backend.c \
	sn-backend.h \
	sn-button.c \
	sn-button.h \
	sn-box.c \
	sn-box.h \
	sn-dialog.c \
	sn-dialog.h \
	sn-icon-box.c \
	sn-icon-box.h \
	sn-item.c \
	sn-item.h \
	sn-plugin.c \
	sn-plugin.h \
	sn-util.c \
	sn-util.h

if ENABLE_X11
libsystray_la_SOURCES += \
	systray.c \
	systray.h \
	systray-box.c \
	systray-box.h \
	systray-manager.c \
	systray-manager.h \
	systray-socket.c \
	systray-socket.h
endif

libsystray_la_CFLAGS = \
	$(LIBX11_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(GTK_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(PLATFORM_CFLAGS) \
	$(DBUSMENU_CFLAGS)

libsystray_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

libsystray_la_LIBADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la \
	$(top_builddir)/common/libpanel-common.la \
	$(LIBX11_LIBS) \
	$(GIO_UNIX_LIBS) \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(XFCONF_LIBS) \
	$(DBUSMENU_LIBS)

libsystray_la_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la \
	$(top_builddir)/common/libpanel-common.la

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = systray.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
	$(desktop_in_files) \
	systray-marshal.list \
	meson.build \
	sn-watcher.xml \
	$(NULL)

DISTCLEANFILES = \
	$(desktop_DATA)

#
# Rules to auto-generate built sources
#
BUILT_SOURCES = \
	$(libsystray_built_sources)

DISTCLEANFILES +=	\
	$(libsystray_built_sources)

sn-watcher.h:
sn-watcher.c: sn-watcher.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix org.kde.StatusNotifierWatcher \
	--generate-c-code sn-watcher \
	$(srcdir)/sn-watcher.xml

systray-marshal.h: systray-marshal.list Makefile
	$(AM_V_GEN) glib-genmarshal --header --internal --prefix=_systray_marshal $< > $@

systray-marshal.c: systray-marshal.list Makefile
	$(AM_V_GEN) echo "#include \"systray-marshal.h\"" > $@ \
	&& glib-genmarshal --prefix=_systray_marshal --body $< >> $@

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
