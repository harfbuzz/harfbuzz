rust = import('unstable-rust')

hb_rs = rust.bindgen(
  input : '../hb.h',
  output : 'hb.rs',
  include_directories: incsrc,
  args : ['--allowlist-function=hb_.*',
          '--allowlist-type=hb_.*',
          '--no-copy=hb_.*',
          ],
  #args : ['--no-rustfmt-bindings'],
)

cargo = find_program('cargo')

buildtype = get_option('buildtype')
if buildtype == 'release' or buildtype == 'debugoptimized'
  cargo_args = ['--profile', buildtype]
else
  cargo_args = []
endif

harfbuzz_fontations = custom_target(
  'harfbuzz_fontations',
  input: ['lib.rs', 'Cargo.toml', 'Cargo.lock'],
  output: ['libharfbuzz_fontations.a'],
  depends: [hb_rs],
  env: ['OUT_DIR=' + meson.current_build_dir()],
  command: [
    cargo, 'build',
    ] + cargo_args + [
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
