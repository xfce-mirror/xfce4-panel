@SET_MAKE@

EXTRA_DIST = 								\
	AUTHORS								\
	COPYING								\
	ChangeLog							\
	HACKING								\
	INSTALL								\
	NEWS								\
	NOTES								\
	README								\
	README.Kiosk							\
	TODO								\
	xfce4-panel.spec						\
	.indent.pro

SUBDIRS =								\
	po								\
	doc								\
	config								\
	icons								\
	libs								\
	panel								\
	taskbar								\
	iconbox								\
	settings							\
	plugins

distclean-local:
	rm -rf xfce4.spec *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

html: Makefile
	make -C doc html

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2
