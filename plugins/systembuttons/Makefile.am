plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libsystembuttons.la

libsystembuttons_la_LDFLAGS =						\
	-avoid-version							\
	-module

libsystembuttons_la_SOURCES = 						\
	systembuttons.c

libsystembuttons_la_CFLAGS =						\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libsystembuttons_la_LIBADD =						\
	@LIBS@

if HAVE_CYGWIN
libsystembuttons_la_LDFLAGS +=                                          \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libsystembuttons_la_LIBADD +=                                           \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
