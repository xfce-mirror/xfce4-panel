@SET_MAKE@


EXTRA_DIST = 								\
	xml-i18n-extract.in						\
	xml-i18n-merge.in						\
	xml-i18n-update.in						\
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

DISTCLEANFILES =							\
	xml-i18n-extract						\
	xml-i18n-merge							\
	xml-i18n-update							\
	xfce4-panel.spec
	
distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

html: Makefile
	make -C doc html

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2
