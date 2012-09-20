#!/bin/bash

emcc main-main.o -o main.js
sed -e '/GENERATED_CODE/r main-unoptimized.js' main-run.template.js > main-run-unoptimized.js
node main-run.js OpenBaskerville-0.0.75.otf
