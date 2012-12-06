#!/bin/sh

VERSION=`git describe`

mvn org.apache.maven.plugins:maven-deploy-plugin:2.7:deploy-file -DrepositoryId=nexus -Durl=http://intra.prezi.com:8081/nexus/content/repositories/releases/ -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=$VERSION -Dfile=harfbuzz.js -Dtype=js -Dfiles=harfbuzz-untyped.js,harfbuzz-unoptimized.js,harfbuzz-untyped-unoptimized.js -Dtypes=js,js,js -DuniqueVersion=false -Dclassifiers=untyped,debug,untyped-debug

