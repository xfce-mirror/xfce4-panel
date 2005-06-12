plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libanotherclock.la

libanotherclock_la_LDFLAGS =						\
	-avoid-version							\
	-module

libanotherclock_la_SOURCES =						\
	anotherclock.c

libanotherclock_la_CFLAGS =						\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@							\
	-DLOCALEDIR=\"$(localedir)\"

libanotherclock_la_LIBADD = 						\
	@LIBS@

if HAVE_CYGWIN
libanotherclock_la_LDFLAGS +=                                           \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libanotherclock_la_LIBADD +=                                            \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
