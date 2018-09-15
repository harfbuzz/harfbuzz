#!/bin/bash

target=i686-w64-mingw32

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
export CPPFLAGS="-O2"
export PKG_CONFIG_LIBDIR=/usr/i686-w64-mingw32/sys-root/mingw/lib/pkgconfig/

../configure --build=`../config.guess` --host=$target "$@"
