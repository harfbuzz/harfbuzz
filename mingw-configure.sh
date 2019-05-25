#!/bin/sh

case $1 in
	i686 | x86_64) ;;
	*) echo "Usage: $0 i686|x86_64" >&2; exit 1 ;;
esac

target=$1-w64-mingw32
shift

unset CC
unset CXX
unset CPP
unset LD
unset LDFLAGS
unset CFLAGS
unset CXXFLAGS
unset PKG_CONFIG_PATH

# Removed -static from the following
export CFLAGS="-static-libgcc"
export CXXFLAGS="-static-libgcc -static-libstdc++"
export CPPFLAGS="-I$HOME/.local/$target/include"
export LDFLAGS=-L$HOME/.local/$target/lib
export PKG_CONFIG_LIBDIR=$HOME/.local/$target/lib/pkgconfig:/usr/$target/sys-root/mingw/lib/pkgconfig/
export PKG_CONFIG_PATH=$HOME/.local/$target/share/pkgconfig:/usr/$target/sys-root/mingw/share/pkgconfig/
export PATH=$HOME/.local/$target/bin:/usr/$target/sys-root/mingw/bin:/usr/$target/bin:$PATH

../configure --build=`../config.guess` --host=$target --prefix=$HOME/.local/$target --with-uniscribe --without-icu "$@"
