# $Id$

INCLUDES = 									\
	-I$(top_srcdir)								\
	-I$(top_builddir)							\
	-DG_LOG_DOMAIN=\"xfce4-panel-wrapper\"					\
	-DDATADIR=\"$(datadir)\"						\
	-DLIBDIR=\"$(libdir)/xfce4\"						\
	-DLIBEXECDIR=\"$(libexecdir)/xfce4\"					\
	-DSYSCONFDIR=\"$(sysconfdir)\"						\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"					\
	$(PLATFORM_CPPFLAGS)

libexec_PROGRAMS = 								\
	xfce4-panel-wrapper

xfce4_panel_wrapper_SOURCES =							\
	main.c									\
	wrapper-module.c							\
	wrapper-module.h							\
	wrapper-plug.c								\
	wrapper-plug.h

xfce4_panel_wrapper_CFLAGS =							\
	$(GTK_CFLAGS)								\
	$(GMODULE_CFLAGS)							\
	$(GTHREAD_CFLAGS)							\
	$(LIBXFCE4UTIL_CFLAGS)							\
	$(PLATFORM_CFLAGS)

xfce4_panel_wrapper_LDFLAGS =							\
	-no-undefined								\
	$(PLATFORM_LDFLAGS)

xfce4_panel_wrapper_LDADD =							\
	$(top_builddir)/libxfce4panel/libxfce4panel.la				\
	$(GTK_LIBS)								\
	$(GMODULE_LIBS)								\
	$(GTHREAD_LIBS)								\
	$(LIBXFCE4UTIL_LIBS)

xfce4_panel_wrapper_DEPENDENCIES =						\
	$(top_builddir)/libxfce4panel/libxfce4panel.la

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
