plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libmailcheck.la
libmailcheck_la_LDFLAGS = "-module -avoid-version"

libmailcheck_la_SOURCES = \
mailcheck.c

libmailcheck_la_CFLAGS = -Wall -I$(top_srcdir) -I$(top_srcdir)/panel  @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ @STARTUP_NOTIFICATION_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\" -DLOCALEDIR=\"$(datadir)/locale\"
libmailcheck_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@ @STARTUP_NOTIFICATION_LIBS@
