plugindir = $(pkglibdir)/panel-plugins

plugin_LTLIBRARIES = libclock.la

libclock_la_LDFLAGS =							\
	-avoid-version							\
	-module

libclock_la_SOURCES =							\
	clock.c

libclock_la_CFLAGS =							\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libclock_la_LIBADD = 							\
	@LIBS@
