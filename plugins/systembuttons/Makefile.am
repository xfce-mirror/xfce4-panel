plugindir = $(pkglibdir)/panel-plugins

plugin_LTLIBRARIES = libsystembuttons.la

libsystembuttons_la_LDFLAGS =						\
	-avoid-version							\
	-module

libsystembuttons_la_SOURCES = 						\
	systembuttons.c

libsystembuttons_la_CFLAGS =						\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libsystembuttons_la_LIBADD =						\
	@LIBS@
