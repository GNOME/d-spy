lib_header_dir = join_paths(get_option('includedir'), 'dspy-@0@'.format(api_version_major))

version_data = configuration_data()
version_data.set('VERSION_MAJOR', api_version_major)
version_data.set('VERSION_MINOR', api_version_minor)
version_data.set('VERSION_MICRO', api_version_micro)
version_data.set('VERSION', meson.project_version())

dspy_version_h = configure_file(
          input: 'dspy-version.h.in',
         output: 'dspy-version.h',
    install_dir: lib_header_dir,
        install: true,
  configuration: version_data)

libdspy_headers = [
  dspy_version_h,

  'dspy.h',
  'dspy-connection.h',
  'dspy-connection-button.h',
  'dspy-introspection-model.h',
  'dspy-method-invocation.h',
  'dspy-method-view.h',
  'dspy-name.h',
  'dspy-name-marquee.h',
  'dspy-name-row.h',
  'dspy-names-model.h',
  'dspy-tree-view.h',
  'dspy-view.h',
  'dspy-version-macros.h',
]

libdspy_sources = [
  'dspy-binding-group.c',
  'dspy-connection.c',
  'dspy-connection-button.c',
  'dspy-introspection-model.c',
  'dspy-list-model-filter.c',
  'dspy-method-invocation.c',
  'dspy-method-view.c',
  'dspy-name.c',
  'dspy-name-marquee.c',
  'dspy-name-row.c',
  'dspy-names-model.c',
  'dspy-node.c',
  'dspy-pattern-spec.c',
  'dspy-signature.c',
  'dspy-simple-popover.c',
  'dspy-tree-view.c',
  'dspy-view.c',
]

libdspy_deps = [
  libgio_dep,
  libgtk_dep,
  libadwaita_dep,
]

libdspy_sources += gnome.compile_resources('libdspy-resources', 'libdspy.gresource.xml',
  c_name: 'libdspy'
)

libdspy = library('dspy-@0@'.format(api_version_major), libdspy_sources,
           dependencies: libdspy_deps,
    include_directories: [ root_include_directory ],
                install: true,
  gnu_symbol_visibility: 'hidden',
                version: '@0@.0.0'.format(api_version_major),
        darwin_versions: '@0@.0'.format(api_version_major + 1),
              link_args: release_link_args,
)

libdspy_dep = declare_dependency(
         dependencies: libdspy_deps,
            link_with: [ libdspy ],
  include_directories: include_directories('.'),
)

libdspy_header_subdir = 'dspy-@0@'.format(api_version_major)
install_headers(libdspy_headers, subdir: libdspy_header_subdir)

pkgg = import('pkgconfig')

pkgg.generate(
    libraries: [libdspy],
      subdirs: libdspy_header_subdir,
         name: 'D-Spy',
     filebase: 'dspy-@0@'.format(api_version_major),
  description: 'Embed D-Spy into applications and IDEs',
     requires: [ 'gtk4', 'libadwaita-1'],
)

