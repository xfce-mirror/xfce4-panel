plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libswitcher.la

libswitcher_la_LDFLAGS =						\
	-avoid-version							\
	-module

libswitcher_la_SOURCES =						\
	switcher.c

libswitcher_la_CFLAGS =							\
	-I$(top_srcdir)							\
	@LIBX11_CFLAGS@							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@							\
	-DLOCALEDIR=\"$(localedir)\"

libswitcher_la_LIBADD =							\
	@LIBS@

if HAVE_CYGWIN
libswitcher_la_LDFLAGS +=                                               \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libswitcher_la_LIBADD +=                                                \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
