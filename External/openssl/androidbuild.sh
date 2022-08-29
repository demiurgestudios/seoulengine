#!/bin/bash -e
#
# Adapted from: https://github.com/ph4r05/android-openssl

# Architectures to build libraries for
declare -a ARCHITECTURES=("armv7a" "arm64-v8a")

# OpenSSL version we're using.
OPENSSL_VERSION="1.0.1t"

# Set acccording to your Android NDK
ANDROID_ABI_VER=4.9
ANDROID_PLATFORM=android-${ANDROID_API_LEVEL}

####################################################################################################

RETURN_DIR="$PWD"
BASEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
UDIR="${BASEDIR}/out"
DIR="${UDIR}/sources_orig"

if [ -d "${UDIR}" ]; then
    rm -rf "${UDIR}"
fi

mkdir "${UDIR}"
cd "${UDIR}"

if [ ! -d "${DIR}" ]; then
    tar -xzvf "${BASEDIR}/openssl-src-with-demiurge-mods.tar.gz"
    mv "openssl-${OPENSSL_VERSION}" "${DIR}"
fi

# Check for rsync.
which rsync 2>/dev/null > /dev/null
RSYNC_OK=$?
if [[ $RSYNC_OK != 0 ]]; then
    echo "> Error: rsync was not found, please install it".
    exit 3
fi

####################################################################################################

function cleanVars () {
    unset AR
    unset AS
    unset CC
    unset CXX
    unset LD
    unset LIBDIR
    unset RANLIB
    unset STRIP
    unset ANDROID_DEV
    unset CROSS_SYSROOT
    unset CFLAGS
    unset LDFLAGS
}

####################################################################################################
function compile () {
    arch_base=$1
    api_level=$3
    sysroot=${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}/arch-arm
    sysroot_inc=${ANDROID_NDK}/sysroot
    prebuilt=${ANDROID_NDK}/toolchains/${arch_base}-${ANDROID_ABI_VER}/prebuilt
    bin=${prebuilt}/darwin-x86_64/bin
    target_host=${bin}/${arch_base}-

    export ANDROID_DEV="${sysroot}/usr"
    export CROSS_SYSROOT="${sysroot}"
    export CFLAGS="-D__ANDROID_API__=${api_level} -isystem${sysroot_inc}/usr/include -isystem${sysroot_inc}/usr/include/${arch_base} --sysroot=${sysroot}"
    export LFLAGS="-L${sysroot}/usr/lib --sysroot=${sysroot}"

    export AR=${target_host}ar
    export AS=${target_host}gcc
    export CC="${target_host}gcc ${CFLAGS}"
    export CXX=${target_host}gcc++
    export LD="${target_host}ld ${LFLAGS}"
    export RANLIB=${target_host}ranlib
    export STRIP=${target_host}strip

    ./Configure android-$2 no-shared no-ec-nistp-64-gcc-128 no-ssl2 no-sctp no-camellia no-capieng no-cast no-cms no-gmp no-idea no-jpake no-md2 no-mdc2 no-rc5 no-sha0 no-rfc3779 no-seed no-store no-whirlpool no-deprecated no-hw no-engine

    make clean
    make depend
    make build_libs
}

####################################################################################################
#
# Builds architecture in its directory.
#
function buildArchitectureSeparately () {
    curArch=$1
    ARCHDIR=${DIR}_${curArch}

    echo -e "\n\n\n"
    echo "================================================================================"
    echo " - ARCH: ${curArch}"
    echo "================================================================================"
    echo "> Copying architecture $curArch to $ARCHDIR/"
    rsync -a --update --exclude '*.o' -v "${DIR}/" "${ARCHDIR}/"

    # Clear old libraries, if needed.
    cd "${ARCHDIR}"
    # clearlib

    echo "> Building architecture ${curArch} in directory: ${ARCHDIR}"
    cd "${ARCHDIR}"

    # Particular build commands
    cleanVars
    case "$curArch" in
    armv7*)
        echo "armv7"
        compile arm-linux-androideabi armv7 16
        LIBDIR="${BASEDIR}/libs/Android/armeabi-v7a"

        # includes
        mkdir -p "${UDIR}/sources/include"
        rsync -avL "${ARCHDIR}/include/" "${UDIR}/sources/include/"
        ;;
    arm64-*)
        echo "arm64"
        compile aarch64-linux-android arm64 21
        LIBDIR="${BASEDIR}/libs/Android/arm64-v8a"
        ;;

    esac
    mkdir -p "$LIBDIR"
    cp lib*.a "$LIBDIR"
}

# Build.
for i in "${ARCHITECTURES[@]}"
do
    buildArchitectureSeparately "${i}"
done

# Return to calling directory.
cd "${RETURN_DIR}"
echo "> DONE for [${PEX_BUILD}]"
