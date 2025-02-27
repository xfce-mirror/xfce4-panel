if get_option('gtk-doc')
  libxfce4panel_doc_name = 'libxfce4panel'

  conf_data = configuration_data()
  conf_data.set_quoted('PACKAGE_NAME', libxfce4panel_doc_name)
  conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
  conf_data.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(libxfce4panel_doc_name, meson.project_version()))

  configure_file(
    input: 'gtkdocentities.ent.in',
    output: 'gtkdocentities.ent',
    configuration: conf_data,
  )

  configure_file(
    configuration: configuration_data({
      'COPYRIGHT_YEAR': copyright_year,
    }),
    input: 'libxfce4panel-docs.xml.in',
    output: 'libxfce4panel-docs.xml',
    install: false,
  )

  gnome.gtkdoc(
    libxfce4panel_doc_name,
    src_dir: 'libxfce4panel',
    ignore_headers: [
      'libxfce4panel-visibility.h',
      'libxfce4panel-marshal.h',
      'xfce-panel-plugin-provider.h',
      'libxfce4panel-enum-types.h',
    ],
    main_xml: meson.project_build_root() / 'docs' / 'reference' / 'libxfce4panel-docs.xml',
    dependencies: libxfce4panel_dep,
    gobject_typesfile: 'libxfce4panel.types',
    install: true,
    check: false,
    module_version: libxfce4panel_version_api,
  )
endif
