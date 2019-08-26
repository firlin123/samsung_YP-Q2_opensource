#=============================================================================
#
#   Makefile for Linux Robust FAT File System for NAND Flash Memory
#
#   Copyright(c) 2004, Samsung Electronics, Co., Ltd.
#   Copyright(c) 2004, Zeen Information Technologies, Inc.
#
#=============================================================================

EXTRA_CFLAGS	+= -Iinclude/linux/rfs -Iinclude/linux/ufd

obj-y		:= large.o

large-objs	:= apbh_common.o gpmi_common.o gpmi_dma.o \
		   nand_common.o nandsb.o

obj-y		+= secmlc-ddp/

