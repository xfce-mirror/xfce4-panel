plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libswitcher.la
libswitcher_la_LDFLAGS = "-module"

libswitcher_la_SOURCES = \
switcher.c

libswitcher_la_CFLAGS = -I../../panel @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\"
libswitcher_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
