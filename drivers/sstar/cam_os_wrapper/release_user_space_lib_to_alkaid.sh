#!/bin/bash

while getopts "a:?" opt
do
    case $opt in
    a)
        alkaid_project_dir=$OPTARG
        ;;
    ?)
        echo "Invalid option: -$OPTARG" >&2
        exit 1
        ;;
    esac
done

if [ -z "${alkaid_project_dir}" ]
then
    echo "Please specify the path to alkaid/project"
    exit 1;
fi

declare -x ARCH="arm"
declare -x CROSS_COMPILE="arm-linux-gnueabihf-"
declare -x PATH=/tools/toolchain/arm-linux-gnueabihf-4.8.3-201404/bin:$PATH
make -f Makefile_lib clean all
cp .build/lib/libcam_os_wrapper.a  libcam_os_wrapper.a.glibc.4.8.3
cp .build/lib/libcam_os_wrapper.so libcam_os_wrapper.so.glibc.4.8.3

declare -x ARCH="arm"
declare -x CROSS_COMPILE="arm-linux-gnueabihf-"
declare -x PATH=/tools/toolchain/gcc-arm-8.2-2018.08-x86_64-arm-linux-gnueabihf/bin:$PATH
make -f Makefile_lib clean all
cp .build/lib/libcam_os_wrapper.a  libcam_os_wrapper.a.glibc.8.2.1
cp .build/lib/libcam_os_wrapper.so libcam_os_wrapper.so.glibc.8.2.1

declare -x ARCH="arm"
declare -x CROSS_COMPILE="arm-linux-gnueabihf-"
declare -x PATH=/tools/toolchain/gcc-sigmastar-9.1.0-2020.07-x86_64_arm-linux-gnueabihf/bin:$PATH
make -f Makefile_lib clean all
cp .build/lib/libcam_os_wrapper.a  libcam_os_wrapper.a.glibc.9.1.0
cp .build/lib/libcam_os_wrapper.so libcam_os_wrapper.so.glibc.9.1.0

declare -x ARCH="arm64"
declare -x CROSS_COMPILE="aarch64-none-linux-gnu-"
declare -x PATH=/tools/toolchain/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin:$PATH
make -f Makefile_lib clean all
cp .build/lib/libcam_os_wrapper.a  libcam_os_wrapper.a.glibc.9.2.1
cp .build/lib/libcam_os_wrapper.so libcam_os_wrapper.so.glibc.9.2.1

declare -x ARCH="arm64"
declare -x CROSS_COMPILE="aarch64-linux-gnu-"
declare -x PATH=/tools/toolchain/gcc-10.2.1-20210303-sigmastar-glibc-x86_64_aarch64-linux-gnu/bin:$PATH
make -f Makefile_lib clean all
cp .build/lib/libcam_os_wrapper.a  libcam_os_wrapper.a.glibc.10.2.1
cp .build/lib/libcam_os_wrapper.so libcam_os_wrapper.so.glibc.10.2.1

declare -x ARCH="arm"
declare -x CROSS_COMPILE="arm-linux-gnueabihf-"
declare -x PATH=/tools/toolchain/gcc-11.1.0-20210608-sigmastar-glibc-x86_64_aarch64-linux-gnu/bin:$PATH
make -f Makefile_lib clean all
cp .build/lib/libcam_os_wrapper.a  libcam_os_wrapper.a.glibc.11.1.0
cp .build/lib/libcam_os_wrapper.so libcam_os_wrapper.so.glibc.11.1.0

declare -x ARCH="arm"
declare -x CROSS_COMPILE="arm-buildroot-linux-uclibcgnueabihf-"
declare -x PATH=/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4/bin:$PATH
make -f Makefile_lib clean all
cp .build/lib/libcam_os_wrapper.a  libcam_os_wrapper.a.uclibc.4.9.4
cp .build/lib/libcam_os_wrapper.so libcam_os_wrapper.so.uclibc.4.9.4

declare -x ARCH="arm"
declare -x CROSS_COMPILE="arm-sigmastar-linux-uclibcgnueabihf-"
declare -x PATH=/tools/toolchain/arm-sigmastar-linux-uclibcgnueabihf-9.1.0/bin:$PATH
make -f Makefile_lib clean all
cp .build/lib/libcam_os_wrapper.a  libcam_os_wrapper.a.uclibc.9.1.0
cp .build/lib/libcam_os_wrapper.so libcam_os_wrapper.so.uclibc.9.1.0


find ${alkaid_project_dir} -name "libcam_os_wrapper.*" -print0 | while read -d $'\0' file
do
    if [[ ${file} == *"libcam_os_wrapper.so"* ]]; then
        library_type=so
    elif [[ ${file} == *"libcam_os_wrapper.a"* ]]; then
        library_type=a
    else
        echo "${file} library type error"
        exit 1
    fi

    if [[ ${file} == *"glibc"* ]]; then
        toolchain_type=glibc
        if [[ ${file} == *"4.8.3"* ]]; then
            toolchain_ver=4.8.3
        elif [[ ${file} == *"8.2.1"* ]]; then
            toolchain_ver=8.2.1
        elif [[ ${file} == *"9.1.0"* ]]; then
            toolchain_ver=9.1.0
        elif [[ ${file} == *"9.2.1"* ]]; then
            toolchain_ver=9.2.1
        elif [[ ${file} == *"10.2.1"* ]]; then
            toolchain_ver=10.2.1
        elif [[ ${file} == *"11.1.0"* ]]; then
            toolchain_ver=11.1.0
        else
            echo "${file} toolchain version error"
            exit 1
        fi
    elif [[ ${file} == *"uclibc"* ]]; then
        toolchain_type=uclibc
        if [[ ${file} == *"4.9.4"* ]]; then
            toolchain_ver=4.9.4
        elif [[ ${file} == *"9.1.0"* ]]; then
            toolchain_ver=9.1.0
        else
            echo "${file} toolchain version error"
            exit 1
        fi
    else
        echo "${file} toolchain type error"
        exit 1
    fi

    library_name=libcam_os_wrapper.${library_type}.${toolchain_type}.${toolchain_ver}

    echo "copy ${library_name} to ${file}"
    cp ${library_name} ${file}
done

rm libcam_os_wrapper.*

exit 0