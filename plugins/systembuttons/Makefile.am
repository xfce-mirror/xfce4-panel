plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = libsystembuttons.la
libsystembuttons_la_LDFLAGS = "-module -avoid-version"

libsystembuttons_la_SOURCES = \
systembuttons.c

libsystembuttons_la_CFLAGS = -Wall -I$(top_srcdir) -I$(top_srcdir)/panel  @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\" -DLOCALEDIR=\"$(datadir)/locale\"
libsystembuttons_la_LIBADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
