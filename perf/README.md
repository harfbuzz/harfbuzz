# Building and Running

Benchmarks are implemented using [Google Benchmark](https://github.com/google/benchmark).

To build the benchmarks in this directory you need to set the benchmark
option while configuring the build with meson:

```
meson configure build -Dbenchmark=enabled --buildtype=release
```

Then build the benchmark binaries with ninja:

```
ninja -Cbuild perf/benchmark-set
```

Finally, to run one of the benchmarks:

```
./build/perf/benchmark-set
```

It's possible to filter the benchmarks being run and customize the output
via flags to the benchmark binary. See the
[Google Benchmark User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md#user-guide) for more details.

# Profiling

Configure the build to include debug information for profiling:

```
CXXFLAGS="-fno-omit-frame-pointer" meson --reconfigure build -Dbenchmark=enabled --buildtype=debug
ninja -Cbuild
```

Then run the benchmark with perf:

```
perf record -g -o ~/tmp/profiles/subset.prof ./build/perf/benchmark-subset --benchmark_filter="BM_subset_codepoints/subset_notocjk/100000" --benchmark_repetitions=5
```
You probably want to filter to a specific benchmark of interest and set the number of repititions high enough to get a good sampling of profile data.

Finally view the profile with:

perf report -i ~/tmp/profiles/subset.prof
