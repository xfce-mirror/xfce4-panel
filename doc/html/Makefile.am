EXTRA_DIST = $(html_DATA)
htmldir=$(pkgdatadir)/doc/html
html_DATA=\
panel-copyright.html \
xfce4-panel.html \
panel-getting-started.html \
panel-settings.html \
panel-use.html 

html:
	xsltproc $(srcdir)/../xml/xfce.xsl $(srcdir)/../xml/xfce4-panel.xml

