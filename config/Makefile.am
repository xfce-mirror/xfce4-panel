confdir = $(sysconfdir)/xdg/xfce4/panel
conf_in_files = 					\
	panels.xml.in	
conf_DATA = $(conf_in_files:.xml.in=.xml)
	
@INTLTOOL_XML_RULE@

EXTRA_DIST = 					\
	$(conf_in_files)

DISTCLEANFILES =				\
	$(conf_DATA)

