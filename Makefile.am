@SET_MAKE@

pkginfodir=$(pkgdatadir)

pkginfo_DATA = AUTHORS COPYING ChangeLog

noinst_DATA = example.xfce4rc


EXTRA_DIST = example.xfce4rc.in xfce4.spec xfce4.spec.in README.xml

SUBDIRS = @MY_SUBDIRS@

distclean-local:
	rm -f example.xfce4rc xfce4.spec

example.xfce4rc:
	@sed -e "s%_pkgdatadir_%$(pkgdatadir)%g" example.xfce4rc.in > example.xfce4rc

rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz
