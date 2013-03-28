#!/bin/bash

if [ ! -d "build" ]; then
    mkdir build
fi
if [ ! -x "./configure" ]; then
    ./autogen.sh
fi
rm build/libharfbuzz*

export INSTALL_DIR=$(pwd)/build
export DEV_IOS=/Developer/Platforms/iPhoneSimulator.platform/Developer
export SDK_IOS=${DEV_IOS}/SDKs/iPhoneSimulator5.0.sdk
export COMPILER_IOS=${DEV_IOS}/usr/llvm-gcc-4.2/bin
export BIN_IOS=${DEV_IOS}/usr/bin
export CC=${COMPILER_IOS}/llvm-gcc-4.2
export CXX=${COMPILER_IOS}/llvm-g++-4.2
export LDFLAGS="-arch i386 -pipe -Os -no-cpp-precomp -isysroot $SDK_IOS"
export CFLAGS=${LDFLAGS}
export CXXFLAGS=${LDFLAGS}
export CPP=${COMPILER_IOS}/llvm-cpp-4.2
export CXXCPP=${COMPILER_IOS}/llvm-cpp-4.2
export LD=${BIN_IOS}/ld
export AR=${BIN_IOS}/ar
export AS=${BIN_IOS}/as
export NM=${BIN_IOS}/nm
export RANLIB=${BIN_IOS}/ranlib
if [ ! -d "$SDK_IOS" ]; then
    echo "$SDK_IOS not found"
    exit 1;
fi

./configure --prefix=$INSTALL_DIR --disable-shared --enable-static --host=i686-apple-darwin && make clean >/dev/null && make -j && cp src/.libs/libharfbuzz.a build/libharfbuzz-i386.a

export DEV_IOS=/Developer/Platforms/iPhoneOS.platform/Developer
export SDK_IOS=${DEV_IOS}/SDKs/iPhoneOS5.0.sdk
export COMPILER_IOS=${DEV_IOS}/usr/llvm-gcc-4.2/bin
export BIN_IOS=${DEV_IOS}/usr/bin
export CC=${COMPILER_IOS}/llvm-gcc-4.2
export CXX=${COMPILER_IOS}/llvm-g++-4.2
export LDFLAGS_DEBUG="-arch armv7 -pipe -Os -gdwarf-2 -no-cpp-precomp -mthumb -isysroot $SDK_IOS"
export LDFLAGS="-arch armv7 -pipe -Os -no-cpp-precomp -mthumb -isysroot $SDK_IOS"
export CFLAGS=${LDFLAGS}
export CXXFLAGS=${CFLAGS}
export CPP=${COMPILER_IOS}/llvm-cpp-4.2
export CXXCPP=${COMPILER_IOS}/llvm-cpp-4.2
export LD=${BIN_IOS}/ld
export AR=${BIN_IOS}/ar
export AS=${BIN_IOS}/as
export NM=${BIN_IOS}/nm
export RANLIB=${BIN_IOS}/ranlib
if [ ! -d "$SDK_IOS" ]; then
    echo "$SDK_IOS not found"
    exit 1;
fi

./configure --prefix=$INSTALL_DIR --disable-shared --enable-static --host=arm-apple-darwin && make clean >/dev/null && make -j && cp src/.libs/libharfbuzz.a build/libharfbuzz-armv7.a
ls -l build
exit
