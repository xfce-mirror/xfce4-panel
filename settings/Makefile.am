plugindir = $(libdir)/libxfce4mcs/settings/

plugin_LTLIBRARIES = libxfce4settings.la
libxfce4settings_la_LDFLAGS = "-module"

libxfce4settings_la_SOURCES = \
xfce_settings.h \
xfce_settings_dialog.c \
xfce_settings_dialog.h \
xfce_settings_plugin.c \
xfce_settings_plugin.h

libxfce4settings_la_CFLAGS = -Wall -I../../panel @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ @MCS_MANAGER_CFLAGS@ -DG_DISABLE_DEPRECATED -DSYSCONFDIR=\"$(sysconfdir)\" -DDATADIR=\"$(pkgdatadir)\" 
libxfce4settings_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@ @MCS_MANAGER_LIBS@ 
