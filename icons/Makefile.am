# $Id$

hicolor48dir = $(datadir)/icons/hicolor/48x48/apps

hicolor16dir = $(datadir)/icons/hicolor/16x16/apps

hicolor48_DATA = \
	xfce4-panel.png \
	xfce4-iconbox.png \
	xfce4-taskbar.png \
	xfce-mouse.png \
	xfce-edit.png \
	xfce-filemanager.png \
	xfce-games.png \
	xfce-graphics.png \
	xfce-internet.png \
	xfce-mail.png \
	xfce-man.png \
	xfce-multimedia.png \
	xfce-printer.png \
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

EXTRA_DIST = $(hicolor48_DATA) $(hicolor16_DATA)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then					\
                echo "Updating Gtk icon cache.";			\
                $(gtk_update_icon_cache);				\
        else								\
                echo "*** Icon cache not updated. Remember to run:";	\
		echo "***";						\
                echo "***   $(gtk_update_icon_cache)";			\
		echo "***";						\
        fi

# vi:set ts=8 sw=8 noet ai nocindent:
