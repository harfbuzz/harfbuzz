#!/bin/bash
set -ex

if [ "$#" -lt 1 ]
then
  echo "Usage $0 ARCH (ARCH can be 32 or 64)"
  exit 1
fi

ARCH=$1
shift

if [ "$ARCH" = "64" ]; then
  TRIPLET=x86_64-w64-mingw32
else
  TRIPLET=i686-w64-mingw32
fi

BUILD=build-win${ARCH}
INSTALL=install-win${ARCH}
DIST=harfbuzz-win${ARCH}

# Install GLFW for MinGW if not already present
if [ ! -f /usr/${TRIPLET}/include/GLFW/glfw3.h ]; then
  GLFW_VER=3.4
  GLFW_ZIP=glfw-${GLFW_VER}.bin.WIN${ARCH}.zip
  if [ ! -f /tmp/${GLFW_ZIP} ]; then
    wget -q -O /tmp/${GLFW_ZIP} "https://github.com/glfw/glfw/releases/download/${GLFW_VER}/${GLFW_ZIP}"
  fi
  GLFW_DIR=$(mktemp -d)
  unzip -q /tmp/${GLFW_ZIP} -d ${GLFW_DIR}
  GLFW_ROOT=${GLFW_DIR}/glfw-${GLFW_VER}.bin.WIN${ARCH}
  sudo mkdir -p /usr/${TRIPLET}/include/GLFW
  sudo cp ${GLFW_ROOT}/include/GLFW/*.h /usr/${TRIPLET}/include/GLFW/
  sudo cp ${GLFW_ROOT}/lib-mingw-w64/libglfw3.a /usr/${TRIPLET}/lib/
  sudo mkdir -p /usr/${TRIPLET}/lib/pkgconfig
  cat > /tmp/glfw3.pc << PCEOF
prefix=/usr/${TRIPLET}
includedir=\${prefix}/include
libdir=\${prefix}/lib
Name: GLFW
Version: ${GLFW_VER}
Libs: -L\${libdir} -lglfw3
Libs.private: -lgdi32
Cflags: -I\${includedir}
PCEOF
  sudo cp /tmp/glfw3.pc /usr/${TRIPLET}/lib/pkgconfig/
  rm -rf ${GLFW_DIR}
fi

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
	-Dgpu=enabled \
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
