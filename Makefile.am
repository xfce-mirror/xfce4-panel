@SET_MAKE@

confdir = $(sysconfdir)/xfce4
conf_DATA = xfce4rc

EXTRA_DIST = 								\
	HACKING								\
	NOTES								\
	xfce4rc							\
	xfce4.spec							\
	.indent.pro

SUBDIRS =								\
	po								\
	doc								\
	icons								\
	panel								\
	settings							\
	plugins								\
	themes

distclean-local:
	rm -rf xfce4.spec *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

ACLOCAL_AMFLAGS = -I m4
