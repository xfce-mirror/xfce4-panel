# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DG_LOG_DOMAIN=\"xfce4-panel\" \
	-DDATADIR=\"$(datadir)\" \
	-DLIBDIR=\"$(libdir)/xfce4\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DDBUS_API_SUBJECT_TO_CHANGE \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfce4-panel

xfce4_panel_built_sources = \
	panel-dbus-service-infos.h \
	panel-dbus-client-infos.h \
	panel-marshal.h \
	panel-marshal.c \
	panel-preferences-dialog-glade.h

xfce4_panel_SOURCES = \
	$(xfce4_panel_built_sources) \
	$(top_srcdir)/common/panel-xfconf.c \
	$(top_srcdir)/common/panel-xfconf.h \
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
	panel-preferences-dialog.c \
	panel-preferences-dialog.h \
	panel-window.c \
	panel-window.h

xfce4_panel_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(EXO_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfce4_panel_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

xfce4_panel_LDADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la \
	$(GTK_LIBS) \
	$(EXO_LIBS) \
	$(GMODULE_LIBS) \
	$(GTHREAD_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(XFCONF_LIBS) \
	$(DBUS_LIBS)

xfce4_panel_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel.la

if MAINTAINER_MODE

panel-dbus-service-infos.h: $(srcdir)/panel-dbus-service-infos.xml Makefile
	dbus-binding-tool --prefix=panel_dbus_service --mode=glib-server $< > $@

panel-dbus-client-infos.h: $(srcdir)/panel-dbus-service-infos.xml Makefile
	dbus-binding-tool --mode=glib-client $< > $@

panel-preferences-dialog-glade.h: $(srcdir)/panel-preferences-dialog.glade Makefile
	exo-csource --static --strip-comments --strip-content --name=panel_preferences_dialog_glade $< >$@

panel-marshal.h: $(srcdir)/panel-marshal.list Makefile
	glib-genmarshal --header --prefix=panel_marshal $< > $@

panel-marshal.c: $(srcdir)/panel-marshal.list Makefile
	echo "#include <panel/panel-marshal.h>" > $@ \
	&& glib-genmarshal --body --prefix=panel_marshal $< >> $@

DISTCLEANFILES = \
	$(xfce4_panel_built_sources)

BUILT_SOURCES = \
	$(xfce4_panel_built_sources)

endif

EXTRA_DIST = \
	panel-dbus-service-infos.xml \
	panel-marshal.list \
	panel-preferences-dialog.glade

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
