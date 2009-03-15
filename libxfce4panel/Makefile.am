# $Id: Makefile.am 24484 2007-01-15 17:20:39Z nick $

INCLUDES = 								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"libxfce4panel\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(PLATFORM_CPPFLAGS)

lib_LTLIBRARIES =							\
	libxfce4panel.la

libxfce4panel_public_built_sources =					\
	libxfce4panel-enum-types.h

libxfce4panel_built_sources = 						\
	$(libxfce4panel_public_built_sources)				\
	libxfce4panel-enum-types.c					\
	libxfce4panel-alias.h						\
	libxfce4panel-aliasdef.c					\
	libxfce4panel-marshal.c						\
	libxfce4panel-marshal.h

libxfce4panel_headers =							\
	libxfce4panel.h							\
	xfce-panel-window.h						\
	xfce-arrow-button.h						\
	xfce-itembar.h							\
	xfce-hvbox.h							\
	xfce-panel-enums.h						\
	xfce-panel-convenience.h					\
	xfce-panel-plugin-iface.h					\
	xfce-panel-internal-plugin.h					\
	xfce-panel-external-plugin.h					\
	xfce-panel-macros.h						\
	xfce-panel-plugin.h

libxfce4panel_includedir =						\
	$(includedir)/xfce4/libxfce4panel

libxfce4panel_include_HEADERS =						\
	$(libxfce4panel_public_built_sources)				\
	$(libxfce4panel_headers)

libxfce4panel_la_SOURCES =						\
	$(libxfce4panel_built_sources)					\
	$(libxfce4panel_headers)					\
	xfce-panel-window.c						\
	xfce-arrow-button.c						\
	xfce-itembar.c							\
	xfce-hvbox.c							\
	xfce-panel-convenience.c					\
	xfce-panel-external-item.c					\
	xfce-panel-external-item.h					\
	xfce-panel-external-plugin.c					\
	xfce-panel-internal-plugin.c					\
	xfce-panel-item-iface.c						\
	xfce-panel-item-iface.h						\
	xfce-panel-plugin-iface.c					\
	xfce-panel-plugin-iface-private.h				\
	xfce-panel-macros.c						\
	xfce-panel-plugin-messages.c					\
	xfce-panel-plugin-messages.h

libxfce4panel_la_CFLAGS =						\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(GTK_CFLAGS)							\
	$(PLATFORM_CFLAGS)

libxfce4panel_la_LDFLAGS = 						\
	-export-dynamic							\
	-export-symbols-regex "^[^_].*"					\
	-version-info $(LIBXFCE4PANEL_VERINFO)				\
	$(PLATFORM_LDFLAGS)

if HAVE_CYGWIN
libxfce4panel_la_LDFLAGS += 						\
	-no-undefined
endif

libxfce4panel_la_LIBADD = 						\
	-lX11								\
	$(GTK_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)

##
## Pkg-config information
##
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfce4panel-1.0.pc

CLEANFILES =								\
	actual-abi							\
	expected-abi							\
	xgen-etc							\
	xgen-eth							\
	xgen-mh								\
	xgen-mc

EXTRA_DIST = 								\
	abicheck.sh							\
	make-libxfce4panel-alias.pl					\
	libxfce4panel.symbols						\
	libxfce4panel-alias.h						\
	libxfce4panel-aliasdef.c					\
	libxfce4panel-marshal.list

##
## Rules to auto-generate built sources
##
if MAINTAINER_MODE
DISTCLEANFILES =							\
	stamp-libxfce4panel-enum-types.h				\
	stamp-libxfce4panel-marshal.h					\
	$(libxfce4panel_built_sources)

BUILT_SOURCES =								\
	$(libxfce4panel_built_sources)

if HAVE_GNUC_VISIBILITY
TESTS =									\
	abicheck.sh
endif

libxfce4panel-alias.h: make-libxfce4panel-alias.pl libxfce4panel.symbols
	$(PERL) $(srcdir)/make-libxfce4panel-alias.pl < $(srcdir)/libxfce4panel.symbols > libxfce4panel-alias.h

libxfce4panel-aliasdef.c: make-libxfce4panel-alias.pl libxfce4panel.symbols
	$(PERL) $(srcdir)/make-libxfce4panel-alias.pl -def < $(srcdir)/libxfce4panel.symbols > libxfce4panel-aliasdef.c

libxfce4panel-enum-types.h: stamp-libxfce4panel-enum-types.h
	@true
stamp-libxfce4panel-enum-types.h: $(libxfce4panel_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __LIBXFCE4PANEL_ENUM_TYPES_H__\n#define __LIBXFCE4PANEL_ENUM_TYPES_H__\n#include <libxfce4util/libxfce4util.h>\nG_BEGIN_DECLS\n" 		\
		--fprod "/* enumerations from \"@filename@\" */\n#include <libxfce4panel/@filename@>\n" \
		--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define XFCE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* !__LIBXFCE4PANEL_ENUM_TYPES_H__ */" \
		$(libxfce4panel_headers) ) >> xgen-eth \
	&& (cmp -s xgen-eth libxfce4panel-enum-types.h || cp xgen-eth libxfce4panel-enum-types.h ) \
	&& rm -f xgen-eth \
	&& echo timestamp > $(@F)

libxfce4panel-enum-types.c: $(libxfce4panel_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include <libxfce4panel-enum-types.h>\n#include <libxfce4panel/libxfce4panel-alias.h>" \
		--fprod "\n/* enumerations from \"@filename@\" */\n" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {" \
		--vprod "\t{ @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "\t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n\treturn type;\n}\n" \
		--ftail "\n#define __LIBXFCE4PANEL_ENUM_TYPES_C__\n#include \"libxfce4panel-aliasdef.c\"\n" \
		$(libxfce4panel_headers) ) >> xgen-etc \
	&& cp xgen-etc libxfce4panel-enum-types.c \
	&& rm -f xgen-etc

libxfce4panel-marshal.h: stamp-libxfce4panel-marshal.h
	@true
stamp-libxfce4panel-marshal.h: $(srcdir)/libxfce4panel-marshal.list Makefile
	( cd $(srcdir) && glib-genmarshal \
		--prefix="_libxfce4panel_marshal" \
		--header libxfce4panel-marshal.list \
		| sed -e 's/marshal_data);$$/marshal_data) G_GNUC_INTERNAL;/' ) >> xgen-mh \
	&& ( cmp -s xgen-mh libxfce4panel-marshal.h || cp xgen-mh libxfce4panel-marshal.h ) \
	&& rm -f xgen-mh \
	&& echo timestamp > $(@F)

libxfce4panel-marshal.c: libxfce4panel-marshal.h Makefile
	echo "#include <libxfce4panel/libxfce4panel-marshal.h>" > xgen-mc \
	&& ( cd $(srcdir) && glib-genmarshal \
		--prefix="_libxfce4panel_marshal" \
		--body libxfce4panel-marshal.list ) >> xgen-mc \
	&& cp xgen-mc libxfce4panel-marshal.c \
	&& rm -f xgen-mc
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
