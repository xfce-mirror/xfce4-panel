plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libswitcher.la
libswitcher_la_LDFLAGS = "-module -avoid-version"

libswitcher_la_SOURCES = \
switcher.c

libswitcher_la_CFLAGS = -Wall -I$(top_srcdir) -I$(top_srcdir)/panel  @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\" -DLOCALEDIR=\"$(datadir)/locale\"
libswitcher_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
