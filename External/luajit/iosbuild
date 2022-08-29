#!/bin/bash

set -e

IXCODE=`xcode-select -print-path`

if [ -d "temp" ]; then
  rm -rf temp
fi
mkdir temp
make clean

# armv7
ISDK=$IXCODE/Platforms/iPhoneOS.platform/Developer
ISDKVER=iPhoneOS11.4.sdk
ISDKP=$IXCODE/Toolchains/XCodeDefault.xctoolchain/usr/bin/
ISDKF="-arch armv7 -isysroot $ISDK/SDKs/$ISDKVER -miphoneos-version-min=8.0 -fno-omit-frame-pointer -DLJ_NO_SYSTEM -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -Wall -Werror"
make clean
make CC=clang HOST_CC="clang -m32 -arch i386 -DLJ_NO_SYSTEM -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -Wall -Werror" CROSS=$ISDKP TARGET_FLAGS="$ISDKF" TARGET_SYS=iOS
mv src/libluajit.a temp/libluajit_armv7.a

# arm64
ISDK=$IXCODE/Platforms/iPhoneOS.platform/Developer
ISDKVER=iPhoneOS11.4.sdk
ISDKP=$IXCODE/Toolchains/XCodeDefault.xctoolchain/usr/bin/
ISDKF="-arch arm64 -isysroot $ISDK/SDKs/$ISDKVER -miphoneos-version-min=8.0 -DLJ_NO_SYSTEM -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -Wall -Werror"
make clean
make CC=clang HOST_CC="clang -m64 -arch x86_64 -DLJ_NO_SYSTEM -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -Wall -Werror" CROSS=$ISDKP TARGET_FLAGS="$ISDKF" TARGET_SYS=iOS
mv src/libluajit.a temp/libluajit_arm64.a

# simulator x86
ISDK=$IXCODE/Platforms/iPhoneSimulator.platform/Developer
ISDKVER=iPhoneSimulator11.4.sdk
ISDKP=$IXCODE/Toolchains/XCodeDefault.xctoolchain/usr/bin/
ISDKF="-arch i386 -isysroot $ISDK/SDKs/$ISDKVER -miphoneos-version-min=8.0 -DLJ_NO_SYSTEM -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -Wall -Werror"
make clean
make CC=clang HOST_CC="clang -m32 -arch i386 -DLJ_NO_SYSTEM -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -Wall -Werror" CROSS=$ISDKP TARGET_FLAGS="$ISDKF" TARGET_SYS=iOS
mv src/libluajit.a temp/libluajit_simulator_x86.a

# simulator x64
ISDK=$IXCODE/Platforms/iPhoneSimulator.platform/Developer
ISDKVER=iPhoneSimulator11.4.sdk
ISDKP=$IXCODE/Toolchains/XCodeDefault.xctoolchain/usr/bin/
ISDKF="-arch x86_64 -isysroot $ISDK/SDKs/$ISDKVER -miphoneos-version-min=8.0 -DLUAJIT_ENABLE_GC64 -DLJ_NO_SYSTEM -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -Wall -Werror"
make clean
make CC=clang HOST_CC="clang -m64 -arch x86_64 -DLUAJIT_ENABLE_GC64 -DLJ_NO_SYSTEM -DLUAJIT_USE_SYSMALLOC -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI -g -O2 -Wall -Werror" CROSS=$ISDKP TARGET_FLAGS="$ISDKF" TARGET_SYS=iOS
mv src/libluajit.a temp/libluajit_simulator_x64.a

# combine into a universal binary.
libtool -o lib/IOS/Release/libluajit.a temp/*.a 2> /dev/null

# cleanup
rm -rf temp
make clean
