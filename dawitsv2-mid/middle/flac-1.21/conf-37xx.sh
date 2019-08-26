#!/bin/bash
#
# STMP37xx configure
#
if [ "${LIB_INSTALL_DIR}" = "" ]; then
# easy way to get parent dir
	cd ../
	LIB_INSTALL_DIR=${PWD}/MediaSrv/lib/libFLAC
	cd -
fi
echo "====================================="
echo " libflac configuring"
echo " install to ${LIB_INSTALL_DIR}"
echo "====================================="

export CC=arm-none-linux-gnueabi-gcc
export AS=arm-none-linux-gnueabi-as
export AR=arm-none-linux-gnueabi-ar
export LD=arm-none-linux-gnueabi-ld
export NM=arm-none-linux-gnueabi-nm
export RANLIB=arm-none-linux-gnueabi-ranlib  

./configure \
	--prefix=${LIB_INSTALL_DIR} \
	--build=i686-linux \
	--host=arm-none-linux-gnueabi \
	--target=arm-none-linux-gnueabi \
	--enable-fast-install \
	--enable-largefile \
	--enable-shared \
	--disable-static \
	--disable-debug \
	--disable-sse \
	--disable-3dnow \
	--disable-altivec \
	--disable-thorough-tests \
	--disable-exhaustive-tests \
	--disable-valgrind-testing \
	--disable-doxygen-docs \
	--disable-local-xmms-plugin \
	--disable-xmms-plugin  \
	--disable-cpplibs \
	--disable-ogg \
	--disable-oggtest \
	--disable-rpath

echo " done..."
