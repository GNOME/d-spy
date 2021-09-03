ABI_MAJOR = '0'

lib_header_dir = join_paths(get_option('includedir'), 'dspy-@0@'.format(ABI_MAJOR))

version_split = meson.project_version().split('.')
version_major = version_split[0]
if version_split[1].startswith('beta') or version_split[1].startswith('alpha')
  version_minor = 0
  version_micro = 0
else
  version_minor = version_split[1]
  version_micro = version_split[2]
endif

version_data = configuration_data()
version_data.set('VERSION_MAJOR', version_major)
version_data.set('VERSION_MINOR', version_minor)
version_data.set('VERSION_MICRO', version_micro)
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
]

libdspy_sources = [
  'dspy-binding-group.c',
  'dspy-connection.c',
  'dspy-connection-button.c',
  'dspy-empty-state.c',
  'dspy-introspection-model.c',
  'dspy-list-model-filter.c',
  'dspy-method-invocation.c',
  'dspy-method-view.c',
  'dspy-multi-paned.c',
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
]

libdspy_sources += gnome.compile_resources('libdspy-resources', 'libdspy.gresource.xml',
  c_name: 'libdspy'
)

libdspy = library('libdspy', libdspy_sources,
           dependencies: libdspy_deps,
    include_directories: [ root_include_directory ],
                install: true,
  gnu_symbol_visibility: 'hidden',
                version: '@0@.0.0'.format(ABI_MAJOR),
        darwin_versions: '@0@.0'.format(ABI_MAJOR.to_int() + 1),
              link_args: release_link_args,
)

libdspy_dep = declare_dependency(
         dependencies: libdspy_deps,
            link_with: [ libdspy ],
  include_directories: include_directories('.'),
)

install_headers(libdspy_headers,
                subdir: 'dspy-@0@'.format(ABI_MAJOR))
