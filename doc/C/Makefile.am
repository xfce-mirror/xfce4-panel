SUBDIRS = \
  	 images

noinst_DATA = \
	xfce4-panel.xml 

docdir=$(datadir)/xfce4/doc/C

doc_DATA= \
	xfce4-panel.html 

EXTRA_DIST = $(noinst_DATA) $(doc_DATA)

html:
	xsltproc $(srcdir)/../xfce-nochunk.xsl $(srcdir)/xfce4-panel.xml

