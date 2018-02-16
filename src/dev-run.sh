#!/bin/bash

for cmd in cmake ninja fswatch; do
    command -v $cmd >/dev/null 2>&1 || { echo >&2 "This script needs $cmd be installed"; exit 1; }
done

[ $# = 0 ] && echo Usage: "src/dev-run.sh [FONT-FILE] [TEXT]" && exit

cmake -DHB_CHECK=ON -DHB_DISABLE_TEST_PROGS=ON -Bbuild -H. -GNinja
ninja -Cbuild

# or "fswatch -0 . -e build/ -e .git"
find src/ | entr printf '\0' | while read -d "" event; do
    clear
    ninja -Cbuild
    build/hb-shape $@
    build/hb-view $@
done

cmake -DHB_CHECK=ON -DHB_DISABLE_TEST_PROGS=OFF -Bbuild -H. -GNinja
ninja -Cbuild
CTEST_OUTPUT_ON_FAILURE=1 CTEST_PARALLEL_LEVEL=8 ninja -Cbuild test
