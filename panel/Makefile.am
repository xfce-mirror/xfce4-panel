
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-panel\" \
	-DDATADIR=\"$(datadir)/xfce4\" \
	-DLIBDIR=\"$(libdir)/xfce4\" \
	-DHELPERDIR=\"$(HELPER_PATH_PREFIX)/xfce4/panel\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DDBUS_API_SUBJECT_TO_CHANGE \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfce4-panel

xfce4_panel_built_sources = \
	panel-dbus-service-infos.h \
	panel-dbus-client-infos.h \
	panel-marshal.c \
	panel-marshal.h \
	panel-plugin-external-wrapper-infos.h \
	panel-preferences-dialog-ui.h

xfce4_panel_SOURCES = \
	$(xfce4_panel_built_sources) \
	main.c \
	panel-application.c \
	panel-application.h \
	panel-base-window.c \
	panel-base-window.h \
	panel-dbus-client.c \
	panel-dbus-client.h \
	panel-dbus-service.c \
	panel-dbus-service.h \
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
	panel-plugin-external-46.c \
	panel-plugin-external-46.h \
	panel-preferences-dialog.c \
	panel-preferences-dialog.h \
	panel-tic-tac-toe.c \
	panel-tic-tac-toe.h \
	panel-window.c \
	panel-window.h

xfce4_panel_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBX11_CFLAGS) \
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
	$(DBUS_LIBS) \
	$(LIBX11_LIBS) \
	-lm

xfce4_panel_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la \
	$(top_builddir)/common/libpanel-common.la

if MAINTAINER_MODE

panel-marshal.h: panel-marshal.list Makefile
	$(AM_V_GEN) glib-genmarshal --prefix=_panel_marshal --internal --header $< > $@

panel-marshal.c: panel-marshal.list Makefile
	$(AM_V_GEN) echo "#include <panel/panel-marshal.h>" > $@ \
	&& glib-genmarshal --prefix=_panel_marshal --body $< >> $@

panel-dbus-service-infos.h: $(srcdir)/panel-dbus-service-infos.xml Makefile
	$(AM_V_GEN) dbus-binding-tool --prefix=panel_dbus_service --mode=glib-server $< > $@

panel-dbus-client-infos.h: $(srcdir)/panel-dbus-service-infos.xml Makefile
	$(AM_V_GEN) dbus-binding-tool --mode=glib-client $< > $@

panel-plugin-external-wrapper-infos.h: $(srcdir)/panel-plugin-external-wrapper-infos.xml Makefile
	$(AM_V_GEN) dbus-binding-tool --prefix=panel_plugin_external_wrapper --mode=glib-server $< > $@

panel-preferences-dialog-ui.h: $(srcdir)/panel-preferences-dialog.glade Makefile
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=panel_preferences_dialog_ui $< >$@

#panel-marshal.h: $(srcdir)/panel-marshal.list Makefile
#	$(AM_V_GEN) glib-genmarshal --header --prefix=panel_marshal $< > $@
#
#panel-marshal.c: $(srcdir)/panel-marshal.list Makefile
#	$(AM_V_GEN) echo "#include <panel/panel-marshal.h>" > $@ \
#	&& glib-genmarshal --body --prefix=panel_marshal $< >> $@

DISTCLEANFILES = \
	$(xfce4_panel_built_sources)

BUILT_SOURCES = \
	$(xfce4_panel_built_sources)

endif

EXTRA_DIST = \
	panel-dbus-service-infos.xml \
	panel-plugin-external-wrapper-infos.xml \
	panel-preferences-dialog.glade \
	panel-marshal.list

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
