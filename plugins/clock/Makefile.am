plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libclock.la

libclock_la_LDFLAGS =							\
	-avoid-version							\
	-module

libclock_la_SOURCES =							\
	clock.c

libclock_la_CFLAGS =							\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@							\
	-DLOCALEDIR=\"$(localedir)\"

libclock_la_LIBADD = 							\
	@LIBS@

if HAVE_CYGWIN
libclock_la_LDFLAGS +=                                                  \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libclock_la_LIBADD +=                                                   \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
