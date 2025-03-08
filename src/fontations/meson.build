rust = import('unstable-rust')

hb_rs = rust.bindgen(
  input : '../hb.h',
  output : 'hb.rs',
  include_directories: incsrc,
  args : ['--allowlist-function=hb_.*',
          '--allowlist-type=hb_.*',
          '--no-copy=hb_.*',
          ],
)

cargo = find_program('cargo')
rustfmt = find_program('rustfmt')

rust_flags = ''
cargo_args = []

buildtype = get_option('buildtype')
if buildtype == 'release' or buildtype == 'debugoptimized'
  cargo_args += ['--profile', buildtype]
endif

opt_level = get_option('optimization')
rust_flags += ' -C opt-level=' + opt_level

cargo_args += ['-Z', 'build-std=std,panic_abort',
               '-Z', 'build-std-features=panic_immediate_abort']

harfbuzz_fontations = custom_target(
  'harfbuzz_fontations',
  input: ['lib.rs', 'Cargo.toml', 'Cargo.lock'],
  output: ['libharfbuzz_fontations.a'],
  depends: [hb_rs],
  env: ['OUT_DIR=' + meson.current_build_dir(),
        'RUSTFLAGS=' + rust_flags,
  ],
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

clippy = run_target(
  'clippy',
  env: ['OUT_DIR=' + meson.current_build_dir()],
  command: [
    cargo, 'clippy',
    '--allow-dirty', '--fix',
    ] + cargo_args + [
    '--lib', '-p', 'harfbuzz_fontations',
    '--target-dir', meson.current_build_dir(),
    '--manifest-path', meson.current_source_dir() / 'Cargo.toml',
  ],
)
clippy_check = run_target(
  'clippy-check',
  env: ['OUT_DIR=' + meson.current_build_dir()],
  command: [
    cargo, 'clippy',
    ] + cargo_args + [
    '--lib', '-p', 'harfbuzz_fontations',
    '--target-dir', meson.current_build_dir(),
    '--manifest-path', meson.current_source_dir() / 'Cargo.toml',
    '--', '-D', 'warnings',
  ],
)

fmt = run_target(
  'fmt',
  env: ['OUT_DIR=' + meson.current_build_dir()],
  command: [
    rustfmt,
    '--edition', '2021',
    '--',
    meson.current_source_dir() / 'lib.rs',
  ],
)
fmt_check = run_target(
  'fmt-check',
  env: ['OUT_DIR=' + meson.current_build_dir()],
  command: [
    rustfmt,
    '--check',
    '--edition', '2021',
    '--',
    meson.current_source_dir() / 'lib.rs',
  ],
)
