#!/bin/bash

for f in $(find . -name '*.log' -not -name 'config.log'); do
    if [[ $(tail -1 $f) = FAIL* ]]; then
        echo '====' $f '===='
        cat $f
    fi
done

# Intentionally exiting with non-zero.
exit 1
