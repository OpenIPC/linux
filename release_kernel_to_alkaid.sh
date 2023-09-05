#!/bin/bash

while getopts "a:c:" opt; do
  case $opt in
    a)
		alkaid_dir=$OPTARG
		;;
    c)
		chip=$OPTARG
		;;
	\?)
	  echo "Invalid option: -$OPTARG" >&2
	  ;;
  esac
done
kernel_dir=$PWD

if [ "${chip}" = "i5" ]
then
	# gcc8.2.1 + glibc
	declare -x ARCH="arm"
	declare -x CROSS_COMPILE="arm-linux-gnueabihf-"
	whereis ${CROSS_COMPILE}gcc

	echo "infinity5_ssc007b_s01b_spinand_defconfig"
	make infinity5_ssc007b_s01b_spinand_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007B -p ipc -f spinand -c i5 -l glibc -v 8.2.1
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007b_s01b_defconfig"
	make infinity5_ssc007b_s01b_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007B -p ipc -f nor -c i5 -l glibc -v 8.2.1
	cd ${kernel_dir}
	make clean

	# gcc4.8.3 + glibc
	declare -x PATH="/tools/toolchain/arm-linux-gnueabihf-4.8.3-201404/bin:$PATH"
	whereis ${CROSS_COMPILE}gcc

	echo "infinity5_ssc007a_s01a_spinand_defconfig"
    whereis ${CROSS_COMPILE}gcc
	make infinity5_ssc007a_s01a_spinand_defconfig
	make clean;make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007A -p ipc -f spinand -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007a_s01a_defconfig"
	make infinity5_ssc007a_s01a_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007A -p ipc -f nor -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007b_s01b_spinand_defconfig"
	make infinity5_ssc007b_s01b_spinand_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007B -p ipc -f spinand -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007b_s01b_defconfig"
	make infinity5_ssc007b_s01b_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007B -p ipc -f nor -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007a_s01a_spinand_uvc_defconfig"
	make infinity5_ssc007a_s01a_spinand_uvc_defconfig
	make clean;make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007A -p usb_cam -f spinand -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007a_s01a_uvc_defconfig"
	make infinity5_ssc007a_s01a_uvc_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007A -p usb_cam -f nor -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007b_s01b_spinand_uvc_defconfig"
	make infinity5_ssc007b_s01b_spinand_uvc_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007B -p usb_cam -f spinand -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007b_s01b_uvc_defconfig"
	make infinity5_ssc007b_s01b_uvc_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007B -p usb_cam -f nor -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007a_s01a_coprocessor_defconfig"
	make infinity5_ssc007a_s01a_coprocessor_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007A -p cop -f nor -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007a_s01a_spinand_coprocessor_defconfig"
	make infinity5_ssc007a_s01a_spinand_coprocessor_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007A -p cop -f spinand -c i5 -l glibc -v 4.8.3
	cd ${kernel_dir}
	make clean

	# gcc4.9.4 + uclibc
	declare -x ARCH="arm"
	declare -x CROSS_COMPILE="arm-buildroot-linux-uclibcgnueabihf-"
    whereis ${CROSS_COMPILE}gcc
	echo "infinity5_ssc007b_s01b_defconfig"
	make infinity5_ssc007b_s01b_defconfig
	make -j20
	cd ${alkaid_dir}/project/kbuild/4.9.84/
	./release.sh -k ${kernel_dir} -b 007B -p ipc -f nor -c i5 -l uclibc -v 4.9.4
	cd ${kernel_dir}
fi
if [ "${chip}" = "i6" ]
then

    declare -x ARCH="arm"
    declare -x CROSS_COMPILE="arm-linux-gnueabihf-"

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_defconfig
    make infinity6_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p ipc -f nor -c i6 -l glibc -v 8.2.1
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_uvc_defconfig
    make infinity6_uvc_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p usbcam -f nor -c i6 -l glibc -v 8.2.1
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_uvc_fastboot_defconfig
    make infinity6_uvc_fastboot_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A-fastboot -p usbcam -f nor -c i6 -l glibc -v 8.2.1
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_ssc009b_s01a_defconfig
    make infinity6_ssc009b_s01a_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009B -p ipc -f nor -c i6 -l glibc -v 8.2.1
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_spinand_defconfig
    make infinity6_spinand_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p ipc -f spinand -c i6 -l glibc -v 8.2.1
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_spinand_uvc_defconfig
    make infinity6_spinand_uvc_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p usbcam -f spinand -c i6 -l glibc -v 8.2.1
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_ssc009b_s01a_spinand_defconfig
    make infinity6_ssc009b_s01a_spinand_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009B -p ipc -f spinand -c i6 -l glibc -v 8.2.1
    cd ${kernel_dir}

    declare -x ARCH="arm"
    declare -x CROSS_COMPILE="arm-buildroot-linux-uclibcgnueabihf-"

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_defconfig
    make infinity6_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p ipc -f nor -c i6 -l uclibc -v 4.9.4
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_uvc_defconfig
    make infinity6_uvc_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p usbcam -f nor -c i6 -l uclibc -v 4.9.4
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_uvc_fastboot_defconfig
    make infinity6_uvc_fastboot_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A-fastboot -p usbcam -f nor -c i6 -l uclibc -v 4.9.4
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_spinand_defconfig
    make infinity6_spinand_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p ipc -f spinand -c i6 -l uclibc -v 4.9.4
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_ssc009b_s01a_spinand_defconfig
    make infinity6_ssc009b_s01a_spinand_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009B -p ipc -f spinand -c i6 -l uclibc -v 4.9.4
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_ssc009b_s01a_defconfig
    make infinity6_ssc009b_s01a_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009B -p ipc -f nor -c i6 -l uclibc -v 4.9.4
    cd ${kernel_dir}

    declare -x ARCH="arm"
    declare -x CROSS_COMPILE="arm-linux-gnueabihf-"
    declare -x PATH="/tools/toolchain/arm-linux-gnueabihf-4.8.3-201404/bin:$PATH"

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_defconfig
    make infinity6_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p ipc -f nor -c i6 -l glibc -v 4.8.3
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_ssc009b_s01a_defconfig
    make infinity6_ssc009b_s01a_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009B -p ipc -f nor -c i6 -l glibc -v 4.8.3
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_spinand_defconfig
    make infinity6_spinand_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p ipc -f spinand -c i6 -l glibc -v 4.8.3
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_ssc009b_s01a_spinand_defconfig
    make infinity6_ssc009b_s01a_spinand_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009B -p ipc -f spinand -c i6 -l glibc -v 4.8.3
    cd ${kernel_dir}
fi

if [ "${chip}" = "i6_dualos" ]
then
    declare -x ARCH="arm"
    declare -x CROSS_COMPILE="arm-linux-gnueabihf-"

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_ssc009a_s01a_spinand_lh_defconfig
    make infinity6_ssc009a_s01a_spinand_lh_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p ipc-rtos -f spinand -c i6 -l glibc -v 7.2.1 -i 4.9.84
    cd ${kernel_dir}

    echo CROSS_COMPILE=$CROSS_COMPILE
    whereis ${CROSS_COMPILE}gcc
    echo make infinity6_ssc009a_s01a_lh_defconfig
    make infinity6_ssc009a_s01a_lh_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 009A -p ipc-rtos -f spinand-ramdisk -c i6 -l glibc -v 7.2.1 -i 4.9.84
    cd ${kernel_dir}
fi

if [ "${chip}" = "i2m" ]
then
    declare -x ARCH="arm"
    declare -x CROSS_COMPILE="arm-linux-gnueabihf-"
    echo "infinity2m_fpga_defconfig"
    make infinity2m_fpga_defconfig
    make clean;make -j20
    cd ${alkaid_dir}/project/kbuild/4.9.84/
    ./release.sh -k ${kernel_dir} -b 000A -p nvr -f nor -c ${chip} -l glibc -v 4.8.3
    cd ${kernel_dir}
fi
