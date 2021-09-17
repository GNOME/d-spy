project('dspy', 'c',
        version: '0.99.0',
  meson_version: '>= 0.56.0',
)

symbolic_version = '41.rc1'

version_split = meson.project_version().split('.')
version_major = version_split[0]
version_minor = version_split[1]
version_micro = version_split[2]

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
config_h.set_quoted('SYMBOLIC_VERSION', symbolic_version)
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('PACKAGE_ICON_NAME', app_id)
config_h.set_quoted('GETTEXT_PACKAGE', 'd-spy')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('APP_ID', app_id)
config_h.set_quoted('PACKAGE_WEBSITE', 'https://gitlab.gnome.org/GNOME/d-spy')
config_h.set10('DEVELOPMENT_BUILD', get_option('development'))

glib_req_version = '2.68'
gtk_req_version = '4.2'
adwaita_req_version = '1.0'

glib_req = '>= @0@'.format(glib_req_version)
gtk_req = '>= @0@'.format(gtk_req_version)
adwaita_req = '>= @0@'.format(adwaita_req_version)

# Specify minimum library versions
glib_major = glib_req_version.split('.')[0].to_int()
glib_minor = glib_req_version.split('.')[1].to_int()
gtk_major = gtk_req_version.split('.')[0].to_int()
gtk_minor = gtk_req_version.split('.')[1].to_int()

libgio_dep = dependency('gio-2.0', version: glib_req)
libgtk_dep = dependency('gtk4', version: gtk_req)
libadwaita_dep = dependency('libadwaita-1', version: adwaita_req)

# Compiler flags and warnings
global_c_args = [
  '-DDSPY_COMPILATION',
  '-DG_LOG_DOMAIN="D-Spy"',
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_@0@_@1@'.format(glib_major, glib_minor),
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_@0@_@1@'.format(glib_major, glib_minor),
  '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_@0@_@1@'.format(gtk_major, gtk_minor),
  '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_@0@_@1@'.format(gtk_major, gtk_minor),
]
if cc.get_id() == 'msvc'
  test_c_args = [
    '/FImsvc_recommended_pragmas.h',
  ]
else
  test_c_args = [
    '-Wcast-align',
    '-Werror=address',
    '-Werror=array-bounds',
    '-Werror=empty-body',
    '-Werror=implicit',
    '-Werror=implicit-function-declaration',
    '-Werror=init-self',
    '-Werror=int-conversion',
    '-Werror=int-to-pointer-cast',
    '-Werror=main',
    '-Werror=misleading-indentation',
    '-Werror=missing-braces',
    '-Werror=missing-include-dirs',
    '-Werror=nonnull',
    '-Werror=overflow',
    '-Werror=parenthesis',
    '-Werror=pointer-arith',
    '-Werror=pointer-to-int-cast',
    '-Werror=return-type',
    '-Werror=sequence-point',
    '-Werror=shadow',
    '-Werror=strict-prototypes',
    '-Werror=trigraphs',
    '-Werror=undef',
    '-Werror=write-strings',
    '-Wformat-nonliteral',
    ['-Werror=format-security', '-Werror=format=2' ],
    '-Wignored-qualifiers',
    '-Wincompatible-pointer-types',
    '-Wimplicit-function-declaration',
    '-Wlogical-op',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wnested-externs',
    '-Wno-cast-function-type',
    '-Wno-missing-field-initializers',
    '-Wno-sign-compare',
    '-Wno-unused-parameter',

    # XXX: temporarily during prototyping
    '-Wno-unused-variable',

    '-Wno-typedef-redefinition',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Wstrict-prototypes',
    '-Wswitch-default',
    '-Wswitch-enum',
    '-Wundef',
    '-Wuninitialized',
    '-Wunused',
    '-fno-strict-aliasing',
  ]
  # Ignore some Werror for clang until GLib fixes volatile usage in
  # GType registration.
  if meson.get_compiler('c').get_id() == 'clang'
    test_c_args += ['-Wno-incompatible-pointer-types-discards-qualifiers']
  endif

  if buildtype != 'plain'
    test_c_args += '-fstack-protector-strong'
  endif
endif

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach

if cc.get_id() != 'msvc' and cc.has_multi_arguments('-Wmissing-declarations')
  global_c_args += '-Wmissing-declarations'
endif

add_project_arguments(global_c_args, language: 'c')

# Detect and set symbol visibility
if get_option('default_library') != 'static'
  if host_machine.system() == 'windows'
    config_h.set('DLL_EXPORT', true)
    if cc.get_id() == 'msvc'
      config_h.set('_DSPY_EXTERN', '__declspec(dllexport) extern')
    elif cc.has_argument('-fvisibility=hidden')
      config_h.set('_DSPY_EXTERN', '__attribute__((visibility("default"))) __declspec(dllexport) extern')
    endif
  elif cc.has_argument('-fvisibility=hidden')
    config_h.set('_DSPY_EXTERN', '__attribute__((visibility("default"))) extern')
  endif
endif

# Various optimizations for non-debug builds including disabling of
# cast checks, asserts, and additional link options.
release_link_args = []
test_link_args = [
  '-Wl,-z,defs',   # Detect and reject underlinking
  '-Wl,-z,now',    # Disable lazy binding
  '-Wl,-z,relro',  # Read-only segments after relocation
]
if not buildtype.startswith('debug')
  add_global_arguments([
    '-DG_DISABLE_ASSERT',
    '-DG_DISABLE_CAST_CHECKS',
  ], language: 'c')
  test_link_args += ['-Wl,-Bsymbolic', '-fno-plt']
endif
foreach link_arg: test_link_args
  if cc.has_link_argument(link_arg)
    release_link_args += link_arg
  endif
endforeach

configure_file(
         output: 'config.h',
  configuration: config_h,
)

root_include_directory = include_directories('.')

subdir('data')
subdir('lib')
subdir('src')
subdir('po')

meson.add_install_script('build-aux/meson/postinstall.py')
