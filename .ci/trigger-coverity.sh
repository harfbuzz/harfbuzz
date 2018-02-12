#!/bin/bash

set -x
set -o errexit -o nounset

if test "x$TRAVIS_EVENT_TYPE" != x"cron"; then exit; fi

BRANCH="$TRAVIS_BRANCH"
if test "x$BRANCH" != xmaster; then exit; fi

git fetch --unshallow
git remote add upstream "https://$GH_TOKEN@github.com/harfbuzz/harfbuzz.git"
git push -q upstream master:coverity_scan
