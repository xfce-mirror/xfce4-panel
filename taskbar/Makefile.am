bin_PROGRAMS =  xfce4-taskbar

xfce4_taskbar_SOURCES = xftaskbar.c 

xfce4_taskbar_CFLAGS =							\
	-I$(srcdir)							\
	-I$(top_srcdir)/libs						\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4MCS_CLIENT_CFLAGS@					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

xfce4_taskbar_LDADD =							\
	../libs/libxfce4panel.la					\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4MCS_CLIENT_LIBS@

