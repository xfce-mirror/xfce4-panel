bin_PROGRAMS =  xfce4-taskbar

xfce4_taskbar_SOURCES = xftaskbar.c 

xfce4_taskbar_CFLAGS =							\
	-I$(srcdir)							\
	-I$(top_srcdir)/libs						\
	@LIBXFCEGUI4_CFLAGS@						\
	@LIBXFCE4MCS_CLIENT_CFLAGS@					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

xfce4_taskbar_LDADD =							\
	-L$(top_srcdir)/libs -lxfce4panel				\
	@LIBXFCEGUI4_LIBS@						\
	@LIBXFCE4MCS_CLIENT_LIBS@

install-exec-local:
	( cd $(DESTDIR)$(bindir) && 					\
	  rm -f xftaskbar4 && 						\
	  $(LN_S) xfce4-taskbar xftaskbar4    				\
	)

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/xftaskbar4
