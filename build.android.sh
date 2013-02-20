#!/bin/bash
PWD="$(pwd)"
ANDROID_PLATFORM=android-14
DESTDIR="$PWD/build"

if [ -z "$ANDROID_NDK" ]; then
    echo "ANDROID_NDK not set (e.g. ~/android/android-ndk-r8d)"
    exit 1
fi
if [ ! -d "build" ]; then
    mkdir build
fi
if [ ! -x "./configure" ]; then
    ./autogen.sh
fi


rm -rf $DESTDIR
TMP="$DESTDIR/.tmp-build-hb"
TOOLCHAIN_DIR="$TMP/ndk-standalone"
"$ANDROID_NDK/build/tools/make-standalone-toolchain.sh" \
        --platform=$ANDROID_PLATFORM \
        --install-dir=$TOOLCHAIN_DIR
PATH=$TOOLCHAIN_DIR/bin:$PATH

CFLAGS="-std=gnu99" ./configure --host=arm-linux-androideabi --prefix=$DESTDIR --disable-shared --enable-static && make clean >/dev/null && make && make install

cp -f src/*-private.hh "$DESTDIR"

cd "$DESTDIR"
mv -f lib/*.a .
mv -f include/harfbuzz/*.h .

echo Cleanup.
rm -rf bin lib include
rm -rf "$TMP"
exit
