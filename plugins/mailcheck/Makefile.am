plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libmailcheck.la

libmailcheck_la_LDFLAGS =						\
	-avoid-version							\
	-module

libmailcheck_la_SOURCES = 						\
	mailcheck.c

libmailcheck_la_CFLAGS =						\
	@LIBSTARTUP_NOTIFICATION_CFLAGS@				\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libmailcheck_la_LIBADD =						\
	@LIBS@
