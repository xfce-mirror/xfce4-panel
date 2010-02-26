# $Id$

INCLUDES = 									\
	-I$(top_srcdir)								\
	-I$(top_builddir)							\
	-DG_LOG_DOMAIN=\"xfce4-panel\"						\
	-DDATADIR=\"$(datadir)\"						\
	-DLIBDIR=\"$(libdir)/xfce4\"						\
	-DLIBEXECDIR=\"$(libexecdir)\"						\
	-DSYSCONFDIR=\"$(sysconfdir)\"						\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"					\
	-DDBUS_API_SUBJECT_TO_CHANGE						\
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = 									\
	xfce4-panel
	
xfce4_panel_built_sources =							\
	panel-dbus-infos.h

xfce4_panel_SOURCES =								\
	$(xfce4_panel_built_sources)						\
	main.c									\
	panel-application.c							\
	panel-application.h							\
	panel-dbus.c								\
	panel-dbus.h								\
	panel-dialogs.c								\
	panel-dialogs.h								\
	panel-glue.c								\
	panel-glue.h								\
	panel-item-dialog.c							\
	panel-item-dialog.h							\
	panel-itembar.c								\
	panel-itembar.h								\
	panel-module.c								\
	panel-module.h								\
	panel-module-factory.c							\
	panel-module-factory.h							\
	panel-plugin-external.c							\
	panel-plugin-external.h							\
	panel-preferences-dialog.c						\
	panel-preferences-dialog.h						\
	panel-private.c								\
	panel-private.h								\
	panel-window.c								\
	panel-window.h

xfce4_panel_CFLAGS =								\
	$(GTK_CFLAGS)								\
	$(GMODULE_CFLAGS)							\
	$(GTHREAD_CFLAGS)							\
	$(EXO_CFLAGS)								\
	$(LIBXFCE4UTIL_CFLAGS)							\
	$(LIBXFCE4UI_CFLAGS)							\
	$(DBUS_CFLAGS)								\
	$(PLATFORM_CFLAGS)

xfce4_panel_LDFLAGS =								\
	-no-undefined								\
	$(PLATFORM_LDFLAGS)

xfce4_panel_LDADD =								\
	$(top_builddir)/libxfce4panel/libxfce4panel.la				\
	$(GTK_LIBS)								\
	$(EXO_LIBS)								\
	$(GMODULE_LIBS)								\
	$(GTHREAD_LIBS)								\
	$(LIBXFCE4UTIL_LIBS)							\
	$(LIBXFCE4UI_LIBS)							\
	$(DBUS_LIBS)

xfce4_panel_DEPENDENCIES =							\
	$(top_builddir)/libxfce4panel/libxfce4panel.la

if MAINTAINER_MODE
DISTCLEANFILES =								\
	$(xfce4_panel_built_sources)

BUILT_SOURCES =									\
	$(xfce4_panel_built_sources)
	
panel-dbus-infos.h: $(srcdir)/panel-dbus-infos.xml Makefile
	dbus-binding-tool --prefix=panel_dbus_service --mode=glib-server $(srcdir)/panel-dbus-infos.xml > panel-dbus-infos.h
endif

EXTRA_DIST =									\
	panel-dbus-infos.h

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
