dspy_sources = [
  'dspy-connection-button.c',
  'dspy-connection.c',
  'dspy-introspection-model.c',
  'dspy-method-invocation.c',
  'dspy-method-view.c',
  'dspy-name-marquee.c',
  'dspy-name-row.c',
  'dspy-name.c',
  'dspy-names-model.c',
  'dspy-node.c',
  'dspy-pattern-spec.c',
  'dspy-signature.c',
  'dspy-simple-popover.c',
  'dspy-tree-view.c',
  'dspy-view.c',
  'dspy-window.c',
  'main.c',
]

dspy_deps = [
  libadwaita_dep,
  libgio_dep,
  libgtk_dep,
]

dspy_sources += gnome.compile_resources('dspy-resources', 'dspy.gresource.xml',
  c_name: 'dspy'
)

build_ident_h = vcs_tag(
     fallback: meson.project_version(),
        input: 'build-ident.h.in',
       output: 'build-ident.h',
)

dspy = executable('d-spy', dspy_sources + [build_ident_h],
         dependencies: dspy_deps,
              install: true,
  include_directories: root_include_directory,
)
