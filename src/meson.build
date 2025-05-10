dspy_sources = [
  'dspy-argument.c',
  'dspy-connection.c',
  'dspy-future-list-model.c',
  'dspy-interface.c',
  'dspy-introspectable-row.c',
  'dspy-introspectable.c',
  'dspy-introspection.c',
  'dspy-method-argument.c',
  'dspy-method-invocation.c',
  'dspy-method.c',
  'dspy-name.c',
  'dspy-names-model.c',
  'dspy-node.c',
  'dspy-property.c',
  'dspy-signal.c',
  'dspy-signature.c',
  'dspy-titled-model.c',
  'dspy-util.c',
  'dspy-window.c',
  'main.c',
]

dspy_deps = [
  libadwaita_dep,
  libgio_dep,
  libgtk_dep,
  libdex_dep,
]

dspy_sources += gnome.compile_resources('dspy-resources', 'dspy.gresource.xml',
  c_name: 'dspy'
)

dspy_include_directories = [root_include_directory]

build_ident_h = vcs_tag(
     fallback: meson.project_version(),
        input: 'build-ident.h.in',
       output: 'build-ident.h',
)

dspy = executable('d-spy', dspy_sources + [build_ident_h],
         dependencies: dspy_deps,
              install: true,
  include_directories: dspy_include_directories,
)
