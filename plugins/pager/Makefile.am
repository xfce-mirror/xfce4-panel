plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libpager.la

libpager_la_LDFLAGS =							\
	-avoid-version							\
	-module

libpager_la_SOURCES = 							\
	pager.c

libpager_la_CFLAGS =							\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libpager_la_LIBADD =							\
	@LIBS@

if HAVE_CYGWIN
libpager_la_LDFLAGS +=                                                  \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libpager_la_LIBADD +=                                                   \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
