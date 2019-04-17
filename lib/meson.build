libdspy_sources = [
  'dspy-connection.c',
  'dspy-introspection-model.c',
  'dspy-name.c',
  'dspy-name-row.c',
  'dspy-names-model.c',
  'dspy-node.c',
  'dspy-signature.c',
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
)

libdspy_dep = declare_dependency(
         dependencies: libdspy_deps,
            link_with: [ libdspy ],
  include_directories: include_directories('.'),
)
