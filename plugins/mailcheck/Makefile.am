plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libmailcheck.la

libmailcheck_la_LDFLAGS =						\
	-avoid-version							\
	-module

libmailcheck_la_SOURCES = 						\
	mailcheck.c

libmailcheck_la_CFLAGS =						\
	-I$(top_srcdir)							\
	@LIBSTARTUP_NOTIFICATION_CFLAGS@				\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXML_CFLAGS@

libmailcheck_la_LIBADD =						\
	@LIBS@

if HAVE_CYGWIN
libmailcheck_la_LDFLAGS +=                                              \
	-no-undefined                                                   \
	-export-symbols $(top_srcdir)/panel/panel.def                   \
	@LIBXML_LDFLAGS@                                                \
	@LIBX11_LDFLAGS@                                                
libmailcheck_la_LIBADD +=                                               \
	@XFCE_MCS_MANAGER_LIBS@                                         \
	@LIBXML_LIBS@                                                   \
	@LIBX11_LIBS@
endif
