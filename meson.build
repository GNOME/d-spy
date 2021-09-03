project('dspy', 'c',
        version: '42.alpha0',
  meson_version: '>= 0.58.0',
)

gnome = import('gnome')
i18n = import('i18n')
cc = meson.get_compiler('c')
buildtype = get_option('buildtype')

if get_option('development')
  app_id = 'org.gnome.dspy.devel'
else
  app_id = 'org.gnome.dspy'
endif

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'dspy')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('APP_ID', app_id)

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
