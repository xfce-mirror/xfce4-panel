@SET_MAKE@

pkginfodir=$(pkgdatadir)
confdir=$(sysconfdir)/xfce4

pkginfo_DATA = AUTHORS COPYING ChangeLog

conf_DATA = xfce4rc


EXTRA_DIST = example.xfce4rc.in xfce4.spec xfce4.spec.in autogen.sh

SUBDIRS = po @MY_SUBDIRS@

distclean-local:
	rm -rf xfce4rc xfce4.spec *.cache

xfce4rc: example.xfce4rc.in
	@sed -e "s%_pkgdatadir_%$(pkgdatadir)%g" example.xfce4rc.in > xfce4rc

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz
