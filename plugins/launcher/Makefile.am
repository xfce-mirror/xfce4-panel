plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = liblauncher.la

liblauncher_la_LDFLAGS =							\
	-avoid-version							\
	-module

liblauncher_la_SOURCES = 						\
	launcher.c

liblauncher_la_CFLAGS =							\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@							\
	-DLOCALEDIR=\"$(localedir)\"

liblauncher_la_LIBADD =							\
	@LIBS@

if HAVE_CYGWIN
liblauncher_la_LDFLAGS +=                        	                \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
liblauncher_la_LIBADD +=                                 	        \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
