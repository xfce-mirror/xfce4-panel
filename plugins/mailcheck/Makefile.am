plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libmailcheck.la
libmailcheck_la_LDFLAGS = "-module"

libmailcheck_la_SOURCES = \
mailcheck.c

libmailcheck_la_CFLAGS = -Wall -I../../panel @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\"
libmailcheck_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
