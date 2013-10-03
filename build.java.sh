#!/bin/bash

swig -cpperraswarn -java -package com.prezi.harfbuzz -outdir swig/build/java/com/prezi/harfbuzz -o swig/build/cpp/harfbuzz_wrap.c -oh swig/build/cpp/harfbuzz_wrap.h src/harfbuzz.i

cp swig/build/cpp/* src

if [ ! -d "build" ]; then
    mkdir build
fi
if [ ! -x "./configure" ]; then
    ./autogen.sh
fi
rm build/libharfbuzz*
export LDFLAGS="-arch x86_64 -pipe -Os -no-cpp-precomp"
export CFLAGS=${LDFLAGS}
export CXXFLAGS=${LDFLAGS}

export INSTALL_DIR=$(pwd)/build

#./configure --prefix=$INSTALL_DIR --disable-static --enable-shared --host=i686-apple-darwin && make clean >/dev/null && make -j && cp src/.libs/libharfbuzz.dylib build/harfbuzz.dylib
./configure --prefix=$INSTALL_DIR --disable-shared --enable-static --host=i686-apple-darwin && make clean >/dev/null && make -j && cp src/.libs/libharfbuzz.a build/harfbuzz.a

echo "### compile wrapper"
#gcc  -m32 -march=i386 -pipe -Os -no-cpp-precomp -c swig/build/cpp/harfbuzz_wrap.c -I ./src -I /System/Library/Frameworks/JavaVM.framework/Headers -o swig/build/cpp/lib/harfbuzz_wrap.o
# -Wl,--add-stdcall-alias
#echo "### link wrapper"
#ld  build/harfbuzz.dylib swig/build/cpp/lib/harfbuzz_wrap.o -dylib -o build/libharfbuzz.so
gcc  -I ./src -I /System/Library/Frameworks/JavaVM.framework/Headers  -pipe -Os -no-cpp-precomp -shared -o swig/build/cpp/lib/harfbuzz_wrap.dylib swig/build/cpp/harfbuzz_wrap.c  build/harfbuzz.a