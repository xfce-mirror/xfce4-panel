# $Id$

INCLUDES = 								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"libseparator\"					\
	-DLOCALEDIR=\"$(localedir)\"					\
	$(PLATFORM_CPPFLAGS)

plugindir =								\
	$(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES =							\
	libseparator.la

libseparator_la_SOURCES = 						\
	separator.c

libseparator_la_CFLAGS =						\
	$(GTK_CFLAGS)							\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libseparator_la_LDFLAGS =						\
	-avoid-version							\
	-module								\
	$(PLATFORM_LDFLAGS)

if HAVE_CYGWIN
libseparator_la_LDFLAGS += 						\
	-no-undefined
endif

libseparator_la_LIBADD =						\
	$(top_builddir)/libxfce4panel/libxfce4panel.la			\
	$(GTK_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(LIBXFCEGUI4_LIBS)

libseparator_la_DEPENDENCIES =						\
	$(top_builddir)/libxfce4panel/libxfce4panel.la

#
# .desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel-plugins

desktop_in_in_files =							\
	separator.desktop.in.in

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
