plugindir = $(pkglibdir)/panel-plugins

plugin_LTLIBRARIES = libpager.la

libpager_la_LDFLAGS =							\
	-avoid-version							\
	-module

libpager_la_SOURCES = 							\
	pager.c

libpager_la_CFLAGS =							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libpager_la_LIBADD =							\
	@LIBS@
