bin_PROGRAMS = xfce4-panel

xfce4_panel_SOURCES =						\
	frap-icon-entry.c					\
	frap-icon-entry.h					\
	main.c							\
	panel-app.c						\
	panel-app.h						\
	panel-app-messages.c					\
	panel-app-messages.h					\
	panel-config.c						\
	panel-config.h						\
	panel-dnd.c						\
	panel-dnd.h						\
	panel-item-manager.c					\
	panel-item-manager.h					\
	panel-properties.c					\
	panel-properties.h					\
	panel-dialogs.c					\
	panel-dialogs.h					\
	panel.c							\
	panel-private.h						\
	panel.h


xfce4_panel_CFLAGS = 							\
	-I$(top_srcdir)							\
	@LIBSTARTUP_NOTIFICATION_CFLAGS@				\
	@LIBX11_CFLAGS@							\
	@LIBXFCE4UTIL_CFLAGS@						\
	@LIBXFCEGUI4_CFLAGS@						\
	-DDATADIR=\"$(datadir)\"					\
	-DLIBDIR=\"$(libdir)/xfce4\"					\
	-DLIBEXECDIR=\"$(libexecdir)/xfce4\"				\
	-DSYSCONFDIR=\"$(sysconfdir)\"					\
	-DLOCALEDIR=\"$(localedir)\"

xfce4_panel_LDFLAGS =							\
	-export-dynamic

if HAVE_CYGWIN
xfce4_panel_LDFLAGS += -no-undefined
endif

xfce4_panel_LDADD =							\
	../libxfce4panel/libxfce4panel.la				\
	@LIBSTARTUP_NOTIFICATION_LIBS@					\
	@LIBX11_LIBS@							\
	@LIBXFCE4UTIL_LIBS@						\
	@LIBXFCEGUI4_LIBS@

xfce4_panel_DEPENDENCIES =						\
	../libxfce4panel/libxfce4panel.la

