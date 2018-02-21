#!/bin/sh

LC_ALL=C
export LC_ALL

test -z "$srcdir" && srcdir=.
test -z "$libs" && libs=.libs
stat=0

IGNORED_SYMBOLS='_fini\>\|_init\>\|_fdata\>\|_ftext\>\|_fbss\>\|__bss_start\>\|__bss_start__\>\|__bss_end__\>\|_edata\>\|_end\>\|_bss_end__\>\|__end__\>\|__gcov_flush\>\|llvm_'

if which nm 2>/dev/null >/dev/null; then
	:
else
	echo "check-defs.sh: 'nm' not found; skipping test"
	exit 77
fi

tested=false
for soname in harfbuzz harfbuzz-icu harfbuzz-subset; do
	def=$soname.def
	if ! test -f "$def"; then
		echo "check-defs.sh: '$def' not found; skipping test it"
		continue
	fi
	for suffix in so dylib; do
		so=$libs/lib$soname.$suffix
		if ! test -f "$so"; then continue; fi

		# On macOS, C symbols are prefixed with _
		if test $suffix = dylib; then prefix="_"; fi

		EXPORTED_SYMBOLS="`nm "$so" | grep ' [BCDGINRSTVW] .' | grep -v " $prefix\\($IGNORED_SYMBOLS\\)" | cut -d' ' -f3`"

		echo
		echo "Checking that $so has the same symbol list as $def"
		{
			echo EXPORTS
			echo "$EXPORTED_SYMBOLS" | sed -e "s/^${prefix}hb/hb/g"
			# cheat: copy the last line from the def file!
			tail -n1 "$def"
		} | c++filt | diff "$def" - >&2 || stat=1

		tested=true
	done
done
if ! $tested; then
	echo "check-defs.sh: no shared libraries found; skipping test"
	exit 77
fi

exit $stat
