plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libactions.la

libactions_la_LDFLAGS =						\
	-avoid-version							\
	-module

libactions_la_SOURCES = 						\
	actions.c

libactions_la_CFLAGS =							\
	-I$(top_srcdir)							\
	@LIBXFCEGUI4_CFLAGS@						\
	-DLOCALEDIR=\"$(localedir)\"

libactions_la_LIBADD =							\
	../../libxfce4panel/libxfce4panel.la				\
	@LIBS@

libactions_la_DEPENDENCIES =						\
	../../libxfce4panel/libxfce4panel.la
	
if HAVE_CYGWIN
libactions_la_LDFLAGS +=                        	                \
	-no-undefined                                                   \
	@LIBX11_LDFLAGS@                                                \
	@LIBXFCEGUI4_LIBS@                                              \
	@LIBX11_LIBS@
endif

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
# 
desktop_in_in_files = actions.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA) $(desktop_in_files)
	
# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@INTERNAL_PLUGIN_PATH@^$(libdir)/xfce4/panel-plugins^" \
		$< > $@

