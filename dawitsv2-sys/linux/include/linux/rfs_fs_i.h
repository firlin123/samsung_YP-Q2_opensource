/************************************************************************/
/*                                                                      */
/*  Copyright (c) 2003-2006 Zeen Information Technologies, Inc.         */
/*  All rights reserved.                                                */
/*                                                                      */
/*  This software is the confidential and proprietary information of    */
/*  Zeen Information Technologies, Inc. ("Confidential Information")    */
/*  You shall not disclose such confidential information and shall use  */
/*  it only in accordance with the terms of the license agreement you   */
/*  entered into the above copyright holder.                            */
/*                                                                      */
/************************************************************************/
/*  This file implements the RFS FAT file system VFS interface.         */
/*                                                                      */
/*  @author  Joosun Hahn                                                */
/*  @author  Sungjoon Ahn                                               */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : RFS Flash File System for NAND Flash Memory               */
/*  FILE    : Zfs_fs_i.h                                                */
/*  PURPOSE : Header file for RFS FAT file system VFS Interface         */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - Put this file in <KERNEL-SRC-DIR>/include/linux directory.        */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 2.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Sungjoon Ahn] : first writing                         */
/*  - 01/10/2003 [Sungjoon Ahn] : added symbolic link support           */
/*  - 15/10/2003 [Joosun Hahn]  : modified for Linux VFS interface      */
/*                                                                      */
/************************************************************************/

#ifndef __RFS_FS_I__
#define __RFS_FS_I__

#include <linux/types.h>
#include <linux/fs.h>

#define RFS_MAGIC               (0x2003BAB1L)
#define RFS_IOCTL_FORMAT        (1)

#define RFS_FNAME_LENGTH        (256*4)   // maximum length of file name
                                          // single unicode can consume up to 4 bytes 

typedef struct rfs_file_id {
        __u32 dev;
        __u32 vol;
        __u32 parent_first_cluster;
        __u32 first_cluster;
        __u8  filename[RFS_FNAME_LENGTH];
        __u32 fn_length;
        __u32 alloc_type;
        __s32 dentry;
        __s32 hint_last_off;
        __u32 hint_last_clu;
} rfs_file_id_t;

typedef struct rfs_dir_entry {
        __u8  type;      // TYPE_FILE, TYPE_DIR, TYPE_SYMLINK, TYPE_SOCKET
        __u32 first_cluster;
        __u8  name[RFS_FNAME_LENGTH];
} rfs_dir_entry_t;      // DIR_ENTRY

typedef struct rfs_inode {
        __u8  type;     // TYPE_FILE, TYPE_DIR, TYPE_SYMLINK, TYPE_SOCKET
        __u32 attr;
        __u32 first_cluster;
        __u32 size;
        __u32 ctime;
        __u32 mtime;
} rfs_inode_t;          // FILE_INFO

typedef struct rfs_sb_info {
        __u32 cluster_size;
        __u32 num_clusters;
        __u32 used_clusters;
        __u32 free_clusters;
        __u32 root_cluster;
} rfs_sb_info_t;  // VOL_INFO

typedef struct rfs_inode_info {
        rfs_file_id_t fid;
} rfs_inode_info_t;

#endif /* __RFS_FS_I__ */
