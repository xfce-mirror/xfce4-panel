@SET_MAKE@

confdir = $(sysconfdir)/xdg/xfce4/panel
conf_DATA = 								\
	contents.xml 							\
	contents.xml.ar							\
	contents.xml.az							\
	contents.xml.ca							\
	contents.xml.eu							\
	contents.xml.fr							\
	contents.xml.he							\
	contents.xml.hu							\
	contents.xml.it							\
	contents.xml.ko							\
	contents.xml.ms							\
	contents.xml.nl							\
	contents.xml.ro							\
	contents.xml.ru							\
	contents.xml.sk							\
	contents.xml.tr							\
	contents.xml.vi							\
	contents.xml.zh_TW

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
	$(conf_DATA)							\
	xfce4.spec							\
	.indent.pro

SUBDIRS =								\
	po								\
	doc								\
	icons								\
	panel								\
	settings							\
	plugins	

distclean-local:
	rm -rf xfce4.spec *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

html: Makefile
	make -C doc html

ACLOCAL_AMFLAGS = -I m4

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2
