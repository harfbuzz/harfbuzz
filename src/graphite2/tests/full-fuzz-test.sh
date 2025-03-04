#!/bin/sh

TESTSDIR=$(dirname $0)
${TESTSDIR}/fuzzcomparerender Awami_compressed_test awami_tests -r --nolimit -t 20 --passes=1 "$@"
${TESTSDIR}/fuzzcomparerender Awami_test awami_tests -r --nolimit -t 20 --passes=1 "$@"
${TESTSDIR}/fuzzcomparerender Padauk my_HeadwordSyllables --nolimit -t 20 --passes=1 "$@"
${TESTSDIR}/fuzzcomparerender Annapurnarc2 udhr_nep --nolimit -t 20 --passes=1 "$@"
${TESTSDIR}/fuzzcomparerender charis_r_gr udhr_yor --nolimit -t 20 --passes=1 "$@"
${TESTSDIR}/fuzzcomparerender Scheherazadegr udhr_arb -r --nolimit -t 20 --passes=1 "@"
