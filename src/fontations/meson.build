rust = import('unstable-rust')

hb_rs = rust.bindgen(
  input : '../hb.h',
  output : 'hb.rs',
  include_directories: incsrc,
  args : ['--allowlist-function=hb_.*', '--allowlist-type=hb_.*'],
  #args : ['--no-rustfmt-bindings'],
)

cargo = find_program('cargo')

harfbuzz_fontations = custom_target(
  'harfbuzz_fontations',
  input: ['lib.rs'],
  output: ['libharfbuzz_fontations.a'],
  depends: [hb_rs],
  env: ['OUT_DIR=' + meson.current_build_dir()],
  command: [
    cargo, 'build',
    '--release',
    '--lib',
    '--target-dir', meson.current_build_dir(),
    '-Z', 'unstable-options',
    '--artifact-dir', meson.current_build_dir(),
    '--manifest-path', meson.current_source_dir() / 'Cargo.toml'
  ],
  install: true,
  install_dir: join_paths(get_option('prefix'), 'lib'),
)

harfbuzz_fontations_dep = declare_dependency(
  link_whole: harfbuzz_fontations,
)
