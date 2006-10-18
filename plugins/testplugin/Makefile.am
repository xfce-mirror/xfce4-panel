#plugindir = $(libexecdir)/xfce4/panel-plugins
#plugin_PROGRAMS = testplugin
noinst_PROGRAMS = testplugin

testplugin_SOURCES =					\
	testplugin.c

testplugin_CFLAGS =					\
	-I$(top_srcdir)					\
	@LIBXFCEGUI4_CFLAGS@				\
	-DLOCALEDIR=\"$(localedir)\"

testplugin_LDFLAGS =					\
	../../libxfce4panel/libxfce4panel.la		\
	@LIBXFCEGUI4_LIBS@

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
# 
desktop_in_in_files = testplugin.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
#desktopdir = $(datadir)/xfce4/panel-plugins
#desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
noinst_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_in_files)

DISTCLEANFILES =							\
	$(noinst_DATA) $(desktop_in_files)
#	$(desktop_DATA) $(desktop_in_files)
	
# get full path into .desktop file
%.desktop.in: %.desktop.in.in
# 	plugin_name=`grep dlname= $(plugin_LTLIBRARIES) | sed "s/dlname='\(.*\)'/\1/"`; \
	sed	-e "s^@EXTERNAL_PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" \
		-e "s^@PLUGIN_NAME@^$$plugin_name^" \
		$< > $@

