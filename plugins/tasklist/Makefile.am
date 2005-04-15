plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libtasklist.la

libtasklist_la_LDFLAGS =						\
	-avoid-version							\
	-module

libtasklist_la_SOURCES = 						\
	tasklist.c

libtasklist_la_CFLAGS =						\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@							\
	-DLOCALEDIR=\"$(localedir)\"

libtasklist_la_LIBADD =						\
	@LIBS@

if HAVE_CYGWIN
libtasklist_la_LDFLAGS +=                                              \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libtasklist_la_LIBADD +=                                               \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
