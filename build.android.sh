#!/bin/bash -e
PWD="$(pwd)"
ANDROID_PLATFORM=android-14
DESTDIR="$PWD/build"
ARCH=arm

if [ ! -z "$1" ]; then
    ARCH=$1
    if [ "$ARCH" != "arm" -a "$ARCH" != "x86" ]; then
        echo "Unknown arhitecture: $ARCH"
        exit 1
    fi
fi

case $ARCH in
    arm) HOST=arm-linux-androideabi
        ;;
    x86) HOST=i686-linux-android
        ;;
esac

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
TOOLCHAIN_DIR="$TMP/ndk-standalone-$ARCH"
"$ANDROID_NDK/build/tools/make-standalone-toolchain.sh" \
        --arch=$ARCH \
        --platform=$ANDROID_PLATFORM \
        --install-dir=$TOOLCHAIN_DIR
PATH=$TOOLCHAIN_DIR/bin:$PATH

CFLAGS="-std=gnu99" ./configure --host=$HOST --prefix=$DESTDIR --disable-shared --enable-static && make clean >/dev/null && make && make install

cp -f src/*-private.hh "$DESTDIR"

cd "$DESTDIR"
mv -f lib/*.a .
mv -f include/harfbuzz/*.h .

echo Cleanup.
rm -rf bin lib include
rm -rf "$TMP"
exit
