EXTRA_DIST = $(html_DATA)
htmldir=$(pkgdatadir)/doc/html
html_DATA=\
copyright.html \
GPL.html \
index.html \
ln-legalnotice.html \
panel-getting-started.html \
panel-prefs.html \
panel-use.html 

html:
	xsltproc $(srcdir)/../xml/xfce.xsl $(srcdir)/../xml/xfce4-panel.xml

