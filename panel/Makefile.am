bin_PROGRAMS = xfce4-panel

xfce4_panel_SOURCES =						\
	add-control-dialog.c					\
	add-control-dialog.h					\
	controls.c						\
	controls.h 						\
	controls_dialog.c 					\
	controls_dialog.h 					\
	global.h 						\
	groups.c 						\
	groups.h 						\
	icons.c 						\
	icons.h 						\
	item.c 							\
	item.h 							\
	item_dialog.c 						\
	item_dialog.h 						\
	main.c							\
	main.h 							\
	mcs_client.c 						\
	mcs_client.h 						\
	panel.c 						\
	panel.h 						\
	plugins.h 						\
	popup.c 						\
	popup.h 						\
	settings.c 						\
	settings.h 						\
	xfce.h 							\
	xfce_support.c 						\
	xfce_support.h						\
	xfcombo.c 						\
	xfcombo.h

xfce4_panelincludedir = $(includedir)/xfce4/panel

xfce4_panelinclude_HEADERS =					\
	add-control-dialog.h					\
	controls.h						\
	global.h 						\
	icons.h 						\
	main.h 							\
	panel.h 						\
	item.h							\
	item_dialog.h						\
	plugins.h 						\
	xfce_support.h 						\
	xfce.h 							\
	xfcombo.h

xfce4_panel_CFLAGS = 							\
	-I$(top_srcdir)							\
	@LIBSTARTUP_NOTIFICATION_CFLAGS@				\
	@LIBX11_CFLAGS@							\
	@LIBDBH_CFLAGS@							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4MCS_CLIENT_CFLAGS@					\
	@LIBXML_CFLAGS@							\
	@GTHREAD_CFLAGS@						\
	-DDATADIR=\"$(datadir)/xfce4\"					\
	-DLIBDIR=\"$(libdir)/xfce4\"					\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DLOCALEDIR=\"$(localedir)\"

xfce4_panel_LDFLAGS =							\
	-export-dynamic

xfce4_panel_LDADD =							\
	@LIBSTARTUP_NOTIFICATION_LIBS@					\
	@LIBX11_LIBS@							\
	@GTHREAD_LIBS@							\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4MCS_CLIENT_LIBS@					\
	@LIBXML_LIBS@							\
	@LIBDBH_LIBS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xfce4-panel-1.0.pc
