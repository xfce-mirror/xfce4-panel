plugindir = $(pkgdatadir)/plugins

plugin_LTLIBRARIES = libtrash.la
libtrash_la_LDFLAGS = "-module"

libtrash_la_SOURCES = \
trash.c

libtrash_la_CFLAGS = -I../../panel @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\"
libtrash_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
