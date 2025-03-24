# Building and Running

Benchmarks are implemented using [Google Benchmark](https://github.com/google/benchmark).

To build the benchmarks in this directory you need to set the benchmark
option while configuring the build with meson:
```
meson build -Dbenchmark=enabled --buildtype=debugoptimized
```

The default build type is `debugoptimized`, which is good enough for
benchmarking, but you can also get the fastest mode with `release`
build type:
```
meson build -Dbenchmark=enabled --buildtype=release
```

You should, of course, enable features you want to benchmark, like
`-Dfreetype`, `-Dfontations`, `-Dcoretext`, etc.

Then build a specific benchmark binaries with ninja, eg.:
```
ninja -Cbuild perf/benchmark-set
```
or just build the whole project:
```
ninja -Cbuild
```

Finally, to run one of the benchmarks:

```
./build/perf/benchmark-set
```

It's possible to filter the benchmarks being run and customize the output
via flags to the benchmark binary. See the
[Google Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md#user-guide) for more details.

The most useful benchmark is `benchmark-font`. You can provide custom fonts to it too.
For example, to run only the "paint" benchmarks, against a given font, you can do:
```
./build/perf/benchmark-font NotoColorEmoji-Regular.ttf --benchmark_filter="paint"
```

Some useful options are: `--benchmark_repetitions=5` to run the benchmark 5 times,
`--benchmark_min_time=.1s` to run the benchmark for at least .1 seconds (defaults
to .5s), and `--benchmark_filter=...` to filter the benchmarks by regular expression.

To compare before/after benchmarks, you need to save the benchmark results in files
for both runs. Use `--benchmark_out=results.json` to output the results in JSON format.
Then you can use:
```
./subprojects/benchmark-1.8.4/tools/compare.py benchmarks before.json after.json
```
Substitute your version of benchmark instead of 1.8.4.

# Profiling

If you like to disable optimizations and enable frame pointers for better profiling output,
you can do so with the following meson command:
```
CXXFLAGS="-fno-omit-frame-pointer" meson --reconfigure build -Dbenchmark=enabled --buildtype=debug
ninja -Cbuild
```
However, this will slow down the benchmarks significantly and might give you inaccurate
information as to where to optimize. It's better to profile the `debugoptimized` build (the default).

Then run the benchmark with perf:
```
perf record -g build/perf/benchmark-subset --benchmark_filter="BM_subset_codepoints/subset_notocjk/100000" --benchmark_repetitions=5
```
You probably want to filter to a specific benchmark of interest and set the number of
repititions high enough to get a good sampling of profile data.

Finally view the profile with:

```
perf report
```

Another useful `perf` tool is the `perf stat` command, which can give you a quick overview
of the performance of a benchmark, as well as stalled cycles, cache misses, and mispredicted branches.
