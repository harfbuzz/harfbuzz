# Configuring HarfBuzz

Most of the time you will not need any custom configuration.  The configuration
options provided by `configure` or `cmake` should be enough.  In particular,
if you just want HarfBuzz library plus hb-shape / hb-view utilities, make sure
FreeType and Cairo are available and found during configuration.

If you are building for distribution, you should more carefully consider whether
you need Glib, ICU, Graphite2, as well as CoreText / Uniscribe / DWrite.  Make
sure the relevant ones are enabled.

If you are building for custom environment (embedded, downloadable app, etc)
where you mostly just want to call `hb_shape()` and the binary size of the
resulting library is very important to you, the rest of this file guides you
through your options to disable features you may not need, in exchange for
binary size savings.

## Compiler Options

Make sure you build with your compiler's "optimize for size" option.  On `gcc`
this is `-Os`, and can be enabled by passing `CXXFLAGS=-Os` either to `configure`
(sticky) or to `make` (non-sticky).  On clang there is an even more extreme flag,
`-Oz`.

HarfBuzz heavily uses inline functions and the optimize-size flag can make the
library smaller by 20% or more.  Moreover, sometimes, based on the target CPU,
the optimize-size builds perform *faster* as well, thanks to lower code
footprint and caching effects.  So, definitely try that even if size is not
extremely tight but you have a huge application.  For example, Chrome does
that.  Note that this configuration also automatically enables certain internal
optimizations.  Search for `HB_OPTIMIZE_SIZE` for details, if you are using
other compilers, or continue reading.

Another compiler option to consider is "link-time optimization", also known as
'lto'.  To enable that, with `gcc` or `clang`, add `-flto` to both `CXXFLAGS`
and `LDFLAGS`, either on `configure` invocation (sticky) or on `make` (non-sticky).
This, also, can have a huge impact on the final size, 20% or more.

Finally, if you are making a static library build or otherwise linking the
library into your app, make sure your linker removes unused functions.  This
can be tricky and differ from environment to environment, but you definitely
want to make sure this happens.  Otherwise, every unused public function will
be adding unneeded bytes to your binary.

Combining the above three build options should already shrink your library a lot.
The rest of this file shows you ways to shrink the library even further at the
expense of removing functionality (that may not be needed).

## Custom configuration

TODO
