project('Moduler-Model', 'c', license: 'GPL-3.0-or-later', version: '1.0')

onnxruntime_dep = dependency('libonnxruntime')
glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')

cc = meson.get_compiler('c')

math_dep = cc.find_library('m')

inc = include_directories('include')

moduler_model = library('moduler_model',
  'src/mm-allocator.c', 'src/mm-context.c', 'src/mm-model.c',
  'src/mm-model-io.c', 'src/mm-model-options.c', 'src/mm-value.c',
  'src/mm-value-info.c', 'src/mm-file.c', 'src/mm-provider.c',
  include_directories: inc, dependencies: [onnxruntime_dep, glib_dep, gio_dep])

moduler_model_dep = declare_dependency(
  include_directories: inc, link_with: moduler_model)
