bin_PROGRAMS = xfce4
xfce4_SOURCES = \
builtins.c \
callbacks.c \
central.c \
controls.c \
controls_dialog.c \
dialogs.c \
iconbutton.c \
icons.c \
info.c \
item.c \
item_dialog.c \
move.c \
popup.c \
settings.c \
side.c \
wmhints.c \
builtins.h \
callbacks.h \
central.h \
controls_dialog.h \
dialogs.h \
item_dialog.h \
move.h \
popup.h \
settings.h \
side.h \
wmhints.h \
xfce.c \
xfce_support.c

pkginclude_HEADERS = \
iconbutton.h \
icons.h \
item.h \
controls.h \
xfce.h \
xfce_support.h \
global.h

xfce4_CFLAGS = @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\" -DVERSION=\"@VERSION@\"
xfce4_LDADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
