SUBDIRS = \
  	 images

noinst_DATA = \
	xfce4-panel.xml 

docdir=$(datadir)/xfce4/doc/he

doc_DATA= \
	xfce4-panel.html 

EXTRA_DIST = $(noinst_DATA) $(doc_DATA)

html: $(srcdir)/xfce4-panel.xml
	xsltproc $(srcdir)/../xfce-nochunk.xsl $(srcdir)/xfce4-panel.xml

