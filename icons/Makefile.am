noinst_DATA = xfce4-panel-icon.h xfce4-panel.png

EXTRA_DIST = $(noinst_DATA)

xfce4-panel-icon.h: $(srcdir)/xfce4-panel.png
	gdk-pixbuf-csource --raw --build-list 				\
	panel_icon_data $(srcdir)/xfce4-panel.png > xfce4-panel-icon.h

all: xfce4-panel-icon.h
