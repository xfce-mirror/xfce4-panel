bin_PROGRAMS = xfce4-panel

xfce4_panel_SOURCES =							\
	controls.c							\
	controls.h 							\
	controls_dialog.c 						\
	controls_dialog.h 						\
	global.h 							\
	groups.c 							\
	groups.h 							\
	icons.c 							\
	icons.h 							\
	item.c 								\
	item.h 								\
	item_dialog.c 							\
	item_dialog.h 							\
	main.c								\
	main.h 								\
	mcs_client.c 							\
	mcs_client.h 							\
	panel.c 							\
	panel.h 							\
	plugins.h 							\
	popup.c 							\
	popup.h 							\
	settings.c 							\
	settings.h 							\
	xfce.h 								\
	xfce_support.c 							\
	xfce_support.h 

pkginclude_HEADERS = 							\
	controls.h							\
	global.h 							\
	icons.h 							\
	main.h 								\
	panel.h 							\
	plugins.h 							\
	xfce_support.h 							\
	xfce.h 

xfce4_panel_CFLAGS = 							\
	-I$(top_srcdir)							\
	@GLIB_CFLAGS@							\
	@GTK_CFLAGS@							\
	@LIBSTARTUP_NOTIFICATION_CFLAGS@				\
	@LIBX11_CFLAGS@							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4MCS_CLIENT_CFLAGS@					\
	@LIBXFCE4UTIL_CFLAGS@						\
	@LIBXML_CFLAGS@							\
	-DDATADIR=\"$(pkgdatadir)\"					\
	-DLIBDIR=\"$(pkglibdir)\"					\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DLOCALEDIR=\"$(localedir)\"

xfce4_panel_LDFLAGS =							\
	-export-dynamic

xfce4_panel_LDADD =							\
	@LIBS@								\
	@GLIB_LIBS@							\
	@GTK_LIBS@							\
	@LIBSTARTUP_NOTIFICATION_LIBS@					\
	@LIBX11_LIBS@							\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4MCS_CLIENT_LIBS@					\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBXML_LIBS@
