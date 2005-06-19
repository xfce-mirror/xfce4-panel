bin_PROGRAMS = xfce4-iconbox

xfce4_iconbox_SOURCES =							\
	iconbox.c							\
	iconbox.h							\
	iconbox-settings.c						\
	iconbox-settings.h

xfce4_iconbox_CFLAGS =							\
	-I$(srcdir)							\
	-I$(top_srcdir)/libs						\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4MCS_CLIENT_CFLAGS@					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

xfce4_iconbox_LDADD =							\
	../libs/libxfce4panel.la					\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4MCS_CLIENT_LIBS@

xfce4_iconbox_DEPENDENCIES =						\
	../libs/libxfce4panel.la


