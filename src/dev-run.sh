#!/bin/bash
# Suggested setup to use the script:
#  (on the root of the project)
#  $ NOCONFIGURE=1 ./autogen.sh --with-freetype --with-glib --with-gobject --with-cairo
#  $ mkdir build && cd build && ../configure && make -j5 && cd ..
#  $ src/dev-run.sh [FONT-FILE] [TEXT]
#
# Or, using cmake:
#  $ cmake -DHB_CHECK=ON -Bbuild -H. -GNinja && ninja -Cbuild
#  $ src/dev-run.sh [FONT-FILE] [TEXT]
#

[ $# = 0 ] && echo Usage: "src/dev-run.sh [FONT-FILE] [TEXT]" && exit
command -v entr >/dev/null 2>&1 || { echo >&2 "This script needs `entr` be installed"; exit 1; }

[ -f 'build/build.ninja' ] && CMAKENINJA=TRUE
# or "fswatch -0 . -e build/ -e .git"
find src/ | entr printf '\0' | while read -d ""; do
	clear
	if [[ $CMAKENINJA ]]; then
		ninja -Cbuild hb-shape hb-view
		build/hb-shape $@
		build/hb-view $@
	else
		make -Cbuild/src -j5 -s lib
		build/util/hb-shape $@
		build/util/hb-view $@
	fi
done

read -n 1 -p "Run the tests (y/n)? " answer
if [[ "$answer" = "y" ]]; then
	if [[ $CMAKENINJA ]]; then
		CTEST_OUTPUT_ON_FAILURE=1 CTEST_PARALLEL_LEVEL=5 ninja -Cbuild test
	else
		make -Cbuild -j5 check && .ci/fail.sh
	fi
fi
