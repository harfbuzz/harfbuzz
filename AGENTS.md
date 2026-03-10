# AGENTS.md

This repository is HarfBuzz, a text shaping engine with stable public API and ABI expectations. Keep changes narrow, preserve existing behavior unless the task explicitly requires a change, and avoid style-only churn.

## Core rules

- Prefer minimal diffs. Do not rename, reorder, or reformat unrelated code.
- Preserve existing behavior unless the task explicitly requires a change.
- Prefer incremental changes over large rewrites. This repository frequently lands feature work as a series of narrowly-scoped helper extraction, refactoring, and follow-up fix commits.
- Scope work to one subsystem or concern at a time where possible. Across the full history, HarfBuzz changes are usually organized as small, well-labeled steps inside a single area.
- Treat public API and ABI stability as a hard constraint.
- Do not add new public API unless the task explicitly asks for it.
- Never change or remove existing public API/ABI unless explicitly requested.
- Follow existing local conventions in the touched file instead of applying a new style.
- Preserve optional-feature behavior. Many code paths are gated by Meson options or compile-time defines.
- Keep out-of-memory behavior in mind. New code should fail safely and fit HarfBuzz's existing allocation and error-handling patterns.
- Prefer HarfBuzz's established `likely`/`unlikely` conventions where they improve clarity and match surrounding code.
- Prefer minimizing error-handling branching, using HarfBuzz's nil-object/null-pattern conventions where appropriate.
- Leave unrelated untracked files and user changes alone. This tree is often used with local test artifacts.

## First read

Start with these files before making non-trivial changes:

- `README.md`: project overview and entry points.
- `BUILD.md`: standard Meson build flow and dependencies.
- `TESTING.md`: common test and debug commands.
- `CONFIG.md`: feature flags and size-sensitive build options.
- `RELEASING.md`: release-related expectations; some generated version files can change after tests.

## Repository map

- `src/`: core library implementation.
- `src/OT/`: OpenType shaping/layout internals (`Color/`, `glyf/`, `Layout/`, `name/`, `Var/`).
- `src/graph/`: graph helpers for subsetting (GSUB/GPOS repacking).
- `src/ms-use/`: Microsoft Universal Shaping Engine data files.
- `src/rust/`: Rust integration for the HarfRust shaper and fontations/skrifa backend. Requires Rust >= 1.87.0.
- `src/wasm/`: experimental WebAssembly shaper and sample code.
- `util/`: installed command-line tools: `hb-shape`, `hb-view`, `hb-info`, `hb-subset`, `hb-vector`, `hb-raster`.
- `test/api/`: C/C++ API unit tests.
- `test/shape/`: shaping regression tests, organized by script and font.
- `test/subset/`: subsetting and instancing tests.
- `test/vector/`: vector drawing tests.
- `test/threads/`: thread-safety tests.
- `test/fuzzing/`: fuzzer entry points and corpora. See `test/fuzzing/README.md`.
- `perf/`: Google Benchmark-based performance tests such as `benchmark-shape`, `benchmark-font`, and `benchmark-subset`. See `perf/README.md`.
- `subprojects/`: Meson wrap files for optional dependencies such as Cairo, FreeType, GLib, ICU, Graphite2, and Google Benchmark.
- `docs/`: gtk-doc documentation inputs.

## Build flow

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

Available `HB_DEBUG_*` flags, defined in `src/hb-debug.hh`: `APPLY`, `ARABIC`, `BLOB`, `CORETEXT`, `DIRECTWRITE`, `DISPATCH`, `FT`, `JUSTIFY`, `KBTS`, `OBJECT`, `PAINT`, `SANITIZE`, `SERIALIZE`, `SHAPE_PLAN`, `SUBSET`, `SUBSET_REPACK`, `UNISCRIBE`, `WASM`. Set a level with `CPPFLAGS=-DHB_DEBUG_<FLAG>=100`.

## Verification

- For any code change, rebuild and make sure the touched targets compile without new warnings before considering the work done.
- For changes under `src/` that affect shaping, run `meson test -C build`.
- For `util/` changes, at minimum rebuild the relevant tool and exercise its CLI on a small sample.
- For `test/subset/` or subset-related library changes, run the full test suite unless the task is explicitly isolated.
- When fixing a bug or hardening a parser, add or update the nearest regression test, fuzz target, fuzz seed, or expected output whenever practical. Recent history strongly favors reproducible regressions over fix-only commits.
- Treat portability regressions as first-class bugs. The full history includes repeated fixes for GCC/Clang/MSVC, old toolchains, 32/64-bit issues, and optional-dependency builds.

For performance-sensitive shaping changes, prefer a release-style benchmark build:

```sh
meson setup build -Dbenchmark=enabled -Dbuildtype=release --reconfigure
meson compile -C build
build/perf/benchmark-shape --benchmark_filter='(ot|harfrust)'
```

For HarfRust comparison work, enable the HarfRust backend explicitly:

```sh
meson setup build -Dbenchmark=enabled -Dharfrust=enabled -Dbuildtype=release --reconfigure
meson compile -C build
HB_SHAPER_LIST=harfrust meson test -C build
build/perf/benchmark-shape --benchmark_filter='(ot|harfrust)'
```

If you modify Rust-side HarfRust integration and Meson does not notice the change, touching `src/rust/shaper.rs` is a known way to force a rebuild of that part of the tree.

The fontations backend can be tested similarly:

```sh
meson setup build -Dfontations=enabled -Dbuildtype=release --reconfigure
meson compile -C build
meson test -C build
```

## Dependencies and options

- Meson (>= 0.60.0) is the primary build system. CMake exists, but prefer Meson unless the task is explicitly about CMake.
- There is also an amalgamated source for embedding HarfBuzz without a build system: `c++ -c src/harfbuzz.cc`.
- The CLI tools require GLib. `hb-view` also requires Cairo. Most tests require GLib.
- Relevant Meson options live in `meson_options.txt`.
- Key feature options are `feature`-typed and use `auto`, `enabled`, or `disabled`.
- Enabled by default: `raster`, `vector`, `subset`, `tests`, `utilities`.
- Disabled by default: `harfrust`, `fontations`, `benchmark`, `graphite2`, `wasm`, `kbts`.
- Auto-detected: `glib`, `gobject`, `cairo`, `chafa`, `freetype`, `icu`, `introspection`, `docs`.
- Platform-specific and off by default: `coretext`, `directwrite`, `gdi`.
- Size-sensitive builds use compile-time configuration in `src/hb-config.hh`. See `CONFIG.md` for guidance and the predefined profiles `HB_MINI`, `HB_LEAN`, and `HB_TINY`.

## Change-specific guidance

- Shaping changes: check nearby script- or font-specific tests under `test/shape/`. Run at least `meson test -C build --suite shape`.
- Subsetting changes: run `meson test -C build --suite subset`. The `src/graph/` code is part of the subsetter's table repacking.
- Raster/vector work: inspect the matching utilities in `util/` and the relevant feature toggles in `meson_options.txt`.
- Raster/vector/SVG work: prefer extracting shared helpers and splitting oversized files by responsibility rather than adding more branching to one large translation unit. Many recent changes moved code into `*-parse`, `*-defs`, `*-clip`, `*-fill`, `*-gradient`, and similar helpers.
- Fuzzing and parser-hardening work: check `test/fuzzing/`, existing fuzzers, and nearby guardrail code first. The project routinely turns crash fixes into durable fuzz coverage.
- Build-system work: keep Meson summaries, feature options, installed utilities, and dependent test/util targets in sync. If the change affects cross-build or packaging behavior, inspect CMake too.
- Build-system work: preserve support for optional dependencies and reduced-feature builds. History shows frequent fixes for builds without GLib, Cairo, FreeType, ICU, Graphite2, HarfRust, raster, vector, or subset enabled.
- Generated data work: prefer updating the generator or makefile flow, then regenerate outputs. Commits in Unicode, Arabic, Indic, emoji, and tag data usually touch both generator inputs and generated tables together.
- Benchmark work: keep correctness first, then compare `ot` versus alternate backends using `perf/benchmark-shape`.
- Rust integration work: inspect `src/rust/meson.build` and `src/rust/Cargo.toml` before changing build glue. The Rust crate defines two optional features: `font` for the fontations/skrifa backend and `shape` for HarfRust.

For API work:

- Public API lives in `src/hb-*.h` headers, not `.hh`.
- Run `meson test -C build --suite api`.
- ABI is tracked. Do not remove or change signatures of public symbols.
- If new API is explicitly requested, document it in `NEWS`, add an `XSince: REPLACEME` annotation in the public header docs, and hook it up in `docs/harfbuzz-sections.txt` and `docs/harfbuzz-docs.xml` when needed.
- If new functionality is exposed through utilities or libraries, update the relevant docs, manpage/user manual entries, Meson install/build wiring, and build summaries in the same change when applicable.
- Keep documentation placement consistent with project practice: public API docs may live in source files when that is the local convention, while generated section indexes stay in `docs/`.

## Avoid

- Do not make broad formatting passes.
- Do not silently change Meson options or feature defaults unless the task requires it.
- Do not assume untracked fonts, images, or generated outputs are disposable.
- Do not disable tests to get a green run without documenting the reason.

## Working mindset

- **Surgical precision:** HarfBuzz is at the bottom of the stack for millions of users. A one-line change can have wide impact.
- **Empirical validation:** Reproduce the issue first, then verify the fix with a test or direct repro.
- **Root cause first:** Understand the bug or request clearly before writing code. If the root cause is unclear or multiple interpretations are plausible, ask instead of guessing.
- **Expect reversions:** The recent history contains many targeted reverts of changes that were correct in isolation but wrong for the wider codebase. If a simplification or optimization changes semantics, validate it from multiple angles before landing it.
- **Historical context:** If code looks unusually complex, it probably handles a real edge case. Use `git blame` and check `NEWS` before simplifying it.
- **Cross-platform mindset:** HarfBuzz runs on everything from embedded systems to web browsers. Stick to established portability patterns and C++11 constraints.
- **Feature guards:** Check `#ifdef` and Meson feature options before assuming a code path is always present.
- **Share before you duplicate:** A recurring pattern in recent commits is to centralize parsing, caching, view options, and destruction helpers once duplication starts to spread.
- **Commit hygiene:** Write descriptive commit messages that explain the root cause, the chosen fix, and how the change was tested. Prefer consistent bracketed subsystem prefixes in subjects, such as `[subset]`, `[raster]`, `[util]`, or `[meson]`.
- **Commit trailers:** When relevant, link the change to issues or PRs in the commit body using trailers such as `Fixes:`. Always include an `Assisted-by:` trailer on commits you write.
- **Leave it better:** If you learn a repo-specific nuance that is not captured here, update this file.
