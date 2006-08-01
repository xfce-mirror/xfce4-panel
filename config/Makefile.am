conf_in_files =	$(wildcard $(srcdir)/*.rc.in)

conf_files = $(conf_in_files:.rc.in=.rc)

confdir = $(sysconfdir)/xdg/xfce4/panel
conf_DATA = panels.xml	$(conf_files)

# copied from INTLTOOL_DESKTOP_RULE
%.rc:   %.rc.in   $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = panels.xml $(conf_in_files)

distclean-local:	
	rm -f *.rc
