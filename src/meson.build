dspy_sources = [
  'main.c',
  'dspy-window.c',
]

dspy_deps = [
  libadwaita_dep,
  libdspy_dep,
  libgio_dep,
  libgtk_dep,
]

dspy_sources += gnome.compile_resources('dspy-resources', 'dspy.gresource.xml',
  c_name: 'dspy'
)

dspy = executable('d-spy', dspy_sources,
         dependencies: dspy_deps,
              install: true,
  include_directories: root_include_directory,
)
