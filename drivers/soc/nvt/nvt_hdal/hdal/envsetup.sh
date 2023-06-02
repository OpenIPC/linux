#!/bin/bash
#Fixed environment
export NVT_PRJCFG_CFG=Linux
export ARCH=arm
export NVT_HDAL_DIR=`pwd`
export PLATFORM_CFLAGS="-march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ftree-vectorize -fno-builtin -fno-common -Wformat=1 -D_BSP_NA51089_"

#Update compiler path to <your path>
export CROSS_COMPILE=/opt/ivot/arm-ca9-linux-uclibcgnueabihf-6.4/usr/bin/arm-ca9-linux-uclibcgnueabihf-
export AS=$CROSS_COMPILE}as
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export LDD=${CROSS_COMPILE}ldd
export AR=${CROSS_COMPILE}ar
export NM=${CROSS_COMPILE}nm
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump

#modify your SDK path here
export SDK_PATH="`pwd`/../../../na51089_linux_sdk"

export NVT_MOD_INSTALL="$SDK_PATH/BSP/root-fs/rootfs"
export NVT_LINUX_VER="4.1.0"
export KERNELDIR="$SDK_PATH/BSP/linux-kernel"
export NVT_VOS_DIR="$SDK_PATH/code/vos"
export ROOTFS_DIR="$SDK_PATH/BSP/root-fs"
export OUTPUT_DIR="$SDK_PATH/output"
export NVT_MULTI_CORES_FLAG=-j`grep -c ^processor /proc/cpuinfo`

if [ ! -d $KERNELDIR ] ; then
    echo Directory $KERNELDIR not exist!
    unset KERNELDIR
fi
if [ ! -d $NVT_VOS_DIR ] ; then
    echo Directory $NVT_VOS_DIR not exist!
    unset NVT_VOS_DIR
fi
if [ ! -d $ROOTFS_DIR ] ; then
    echo Directory $ROOTFS_DIR not exist!
    unset ROOTFS_DIR
fi
if [ ! -d $OUTPUT_DIR ] ; then
    echo Directory $OUTPUT_DIR created!
    mkdir $OUTPUT_DIR
fi
