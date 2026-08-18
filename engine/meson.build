project('ibus-engine-inkey', 'c',
  version : '0.1.0',
  default_options : ['c_std=gnu11', 'warning_level=2'])

ibus_dep = dependency('ibus-1.0')
glib_dep = dependency('glib-2.0')
gobject_dep = dependency('gobject-2.0')

core_lib_dir = meson.project_source_root() / '..' / 'core' / 'target' / 'release'

core_dep = declare_dependency(
  link_args : ['-L' + core_lib_dir, '-linkey_core', '-Wl,-rpath,' + core_lib_dir],
  include_directories : include_directories('src'),
)

engine_exe = executable('ibus-engine-inkey',
  'src/main.c',
  'src/engine.c',
  dependencies : [ibus_dep, glib_dep, gobject_dep, core_dep],
  install : false,
)

conf_data = configuration_data()
conf_data.set('EXEC_PATH', engine_exe.full_path())

configure_file(
  input : 'data/inkey.xml.in',
  output : 'inkey.xml',
  configuration : conf_data,
)

test_modifier_passthrough_exe = executable('test-modifier-passthrough',
  'tests/test_modifier_passthrough.c',
  'src/engine.c',
  dependencies : [ibus_dep, glib_dep, gobject_dep, core_dep],
  install : false,
)
test('modifier passthrough', test_modifier_passthrough_exe)

test_word_boundary_exe = executable('test-word-boundary',
  'tests/test_word_boundary.c',
  'src/engine.c',
  dependencies : [ibus_dep, glib_dep, gobject_dep, core_dep],
  install : false,
)
test('word boundary', test_word_boundary_exe)

test_commit_e2e_exe = executable('test-commit-e2e',
  'tests/test_commit_e2e.c',
  'src/engine.c',
  dependencies : [ibus_dep, glib_dep, gobject_dep, core_dep],
  install : false,
)
test('commit end-to-end', test_commit_e2e_exe)
