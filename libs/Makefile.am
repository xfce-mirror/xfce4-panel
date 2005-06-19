lib_LTLIBRARIES = libxfce4panel.la

generated_files = 							\
	xfce-enum-types.c						\
	xfce-enum-types.h						\
	xfce-marshal.c							\
	xfce-marshal.h

libxfce4panel_la_SOURCES =						\
	$(generated_files)						\
	xfce-arrow-button.c						\
	xfce-arrow-button.h						\
	xfce-itembar.c							\
	xfce-itembar.h							\
	xfce-item.c							\
	xfce-item.h							\
	xfce-paint-private.c						\
	xfce-paint-private.h						\
	xfce-panel-window.c						\
	xfce-panel-window.h						\
	xfce-separator-item.c						\
	xfce-separator-item.h

xfce4_panelincludedir = $(includedir)/xfce4/panel

xfce4_panelinclude_HEADERS =						\
	xfce-arrow-button.h						\
	xfce-itembar.h							\
	xfce-item.h							\
	xfce-panel-window.h						\
	xfce-separator-item.h
								
libxfce4panel_la_CFLAGS =						\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/lib						\
	@LIBXFCEGUI4_CFLAGS@
	
libxfce4panel_la_LDFLAGS = 						\
	-export-dynamic							\
	-version-info @LIBXFCE4PANEL_VERINFO@

EXTRA_DIST = 								\
	xfce-marshal.list						\
	$(generated_files)

xfce_headers = \
	xfce-panel-window.h

xfce-enum-types.h: $(xfce_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef _XFCE_ENUM_TYPES_H\n#define _XFCE_ENUM_TYPES_H\n#include <libxfce4util/libxfce4util.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define XFCE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* !_XFCE_ENUM_TYPES_H__ */" \
		$(xfce_headers) ) >> xgen \
	&& (cmp -s xgen xfce-enum-types.h || cp xgen xfce-enum-types.h ) \
	&& rm -f xgen xgen~

xfce-enum-types.c: xfce-enum-types.h $(xfce_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include <xfce-enum-types.h>\n#include <xfce-panel-window.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {"\
		--vprod "\t{ @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "\t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n\treturn type;\n}\n" \
		$(xfce_headers) ) > xgen \
	&& cp xgen xfce-enum-types.c  \
	&& rm -f xgen xgen~

xfce-marshal.h: $(srcdir)/xfce-marshal.list Makefile
	glib-genmarshal --header --prefix="_xfce_marshal" \
            $(srcdir)/xfce-marshal.list > tmp-marshal.h && \
        cp tmp-marshal.h $(@F) && \
        rm -f tmp-marshal.h tmp-marshal.h~
                                                                                
xfce-marshal.c: xfce-marshal.h $(srcdir)/xfce-marshal.list Makefile
	glib-genmarshal --body --prefix="_xfce_marshal" \
            $(srcdir)/xfce-marshal.list > tmp-marshal.c && \
        cp tmp-marshal.c $(@F) && \
        rm -f tmp-marshal.c tmp-marshal.c~

DISTCLEANFILES =							\
	xfce-enum-types.h						\
	xfce-enum-types.c						\
	xfce-marshal.h							\
	xfce-marshal.c
