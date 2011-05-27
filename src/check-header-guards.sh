#!/bin/sh

LC_ALL=C
export LC_ALL

test -z "$srcdir" && srcdir=.
stat=0

test "x$HBHEADERS" = x && HBHEADERS=`find . -maxdepth 1 -name 'hb*.h'`
test "x$HBSOURCES" = x && HBSOURCES=`find . -maxdepth 1 -name 'hb-*.cc' -or -name 'hb-*.hh'`


cd "$srcdir"

for x in $HBHEADERS $HBSOURCES; do
	echo "$x" | grep '[^h]$' -q && continue;
	x=`echo "$x" | sed 's@.*/@@'`
	tag=`echo "$x" | tr 'a-z.-' 'A-Z_'`
	lines=`grep "\<$tag\>" "$x" | wc -l`
	if test "x$lines" != x3; then
		echo "Ouch, header file $x does not have correct preprocessor guards"
		stat=1
	fi
done

exit $stat
