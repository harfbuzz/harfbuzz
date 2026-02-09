Adding tests
============

`test/draw` validates `hb-draw` CLI output by comparing exact stdout with
golden files.

Test file format (`data/tests/*.tests`)
---------------------------------------

Each non-empty, non-comment line has 5 semicolon-separated fields:

```
name;font-file;options;text;expected-file
```

- `name`: TAP test name.
- `font-file`: relative to the `.tests` file (or absolute path).
- `options`: shell-split arguments passed to `hb-draw`.
- `text`: literal UTF-8 text argument passed to `hb-draw`.
- `expected-file`: path to golden output, relative to the `.tests` file.

Example:

```
reuse-svg;../../../api/fonts/Roboto-Regular.abc.ttf;--font-size=64;abc;../expected/reuse-svg.svg
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
