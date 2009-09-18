# $Id$

AUTOMAKE_OPTIONS = 1.8

# The name of the module.
DOC_MODULE=libxfce4panel

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR=../../libxfce4panel

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS=--type-init-func="gtk_type_init(0);g_type_class_ref(G_TYPE_OBJECT)"

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=

# Used for dependencies
HFILE_GLOB=$(top_srcdir)/libxfce4panel/*.h
CFILE_GLOB=$(top_srcdir)/libxfce4panel/*.c

# Header files to ignore when scanning.
IGNORE_HFILES=								\
	xfce-marshal.h							\
	xfce-panel-external-item.h					\
	xfce-panel-item-iface.h						\
	xfce-panel-plugin-iface-private.h				\
	xfce-panel-plugin-messages.h

# Images to copy into HTML directory.
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files=								\
	version.xml

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
INCLUDES=								\
	-I$(top_srcdir)							\
	-I$(top_builddir)						\
	$(LIBXFCEGUI4_CFLAGS)

GTKDOC_LIBS=								\
	$(top_builddir)/libxfce4panel/libxfce4panel.la			\
	$(LIBXFCEGUI4_LIBS)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST +=								\
	version.xml.in

# required for gtk-doc
dist-hook: all
