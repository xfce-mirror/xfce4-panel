bin_PROGRAMS = xfce4
xfce4_SOURCES = \
builtins.c \
callbacks.c \
central.c \
dialogs.c \
icons.c \
item.c \
item_dialog.c \
module.c \
move.c \
popup.c \
settings.c \
side.c \
wmhints.c \
builtins.h \
callbacks.h \
central.h \
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
icons.h \
item.h \
module.h \
xfce.h \
xfce_support.h \
global.h

xfce4_CFLAGS = @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ -DG_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\"
xfce4_LDADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@
