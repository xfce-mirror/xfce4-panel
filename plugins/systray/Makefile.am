plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libsystray.la

libsystray_la_LDFLAGS =							\
	-avoid-version							\
	-module

libsystray_la_SOURCES = 						\
	systray.c

libsystray_la_CFLAGS =							\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@							\
	-DLOCALEDIR=\"$(localedir)\"

libsystray_la_LIBADD =							\
	@LIBS@

if HAVE_CYGWIN
libsystray_la_LDFLAGS +=                        	                \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libsystray_la_LIBADD +=                                 	        \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
