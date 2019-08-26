#=============================================================================
#
#   Makefile for Linux Robust FAT File System for NAND Flash Memory
#
#   Copyright(c) 2004, Samsung Electronics, Co., Ltd.
#   Copyright(c) 2004, Zeen Information Technologies, Inc.
#
#=============================================================================

EXTRA_CFLAGS	+= -I$(TOPDIR)/include/linux/rfs -I$(TOPDIR)/include/linux/ufd
EXTRA_CFLAGS    += -Iinclude/linux/rfs -Iinclude/linux/ufd

obj-y		:= nand.o

nand-objs	:= fm_driver_stmp37xx.o SECMLC_common.o SECMLC_dma.o
