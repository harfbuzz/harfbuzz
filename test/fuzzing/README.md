## Security Fuzzers

To build the fuzzers with libFuzzer to perform actual fuzzing, build with:

```shell
CXX=clang++ CXXFLAGS="-fsanitize=address,fuzzer-no-link" meson fuzzbuild --default-library=static -Dfuzzer_ldflags="-fsanitize=address,fuzzer"

ninja -Cfuzzbuild
```

Then, run the fuzzer like this:

fuzzbuild/test/fuzzing/hb-{shape,draw,subset,set,depend}-fuzzer [-max_len=2048] [CORPUS_DIR]

Where max_len specifies the maximal length of font files to handle.
The smaller the faster.

Note: The `depend` fuzzer requires `-Ddepend_api=true` meson option.

For more details consult the following locations:
  - http://llvm.org/docs/LibFuzzer.html

## Depend Closure Parity Test

The `hb-depend-closure-parity` test is distinct from the security fuzzers.
While security fuzzers test robustness against malformed input, the closure
parity test validates correctness by emulating the glyph closure calculation
used for subsetting with one generated from the dependency graph to avoid
implementation divergence. It should only be run on valid fonts.
