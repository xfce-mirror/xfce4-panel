bin_PROGRAMS = xfce4-panel
xfce4_panel_SOURCES = \
callbacks.c \
callbacks.h \
controls.c \
controls.h \
controls_dialog.c \
controls_dialog.h \
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
mcs_client.c \
mcs_client.h \
panel.c \
panel.h \
plugins.h \
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
plugins.h \
xfce_support.h \
xfce.h 

xfce4_panel_CFLAGS = -Wall @CFLAGS@ @X_CFLAGS@ @BASE_DEPENDENCIES_CFLAGS@ @MCS_CLIENT_CFLAGS@ -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\" -DSYSCONFDIR=\"$(sysconfdir)\" -DVERSION=\"@VERSION@\" -DLOCALEDIR=\"$(datadir)/locale\"
xfce4_panel_LDADD = @LIBS@ @X_LIBS@  @BASE_DEPENDENCIES_LIBS@ @MCS_CLIENT_LIBS@ 
