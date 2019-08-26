#=============================================================================
#
#   Makefile for Linux Robust FAT File System for NAND Flash Memory
#
#   Copyright(c) 2004, Samsung Electronics, Co., Ltd.
#   Copyright(c) 2004, Zeen Information Technologies, Inc.
#
#=============================================================================

EXTRA_CFLAGS	+= -Iinclude/linux/ufd -Iinclude/linux/rfs

obj-y		:= ufd.o

ufd-objs	:= fd_global.o
ufd-objs	+= fd_if.o fd_logical.o fd_bm.o fd_physical.o
ufd-objs	+= fm_raw_blkdev.o 

obj-y		+= lld/large/

#ufd-objs	+= $(addprefix lld/large/, \
#		   apbh_common.o gpmi_common.o gpmi_dma.o \
#		   nand_common.o nandsb.o)

#ufd-objs	+= $(addprefix lld/large/secmlc-ddp/, \
#		   fm_driver_stmp37xx.o SECMLC_common.o SECMLC_dma.o)

