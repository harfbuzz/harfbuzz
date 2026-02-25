Adding tests
============

`test/vector` validates `hb-vector` CLI output by comparing exact stdout with
golden files.

Test file format (`data/tests/*.tests`)
---------------------------------------

Each non-empty, non-comment line has 4 semicolon-separated fields:

```
font-file;options;unicodes;expected-file
```

- `font-file`: relative to the `.tests` file (or absolute path).
- `options`: shell-split arguments passed to `hb-vector`.
- `unicodes`: Unicode sequence, using shape-style codepoint format
  (for example `U+0061,U+0062,U+0063`).
- `expected-file`: path to golden output, relative to the `.tests` file.
  This is also used as the TAP test name.

Example:

```
../../../api/fonts/Roboto-Regular.abc.ttf;--font-size=64;U+0061,U+0062,U+0063;../expected/basic/reuse-svg.svg
```

Running vector tests
--------------------

In an in-tree build:

```sh
meson test -C build-default -v --suite vector
```

Updating expected output
------------------------

Regenerate a golden file with the current `hb-vector` binary and replace the
file under `data/expected/`.

Recording a new test case
-------------------------

Use `record-test.sh`:

```sh
./record-test.sh -o data/tests/basic.tests --test-name reuse-svg ../../build-default/util/hb-vector ../api/fonts/Roboto-Regular.abc.ttf --font-size=64 abc
```

It subsets the input font, verifies subset-vs-original rendering with
`hb-view`, stores the subset font under `../fonts/<sha1>.ttf` (relative to the target
`.tests` file), writes expected output under
`../expected/<tests-stem>/<test-name>.<ext>` by default (unless
`--expected-dir` or `--expected-file` is provided), and
appends one test line to the target `.tests` file. `hb-shape`, `hb-view`, and
`hb-subset` are resolved as sibling executables next to the provided
`hb-vector` binary. The emitted test line uses Unicode codepoint input in the
`unicodes` field.
