# $Id$

INCLUDES = 								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"libwindowlist\"				\
	-DLOCALEDIR=\"$(localedir)\"					\
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE					\
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS =								\
	xfce4-popup-windowlist

xfce4_popup_windowlist_SOURCES =					\
	xfce4-popup-windowlist.c					\
	xfce4-popup-windowlist.h

xfce4_popup_windowlist_CFLAGS =						\
	$(LIBX11_CFLAGS)						\
	$(GTK_CFLAGS)							\
	$(PLATFORM_CFLAGS)

xfce4_popup_windowlist_LDADD =						\
	$(LIBX11_LDFLAGS)						\
	$(LIBX11_LIBS)							\
	$(GTK_LIBS)

plugindir =								\
	$(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES =							\
	libwindowlist.la

libwindowlist_la_SOURCES = 						\
	windowlist.h							\
	windowlist.c							\
	windowlist-dialog.h						\
	windowlist-dialog.c						\
	xfce4-popup-windowlist.h

libwindowlist_la_CFLAGS =						\
	$(GTK_CFLAGS)							\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(LIBWNCK_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libwindowlist_la_LDFLAGS =						\
	-avoid-version							\
	-module								\
	$(PLATFORM_LDFLAGS)

if HAVE_CYGWIN
libwindowlist_la_LDFLAGS += 						\
	-no-undefined
endif

libwindowlist_la_LIBADD =						\
	$(top_builddir)/libxfce4panel/libxfce4panel.la			\
	$(GTK_LIBS)							\
	$(LIBWNCK_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(LIBXFCEGUI4_LIBS)

libwindowlist_la_DEPENDENCIES =						\
	$(top_builddir)/libxfce4panel/libxfce4panel.la

#
# .desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel-plugins

desktop_in_in_files =							\
	windowlist.desktop.in.in

desktop_in_files = 							\
	$(desktop_in_in_files:.desktop.in.in=.desktop.in)

%.desktop.in: %.desktop.in.in
	sed -e "s,\@libdir\@,$(libdir),g" < $< > $@

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA)							\
	$(desktop_in_files)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
