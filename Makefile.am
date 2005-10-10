@SET_MAKE@

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
	.indent.pro

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	xfce4-panel.spec

# docs should be at the end to make sure the library is build before 
# scanning it.
SUBDIRS =								\
	config								\
	icons								\
	libxfce4panel							\
	panel								\
	plugins								\
	po								\
	docs

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2
