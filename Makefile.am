
SUBDIRS = \
	libxfce4panel \
	common \
	panel \
	plugins \
	wrapper \
	migrate \
	docs \
	icons \
	po

distclean-local:
	rm -rf *.spec *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

desktopdir = $(datadir)/applications
desktop_in_files = \
	panel-desktop-handler.desktop.in \
	panel-preferences.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

EXTRA_DIST = \
	$(desktop_in_files) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

DISTCLEANFILES = \
	$(desktop_DATA) \
	intltool-extract \
	intltool-merge \
	intltool-update \
	intltool-update-tmp

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
