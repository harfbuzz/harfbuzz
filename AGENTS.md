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
  - `src/OT/`: OpenType shaping/layout internals (`Color/`, `glyf/`, `Layout/`, `name/`, `Var/`).
  - `src/graph/`: graph helpers for subsetting (GSUBGPOS repacking).
  - `src/ms-use/`: Microsoft Universal Shaping Engine data files.
  - `src/rust/`: Rust integration—HarfRust shaper and fontations/skrifa font backend. Requires Rust >= 1.87.0.
  - `src/wasm/`: WebAssembly shaper (experimental); includes sample code.
- `util/`: installed command-line tools: `hb-shape`, `hb-view`, `hb-info`, `hb-subset`, `hb-vector`, `hb-raster`.
- `test/`: organized into subdirectories:
  - `test/api/`: C/C++ API unit tests.
  - `test/shape/`: shaping regression tests (per-script and per-font).
  - `test/subset/`: subsetting and instancing tests.
  - `test/vector/`: vector drawing tests.
  - `test/threads/`: thread-safety tests.
  - `test/fuzzing/`: fuzzer entry points and corpora (see `test/fuzzing/README.md`).
- `perf/`: benchmarks using Google Benchmark (`benchmark-shape`, `benchmark-font`, `benchmark-subset`, etc.). See `perf/README.md`.
- `subprojects/`: Meson wrap files for optional dependencies (Cairo, FreeType, GLib, ICU, Graphite2, Google Benchmark, etc.).
- `docs/`: documentation build inputs (gtk-doc).

## Working style

- Prefer minimal diffs. Do not rename, reorder, or reformat unrelated code.
- Treat public API and ABI stability as a hard constraint.
- Do not add new public API unless the task explicitly asks for it. Never change or remove existing public API/ABI unless explicitly requested.
- Follow existing local conventions in the touched file instead of applying a new style.
- When editing generated or table-like files, confirm whether the source generator should be changed instead.
- Preserve optional-feature behavior. Many code paths are gated by Meson options or compile-time defines.
- Keep out-of-memory behavior in mind. New code should fail safely and fit HarfBuzz's existing allocation/error patterns.
- Prefer HarfBuzz's established `likely`/`unlikely` conventions where they improve clarity and match surrounding code.
- Prefer minimizing error-handling branching, using HarfBuzz's nil-object/null-pattern conventions where appropriate.
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
- Run one test suite: `meson test -C build -v --suite vector`
- Reconfigure with debug logging: `CPPFLAGS=-DHB_DEBUG_SUBSET=100 meson setup build --reconfigure`
- ASan build: `meson setup build -Db_sanitize=address --reconfigure && meson compile -C build && meson test -C build`

Available `HB_DEBUG_*` flags (defined in `src/hb-debug.hh`): `APPLY`, `ARABIC`, `BLOB`, `CORETEXT`, `DIRECTWRITE`, `DISPATCH`, `FT`, `JUSTIFY`, `KBTS`, `OBJECT`, `PAINT`, `SANITIZE`, `SERIALIZE`, `SHAPE_PLAN`, `SUBSET`, `SUBSET_REPACK`, `UNISCRIBE`, `WASM`. Set to a level (e.g. `100`) via `CPPFLAGS=-DHB_DEBUG_<FLAG>=100`.

## Verification guidance

- For any code change, rebuild and make sure the touched targets compile without new warnings before considering the work done.
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

The fontations font backend can be tested similarly:

```sh
meson setup build -Dfontations=enabled -Dbuildtype=release --reconfigure
meson compile -C build
meson test -C build
```

## Dependencies and options

- Meson (>= 0.60.0) is the primary build system. CMake exists, but prefer Meson unless the task is explicitly about CMake.
- There is also an amalgamated source (`src/harfbuzz.cc`) for embedding HarfBuzz without a build system: `c++ -c src/harfbuzz.cc`.
- The CLI tools require GLib; `hb-view` additionally requires Cairo. Most tests require GLib.
- Relevant Meson options live in `meson_options.txt`. Key feature options (all `feature` type—`auto`/`enabled`/`disabled`):
  - Always on by default: `raster`, `vector`, `subset`, `tests`, `utilities`.
  - Off by default: `harfrust`, `fontations`, `benchmark`, `graphite2`, `wasm`, `kbts`.
  - Auto-detected: `glib`, `gobject`, `cairo`, `chafa`, `freetype`, `icu`, `introspection`, `docs`.
  - Platform-specific (off by default): `coretext`, `directwrite`, `gdi`.
- Size-sensitive builds use compile-time configuration in `src/hb-config.hh` and the guidance in `CONFIG.md`. Pre-defined profiles: `HB_MINI`, `HB_LEAN`, `HB_TINY`.

## Change-specific advice

- Shaping changes: check nearby script- or font-specific tests under `test/shape/`. Run at least `meson test -C build --suite shape`.
- Subsetting changes: run `meson test -C build --suite subset`. The `src/graph/` code is part of the subsetter's table repacking.
- Raster/vector work: inspect the matching utilities in `util/` and feature toggles in `meson_options.txt`.
- API changes: run `meson test -C build --suite api`. Public API lives in `src/hb-*.h` headers (no `.hh`). ABI is tracked; do not remove or change signatures of public symbols.
- New API, when explicitly requested: document it in `NEWS`, add an `XSince: REPLACEME` annotation in the public header docs, and hook it up in `docs/harfbuzz-sections.txt` (and `docs/harfbuzz-docs.xml` when needed).
- Benchmark work: keep correctness first, then compare `ot` versus alternate backends using `perf/benchmark-shape`.
- Rust integration work: inspect `src/rust/meson.build` and `src/rust/Cargo.toml` before changing build glue. The Rust crate defines two optional features: `font` (fontations/skrifa backend) and `shape` (HarfRust backend).

## Avoid

- Do not make broad formatting passes.
- Do not silently change Meson options or feature defaults unless the task requires it.
- Do not assume untracked fonts, images, or generated outputs are disposable.
- Do not disable tests to get a green run without documenting the reason.

## Wisdom

- **Surgical Precision:** HarfBuzz is at the bottom of the stack for millions of users. A single-line change can have massive ripple effects. Prioritize minimal, targeted diffs over broad refactors.
- **Empirical Validation:** Never assume a fix works until you've reproduced the failure with a test case and then seen it pass with your changes.
- **Root Cause First:** Understand the bug or request clearly before writing code. If the root cause is still unclear or multiple interpretations are plausible, ask instead of guessing.
- **Historical Context:** If a piece of code looks unnecessarily complex, it likely handles a specific edge case for a legacy font or a broken shaper implementation. Use `git blame` and check `NEWS` before "simplifying" it.
- **Cross-Platform Mindset:** HarfBuzz runs on everything from embedded systems to web browsers. The project builds with C++11 and avoids platform-specific assumptions. Stick to the established portability patterns in the codebase.
- **Feature Guards:** Many code paths are conditionally compiled. Check `#ifdef`/`#ifndef` guards and Meson feature options before assuming a function or code path is always available.
- **Commit Hygiene:** Write descriptive commit messages that explain the root cause, the chosen fix, and how the change was tested.
- **Leave it Better:** If you discover a nuance about the build system or a specific sub-directory that wasn't documented here, update this file.
