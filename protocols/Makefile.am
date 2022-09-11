AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"libpanel-protocols\" \
	$(PLATFORM_CPPFLAGS)

noinst_LTLIBRARIES = \
	libpanel-protocols.la

libpanel_protocols_built_sources = \
	wlr-foreign-toplevel-management-unstable-v1.c \
	wlr-foreign-toplevel-management-unstable-v1-client.h

libpanel_protocols_la_SOURCES = \
	wlr-foreign-toplevel-management-unstable-v1.c

libpanel_protocols_la_CFLAGS = \
	$(WAYLAND_CLIENT_CFLAGS) \
	$(PLATFORM_CFLAGS)

libpanel_protocols_la_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libpanel_protocols_la_LIBADD = \
	$(WAYLAND_CLIENT_LIBS)

%.c: %.xml
	$(AM_V_GEN) wayland-scanner private-code $< $@

%-client.h: %.xml
	$(AM_V_GEN) wayland-scanner client-header $< $@

DISTCLEANFILES = \
	$(libpanel_protocols_built_sources)

BUILT_SOURCES = \
	$(libpanel_protocols_built_sources)

EXTRA_DIST = \
	wlr-foreign-toplevel-management-unstable-v1.xml

# required for make distcheck
dist-hook: all
