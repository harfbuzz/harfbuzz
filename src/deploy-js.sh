#!/bin/sh

VERSION=`git describe`

mvn org.apache.maven.plugins:maven-deploy-plugin:2.7:deploy-file -DrepositoryId=nexus -Durl=http://intra.prezi.com:8081/nexus/content/repositories/releases/ -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=$VERSION -Dfile=harfbuzz.js -Dtype=js -Dfiles=harfbuzz-unoptimized.js -Dtypes=js -DuniqueVersion=false -Dclassifiers=debug 

