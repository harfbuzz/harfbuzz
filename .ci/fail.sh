#!/bin/bash

for f in $(find . -name '*.log' -not -name 'config.log'); do
    echo '====' $f '===='
    cat $f
done

# Intentionally exiting with non-zero.
exit 1
