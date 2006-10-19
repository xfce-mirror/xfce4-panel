bin_PROGRAMS = xfce4-popup-windowlist

xfce4_popup_windowlist_SOURCES = \
	xfce4-popup-windowlist.c \
	xfce4-popup-windowlist.h

xfce4_popup_windowlist_CFLAGS = \
	$(LIBX11_CFLAGS) \
	$(GTK_CFLAGS)

xfce4_popup_windowlist_LDADD = \
	$(LIBX11_LDFLAGS) \
	$(LIBX11_LIBS) \
	$(GTK_LIBS)

plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libwindowlist.la

libwindowlist_la_LDFLAGS =						\
	-avoid-version							\
	-module

libwindowlist_la_SOURCES = 						\
	windowlist.h							\
	windowlist.c							\
	windowlist-dialog.h						\
	windowlist-dialog.c						\
	xfce4-popup-windowlist.h

libwindowlist_la_CFLAGS =						\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	-DLOCALEDIR=\"$(localedir)\"

libwindowlist_la_LIBADD =						\
	../../libxfce4panel/libxfce4panel.la				\
	@LIBS@

libwindowlist_la_DEPENDENCIES =						\
	../../libxfce4panel/libxfce4panel.la
	
if HAVE_CYGWIN
libwindowlist_la_LDFLAGS +=						\
	-no-undefined							\
	@LIBX11_LDFLAGS@						\
	@LIBXFCEGUI4_LIBS@						\
	@LIBX11_LIBS@
endif

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
# 
desktop_in_in_files = windowlist.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA) $(desktop_in_files)
	
# get full path into .desktop file
%.desktop.in: %.desktop.in.in $(plugin_LTLIBRARIES)
	plugin_name=`grep dlname= $(plugin_LTLIBRARIES) | sed "s/dlname='\(.*\)'/\1/"`; \
	sed	-e "s^@INTERNAL_PLUGIN_PATH@^$(libdir)/xfce4/panel-plugins^" \
		-e "s^@PLUGIN_NAME@^$$plugin_name^" \
		$< > $@

