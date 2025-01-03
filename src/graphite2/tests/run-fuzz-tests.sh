#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright 2012, SIL International, All rights reserved.

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
