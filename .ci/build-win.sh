#!/bin/bash
set -ex

if [ "$#" -lt 2 ]
then
  echo "Usage $0 ARCH CPU"
  exit 1
fi

ARCH=$1
CPU=$2

BUILD=build-win${ARCH}
DIST=harfbuzz-win${ARCH}

meson setup \
	--cross-file=.ci/win${ARCH}-cross-file.txt \
	--wrap-mode=default \
	-Dtests=disabled \
	-Dcairo=enabled \
	-Dcairo:fontconfig=disabled \
	-Dcairo:freetype=disabled \
	-Dcairo:dwrite=disabled \
	-Dcairo:tests=disabled \
	-Dglib=enabled \
	-Dlibffi:tests=false \
	-Dfreetype=disabled \
	-Dicu=disabled \
	-Dchafa=disabled \
	-Dgdi=enabled \
	-Ddirectwrite=enabled \
	${BUILD}

# building with all the cores won't work fine with CricleCI for some reason
meson compile -C ${BUILD} -j3

rm -rf ${BUILD}/${DIST}
mkdir ${BUILD}/${DIST}
cp ${BUILD}/util/hb-*.exe ${BUILD}/${DIST}
find ${BUILD} -name '*.dll' -exec cp {} ${BUILD}/${DIST} \;
${CPU}-w64-mingw32-strip ${BUILD}/${DIST}/*.{dll,exe}
rm -f ${DIST}.zip
(cd ${BUILD} && zip -r ../${DIST}.zip ${DIST})
echo "${DIST}.zip is ready."
