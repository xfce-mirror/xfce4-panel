SUBDIRS = 								\
	XFce 								\
	Gnome 								\
	Curve 								\
	Noia 								\
	Crystal 							\
	FreeIcons

install-data-hook:
	cd $(DESTDIR)$(datadir)/xfce4/icons && \
	  rm -f hicolor && \
	  $(LN_S) Curve hicolor
