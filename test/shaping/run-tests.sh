#!/bin/sh

test "x$srcdir" = x && srcdir=.
test "x$builddir" = x && builddir=.
test "x$top_builddir" = x && top_builddir=../..

hb_shape="$top_builddir/util/hb-shape$EXEEXT --verify"
#hb_shape="$top_builddir/util/hb-shape$EXEEXT"

fails=0

reference=false
if test "x$1" = x--reference; then
	reference=true
	shift
fi

if test $# = 0; then
	set /dev/stdin
fi

for f in "$@"; do
	$reference || echo "Running tests in $f"
	while IFS=: read fontfile options unicodes glyphs_expected; do
		if echo "$fontfile" | grep -q '^#'; then
			$reference || echo "# hb-shape $fontfile --unicodes $unicodes"
			continue
		fi
		$reference || echo "hb-shape $fontfile $options --unicodes $unicodes"
		glyphs=`$hb_shape "$srcdir/$fontfile" $options --unicodes "$unicodes"`
		if test $? != 0; then
			echo "hb-shape failed." >&2
			fails=$((fails+1))
			#continue
		fi
		if $reference; then
			echo "$fontfile:$options:$unicodes:$glyphs"
			continue
		fi
		if ! test "x$glyphs" = "x$glyphs_expected"; then
			echo "Actual:   $glyphs" >&2
			echo "Expected: $glyphs_expected" >&2
			fails=$((fails+1))
		fi
	done < "$f"
done

if test $fails != 0; then
	$reference || echo "$fails tests failed."
	exit 1
else
	$reference || echo "All tests passed."
fi
