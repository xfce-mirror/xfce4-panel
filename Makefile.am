# $Id$

SUBDIRS =								\
	config								\
	icons								\
	libxfce4panel							\
	panel								\
	plugins								\
	mcs-plugin							\
	po								\
	docs

distclean-local:
	rm -rf *.spec *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

html:
	make -C docs html

EXTRA_DIST = 								\
	README								\
	COPYING								\
	COPYING.LIB							\
	HACKING								\
	INSTALL								\
	xfce4-panel.spec.in						\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in

DISTCLEANFILES =							\
	xfce4-panel.spec						\
	intltool-extract						\
	intltool-merge							\
	intltool-update

DISTCHECK_CONFIGURE_FLAGS =						\
	--enable-gtk-doc

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
