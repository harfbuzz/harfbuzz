#!/bin/bash

if [ ! -d "swig/build/cpp/lib" ]; then
    mkdir -p swig/build/cpp/lib
fi

if [ ! -d "swig/build/java/com/prezi/harfbuzz" ]; then
    mkdir -p swig/build/java/com/prezi/harfbuzz
fi

if [ ! -d "build" ]; then
    mkdir build
fi

rm build/libharfbuzz*
rm swig/build/cpp/*.c
rm swig/build/cpp/*.h
rm swig/build/cpp/lib/*


echo "Generating JNI code with SWIG"
swig -cpperraswarn -java -package com.prezi.harfbuzz -outdir swig/build/java/com/prezi/harfbuzz -o swig/build/cpp/harfbuzz_wrap.c -oh swig/build/cpp/harfbuzz_wrap.h src/harfbuzz.i

if [ ! -x "./configure" ]; then
    ./autogen.sh
fi

export LDFLAGS="-pipe -Os -no-cpp-precomp"
export CFLAGS=${LDFLAGS}
export CXXFLAGS=${LDFLAGS}

export INSTALL_DIR=$(pwd)/build

#ugly way to fing jni.h automatically (with a chance that it will be int the sdk7)
JNIHEADERS=`locate jni.h | grep 7 | head -n 1 | xargs dirname`


if [ $? -ne 0 ]; then
    JNIHEADERS=/System/Library/Frameworks/JavaVM.framework/Headers
    echo "Unable to find jni.h with locate using default"
fi
echo "JNIHEADERS=${JNIHEADERS}"

./configure --prefix=$INSTALL_DIR --disable-shared --enable-static && make clean >/dev/null && make -j && cp src/.libs/libharfbuzz.a build/harfbuzz.a

echo "### compile wrapper"
#gcc  -m32 -march=i386 -pipe -Os -no-cpp-precomp -c swig/build/cpp/harfbuzz_wrap.c -I ./src -I /System/Library/Frameworks/JavaVM.framework/Headers -o swig/build/cpp/lib/harfbuzz_wrap.o
# -Wl,--add-stdcall-alias
#echo "### link wrapper"
#ld  build/harfbuzz.dylib swig/build/cpp/lib/harfbuzz_wrap.o -dylib -o build/libharfbuzz.so
gcc  -I ./src -I $JNIHEADERS  -pipe -Os -no-cpp-precomp -shared -o swig/build/cpp/lib/harfbuzz_wrap.so swig/build/cpp/harfbuzz_wrap.c  build/harfbuzz.a