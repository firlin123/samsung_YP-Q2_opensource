#=============================================================================
#
#   Makefile for Linux Robust FAT File System for NAND Flash Memory
#
#   Copyright(c) 2004, Samsung Electronics, Co., Ltd.
#   Copyright(c) 2004, Zeen Information Technologies, Inc.
#
#=============================================================================

EXTRA_CFLAGS	+= -Iinclude/linux/rfs -Iinclude/linux/ufd

obj-m		:= rfs.o

rfs-objs	:= fm_global.o fm_sem.o fm_blkdev.o 
rfs-objs	+= super.o 

rfs-objs	+= ftl_4k.o
rfs-objs	+= $(addprefix ./, \
		   rfs_linuxk.o rfs_cache.o rfs_log.o rfs_code_convert.o)

