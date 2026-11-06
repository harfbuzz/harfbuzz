# harfbuzz-icu links to libstdc++ because icu does, and harfbuzz-cairo links transitively
# to libxml2 which on some platforms is also linked against icu
CHECK_LIBSTDCXX_LIBS = [
    "harfbuzz",
    "harfbuzz-subset",
    "harfbuzz-raster",
    "harfbuzz-vector",
    "harfbuzz-gobject",
]

CHECK_SYMBOL_LIBS = CHECK_LIBSTDCXX_LIBS + [
    "harfbuzz-cairo",
    "harfbuzz-icu",
]
