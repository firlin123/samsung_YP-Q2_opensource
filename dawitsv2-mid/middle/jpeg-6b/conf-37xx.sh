#!/bin/bash

if [ "${LIB_INSTALL_DIR}" = "" ]; then
# easy way to get parent dir
	cd ../../../
	LIB_INSTALL_DIR=${PWD}/lib/target
	cd -
fi
echo "====================================="
echo " libjpeg configuring"
echo " install to ${LIB_INSTALL_DIR}"
echo "====================================="

export CC=armv5-empeg-linux-gnueabi-gcc
export AS=armv5-empeg-linux-gnueabi-as
export AR=armv5-empeg-linux-gnueabi-ar
export LD=armv5-empeg-linux-gnueabi-ld
export NM=armv5-empeg-linux-gnueabi-nm
export RANLIB=armv5-empeg-linux-gnueabi-ranlib

./configure \
	--build=i686-linux \
	--host=armv5-empeg-linux-gnueabi  \
	--target=armv5-empeg-linux-gnueabi \
	--prefix=${LIB_INSTALL_DIR} \
	--enable-shared 

echo " done..."