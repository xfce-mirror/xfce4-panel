# $Id$

EXTRA_DIST = 								\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	AUTHORS								\
	COPYING								\
	COPYING_LIBS							\
	ChangeLog							\
	HACKING								\
	INSTALL								\
	NEWS								\
	README								\
	README.Kiosk							\
	README.Plugins							\
	.indent.pro							\
	xfce4-panel.spec

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update

# docs should be at the end to make sure the library is build before 
# scanning it.
SUBDIRS =								\
	config								\
	icons								\
	libxfce4panel							\
	panel								\
	plugins								\
	mcs-plugin							\
	po								\
	docs

AUTOMAKE_OPTIONS =							\
	1.8								\
	dist-bzip2

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

html:
	make -C docs html

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
