plugindir = $(libdir)/xfce4/mcs-plugins

plugin_LTLIBRARIES = libxfce4settings.la

libxfce4settings_la_LDFLAGS =						\
	-avoid-version							\
	-module

if HAVE_CYGWIN
libxfce4settings_la_LDFLAGS +=                                          \
	-no-undefined                                                   \
	-export-symbols $(datadir)/xfce4/devel/mcs-manager.def
endif

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

xftaskbar_plugindir = $(libdir)/xfce4/mcs-plugins

xftaskbar_plugin_LTLIBRARIES = xftaskbar_plugin.la

xftaskbar_plugin_la_LDFLAGS =						\
	-avoid-version							\
	-module

if HAVE_CYGWIN
xftaskbar_plugin_la_LDFLAGS +=                                          \
	-no-undefined                                                   \
	-export-symbols $(datadir)/xfce4/devel/mcs-manager.def
endif

xftaskbar_plugin_la_SOURCES =						\
	xftaskbar_plugin.c

xftaskbar_plugin_la_CFLAGS = 						\
	@XFCE_MCS_MANAGER_CFLAGS@ 					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				

xftaskbar_plugin_la_LIBADD = 						\
	@XFCE_MCS_MANAGER_LIBS@

appsdir = $(datadir)/applications
apps_DATA = 								\
	xfce-panel-settings.desktop					\
	xfce-taskbar-settings.desktop

EXTRA_DIST = $(apps_DATA)

