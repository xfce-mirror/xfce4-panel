hicolor48dir = $(datadir)/icons/hicolor/48x48/apps

hicolor16dir = $(datadir)/icons/hicolor/16x16/apps

hicolor48_DATA = \
	xfce4-panel.png \
	xfce-edit.png \
	xfce-filemanager.png \
	xfce-games.png \
	xfce-graphics.png \
	xfce-internet.png \
	xfce-mail.png \
	xfce-man.png \
	xfce-multimedia.png \
	xfce-print.png \
	xfce-schedule.png \
	xfce-sound.png \
	xfce-terminal.png \
	xfce-unknown.png \
	xfce-utils.png

hicolor16_DATA = \
	xfce-system-exit.png \
	xfce-system-info.png \
	xfce-system-lock.png \
	xfce-system-settings.png

noinst_DATA = xfce4-panel-icon.h

EXTRA_DIST = $(noinst_DATA) $(hicolor48_DATA) $(hicolor16_DATA)

xfce4-panel-icon.h: $(srcdir)/xfce4-panel.png
	gdk-pixbuf-csource --raw --build-list 				\
	panel_icon_data $(srcdir)/xfce4-panel.png > xfce4-panel-icon.h

