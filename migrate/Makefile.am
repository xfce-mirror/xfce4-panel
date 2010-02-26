
INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DG_LOG_DOMAIN=\"xfce4-panel-migrate\" \
	-DDATADIR=\"$(datadir)\" \
	-DLIBDIR=\"$(libdir)/xfce4\" \
	-DLIBEXECDIR=\"$(libexecdir)/xfce4\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

libexec_PROGRAMS = \
	xfce4-panel-migrate

xfce4_panel_migrate_SOURCES = \
	main.c \
	migrate_46.c \
	migrate_46.h

xfce4_panel_migrate_CFLAGS = \
	$(GTK_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfce4_panel_migrate_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

xfce4_panel_migrate_LDADD = \
	$(GTK_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
