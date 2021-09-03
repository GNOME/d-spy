project('dspy', 'c',
        version: '42.alpha0',
  meson_version: '>= 0.58.0',
)

gnome = import('gnome')
i18n = import('i18n')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'dspy')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(
         output: 'config.h',
  configuration: config_h,
)

libgio_dep = dependency('gio-2.0', version: '>= 2.60')
libgtk_dep = dependency('gtk+-3.0', version: '>= 3.24')

root_include_directory = include_directories('.')

subdir('data')
subdir('lib')
subdir('src')
subdir('po')

meson.add_install_script('build-aux/meson/postinstall.py')
