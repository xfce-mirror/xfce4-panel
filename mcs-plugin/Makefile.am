# $Id$

INCLUDES = 								\
	-I$(top_srcdir)							\
	-DBINDIR=\"$(bindir)\"						\
	-DDATADIR=\"$(datadir)\"					\
	-DG_LOG_DOMAIN=\"panel-mcs-plugin\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(PLATFORM_CPPFLAGS)

#
# build mcs wrapper plugin if xfce-mcs-manager is present.
#
if HAVE_XFCE_MCS_MANAGER
plugindir =								\
	$(libdir)/xfce4/mcs-plugins

plugin_LTLIBRARIES =							\
	libxfce4settings.la

libxfce4settings_la_SOURCES =						\
	mcs_plugin.c

libxfce4settings_la_CFLAGS =						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(XFCE_MCS_MANAGER_CFLAGS)					\
	$(PLATFORM_CFLAGS)

libxfce4settings_la_LDFLAGS =						\
	-avoid-version							\
	-no-undefined							\
	-module								\
	$(PLATFORM_LDFLAGS)

libxfce4settings_la_LIBADD =						\
	$(LIBXFCEGUI4_LIBS)						\
	$(XFCE_MCS_MANAGER_LIBS)
endif

#
# .desktop file
#
desktopdir = $(datadir)/applications
desktop_in_files =							\
	xfce4-panel-manager.desktop.in
desktop_DATA =								\
	$(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

CLEANFILES =								\
	$(desktop_DATA)

EXTRA_DIST =								\
	$(desktop_in_files)

