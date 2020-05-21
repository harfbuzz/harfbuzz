#!/bin/sh

# Usage: ./mingw-meson.sh prefix host
# host being i686 or x86_64

case $2 in
	i686 | x86_64) ;;
	*) echo "Usage: $0 prefix i686|x86_64" >&2; exit 1 ;;
esac

cpu=$2
host=$2-w64-mingw32
if test "x$2"  = "xx86_64" ; then
    arch=x86-64
else
    arch=i686
fi

rm -rf builddir && mkdir builddir && cd builddir && cp ../meson-mingw-cross.txt .

sed -i -e "s/@cpu_family@/$cpu/g;s/@cpu@/$cpu/g;s/@host@/$host/g;s/@arch@/$arch/g" meson-mingw-cross.txt

meson .. \
      --prefix=$1 \
      --libdir=lib \
      --buildtype=release \
      --strip \
      --cross-file meson-mingw-cross.txt \
      --default-library shared \
      -Dgraphite=enabled

ninja
ninja install
