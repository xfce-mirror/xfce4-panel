SUBDIRS =	\
	C

noinst_DATA = \
	xfce-nochunk.xsl

EXTRA_DIST = $(noinst_DATA)

html: Makefile
	for i in $(SUBDIRS); do make -C $$i html; done

