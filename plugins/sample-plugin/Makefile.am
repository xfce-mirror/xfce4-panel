noinst_LTLIBRARIES = libsample.la

libsample_la_LDFLAGS =						\
	-avoid-version						\
	-module

libsample_la_SOURCES = 						\
	sample.c

libsample_la_CFLAGS =						\
	-I$(top_srcdir)						\
	@LIBXFCEGUI4_CFLAGS@					\
	@LIBXML_CFLAGS@

libsample_la_LIBADD =						\
	@LIBS@

