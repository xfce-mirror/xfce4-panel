plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libsystembuttons.la

libsystembuttons_la_LDFLAGS =						\
	-avoid-version							\
	-module

libsystembuttons_la_SOURCES = 						\
	systembuttons.c

libsystembuttons_la_CFLAGS =						\
	-I$(top_srcdir)/panel						\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libsystembuttons_la_LIBADD =						\
	@LIBS@
