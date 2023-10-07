
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-panel-wrapper\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

wrapperdir = $(HELPER_PATH_PREFIX)/xfce4/panel

wrapper_PROGRAMS = \
	wrapper-2.0

wrapper_built_sources = \
	wrapper-external-exported.c \
	wrapper-external-exported.h

wrapper_2_0_SOURCES = \
	$(wrapper_built_sources) \
	main.c \
	wrapper-module.c \
	wrapper-module.h \
	wrapper-plug.c \
	wrapper-plug.h

if ENABLE_X11
wrapper_2_0_SOURCES += \
	wrapper-plug-x11.c \
	wrapper-plug-x11.h
endif

if HAVE_GTK_LAYER_SHELL
wrapper_2_0_SOURCES += \
	wrapper-plug-wayland.c \
	wrapper-plug-wayland.h
endif

wrapper_2_0_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GTK_LAYER_SHELL_CFLAGS) \
	$(LIBXFCE4WINDOWING_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(PLATFORM_CFLAGS)

wrapper_2_0_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

wrapper_2_0_LDADD = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la \
	$(top_builddir)/common/libpanel-common.la \
	$(GTK_LIBS) \
	$(GIO_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LAYER_SHELL_LIBS) \
	$(LIBXFCE4WINDOWING_LIBS) \
	$(LIBXFCE4UTIL_LIBS)

wrapper_2_0_DEPENDENCIES = \
	$(top_builddir)/libxfce4panel/libxfce4panel-$(LIBXFCE4PANEL_VERSION_API).la

if MAINTAINER_MODE

#wrapper-marshal.h: $(top_builddir)/panel/panel-marshal.list Makefile
#	$(AM_V_GEN)glib-genmarshal --header --prefix=wrapper_marshal $< > $@
#
#wrapper-marshal.c: $(top_builddir)/panel/panel-marshal.list Makefile
#	$(AM_V_GEN) echo "#include <wrapper/wrapper-marshal.h>" > $@ \
#	&& glib-genmarshal --body --prefix=wrapper_marshal $< >> $@

wrapper-external-exported.c:
wrapper-external-exported.h: wrapper-external-infos.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix org.xfce.Panel.External \
	--c-namespace WrapperExternal \
	--generate-c-code wrapper-external-exported \
	$(srcdir)/wrapper-external-infos.xml

BUILT_SOURCES = \
	$(wrapper_built_sources)

DISTCLEANFILES = \
	$(wrapper_built_sources)

endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
