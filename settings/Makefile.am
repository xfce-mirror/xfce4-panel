plugindir = @XFCE_MCS_MANAGER_PLUGINSDIR@

plugin_LTLIBRARIES = libxfce4settings.la

libxfce4settings_la_LDFLAGS =						\
	-avoid-version							\
	-module

libxfce4settings_la_SOURCES =						\
	xfce_settings.h							\
	xfce_settings_dialog.c						\
	xfce_settings_dialog.h						\
	xfce_settings_plugin.c						\
	xfce_settings_plugin.h

libxfce4settings_la_CFLAGS =						\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@XFCE_MCS_MANAGER_CFLAGS@					\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DDATADIR=\"$(datadir)/xfce4\"					\
	-DLOCALEDIR=\"$(localedir)\"

libxfce4settings_la_LIBADD =						\
	@XFCE_MCS_MANAGER_LIBS@

appsdir = $(datadir)/applications
apps_DATA = xfce-panel-settings.desktop
