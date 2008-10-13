# $Id$

#
# .desktop file
#
desktopdir = $(datadir)/applications
desktop_in_files =							\
	xfce4-panel-manager.desktop.in
desktop_DATA =								\
	$(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

CLEANFILES =								\
	$(desktop_DATA)

EXTRA_DIST =								\
	$(desktop_in_files)

