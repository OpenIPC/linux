#!/bin/sh
AXERA_TARGET=$1

if [ ! -z $2 ];then
	AXERA_MODULE_NAME="$2 "
fi

BUILD_AXVERSION=$3

AXERA_JENKINS="jenkins"
AXERA_COMPILE_BY=$(whoami | sed 's/\\/\\\\/')

if [[ $AXERA_COMPILE_BY == *$AXERA_JENKINS* ]]
then
COMPILE_USER=" JK"
fi

# Generate a temporary ax_module_version.h
rm -rf $AXERA_TARGET
{ echo /\* This file is auto generated, please DO NOT modify \*/
  echo \#ifndef __AX_MODULE_VERSION_H__
  echo
  echo \#define AXERA_MODULE_VERSION \"[Axera version]: $AXERA_MODULE_NAME \" BUILD_AXVERSION \" \" __DATE__ \" \" __TIME__ \"$COMPILE_USER\"
  echo static const char axera_module_version[] = AXERA_MODULE_VERSION\;
  echo
  echo \#endif //__AX_MODULE_VERSION_H__
} > $AXERA_TARGET
