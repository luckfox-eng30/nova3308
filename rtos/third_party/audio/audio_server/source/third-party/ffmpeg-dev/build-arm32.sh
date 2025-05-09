#! /usr/bin/env bash
#
# build script for cross compiling ffmpeg for android-arm32
# Just configure the ${HOME_NDK}
# Android 7.0 (API level 24).
# Android 6.0 (API level 23).
# Android 5.1 (API level 22).
#
# @Todo: remove compilation warning
# @Todo: verify default opus decoder availability
# @Todo: deep detection for android level compatibility
#
# ${HOME_NDK}/sysroot/usr/include/media
# ${HOME_NDK}/platforms/android-XX/arch-arm/usr/lib/libmediandk.so
#

HOME_NDK=/home/mid_sdk/android-ndk-r16b

FFMPEG_ROOT=`pwd`
ROCKIT_ROOT=${FFMPEG_ROOT}/../../
HOME_PREBUILT=${FFMPEG_ROOT}/../prebuilt
CPU_ARCH=armv7-a
OS_ARCH=arm
OS_CROSS=${OS_ARCH}-linux-androideabi
FFMPEG_ARCH=arm32
FFMPEG_LIB_NAME=libffmpeg_58.so
PREFIX=${FFMPEG_ROOT}/FFmpeg/android/${CPU_ARCH}

# https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html
OPTIMIZE_CFLAGS="-march=${CPU_ARCH} -mtune=cortex-a9"
MODULE_CONFIG_FILE_NAME="module_config_default.sh"

# Android NDK
TARGET_PLATFORM_LEVEL=23
SYS_ROOT=${HOME_NDK}/platforms/android-${TARGET_PLATFORM_LEVEL}/arch-arm
SYS_HEADER=${HOME_NDK}/sysroot
TOOLCHAINS=${HOME_NDK}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64

# Detect NDK ToolChain
[ -d ${HOME_NDK}   ] && echo "Found ${HOME_NDK}"   || echo "NOT Found:${HOME_NDK}"
[ -d ${SYS_ROOT}   ] && echo "Found ${SYS_ROOT}"   || echo "NOT Found:${SYS_ROOT}"
[ -d ${TOOLCHAINS} ] && echo "Found ${TOOLCHAINS}" || echo "NOT Found:${TOOLCHAINS}"
if [ -d ${TOOLCHAINS} ];
then
    echo "build ffmpeg project."
else
    echo "please config and check HOME_NDK...."
    exit 1;
fi

# Process Input Command
CLEAN_PROJECT=0
while getopts "cf:" opt
do
    case $opt in
        c)
        echo "clean ffmpeg project"
        CLEAN_PROJECT=1
        ;;
        f)
        echo "select config: module_config_$OPTARG.sh"
        MODULE_CONFIG_FILE_NAME="module_config_$OPTARG.sh"
        ;;
        ?)
        echo "Unknown Options! $opt"
        exit 1;;
    esac
done

# Check/Clone FFmpeg Project Source Code
FFMPG_GIT=https://github.com/FFmpeg/FFmpeg
if [ -d "${FFMPEG_ROOT}/FFmpeg/.git" ];
then
    git remote -v
    git branch
else
    git clone ${FFMPG_GIT}
    echo "ffmpeg clone completed"
    cd FFmpeg
    git remote -v
    git fetch origin
    git checkout origin/release/4.0 -b release/4.0
fi

COMMON_FF_LIBS=
COMMON_FF_HEADERS=
# Using rockit librtmem to manage memory allocation for debug
if [ "$1" = "debug" ]
then
MEM_COMPILER_OPT=--disable-optimizations
MALLOC_OPT=--malloc-prefix=av_mem_
COMMON_FF_LIBS="${COMMON_FF_LIBS} -lrtmem"
COMMON_FF_HEADERS="${COMMON_FF_HEADERS} -I${HOME_PREBUILT}/headers/rtmem"
OPTIMIZE_CFLAGS="${OPTIMIZE_CFLAGS} -fno-omit-frame-pointer"

if ! grep -q "rt_mem.h" ${FFMPEG_ROOT}/FFmpeg/libavutil/mem.c
then
    sed -i '/ifdef MALLOC_PREFIX/a\#include "rt_mem.h"\' ${FFMPEG_ROOT}/FFmpeg/libavutil/mem.c
fi
fi

cd ${FFMPEG_ROOT}/FFmpeg/

# Clean Project
if [ ${CLEAN_PROJECT} -eq 1 ]
then
    make clean
    exit 0
fi

# FFmpeg Build Params
if [ -f ${FFMPEG_ROOT}/config/${MODULE_CONFIG_FILE_NAME} ]
then
    export COMMON_FF_CFG_FLAGS=
    source ${FFMPEG_ROOT}/config/${MODULE_CONFIG_FILE_NAME}
else
    echo "Invail config file name: ${FFMPEG_ROOT}/config/${MODULE_CONFIG_FILE_NAME}"
    exit 1
fi

EN_LIB_SSL=1
EN_LIB_OPUS=1
EN_LIB_ICONV=1
export PKG_CONFIG_PATH=${FFMPEG_ROOT}/../prebuilt/pkgconfig
# openssl
pkg-config --exists openssl
if [ $? -eq 0 ] && [ ${EN_LIB_SSL} -eq 1 ]; then
    COMMON_FF_LIBS="${COMMON_FF_LIBS} -lssl -lcrypto"
    COMMON_FF_HEADERS="${COMMON_FF_HEADERS} -I${HOME_PREBUILT}/headers/boringssl/include"
    export COMMON_FF_CFG_FLAGS="$COMMON_FF_CFG_FLAGS --enable-openssl"
fi
# libopus
pkg-config --exists opus
if [ $? -eq 0 ] && [ ${EN_LIB_OPUS} -eq 1 ]; then
    COMMON_FF_LIBS="${COMMON_FF_LIBS} -lrtopus"
    COMMON_FF_HEADERS="${COMMON_FF_HEADERS} -I${HOME_PREBUILT}/headers/libopus/include"
    export COMMON_FF_CFG_FLAGS="$COMMON_FF_CFG_FLAGS --enable-libopus"
fi
# iconv
if [ ${EN_LIB_ICONV} -eq 1 ]; then
    COMMON_FF_LIBS="${COMMON_FF_LIBS} -liconv"
    COMMON_FF_HEADERS="${COMMON_FF_HEADERS} -I${HOME_PREBUILT}/headers/iconv/include"
    export COMMON_FF_CFG_FLAGS="$COMMON_FF_CFG_FLAGS --enable-iconv"
fi
echo ${COMMON_FF_CFG_FLAGS}

./configure --target-os=android \
            --prefix=${PREFIX} \
            --enable-cross-compile \
            --extra-libs="-lgcc" \
            --arch=${OS_ARCH} \
            --cc=${TOOLCHAINS}/bin/${OS_CROSS}-gcc \
            --cross-prefix=${TOOLCHAINS}/bin/${OS_CROSS}- \
            --nm=${TOOLCHAINS}/bin/${OS_CROSS}-nm \
            --sysroot=${SYS_ROOT} \
            --extra-cflags="${COMMON_FF_HEADERS} \
                        -I${SYS_HEADER}/usr/include \
                        -I${SYS_HEADER}/usr/include/${OS_CROSS} \
                        -I${PREFIX}/include \
                        -O3 -fpic -fasm -std=c99 \
                        -DANDROID \
                        -D__ANDROID_API__=${TARGET_PLATFORM_LEVEL} \
                        -DHAVE_BORINGSSL \
                        -DHAVE_SYS_UIO_H=1 \
                        -Dipv6mr_interface=ipv6mr_ifindex \
                        -Wno-attributes -Wno-unused-function \
                        -Wno-psabi -fno-short-enums \
                        -fno-strict-aliasing \
                        -finline-limit=300 ${OPTIMIZE_CFLAGS}" \
            --extra-ldflags="-Wl,-rpath-link=${SYS_ROOT}/usr/lib \
                        -L${SYS_ROOT}/usr/lib \
                        -L${PREFIX}/lib \
                        -L${HOME_PREBUILT}/${FFMPEG_ARCH} \
                        -nostdlib -lc -lm -ldl -llog ${COMMON_FF_LIBS}" \
            --enable-asm \
            --enable-neon \
            ${COMMON_FF_CFG_FLAGS} \
            ${MALLOC_OPT} \
            ${MEM_COMPILER_OPT} \
            --pkg-config=$(which pkg-config)

FFMPEG_BUILD_FLAGS="ffmpeg open source projects(http://www.ffmpeg.org/), build for android-arm-32bit. ${COMMON_FF_CFG_FLAGS}"
sed -i "/^#define *FFMPEG_CONFIGURATION */c#define FFMPEG_CONFIGURATION \"${FFMPEG_BUILD_FLAGS}\"" config.h

# build ffmpeg library if config is ok
if [ $? -eq 0 ]; then
    make -j8 install
fi

#Binutils supports 2 linkers, ld.gold and ld.bfd.  One of them is
#configured as the default linker, ld, which is used by GCC.  Sometimes,
#we want to use the alternate linker with GCC at run-time.  This patch
#adds -fuse-ld=bfd and -fuse-ld=gold options to GCC driver.  It changes
#collect2.c to pick either ld.bfd or ld.gold.

ls ${TOOLCHAINS}/bin/${OS_CROSS}-ar
ls $TOOLCHAINS/bin/${OS_CROSS}-ld
ls ${TOOLCHAINS}/lib/gcc/${OS_CROSS}/4.9.x/libgcc.a

${TOOLCHAINS}/bin/${OS_CROSS}-ar d libavcodec/libavcodec.a inverse.o

${TOOLCHAINS}/bin/${OS_CROSS}-ld \
          -rpath-link=${SYS_ROOT}/usr/lib \
          -L${SYS_ROOT}/usr/lib \
          -L${HOME_PREBUILT}/${FFMPEG_ARCH} \
          -soname ${FFMPEG_LIB_NAME} -shared -nostdlib  -z noexecstack -Bsymbolic \
          --whole-archive --no-undefined -o ${PREFIX}/${FFMPEG_LIB_NAME} \
          libavcodec/libavcodec.a libavformat/libavformat.a \
          libavutil/libavutil.a libswresample/libswresample.a \
          -lc -lm -lz -ldl -llog ${COMMON_FF_LIBS} \
          --dynamic-linker=/system/bin/linker \
          ${TOOLCHAINS}/lib/gcc/${OS_CROSS}/4.9.x/armv7-a/libgcc.a

${TOOLCHAINS}/bin/${OS_CROSS}-strip --strip-unneeded ${PREFIX}/${FFMPEG_LIB_NAME}

# copy new files to prebuilt if linux-android-ld is ok.
if [ $? -eq 0 ]; then
    cp ${PREFIX}/${FFMPEG_LIB_NAME} ${HOME_PREBUILT}/${FFMPEG_ARCH}/${FFMPEG_LIB_NAME} -rf
    rm ${HOME_PREBUILT}/headers/ffmpeg-4.0/include -rf
    cp ${PREFIX}/include ${HOME_PREBUILT}/headers/ffmpeg-4.0/ -rf
    ls -alh ${HOME_PREBUILT}/${FFMPEG_ARCH}/
fi

cd -
