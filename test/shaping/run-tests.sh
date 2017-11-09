#!/bin/sh

test "x$srcdir" = x && srcdir=.
test "x$builddir" = x && builddir=.
test "x$top_builddir" = x && top_builddir=../..

extra_options="--verify"
hb_shape="$top_builddir/util/hb-shape$EXEEXT"
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
		$reference || echo "hb-shape $fontfile $extra_options $options --unicodes $unicodes"
		glyphs1=`$hb_shape --font-funcs=ft "$srcdir/$fontfile" $extra_options $options --unicodes "$unicodes"`
		if test $? != 0; then
			echo "hb-shape --font-funcs=ft failed." >&2
			fails=$((fails+1))
			#continue
		fi
		glyphs2=`$hb_shape --font-funcs=ot "$srcdir/$fontfile" $extra_options $options --unicodes "$unicodes"`
		if test $? != 0; then
			echo "hb-shape --font-funcs=ot failed." >&2
			fails=$((fails+1))
			#continue
		fi
		if ! test "x$glyphs1" = "x$glyphs2"; then
			echo "FT funcs: $glyphs1" >&2
			echo "OT funcs: $glyphs2" >&2
			fails=$((fails+1))
		fi
		if $reference; then
			echo "$fontfile:$options:$unicodes:$glyphs1"
			continue
		fi
		if ! test "x$glyphs1" = "x$glyphs_expected"; then
			echo "Actual:   $glyphs1" >&2
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
