bin_PROGRAMS = xfce4
xfce4_SOURCES = \
builtins.c \
builtins.h \
callbacks.c \
callbacks.h \
controls.c \
controls.h \
controls_dialog.c \
controls_dialog.h \
dialogs.c \
dialogs.h \
global.h \
groups.c \
groups.h \
handle.c \
handle.h \
icons.c \
icons.h \
info.c \
item.c \
item.h \
item_dialog.c \
item_dialog.h \
main.c \
main.h \
panel.c \
panel.h \
popup.c \
popup.h \
settings.c \
settings.h \
xfce.h \
xfce_support.c \
xfce_support.h 

pkginclude_HEADERS = \
controls.h \
debug.h \
global.h \
icons.h \
main.h \
panel.h \
xfce_support.h \
xfce.h 

xfce4_CFLAGS = -Wall @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\" -DSYSCONFDIR=\"$(sysconfdir)\" -DVERSION=\"@VERSION@\"
xfce4_LDADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
