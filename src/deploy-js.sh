#!/bin/sh

mvn install:install-file -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=0.9.5-SNAPSHOT -Dfile=harfbuzz.js -Dpackaging=js -Dtype=js 

mvn install:install-file -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=0.9.5-SNAPSHOT -Dfile=harfbuzz-unoptimized.js -Dpackaging=js -Dtype=js -Dclassifier=debug 

mvn org.apache.maven.plugins:maven-deploy-plugin:2.7:deploy-file -DrepositoryId=nexus -Durl=http://intra.prezi.com:8081/nexus/content/repositories/snapshots/ -DartifactId=harfbuzz-js -DgroupId=org.harfbuzz -Dversion=0.9.5-SNAPSHOT -Dfile=harfbuzz.js -Dtype=js -Dfiles=harfbuzz-unoptimized.js -Dtypes=js -DuniqueVersion=false -Dclassifiers=debug 

