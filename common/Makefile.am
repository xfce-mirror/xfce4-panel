
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libpanel-common\" \
	-DLIBXFCE4PANEL_COMPILATION \
	-DHELPDIR=\"$(docdir)/html\" \
	$(PLATFORM_CPPFLAGS)

noinst_LTLIBRARIES = \
	libpanel-common.la

libpanel_common_la_SOURCES = \
	panel-debug.c \
	panel-debug.h \
	panel-utils.c \
	panel-utils.h \
	panel-xfconf.c \
	panel-xfconf.h

libpanel_common_la_CFLAGS = \
	$(XFCONF_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4WINDOWING_CFLAGS) \
	$(WAYLAND_CLIENT_CFLAGS) \
	$(PLATFORM_CFLAGS)

libpanel_common_la_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libpanel_common_la_LIBADD = \
	$(XFCONF_LIBS) \
	$(GTK_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4WINDOWING_LIBS) \
	$(WAYLAND_CLIENT_LIBS)

libpanel_common_resources_deps = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir)/resources $(srcdir)/resources/xfce4-panel.gresource.xml)
 
xfce4-panel-resources.c: $(srcdir)/resources/xfce4-panel.gresource.xml $(libpanel_common_resources_deps) Makefile
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/resources --generate-source $<

BUILT_SOURCES = \
	xfce4-panel-resources.c \
	$(NULL)

DISTCLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

resources = \
	resources/actions-dialog.glade \
	resources/applicationsmenu-dialog.glade \
	resources/clock-dialog.glade \
	resources/directorymenu-dialog.glade \
	resources/launcher-dialog.glade \
	resources/pager-dialog.glade \
	resources/panel-preferences-dialog.glade \
	resources/separator-dialog.glade \
	resources/showdesktop-dialog.glade \
	resources/sn-dialog.glade \
	resources/tasklist-dialog.glade \
	resources/windowmenu-dialog.glade \
	resources/xfce4-panel.gresource.xml \
	$(NULL)

EXTRA_DIST = \
	panel-dbus.h \
	panel-private.h \
	meson.build \
	$(resources) \
	$(NULL)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
