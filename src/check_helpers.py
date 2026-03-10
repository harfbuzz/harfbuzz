CHECK_SYMBOL_LIBS = [
    "harfbuzz",
    "harfbuzz-subset",
    "harfbuzz-raster",
    "harfbuzz-vector",
    "harfbuzz-icu",
    "harfbuzz-gobject",
    "harfbuzz-cairo",
]

# harfbuzz-icu links to libstdc++ because icu does.
CHECK_LIBSTDCXX_LIBS = [
    "harfbuzz",
    "harfbuzz-subset",
    "harfbuzz-raster",
    "harfbuzz-vector",
    "harfbuzz-gobject",
    "harfbuzz-cairo",
]
