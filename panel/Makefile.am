bin_PROGRAMS = xfce4-panel

if HAVE_CYGWIN
develdir   = $(datadir)/xfce4/devel
devel_DATA = panel.def
endif

xfce4_panel_SOURCES =						\
	add-control-dialog.c					\
	add-control-dialog.h					\
	controls.c						\
	controls.h 						\
	controls_dialog.c 					\
	controls_dialog.h 					\
	global.h 						\
	icons.c 						\
	icons.h 						\
	item-control.c 							\
	item-control.h 							\
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
	settings.c 						\
	settings.h 						\
	xfce.h 							\
	xfce_support.c 						\
	xfce_support.h

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
	xfce.h

xfce4_panel_CFLAGS = 							\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/libs						\
	@LIBSTARTUP_NOTIFICATION_CFLAGS@				\
	@LIBX11_CFLAGS@							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4MCS_CLIENT_CFLAGS@					\
	@LIBXML_CFLAGS@							\
	-DDATADIR=\"$(datadir)/xfce4\"					\
	-DLIBDIR=\"$(libdir)/xfce4\"					\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DLOCALEDIR=\"$(localedir)\"

xfce4_panel_LDFLAGS =							\
	-export-dynamic

if HAVE_CYGWIN
xfce4_panel_LDFLAGS += -no-undefined
endif

xfce4_panel_LDADD =							\
	$(top_srcdir)/libs/libxfce4panel.la				\
	@LIBSTARTUP_NOTIFICATION_LIBS@					\
	@LIBX11_LIBS@							\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4MCS_CLIENT_LIBS@					\
	@LIBXML_LIBS@

xfce4_panel_DEPENDENCIES =						\
	$(top_srcdir)/libs/libxfce4panel.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xfce4-panel-1.0.pc

if HAVE_CYGWIN
panel.def: $(xfce4_panel_SOURCES) create-def-file.sh
	sh create-def-file.sh $(xfce4_panel_SOURCES)

EXTRA_DIST = $(devel_DATA) create-def-file.sh
endif
