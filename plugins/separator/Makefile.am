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

if HAVE_CYGWIN
libseparator_la_LDFLAGS +=                                              \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libseparator_la_LIBADD +=                                               \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
