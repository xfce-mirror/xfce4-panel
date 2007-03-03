# $Id$

INCLUDES = 									\
	-I$(top_srcdir)								\
	-I$(top_builddir)							\
	-DG_LOG_DOMAIN=\"xfce4-panel\"						\
	-DDATADIR=\"$(datadir)\"						\
	-DLIBDIR=\"$(libdir)/xfce4\"						\
	-DLIBEXECDIR=\"$(libexecdir)/xfce4\"					\
	-DSYSCONFDIR=\"$(sysconfdir)\"						\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"					\
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = 									\
	xfce4-panel

xfce4_panel_SOURCES =								\
	main.c									\
	panel.c									\
	panel.h									\
	panel-app.c								\
	panel-app.h								\
	panel-app-messages.c							\
	panel-app-messages.h							\
	panel-config.c								\
	panel-config.h								\
	panel-dnd.c								\
	panel-dnd.h								\
	panel-item-manager.c							\
	panel-item-manager.h							\
	panel-private.h								\
	panel-properties.c							\
	panel-properties.h							\
	panel-dialogs.c								\
	panel-dialogs.h								\
	frap-icon-entry.c							\
	frap-icon-entry.h

xfce4_panel_CFLAGS =								\
	$(LIBX11_CFLAGS)							\
	$(LIBSTARTUP_NOTIFICATION_CFLAGS)					\
	$(LIBXFCE4UTIL_CFLAGS)							\
	$(LIBXFCEGUI4_CFLAGS)							\
	$(PLATFORM_CFLAGS)

xfce4_panel_LDFLAGS =								\
	-no-undefined								\
	$(PLATFORM_LDFLAGS)

xfce4_panel_LDADD =								\
	$(top_builddir)/libxfce4panel/libxfce4panel.la				\
	$(LIBXFCE4UTIL_LIBS)							\
	$(LIBXFCEGUI4_LIBS)							\
	$(LIBSTARTUP_NOTIFICATION_LIBS)						\
	$(LIBX11_LIBS)

xfce4_panel_DEPENDENCIES =							\
	$(top_builddir)/libxfce4panel/libxfce4panel.la

#
# Dummy Scripts
#
if ENABLE_DUMMY_SCRIPTS
bin_SCRIPTS =									\
	xftaskbar4								\
	xfce4-iconbox

EXTRA_DIST =									\
	xftaskbar4								\
	xfce4-iconbox
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
