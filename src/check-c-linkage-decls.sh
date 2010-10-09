#!/bin/sh

LC_ALL=C
export LC_ALL

test -z "$srcdir" && srcdir=.
stat=0

cd "$srcdir"

for x in hb-*.c hb-*.cc hb-*.h hb-*.hh ; do
	if ! grep -q HB_BEGIN_DECLS "$x" || ! grep -q HB_END_DECLS "$x"; then
		echo "Ouch, file $x does not HB_BEGIN_DECLS / HB_END_DECLS"
		stat=1
	fi
done

exit $stat
