#!/bin/sh

LC_ALL=C
export LC_ALL

test -z "$srcdir" && srcdir=.
stat=0

test "x$HBHEADERS" = x && HBHEADERS=`find . -maxdepth 1 -name 'hb*.h'`
test "x$HBSOURCES" = x && HBSOURCES=`find . -maxdepth 1 -name 'hb-*.cc' -or -name 'hb-*.hh'`


for x in $HBHEADERS $HBSOURCES; do
	test -f $srcdir/$x && x=$srcdir/$x
	if ! grep -q HB_BEGIN_DECLS "$x" || ! grep -q HB_END_DECLS "$x"; then
		echo "Ouch, file $x does not HB_BEGIN_DECLS / HB_END_DECLS"
		stat=1
	fi
done

exit $stat
