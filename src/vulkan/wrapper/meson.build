wrapper_icd = custom_target(
  'wrapper_icd',
  input: [vk_icd_gen, vk_api_xml],
  output: 'wrapper_icd.@0@.json'.format(host_machine.cpu()),
  command: [
    prog_python,
    '@INPUT0@',
    '--api-version', '1.3',
    '--xml', '@INPUT1@',
    '--lib-path', join_paths(get_option('prefix'), get_option('libdir'), 'libvulkan_wrapper.so'),
    '--out', '@OUTPUT@',
  ],
  build_by_default: true,
  install_dir: with_vulkan_icd_dir,
  install: true,
)

wrapper_entrypoints = custom_target(
  'wrapper_entrypoints',
  input: [vk_entrypoints_gen, vk_api_xml],
  output: ['wrapper_entrypoints.h', 'wrapper_entrypoints.c'],
  command: [
    prog_python,
    '@INPUT0@',
    '--xml', '@INPUT1@',
    '--proto',
    '--weak',
    '--beta', with_vulkan_beta.to_string(),
    '--out-h', '@OUTPUT0@',
    '--out-c', '@OUTPUT1@',
    '--prefix', 'wrapper',
  ],
  depend_files: vk_entrypoints_gen_depend_files,
)

wrapper_trampolines_gen = files('vk_wrapper_trampolines_gen.py')

wrapper_trampolines = custom_target(
  'wrapper_trampolines',
  input: [wrapper_trampolines_gen, vk_api_xml],
  output: ['wrapper_trampolines.c', 'wrapper_trampolines.h'],
  command: [
    prog_python,
    '@INPUT0@',
    '--xml', '@INPUT1@',
    '--out-c', '@OUTPUT0@',
    '--out-h', '@OUTPUT1@',
    '--beta', with_vulkan_beta.to_string(),
  ],
  depend_files: vk_dispatch_trampolines_gen_depend_files,
)

wrapper_features_gen = files('vk_wrapper_features_gen.py')

wrapper_features = custom_target(
  'vk_wrapper_features',
  input: [wrapper_features_gen, vk_api_xml],
  output: ['wrapper_features.c'],
  command: [
    prog_python,
    '@INPUT0@',
    '--xml', '@INPUT1@',
    '--out-c', '@OUTPUT0@',
    '--beta', with_vulkan_beta.to_string(),
  ],
  depend_files: vk_physical_device_features_gen_depend_files,
)

unwrapper_gen = files('vk_unwrapper_gen.py')

unwrappers = custom_target(
  'vk_unwrappers',
  input: [unwrapper_gen, vk_api_xml, files('vk_unwrapper.c.mako'), files('vk_unwrapper.h.mako')],
  output: ['vk_unwrappers.c', 'vk_unwrappers.h'],
  command: [
    prog_python,
    '@INPUT0@',
    '--xml', '@INPUT1@',
    '--out-c', '@OUTPUT0@',
    '--out-h', '@OUTPUT1@',
    '--in-c', '@INPUT2@',
    '--in-h', '@INPUT3@',
    # '--beta', with_vulkan_beta.to_string(),
  ],
  depend_files: vk_physical_device_features_gen_depend_files,
)

vk_printers = custom_target(
  'vk_printers',
  input: [unwrapper_gen, vk_api_xml, files('vk_printer.c.mako'), files('vk_printer.h.mako')],
  output: ['vk_printers.c', 'vk_printers.h'],
  command: [
    prog_python,
    '@INPUT0@',
    '--xml', '@INPUT1@',
    '--out-c', '@OUTPUT0@',
    '--out-h', '@OUTPUT1@',
    '--in-c', '@INPUT2@',
    '--in-h', '@INPUT3@',
    # '--beta', with_vulkan_beta.to_string(),
  ],
  depend_files: vk_physical_device_features_gen_depend_files,
)


# adrenotools_subproj = subproject('libadrenotools')

#adrenotools_dep = declare_dependency(
#  include_directories: adrenotools_subproj.get_variable('adrenotools_inc'),
#  link_with: adrenotools_subproj.get_variable('adrenotools_lib'),
#)


wrapper_files = files(
  'wrapper_device.c',
  'wrapper_device_memory.c',
  'wrapper_instance.c',
  'wrapper_physical_device.c',
  'wrapper_image.c',
  'vk_texcompress_bc.c',
)

wrapper_deps = [
  idep_vulkan_runtime,
  idep_vulkan_util,
#  adrenotools_dep,
  idep_vulkan_wsi,
  dep_android,
]

bc_decode_shader = custom_target(
  'bc_decode_spv.h',
  input : files('bc_decode.comp'),
  output : 'bc_decode.spv.h',
  command : [
      prog_glslang, '-V', '-S', 'comp', '-x', '-o', '@OUTPUT@', '@INPUT@',
      glslang_quiet, glslang_depfile,
  ],
  depfile : 'bc_decode_spv.h.d',
)

buffer_to_image = custom_target(
  'buffer_to_image.spv.h',
  input : files('buffer_to_image.comp'),
  output : 'buffer_to_image.spv.h',
  command : [
      prog_glslang, '-V', '-S', 'comp', '-x', '-o', '@OUTPUT@', '@INPUT@',
      glslang_quiet, glslang_depfile,
  ],
  depfile : 'buffer_to_image.spv.h.d',
)

magenta = custom_target(
  'magenta.spv.h',
  input : files('fill_magenta.comp'),
  output : 'magenta.spv.h',
  command : [
      prog_glslang, '-V', '-S', 'comp', '-x', '-o', '@OUTPUT@', '@INPUT@',
      glslang_quiet, glslang_depfile,
  ],
  depfile : 'magenta.spv.h.d',
)

s3tc = custom_target(
  's3tc.spv.h',
  input : [files('s3tc.comp')],
  output : 's3tc.spv.h',
  command : [
      prog_glslang, '-V', '-S', 'comp', '-x', '-o', '@OUTPUT@', '@INPUT@',
      glslang_quiet, glslang_depfile,
  ],
  depfile : 's3tc.spv.h.d',
)

bc7 = custom_target(
  'bc7.spv.h',
  input : [files('bc7.comp')],
  output : 'bc7.spv.h',
  command : [
      prog_glslang, '-V', '-S', 'comp', '-x', '-o', '@OUTPUT@', '@INPUT@',
      glslang_quiet, glslang_depfile,
  ],
  depfile : 'bc7.spv.h.d',
)


libvulkan_wrapper = shared_library(
  'vulkan_wrapper',
  [wrapper_entrypoints, wrapper_trampolines, wrapper_features, wrapper_files, unwrappers, vk_printers, bc_decode_shader, buffer_to_image, magenta, s3tc, bc7],
  include_directories: [
    inc_include,
    inc_src,
  ],
  dependencies: [wrapper_deps, vulkan_wsi_deps],
  gnu_symbol_visibility: 'hidden',
  install: true,
)
