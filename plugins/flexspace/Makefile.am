plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libflexspace.la

libflexspace_la_LDFLAGS =						\
	-avoid-version							\
	-module

libflexspace_la_SOURCES = 						\
	flexspace.c

libflexspace_la_CFLAGS =						\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@							\
	-DLOCALEDIR=\"$(localedir)\"

libflexspace_la_LIBADD =						\
	@LIBS@

if HAVE_CYGWIN
libflexspace_la_LDFLAGS +=                                              \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libflexspace_la_LIBADD +=                                               \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
