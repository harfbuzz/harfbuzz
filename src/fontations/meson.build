harfbuzz_dep = dependency('harfbuzz', required: true)

harfbuzz_fontations_rust = static_library(
  'harfbuzz_fontations_rust',
  'lib.rs',
  dependencies: [harfbuzz_dep],
  rust_abi: 'c',
  install: true,
)
