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

if HAVE_CYGWIN
libsample_la_LDFLAGS +=                                                  \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libsample_la_LIBADD +=                                                   \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
