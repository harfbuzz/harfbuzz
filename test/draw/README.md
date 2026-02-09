Adding tests
============

`test/draw` validates `hb-draw` CLI output by comparing exact stdout with
golden files.

Test file format (`data/tests/*.tests`)
---------------------------------------

Each non-empty, non-comment line has 4 semicolon-separated fields:

```
font-file;options;text;expected-file
```

- `font-file`: relative to the `.tests` file (or absolute path).
- `options`: shell-split arguments passed to `hb-draw`.
- `text`: literal UTF-8 text argument passed to `hb-draw`.
- `expected-file`: path to golden output, relative to the `.tests` file.
  This is also used as the TAP test name.

Example:

```
../../../api/fonts/Roboto-Regular.abc.ttf;--font-size=64;abc;../expected/reuse-svg.svg
```

Running draw tests
------------------

In an in-tree build:

```sh
meson test -C build-default -v --suite draw
```

Updating expected output
------------------------

Regenerate a golden file with the current `hb-draw` binary and replace the
file under `data/expected/`.

Recording a new test case
-------------------------

Use `record-test.sh`:

```sh
./record-test.sh -o data/tests/basic.tests --test-name reuse-svg ../../build-default/util/hb-draw ../api/fonts/Roboto-Regular.abc.ttf --font-size=64 abc
```

It subsets the input font, verifies subset-vs-original rendering with
`hb-view`, stores the subset font under `../fonts/<sha1>.ttf` (relative to the target
`.tests` file), writes expected output under
`../expected/<test-name>.<ext>` (unless `--expected-file` is provided), and
appends one test line to the target `.tests` file. `hb-shape`, `hb-view`, and
`hb-subset` are resolved as sibling executables next to the provided
`hb-draw` binary.
