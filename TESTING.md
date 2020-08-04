## Build & Run

Depending on what area you are working in change or add `HB_DEBUG_<whatever>`.
Values defined in `hb-debug.hh`.

```shell
CPPFLAGS='-DHB_DEBUG_SUBSET=100' meson setup build
meson test -C build
```

### Run tests with asan

```shell
meson setup build -Db_sanitize=address
meson compile -C build
meson test -C build
```

### Debug with GDB

```
meson setup build
meson compile -C build
meson test -C build --gdb testname
```

### Enable Debug Logging

```shell
CPPFLAGS=-DHB_DEBUG_SUBSET=100 meson build
ninja -C build
```

## Build and Test

```shell
meson build
ninja -Cbuild
meson test -Cbuild
```

## Test with the Fuzzer

```shell
CXXFLAGS="-fsanitize=address,fuzzer-no-link" meson build --default-library=static -Dfuzzer_ldflags="-fsanitize=address,fuzzer" -Dexperimental_api=true
ninja -C build test/fuzzing/hb-{shape,draw,subset,set}-fuzzer
build/test/fuzzing/hb-subset-fuzzer
```

## Profiling

```
meson build
meson compile -C build
build/perf/perf
```

