#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only

# set -x


arch=arm
cross_compile=arm-gcc7.3-linux-musleabi-
soc=xmorca
out=.

function usage()
{
	echo "$(basename $0) --build/--clean/--help --arch=arm \
		--cross_compile=<toolchain prefix> --out=<out dir> \
		--soc <soc name> --hwgzip=<hwgzip tool file> "
	exit 1
}

ARGS=$(getopt -o bc \
	--long "build,clean,arch:,cross_compile:,\
out:,soc:,compress:,hwgzip:,help" \
	-n "$(basename $0)" -- "$@")
if [ $? != 0 ];then
	echo "Invalid args"
	usage
	exit 1
fi

# echo ARGS=$ARGS
eval set -- "$ARGS"

while true; do
	case $1 in
		-b | --build)
			build=1
			shift
			;;
		-c | --clean)
			clean=1
			shift
			;;
		--arch)
			arch=$2
			shift 2
			;;
		--cross_compile)
			cross_compile=$2
			shift 2
			;;
		--out)
			out=$2
			shift 2
			;;
		--soc)
			soc=$2
			shift 2
			;;
		--compress)
			compress=$2
			shift 2
			;;
		--hwgzip)
			hwgzip=$2
			shift 2
			;;
		--help)
			usage
			;;
		--)
			shift
			break
			;;
		*)
			echo "Arg error"
			exit 1
			;;
	esac
done

if [ "ss$out" = "ss" ]; then
	out=.
fi

kernel_version=linux-5.10.y

kernel_config=$out/.config

loadaddr=$(test -f $kernel_config && sed -n '/CONFIG_BSP_ZRELADDR=/p' ${kernel_config} \
	| sed 's/CONFIG_BSP_ZRELADDR=//')

entryaddr=$loadaddr

image=$out/arch/$arch/boot/Image

zimage=$out/arch/$arch/boot/zImage
zimage_dtb=$out/arch/$arch/boot/zImage-dtb

hwzimage=$out/arch/$arch/boot/hwzImage
hwzimage_dtb=$out/arch/$arch/boot/hwzImage-dtb

uimage=$out/arch/$arch/boot/uImage
uimage_dtb=$out/arch/$arch/boot/uImage-dtb

dtb=$out/lotus/machine/${soc}/dts/${soc}.dtb

function create_hw_gzip_image()
{
	# entryaddr=$(printf 0x%x $((loadaddr + 16)))

	if [ -n "$hwgzip" ]; then
		$hwgzip -9 -c $image >$hwzimage
		dd if=$hwzimage of=$hwzimage.4aligned bs=4 conv=sync
		mkimage -A $arch -O linux -C $compress -T kernel -a ${loadaddr} \
			-e ${entryaddr} -n ${kernel_version} \
			-d $hwzimage.4aligned $hwzimage.u
		cat $hwzimage.u $dtb >$hwzimage-dtb
	fi
}

function create_arm_uimage()
{
	cat $zimage $dtb >$zimage_dtb
	mkimage -A $arch -O linux -C none -T kernel -a ${loadaddr} \
		-e ${entryaddr} -n ${kernel_version} \
		-d $zimage_dtb $uimage
}

function create_arm64_uimage()
{
	dd if=$zimage of=$zimage-aligned-4k bs=4096 conv=sync
	mkimage -A $arch -O linux -C $compress -T kernel -a ${loadaddr} \
		-e ${entryaddr} -n ${kernel_version} \
		-d $zimage-aligned-4k $uimage
	cat $uimage $dtb >$uimage_dtb
	cp $uimage_dtb $uimage
}

if [ "$build" = "1" ]; then
	make ARCH=$arch CROSS_COMPILE=$cross_compile O=$out dtbs
	if [ "$arch" == "arm" ]; then
		create_arm_uimage
		create_hw_gzip_image
	elif [ "$arch" == "arm64" ]; then
		create_arm64_uimage
		create_hw_gzip_image
	else
		echo "Invalid ARCH: $arch"
	fi
fi

if [ "$clean" = "1" ]; then
	rm -f $zimage
	rm -f $zimage_dtb
	rm -f $uimage
	rm -f $uimage_dtb
fi
