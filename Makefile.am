@SET_MAKE@

confdir = $(sysconfdir)/xdg/xfce4/panel
conf_DATA = 								\
	contents.xml 							\
	contents.xml.ar							\
	contents.xml.az							\
	contents.xml.ca							\
	contents.xml.eu							\
	contents.xml.he							\
	contents.xml.hu							\
	contents.xml.it							\
	contents.xml.ms							\
	contents.xml.nl							\
	contents.xml.ro							\
	contents.xml.ru							\
	contents.xml.sk							\
	contents.xml.vi

EXTRA_DIST = 								\
	HACKING								\
	NOTES								\
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
