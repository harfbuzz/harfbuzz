#!/bin/sh

TESTSDIR=$(dirname $0)
for FONT in $(ls $TESTSDIR/fuzz-tests)
do
    for TXT in $(ls $TESTSDIR/fuzz-tests/$FONT)
    do
        test -e ".$FONT-$TXT.pipe" && rm ".$FONT-$TXT.fuzz"
        mknod ".$FONT-$TXT.pipe" p
        (cat $TESTSDIR/fuzz-tests/$FONT/$TXT/*.fuzz > ".$FONT-$TXT.pipe") &
        $TESTSDIR/fuzzcomparerender $FONT $TXT \
            --valgrind \
            --input=".$FONT-$TXT.pipe" \
            --logfile="regressions-$FONT-TXT.log"
        rm ".$FONT-$TXT.pipe"
    done
done
