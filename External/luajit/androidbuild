#!/bin/bash

set -e

NDK=$ANDROID_NDK

# ARM64 - note that minimum NDK that supports ARM64 is 21
NDKABI=21
NDKVER=$NDK/toolchains/aarch64-linux-android-4.9
NDKP=$NDKVER/prebuilt/darwin-x86_64/bin/aarch64-linux-android-
NDKF="--sysroot $NDK/platforms/android-$NDKABI/arch-arm64"
NDKARCH="-march=armv8-a -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -fomit-frame-pointer -Wall -Werror"

# Build
make clean
make HOST_CC="gcc -m64 -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -fomit-frame-pointer -Wall -Werror" CROSS=$NDKP TARGET_SYS=Linux TARGET_FLAGS="$NDKF $NDKARCH"

# Move to output
mkdir -p lib/Android/Release/arm64-v8a
mv src/libluajit.a lib/Android/Release/arm64-v8a/libluajit.a

# Cleanup
make clean

# ARMv7
NDKABI=16
NDKVER=$NDK/toolchains/arm-linux-androideabi-4.9
NDKP=$NDKVER/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-
NDKF="--sysroot $NDK/platforms/android-$NDKABI/arch-arm"
NDKARCH="-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3 -Wl,--fix-cortex-a8 -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -fomit-frame-pointer -Wall -Werror"

# Build
make clean
make HOST_CC="gcc -m32 -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -fomit-frame-pointer -Wall -Werror" CROSS=$NDKP TARGET_SYS=Linux TARGET_FLAGS="$NDKF $NDKARCH"

# Move to output
mkdir -p lib/Android/Release/armeabi-v7a
mv src/libluajit.a lib/Android/Release/armeabi-v7a/libluajit.a

# Cleanup
make clean
