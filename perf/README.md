Benchmarking
============

Build the project like,

```
meson build -Dbenchmark=true -Dexperimental_api=true -Doptimization=2
ninja -Cbuild
```

Now you have different options,

* Run all the benchmarks,

```
ninja -Cbuild && build/perf/perf
```

* Run shaping perf tool with a specific font and text,

```
ninja -Cbuild && build/perf/perf-shaping-bench perf/fonts/Amiri-Regular.ttf perf/texts/fa-thelittleprince.txt
```

* Run extents perf tool with a specific font,

```
ninja -Cbuild && build/perf/perf-extents-bench test/api/fonts/Estedad-VF.ttf
```

Other than Google Benchmark based benchmarks there is also `run.sh` which builds `hb-shape` itself
and runs the `perf` tool afterward usually provided by your package manager.
