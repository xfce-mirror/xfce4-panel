# $Id$

INCLUDES = 								\
	-I$(top_srcdir)							\
	-I$(top_builddir)						\
	-DBINDIR=\"$(bindir)\"						\
	-DG_LOG_DOMAIN=\"libtasklist\"					\
	$(PLATFORM_CPPFLAGS)

plugindir =								\
	$(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES =							\
	libtasklist.la

libtasklist_la_SOURCES = 						\
	tasklist-box.c							\
	tasklist-box.h							\
	tasklist-plugin.c						\
	tasklist-plugin.h

libtasklist_la_CFLAGS =							\
	$(GTK_CFLAGS)							\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(LIBXFCE4UI_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libtasklist_la_LDFLAGS =						\
	-avoid-version							\
	-module								\
	$(PLATFORM_LDFLAGS)

if HAVE_CYGWIN
libtasklist_la_LDFLAGS += 						\
	-no-undefined
endif

libtasklist_la_LIBADD =							\
	$(top_builddir)/libxfce4panel/libxfce4panel.la			\
	$(GTK_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(LIBXFCE4UI_LIBS)

libtasklist_la_DEPENDENCIES =						\
	$(top_builddir)/libxfce4panel/libxfce4panel.la

#
# .desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel-plugins

desktop_in_in_files =							\
	tasklist.desktop.in.in

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
