plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libpager.la
libpager_la_LDFLAGS = "-module -avoid-version"

libpager_la_SOURCES = \
pager.c

libpager_la_CFLAGS = -Wall -I$(top_srcdir) -I$(top_srcdir)/panel  @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\" -DLOCALEDIR=\"$(datadir)/locale\"
libpager_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
