#!/bin/bash

for f in $(find . -name '*.log' -not -name 'config.log'); do
    last=$(tail -1 $f)
    if [[ $last = FAIL* || $last = *failed* ]]; then
        echo '====' $f '===='
        cat $f
    fi
done

# Intentionally exiting with non-zero.
exit 1
