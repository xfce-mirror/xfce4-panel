AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libstatusnotifier\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

plugindir = \
	$(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = \
	libstatusnotifier.la

libstatusnotifier_built_sources = \
	sn-watcher.h \
	sn-watcher.c \
	sn-dialog-ui.h

libstatusnotifier_la_SOURCES = \
	$(libstatusnotifier_built_sources) \
	sn-config.c \
	sn-config.h \
	sn-backend.c \
	sn-backend.h \
	sn-button.c \
	sn-button.h \
	sn-box.c \
	sn-box.h \
	sn-dialog.c \
	sn-dialog.h \
	sn-icon-box.c \
	sn-icon-box.h \
	sn-item.c \
	sn-item.h \
	sn-plugin.c \
	sn-plugin.h \
	sn-util.c \
	sn-util.h

libstatusnotifier_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(DBUSMENU_CFLAGS) \
	$(PLATFORM_CFLAGS) \
	-DDATADIR=\""$(datadir)"\"

libstatusnotifier_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

libstatusnotifier_la_LIBADD = \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4PANEL_LIBS) \
	$(XFCONF_LIBS) \
	$(DBUSMENU_LIBS)

desktopdir = \
	$(datadir)/xfce4/panel/plugins

desktop_in_files = \
	statusnotifier.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	sn-dialog.glade \
	$(desktop_in_files)

DISTCLEANFILES = \
	$(desktop_DATA)

if MAINTAINER_MODE

sn-watcher.h:
sn-watcher.c: sn-watcher.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix org.kde.StatusNotifierWatcher \
	--generate-c-code sn-watcher \
	$(srcdir)/sn-watcher.xml

sn-dialog-ui.h: sn-dialog.glade
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=sn_dialog_ui $< >$@

BUILT_SOURCES = \
	$(libstatusnotifier_built_sources)

DISTCLEANFILES += \
	$(libstatusnotifier_built_sources)

endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
