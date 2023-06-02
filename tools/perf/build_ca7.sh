#!/bin/sh
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ WERROR=0 JOBS=8
arm-linux-gnueabihf-strip --strip-unneeded perf
