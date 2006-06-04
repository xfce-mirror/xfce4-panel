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
	plugin.c

libxfce4settings_la_CFLAGS =						\
	-I$(top_srcdir)							\
	-DLOCALEDIR=\"$(localedir)\"					\
	@LIBXFCEGUI4_CFLAGS@						\
	@XFCE_MCS_MANAGER_CFLAGS@

libxfce4settings_la_LIBADD =						\
	@XFCE_MCS_MANAGER_LIBS@

# .desktop file
#
desktop_in_files = xfce4-panel-manager.desktop.in
desktop_files = $(desktop_in_files:.desktop.in=.desktop)
	
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_files)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_files)

DISTCLEANFILES =							\
	$(desktop_files)
	
