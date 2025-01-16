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

  gnome.gtkdoc(
    libxfce4panel_doc_name,
    src_dir: meson.project_source_root() / 'libxfce4panel',
    ignore_headers: [
      'libxfce4panel-visibility.h',
      'libxfce4panel-marshal.h',
      'xfce-panel-plugin-provider.h',
      'libxfce4panel-enum-types.h',
    ],
    main_xml: 'libxfce4panel-docs.xml',
    dependencies: libxfce4panel_dep,
    gobject_typesfile: meson.current_source_dir() / 'libxfce4panel.types',
    install: true,
    check: false,
    module_version: libxfce4panel_version_api,
  )
endif
