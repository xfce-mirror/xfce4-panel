ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

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

distuninstallcheck_listfiles = 						\
	find . -type f -print | grep -v ./share/icons/hicolor/icon-theme.cache

desktopdir = $(datadir)/applications
desktop_in_files = \
	panel-desktop-handler.desktop.in \
	panel-preferences.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log xfce-4.6-master..HEAD > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

EXTRA_DIST = \
	$(desktop_in_files) \
	m4/introspection.m4 \
	m4/vapigen.m4 \
	meson.build \
	meson_options.txt \
	po/meson.build \
	xfce-revision.h.in \
	$(XDT_GEN_VISIBILITY_DIST) \
	$(NULL)

DISTCLEANFILES = \
	$(desktop_DATA)

AM_DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc \
	--enable-introspection \
	--enable-vala=no

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
