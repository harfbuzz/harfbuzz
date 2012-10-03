#!/bin/bash

make
emcc .libs/libharfbuzz.dylib hb-ucdn/.libs/ucdn.o -o harfbuzz-unoptimized.js
#sed -e '/GENERATED_CODE/r main-unoptimized.js' main-run.template.js > main-run-unoptimized.js
#node main-run.js OpenBaskerville-0.0.75.otf
