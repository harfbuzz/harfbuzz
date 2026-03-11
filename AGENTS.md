# AGENTS.md

HarfBuzz is a low-level text shaping engine with stable public API and ABI expectations. Keep changes narrow, preserve behavior unless the task explicitly requires otherwise, and avoid style-only churn.

## Read first

Before non-trivial work, read:

- `README.md`
- `BUILD.md`
- `TESTING.md`
- `CONFIG.md`
- `RELEASING.md`

## Core rules

- Prefer minimal diffs. Do not rename, reorder, or reformat unrelated code.
- Scope changes to one subsystem or concern when possible. Prefer incremental follow-up commits over large rewrites.
- Public API and ABI are hard constraints. Do not add new API unless explicitly asked. Do not change or remove existing API/ABI unless explicitly asked.
- Follow local conventions in the touched file.
- Preserve optional-feature behavior, reduced-feature builds, and compile-time feature guards.
- Keep out-of-memory behavior in mind. New code should fail safely and follow existing allocation and error-handling patterns.
- Prefer HarfBuzz's established `likely`/`unlikely` and nil-object/null-pattern conventions where they fit the surrounding code.
- Prefer HarfBuzz helpers from `hb-algs.hh` such as `hb_memcpy`, `hb_memcmp`, and `hb_memset` over raw `memcpy`, `memcmp`, and `memset` where those helpers are available.
- Do not introduce STL containers in library code. Follow HarfBuzz container and utility patterns instead.
- Leave unrelated user changes and untracked artifacts alone.

## Repo map

- `src/`: core library code.
- `src/OT/`: OpenType shaping/layout internals.
- `src/graph/`: subsetter graph helpers.
- `src/rust/`: HarfRust and fontations integration.
- `util/`: installed tools such as `hb-shape`, `hb-view`, `hb-info`, `hb-subset`, `hb-vector`, `hb-raster`.
- `test/`: API, shape, subset, vector, threads, and fuzzing tests.
- `perf/`: benchmarks.
- `docs/`: API/manual docs.

## Build and verify

Default flow:

```sh
meson setup build --reconfigure
meson compile -C build
meson test -C build
```

If `build/` does not exist yet, omit `--reconfigure`.

General expectations:

- Rebuild touched targets and make sure there are no new warnings.
- For shaping changes, run at least `meson test -C build --suite shape`.
- For subset changes, run at least `meson test -C build --suite subset`.
- For API changes, run at least `meson test -C build --suite api`.
- For `util/` changes, rebuild the relevant tool and exercise it on a small sample.
- When fixing a bug or hardening a parser, add or update the nearest regression test, fuzz seed, fuzz target, or expected output when practical.
- Treat portability regressions as real bugs. HarfBuzz regularly fixes GCC, Clang, MSVC, 32/64-bit, and reduced-feature build issues.

## Change-specific guidance

- Shaping work: check nearby tests under `test/shape/`.
- Subsetting work: check `test/subset/`; `src/graph/` is part of the subsetter.
- Raster/vector/SVG work: prefer extracting helpers and splitting oversized files by responsibility instead of adding more branching to large translation units.
- Fuzzing/parser-hardening work: inspect `test/fuzzing/` and existing guardrail code first.
- Build-system work: keep Meson options, summaries, installed tools, tests, and dependent targets in sync. If packaging or cross-build behavior is affected, inspect CMake too.
- Generated data work: update the generator or makefile flow, then regenerate outputs. Do not hand-edit generated tables unless the task explicitly calls for it.
- Rust integration work: inspect `src/rust/meson.build` and `src/rust/Cargo.toml` before changing build glue.

For API work:

- Public API lives in `src/hb-*.h`, not `.hh`.
- If new API is explicitly requested, document it in `NEWS`, add `XSince: REPLACEME` in the public header docs, and update `docs/harfbuzz-sections.txt` and `docs/harfbuzz-docs.xml` when needed.
- If functionality becomes user-visible through tools or libraries, update the relevant docs and install/build wiring in the same change when appropriate.
- Keep documentation placement consistent with local practice: public API docs may live in source files, while generated section indexes live in `docs/`.

## Avoid

- Do not make broad formatting passes.
- Do not silently change Meson defaults or feature defaults unless required.
- Do not assume generated outputs, local fonts, or untracked artifacts are disposable.
- Do not disable tests to get a green run without documenting why.

## Working mindset

- Reproduce the issue before fixing it when practical.
- Understand root cause before writing code. If there is real ambiguity, ask instead of guessing.
- Validate simplifications carefully. This repo has many targeted reverts where a local cleanup broke broader behavior.
- If code looks unusually complex, it probably handles a real edge case. Check history before simplifying it.
- Centralize duplicated parsing, caching, or helper logic once duplication starts spreading.

## Commit guidance

- Before any commit, run the entire test suite with `meson test -C build`.
- Exception: simple documentation-only changes may be committed without running tests if they do not affect code, build logic, generated outputs, or test inputs.
- Use descriptive commit messages with consistent bracketed subsystem prefixes such as `[subset]`, `[raster]`, `[util]`, or `[meson]`.
- Wrap commit message bodies to about 70 columns.
- For multi-line commit messages, write the message from a file or editor-backed input. Do not pass escaped `\n` sequences via shell `-m` arguments.
- Explain root cause, fix, and testing in the commit body when testing was actually performed or is relevant.
- When relevant, link issues or PRs with trailers such as `Fixes:`.
- Always include an `Assisted-by:` trailer on commits you write through the agent.

## Wisdom

- HarfBuzz sits at the bottom of the stack. Small changes can have very wide effects.
- If code looks unusually complex, assume it handles a real edge case until proven otherwise.
- Simplifications and optimizations are common sources of later reverts; validate them broadly.
- Prefer reproducible regressions over fix-only changes.
- When duplication starts to spread, extract the shared helper before it calcifies.
