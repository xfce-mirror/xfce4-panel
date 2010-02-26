# $Id$

INCLUDES = 								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"libxfce4panel\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DLIBXFCE4PANEL_COMPILATION					\
	$(PLATFORM_CPPFLAGS)

lib_LTLIBRARIES =							\
	libxfce4panel.la

libxfce4panel_headers =							\
	libxfce4panel.h							\
	xfce-arrow-button.h						\
	xfce-hvbox.h							\
	xfce-panel-convenience.h					\
	xfce-panel-macros.h						\
	xfce-panel-plugin.h

libxfce4panel_includedir =						\
	$(includedir)/xfce4/libxfce4panel

libxfce4panel_include_HEADERS =						\
	$(libxfce4panel_headers)

libxfce4panel_la_SOURCES =						\
	$(libxfce4panel_headers)					\
	xfce-arrow-button.c						\
	xfce-hvbox.c							\
	xfce-panel-convenience.c					\
	xfce-panel-plugin.c						\
	xfce-panel-plugin-provider.c					\
	xfce-panel-plugin-provider.h

libxfce4panel_la_CFLAGS =						\
	$(GTK_CFLAGS)							\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libxfce4panel_la_LDFLAGS = 						\
	-no-undefined							\
	-export-dynamic							\
	-export-symbols-regex "^[^_].*"					\
	-version-info $(LIBXFCE4PANEL_VERINFO)				\
	$(PLATFORM_LDFLAGS)

libxfce4panel_la_LIBADD = 						\
	$(GTK_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)

#
# Pkg-config file
#
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfce4panel-1.0.pc

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
