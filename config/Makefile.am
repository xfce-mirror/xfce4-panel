conf_in_files =	$(wildcard $(srcdir)/*.rc.in)

conf_files = $(conf_in_files:.rc.in=.rc)

confdir = $(sysconfdir)/xdg/xfce4/panel
conf_DATA = panels.xml systray-4.rc xfce4-menu-5.rc clock-14.rc $(conf_files)

# copied from INTLTOOL_DESKTOP_RULE
%.rc:   %.rc.in   $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = panels.xml systray-4.rc xfce4-menu-5.rc clock-14.rc $(conf_in_files)

distclean-local:	
	rm -f $(conf_files)
