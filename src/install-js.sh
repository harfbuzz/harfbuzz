#!/bin/sh

VERSION=`git describe`

mvn install:install-file -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=$VERSION -Dfile=harfbuzz.js -Dpackaging=js -Dtype=js 
mvn install:install-file -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=$VERSION -Dfile=harfbuzz-untyped.js -Dpackaging=js -Dtype=js -Dclassifier=untyped 
mvn install:install-file -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=$VERSION -Dfile=harfbuzz-unoptimized.js -Dpackaging=js -Dtype=js -Dclassifier=debug 
mvn install:install-file -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=$VERSION -Dfile=harfbuzz-untyped-unoptimized.js -Dpackaging=js -Dtype=js -Dclassifier=untyped-debug 

