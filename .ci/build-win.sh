#!/bin/bash
set -ex

if [ "$#" -lt 1 ]
then
  echo "Usage $0 ARCH (ARCH can be 32 or 64)"
  exit 1
fi

ARCH=$1
shift

BUILD=build-win${ARCH}
INSTALL=install-win${ARCH}
DIST=harfbuzz-win${ARCH}

meson setup \
	--buildtype=release \
	--prefix="${PWD}/${BUILD}/${INSTALL}" \
	--cross-file=.ci/win${ARCH}-cross-file.txt \
	--wrap-mode=default \
	--strip \
	-Dtests=enabled \
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
	${BUILD} \
	"$@"

# building with all the cores won't work fine with CricleCI for some reason
meson compile -C ${BUILD} -j3
meson install -C ${BUILD}

mkdir ${BUILD}/${DIST}
cp ${BUILD}/${INSTALL}/bin/hb-*.exe ${BUILD}/${DIST}
cp ${BUILD}/${INSTALL}/bin/*.dll ${BUILD}/${DIST}
rm -f ${DIST}.zip
(cd ${BUILD} && zip -r ../${DIST}.zip ${DIST})
echo "${DIST}.zip is ready."
