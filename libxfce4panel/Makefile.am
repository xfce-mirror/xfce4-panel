# $Id$

INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libxfce4panel\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DLIBXFCE4PANEL_COMPILATION \
	$(PLATFORM_CPPFLAGS)

lib_LTLIBRARIES = \
	libxfce4panel.la

libxfce4panel_built_sources = \
	libxfce4panel-alias.h \
	libxfce4panel-aliasdef.c \
	libxfce4panel-marshal.c \
	libxfce4panel-marshal.h

libxfce4panel_headers = \
	libxfce4panel.h \
	xfce-arrow-button.h \
	xfce-hvbox.h \
	xfce-panel-convenience.h \
	xfce-panel-enums.h \
	xfce-panel-macros.h \
	xfce-panel-plugin.h \
	xfce-panel-image.h

libxfce4panel_includedir = \
	$(includedir)/xfce4/libxfce4panel

libxfce4panel_include_HEADERS = \
	$(libxfce4panel_headers)

libxfce4panel_la_SOURCES = \
	$(libxfce4panel_built_sources) \
	$(libxfce4panel_headers) \
	xfce-arrow-button.c \
	xfce-hvbox.c \
	xfce-panel-convenience.c \
	xfce-panel-plugin.c \
	xfce-panel-plugin-provider.c \
	xfce-panel-plugin-provider.h \
	xfce-panel-image.c

libxfce4panel_la_CFLAGS =	 \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfce4panel_la_LDFLAGS = \
	-no-undefined \
	-export-dynamic \
	-version-info $(LIBXFCE4PANEL_VERINFO) \
	-export-symbols-regex '^[^_].*' \
	$(PLATFORM_LDFLAGS)

libxfce4panel_la_LIBADD = \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS)

#
# Pkg-config file
#
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfce4panel-1.0.pc

#
# Build sources
#
if MAINTAINER_MODE
libxfce4panel-marshal.h: libxfce4panel-marshal.list Makefile
	$(AM_V_GEN) glib-genmarshal --prefix=_libxfce4panel_marshal --internal --header $< > $@

libxfce4panel-marshal.c: libxfce4panel-marshal.list Makefile
	$(AM_V_GEN) echo "#include <libxfce4panel/libxfce4panel-marshal.h>" > $@ \
	&& glib-genmarshal --prefix=_libxfce4panel_marshal --body $< >> $@

libxfce4panel-alias.h: make-libxfce4panel-alias.pl libxfce4panel.symbols
	$(AM_V_GEN) $(PERL) $(srcdir)/make-libxfce4panel-alias.pl $(srcdir)/libxfce4panel.symbols > $@

libxfce4panel-aliasdef.c: make-libxfce4panel-alias.pl libxfce4panel.symbols
	$(AM_V_GEN) $(PERL) $(srcdir)/make-libxfce4panel-alias.pl -def < $(srcdir)/libxfce4panel.symbols > $@

DISTCLEANFILES = \
	$(libxfce4panel_built_sources)

BUILT_SOURCES = \
	$(libxfce4panel_built_sources)

CLEANFILES = \
	actual-abi \
	expected-abi
endif

EXTRA_DIST = \
	abicheck.sh \
	libxfce4panel.symbols \
	libxfce4panel-alias.h \
	libxfce4panel-aliasdef.c \
	libxfce4panel-marshal.list \
	make-libxfce4panel-alias.pl

if HAVE_GNUC_VISIBILITY
TESTS =	abicheck.sh
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
