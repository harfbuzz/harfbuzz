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

GDB=gdb
# if gdb doesn't exist, hopefully lldb exist
command -v $GDB >/dev/null 2>&1 || export GDB="lldb"

[ -f 'build/build.ninja' ] && CMAKENINJA=TRUE
# or "fswatch -0 . -e build/ -e .git"
find src/ | entr printf '\0' | while read -d ""; do
	clear
	echo '===================================================='
	if [[ $CMAKENINJA ]]; then
		ninja -Cbuild hb-shape hb-view && {
			build/hb-shape $@
			build/hb-view $@
		}
	else
		make -Cbuild/src -j5 -s lib && {
			build/util/hb-shape $@
			build/util/hb-view $@
		}
	fi
done

read -n 1 -p "[T]est, [D]ebug, [R]estart, [Q]uit? " answer
case "$answer" in
t|T )
	if [[ $CMAKENINJA ]]; then
		CTEST_OUTPUT_ON_FAILURE=1 CTEST_PARALLEL_LEVEL=5 ninja -Cbuild test
	else
		make -Cbuild -j5 check && .ci/fail.sh
	fi
;;
d|D )
	if [[ $CMAKENINJA ]]; then
		echo "Not supported on cmake builds yet"
	else
		build/libtool --mode=execute $GDB -- build/util/hb-shape $@
	fi
;;
r|R )
	src/dev-run.sh $@
;;
* )
	exit
;;
esac
