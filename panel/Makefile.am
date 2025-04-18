
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-panel\" \
	-DPREFIX=\"$(prefix)\" \
	-DDATADIR=\"$(datadir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DHELPERDIR=\"$(HELPER_PATH_PREFIX)/xfce4/panel\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfce4-panel

xfce4_panel_built_sources = \
	$(top_srcdir)/common/xfce4-panel-resources.c \
	panel-gdbus-exported-service.h \
	panel-gdbus-exported-service.c \
	panel-marshal.c \
	panel-marshal.h \
	panel-plugin-external-wrapper-exported.h \
	panel-plugin-external-wrapper-exported.c \
	$(NULL)

xfce4_panel_SOURCES = \
	$(xfce4_panel_built_sources) \
	main.c \
	panel-application.c \
	panel-application.h \
	panel-base-window.c \
	panel-base-window.h \
	panel-dbus-service.c \
	panel-dbus-service.h \
	panel-dbus-client.c \
	panel-dbus-client.h \
	panel-dialogs.c \
	panel-dialogs.h \
	panel-item-dialog.c \
	panel-item-dialog.h \
	panel-itembar.c \
	panel-itembar.h \
	panel-module.c \
	panel-module.h \
	panel-module-factory.c \
	panel-module-factory.h \
	panel-plugin-external.c \
	panel-plugin-external.h \
	panel-plugin-external-wrapper.c \
	panel-plugin-external-wrapper.h \
	panel-preferences-dialog.c \
	panel-preferences-dialog.h \
	panel-tic-tac-toe.c \
	panel-tic-tac-toe.h \
	panel-window.c \
	panel-window.h

if ENABLE_X11
xfce4_panel_SOURCES += \
	panel-plugin-external-wrapper-x11.c \
	panel-plugin-external-wrapper-x11.h
endif

if HAVE_GTK_LAYER_SHELL
xfce4_panel_SOURCES += \
	panel-plugin-external-wrapper-wayland.c \
	panel-plugin-external-wrapper-wayland.h
endif

xfce4_panel_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBX11_CFLAGS) \
	$(GTK_LAYER_SHELL_CFLAGS) \
	$(LIBXFCE4WINDOWING_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfce4_panel_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

xfce4_panel_LDADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la \
	$(top_builddir)/common/libpanel-common.la \
	$(GTK_LIBS) \
	$(GMODULE_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBX11_LIBS) \
	$(GTK_LAYER_SHELL_LIBS) \
	$(LIBXFCE4WINDOWING_LIBS) \
	-lm

xfce4_panel_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la \
	$(top_builddir)/common/libpanel-common.la

panel-marshal.h: panel-marshal.list Makefile
	$(AM_V_GEN) glib-genmarshal --prefix=_panel_marshal --internal --header $< > $@

panel-marshal.c: panel-marshal.list Makefile
	$(AM_V_GEN) echo "#include <panel/panel-marshal.h>" > $@ \
	&& glib-genmarshal --prefix=_panel_marshal --body $< >> $@

panel-gdbus-exported-service.h:
panel-gdbus-exported-service.c: panel-dbus-service-infos.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix org.xfce.Panel \
	--c-namespace XfcePanel   \
	--generate-c-code panel-gdbus-exported-service \
	$(srcdir)/panel-dbus-service-infos.xml

panel-plugin-external-wrapper-exported.h:
panel-plugin-external-wrapper-exported.c: panel-plugin-external-wrapper-infos.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix org.xfce.Panel.Wrapper \
	--c-namespace XfcePanelPluginWrapper   \
	--generate-c-code panel-plugin-external-wrapper-exported \
	$(srcdir)/panel-plugin-external-wrapper-infos.xml

DISTCLEANFILES = \
	$(xfce4_panel_built_sources)

BUILT_SOURCES = \
	$(xfce4_panel_built_sources)

EXTRA_DIST = \
	panel-dbus-service-infos.xml \
	panel-plugin-external-wrapper-infos.xml \
	panel-marshal.list \
	meson.build \
	$(NULL)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
