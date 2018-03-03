#!/bin/bash

set -x
set -o errexit -o nounset

pip install --user nose
pip install --user cpp-coveralls
export PATH=$HOME/.local/bin:$PATH

rm -f src/.libs/NONE.gcov
touch src/NONE
coveralls -e docs
