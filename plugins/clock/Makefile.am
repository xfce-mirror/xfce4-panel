plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libclock.la
libclock_la_LDFLAGS = "-module"

libclock_la_SOURCES = \
clock.c

libclock_la_CFLAGS = -I../../panel @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\"
libclock_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
