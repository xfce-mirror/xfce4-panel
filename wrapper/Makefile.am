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

xfce4_panel_wrapper_built_sources =						\
	wrapper-dbus-client-infos.h \
	wrapper-marshal.h							\
	wrapper-marshal.c

xfce4_panel_wrapper_SOURCES =							\
	$(xfce4_panel_wrapper_built_sources)					\
	main.c									\
	wrapper-plug.c								\
	wrapper-plug.h

xfce4_panel_wrapper_CFLAGS =							\
	$(GTK_CFLAGS)								\
	$(DBUS_CFLAGS)								\
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
	$(DBUS_LIBS)								\
	$(GMODULE_LIBS)								\
	$(GTHREAD_LIBS)								\
	$(LIBXFCE4UTIL_LIBS)

xfce4_panel_wrapper_DEPENDENCIES =						\
	$(top_builddir)/libxfce4panel/libxfce4panel.la

if MAINTAINER_MODE
wrapper-dbus-client-infos.h: $(top_builddir)/panel/panel-dbus-service-infos.xml Makefile
	dbus-binding-tool --mode=glib-client $< \
	| sed -e 's/_panel_dbus_client/wrapper_dbus_client/g' > $@

wrapper-marshal.h: $(top_builddir)/panel/panel-marshal.list Makefile
	glib-genmarshal --header --prefix=wrapper_marshal $< > $@

wrapper-marshal.c: $(top_builddir)/panel/panel-marshal.list Makefile
	glib-genmarshal --body --prefix=wrapper_marshal $< > $@

BUILT_SOURCES =									\
	$(xfce4_panel_wrapper_built_sources)

DISTCLEANFILES =								\
	$(xfce4_panel_wrapper_built_sources)
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
