plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libseparator.la

libseparator_la_LDFLAGS =						\
	-avoid-version							\
	-module

libseparator_la_SOURCES = 						\
	separator.c

libseparator_la_CFLAGS =						\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libseparator_la_LIBADD =						\
	@LIBS@
