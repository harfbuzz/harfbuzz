# AGENTS.md

This repository is HarfBuzz, a text shaping engine with stable public API and ABI expectations. Keep changes narrow, preserve existing behavior unless the task explicitly requires a change, and avoid style-only churn.

## First read

Start with these files before making non-trivial changes:

- `README.md`: project overview and entry points.
- `BUILD.md`: standard Meson build flow and dependencies.
- `TESTING.md`: common test and debug commands.
- `CONFIG.md`: feature flags and size-sensitive build options.
- `RELEASING.md`: release-related expectations; note that some generated version files can change after tests.

## Repository map

- `src/`: core library implementation.
- `src/OT/`: OpenType shaping/layout internals.
- `src/graph/`: graph helpers.
- `src/rust/`: Rust integration, including HarfRust support.
- `src/wasm/`: WebAssembly-related code.
- `util/`: command-line tools such as `hb-shape`, `hb-view`, `hb-info`, `hb-subset`, and raster/vector utilities.
- `test/`: API, shaping, subset, vector, thread, and fuzzing tests.
- `perf/`: benchmark programs, including `benchmark-shape`.
- `docs/`: documentation build inputs.

## Working style

- Prefer minimal diffs. Do not rename, reorder, or reformat unrelated code.
- Treat public API and ABI stability as a hard constraint.
- Follow existing local conventions in the touched file instead of applying a new style.
- When editing generated or table-like files, confirm whether the source generator should be changed instead.
- Preserve optional-feature behavior. Many code paths are gated by Meson options or compile-time defines.
- Leave unrelated untracked files and user changes alone. This tree is often used with local test artifacts.

## Build and test

Default development flow:

```sh
meson setup build --reconfigure
meson compile -C build
meson test -C build
```

If `build/` does not exist yet, omit `--reconfigure`:

```sh
meson setup build
meson compile -C build
meson test -C build
```

Useful variants:

- Run one test under GDB: `meson test -C build --gdb <testname>`
- Reconfigure with debug logging: `CPPFLAGS=-DHB_DEBUG_SUBSET=100 meson setup build --reconfigure`
- ASan build: `meson setup build -Db_sanitize=address --reconfigure && meson compile -C build && meson test -C build`
- Vector suite example: `meson test -C build -v --suite vector`

## Verification guidance

- For changes under `src/` that affect shaping, run `meson test -C build`.
- For `util/` changes, at minimum rebuild the relevant tool and exercise its CLI on a small sample.
- For `test/subset/` or subset-related library changes, run the full test suite unless the task is explicitly isolated.
- For performance-sensitive shaping changes, prefer a release-style benchmark build:

```sh
meson setup build -Dbenchmark=enabled -Dbuildtype=release --reconfigure
meson compile -C build
build/perf/benchmark-shape --benchmark_filter='(ot|harfrust)'
```

- For HarfRust comparison work, enable the HarfRust backend explicitly:

```sh
meson setup build -Dbenchmark=enabled -Dharfrust=enabled -Dbuildtype=release --reconfigure
meson compile -C build
HB_SHAPER_LIST=harfrust meson test -C build
build/perf/benchmark-shape --benchmark_filter='(ot|harfrust)'
```

If you modify Rust-side HarfRust integration and Meson does not notice the change, touching `src/rust/shaper.rs` is a known way to force a rebuild of that part of the tree.

## Dependencies and options

- Meson is the primary build system. CMake exists, but prefer Meson unless the task is explicitly about CMake.
- Tests and utilities depend on optional libraries such as GLib, Cairo, and FreeType. Do not assume every local build has every feature enabled.
- Relevant Meson options live in `meson_options.txt`; common ones include `tests`, `utilities`, `benchmark`, `subset`, `raster`, `vector`, `harfrust`, and `fontations`.
- Size-sensitive builds use compile-time configuration in `src/hb-config.hh` and the guidance in `CONFIG.md`.

## Change-specific advice

- Shaping changes: check nearby script- or font-specific tests under `test/shape/`.
- Raster/vector work: inspect the matching utilities in `util/` and feature toggles in `meson_options.txt`.
- Benchmark work: keep correctness first, then compare `ot` versus alternate backends using `perf/benchmark-shape`.
- Rust integration work: inspect `src/rust/meson.build` and related Cargo files before changing build glue.

## Avoid

- Do not make broad formatting passes.
- Do not silently change Meson options or feature defaults unless the task requires it.
- Do not assume untracked fonts, images, or generated outputs are disposable.
- Do not disable tests to get a green run without documenting the reason.

## Wisdom

- **Surgical Precision:** HarfBuzz is at the bottom of the stack for millions of users. A single-line change can have massive ripple effects. Prioritize minimal, targeted diffs over broad refactors.
- **Empirical Validation:** Never assume a fix works until you've reproduced the failure with a test case and then seen it pass with your changes.
- **Historical Context:** If a piece of code looks unnecessarily complex, it likely handles a specific edge case for a legacy font or a broken shaper implementation. Use `git blame` and check `NEWS` before "simplifying" it.
- **Cross-Platform Mindset:** HarfBuzz runs on everything from embedded systems to web browsers. Avoid platform-specific assumptions and stick to the established portability patterns in the codebase.
- **Leave it Better:** If you discover a nuance about the build system or a specific sub-directory that wasn't documented here, update this file.
