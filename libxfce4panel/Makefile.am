# $Id: Makefile.am 24484 2007-01-15 17:20:39Z nick $

INCLUDES = 								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"libxfce4panel\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(PLATFORM_CPPFLAGS)

lib_LTLIBRARIES =							\
	libxfce4panel.la

libxfce4panel_la_SOURCES =						\
	$(generated_files)						\
	xfce-panel-window.c						\
	xfce-panel-window.h						\
	xfce-arrow-button.c						\
	xfce-arrow-button.h						\
	xfce-itembar.c							\
	xfce-itembar.h							\
	xfce-hvbox.c							\
	xfce-hvbox.h							\
	xfce-panel-enums.h						\
	xfce-panel-convenience.c					\
	xfce-panel-convenience.h					\
	xfce-panel-external-item.c					\
	xfce-panel-external-item.h					\
	xfce-panel-external-plugin.c					\
	xfce-panel-external-plugin.h					\
	xfce-panel-internal-plugin.c					\
	xfce-panel-internal-plugin.h					\
	xfce-panel-item-iface.c						\
	xfce-panel-item-iface.h						\
	xfce-panel-plugin-iface.c					\
	xfce-panel-plugin-iface.h					\
	xfce-panel-plugin-iface-private.h				\
	xfce-panel-macros.c						\
	xfce-panel-plugin-messages.c					\
	xfce-panel-plugin-messages.h

xfce4_panelincludedir =							\
	$(includedir)/xfce4/libxfce4panel

xfce4_panelinclude_HEADERS =						\
	xfce-panel-enum-types.h						\
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

libxfce4panel_la_CFLAGS =						\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(GTK_CFLAGS)							\
	$(PLATFORM_CFLAGS)

libxfce4panel_la_LDFLAGS = 						\
	-export-dynamic							\
	-version-info $(LIBXFCE4PANEL_VERINFO)				\
	$(PLATFORM_LDFLAGS)

if HAVE_CYGWIN
libxfce4panel_la_LDFLAGS += 						\
	-no-undefined
endif

libxfce4panel_la_LIBADD = 						\
	$(GTK_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)

pkgconfigdir =								\
	$(libdir)/pkgconfig

pkgconfig_DATA =							\
	libxfce4panel-1.0.pc

generated_files = 							\
	xfce-panel-enum-types.c						\
	xfce-panel-enum-types.h						\
	xfce-marshal.c							\
	xfce-marshal.h

EXTRA_DIST = 								\
	xfce-marshal.list						\
	$(generated_files)

DISTCLEANFILES =							\
	$(generated_files)

xfce_headers =								\
	$(srcdir)/xfce-panel-window.h					\
	$(srcdir)/xfce-panel-enums.h

xfce-panel-enum-types.h: $(xfce_headers)
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __XFCE_PANEL_ENUM_TYPES_H__\n#define __XFCE_PANEL_ENUM_TYPES_H__\n#include <libxfce4util/libxfce4util.h>\nG_BEGIN_DECLS\n" 		\
		--fprod "/* enumerations from \"@filename@\" */\n#include <libxfce4panel/@filename@>\n" \
		--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define XFCE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* !__XFCE_PANEL_ENUM_TYPES_H__ */" \
		$(xfce_headers) ) >> xgen \
	&& (cmp -s xgen xfce-panel-enum-types.h || cp xgen xfce-panel-enum-types.h ) \
	&& rm -f xgen xgen~

xfce-panel-enum-types.c: xfce-panel-enum-types.h
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include <xfce-panel-enum-types.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {" \
		--vprod "\t{ @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "\t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n\treturn type;\n}\n" \
		$(xfce_headers) ) > xgen \
	&& cp xgen xfce-panel-enum-types.c \
	&& rm -f xgen xgen~

xfce-marshal.h: $(srcdir)/xfce-marshal.list
	( cd $(srcdir) && glib-genmarshal \
		--prefix="_xfce_marshal" \
		--header xfce-marshal.list \
		| sed -e 's/marshal_data);$$/marshal_data) G_GNUC_INTERNAL;/' ) > xgen-xpmh \
	&& cp xgen-xpmh $(@F) \
        && rm -f xgen-xpmh xgen-xpmh~

xfce-marshal.c: xfce-marshal.h
	( cd $(srcdir) && glib-genmarshal \
		--prefix="_xfce_marshal" \
		--body xfce-marshal.list ) > xgen-xpmc \
	&& cp xgen-xpmc xfce-marshal.c \
        && rm -f xgen-xpmc xgen-xpmc~

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
