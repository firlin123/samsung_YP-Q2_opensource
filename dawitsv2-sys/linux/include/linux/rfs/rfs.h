/************************************************************************/
/*                                                                      */
/*  Copyright (c) 2004 Flash Planning Group, Samsung Electronics, Inc.  */
/*  Copyright (c) 2004 Zeen Information Technologies, Inc.              */
/*  All right reserved.                                                 */
/*                                                                      */
/*  This software is the confidential and proprietary information of    */
/*  Samsung Electronics, Inc. and Zeen Information Technologies, Inc.   */
/*  ("Confidential Information"). You shall not disclose such           */
/*  confidential information and shall use it only in accordance with   */
/*  the terms of the license agreement you entered into with one of     */
/*  the above copyright holders.                                        */
/*                                                                      */
/************************************************************************/
/*  This file implements the RFS FAT file system core.                  */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : rfs.h                                                     */
/*  PURPOSE : Header file for Linux Robust FAT File System Core         */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - Directory Entry, MBR, PBR data structures, etc.                   */
/*  - FAT related external function declarations                        */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*  - 01/10/2003 [Joosun Hahn]  : added MBR/PBR data structures         */
/*                                added endian macros                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _RFS_H
#define _RFS_H

#include "fm_global.h"
#include "rfs_cache.h"
#include "rfs_log.h"
#include "rfs_api.h"

/* ----------------------- */
/* check point insertion   */
/* ----------------------- */

#ifdef USE_CHECK_POINT
extern unsigned short check_this;
#define check_point() \
	{ \
		if (check_this == __LINE__) { \
			printk("Reset this system at %s:%d\n", __FUNCTION__, __LINE__);\
			machine_restart(0);\
		}\
	}
#else
#define check_point() 
#endif

/*----------------------------------------------------------------------*/
/*  FFS Configurations                                                  */
/*----------------------------------------------------------------------*/

#define TARGET_LITTLE_ENDIAN    1           // target CPU is little-endian

#define USE_FILE_DATE           1

#define USE_VFAT_NAME           1           // comment out this definition
                                            // to use extended dir entries 
                                            // thereby keeping long file names 
                                            // also for 8.3 format file names

#define USE_FILE_DATA_CACHE     0
#define USE_FILE_DATA_CACHE_INV 1

#define EXCESSIVE_SEEK          1           // comment out this definition
                                            // to disable the file 'holes'

#define AUTO_FORMAT             0           // comment out this definition
                                            // to disable the auto-format
                                            // feature, i.e. an unformatted
                                            // device is automatically 
                                            // formatted at the mount time
#define AUTO_FATTYPE            FAT16       // default fat type and
#define AUTO_CLUSIZE            8           // sectors per cluster 
                                            // for the auto-format feature
     
#define ANTI_FRAG_CLU_ALLOC     1           // comment out this macro def. to
                                            // disable the anti-fragmentation
                                            // cluster alloccation scheme

#define CONFIG_RFS_FAT32        1

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/
//#define CONFIG_FREE_CLUSTER_MANAGEMENT   1

#define FSIZE_0_CLUSTER_ALLOC   1           // Set 1 for DOS FAT Compatible

#ifdef USE_FILE_LOG
#define PRE_CLU_ALLOC_SIZE      64
#else
#define PRE_CLU_ALLOC_SIZE      1
#endif

#ifdef USE_FILE_DATA_CACHE
#define FILE_DATA_CACHING_TH    8           // num of sectors
#endif

#if defined(ZFLASH_LINUX) && defined(ZFLASH_APP)
#undef  PRE_CLU_ALLOC_SIZE
#define PRE_CLU_ALLOC_SIZE      1           // for mkrfs
#endif

#define FFS_MAX_OPEN            32          // max number of opened file
#define FFS_MAX_DENTRY          512         // max number of root_dir entries

#define DENTRY_T_SIZE           32          // MS-DOS FAT Compatible

#define BACKWARD                0
#define FORWARD                 1

#define SIGNATURE               0xAA55
#define FSINFO_SIG1             0x41615252
#define FSINFO_SIG2             0x61417272

#define VOL_LABEL               "Q2         " // size should be 11

#define OEM_NAME                "MSWIN4.1"  // size should be 8
#define STR_FAT12               "FAT12   "  // size should be 8
#define STR_FAT16               "FAT16   "  // size should be 8
#define STR_FAT32               "FAT32   "  // size should be 8

#define FAT12                   0x01        // FAT12
#define FAT16_DOS3              0x04        // FAT16 < 32M
#define FAT16_DOS4              0x06        // FAT16
#define FAT32_CHS               0x0B        // Win95 FAT32
#define FAT32                   0x0C        // Win95 FAT32 (LBA)
#define FAT16                   0x0E        // Win95 FAT16 (LBA). 0x0e = FAT16(LBA), 0x1E = Hidden FAT16(LBA) 

#define FAT12_THRESHOLD         4087        // 4085 + clu 0 + clu 1
#define FAT16_DOS3_THRESHOLD    16387       // 16385 + 2 
#define FAT16_THRESHOLD         65527       // 65525 + 2
#define FAT32_THRESHOLD         268435447   // 268435445 + 2

#define CLUSTER_16(x)           ((UINT16)(x))
#define CLUSTER_32(x)           ((UINT32)(x))

#define START_SECTOR(x)  \
        (((x)-2)*p_fs->sectors_per_clu+p_fs->data_start_sector)
#define START_CLUSTER(x) \
        ((GET32_ALIGNED((x)->start_clu_hi)<<16)|GET16_ALIGNED((x)->start_clu))

#define CLUSTER_SIZE_4KB        4096
#define CLUSTER_SIZE_8KB        8192
#define CLUSTER_SIZE_16KB       16384
#define CLUSTER_SIZE_32KB       32768 

#if 0     
#define MAX_CLUSTERS_VOL_0      (BLOCKSIZE*NO_OF_BLOCKS_0/CLUSTER_SIZE_4KB)
#if (MAX_DRIVE >= 2)
#define MAX_CLUSTERS_VOL_1      (BLOCKSIZE*NO_OF_BLOCKS_1/CLUSTER_SIZE_4KB)
#endif
#if (MAX_DRIVE >= 3)
#define MAX_CLUSTERS_VOL_2      (BLOCKSIZE*NO_OF_BLOCKS_2/CLUSTER_SIZE_4KB)
#endif
#if (MAX_DRIVE >= 4)
#define MAX_CLUSTERS_VOL_3      (BLOCKSIZE*NO_OF_BLOCKS_3/CLUSTER_SIZE_4KB)
#endif

#else
#define MAX_CLUSTERS_VOL_0      (1024*1024) //(512*1024)
#if (MAX_DRIVE >= 2)
#define MAX_CLUSTERS_VOL_1      (1024*1024) //(512*1024)
#endif
#if (MAX_DRIVE >= 3)
#define MAX_CLUSTERS_VOL_2      (512*1024)
#endif
#if (MAX_DRIVE >= 4)
#define MAX_CLUSTERS_VOL_3      (512*1024)
#endif
#endif /* ZFLASH_DVS */

#define GET16(p_src) \
        ( ((UINT16)(p_src)[0]) | ((UINT16)(p_src)[1] << 8) )
#define GET24(p_src) \
        ( ((UINT32)(p_src)[0]) | ((UINT32)(p_src)[1] << 8) | ((UINT32)(p_src)[2] << 16) )
#define GET32(p_src) \
        ( ((UINT32)(p_src)[0]) | ((UINT32)(p_src[1]) << 8) | ((UINT32)(p_src)[2] << 16) | \
          ((UINT32)(p_src)[3] << 24) )

#define SET16(p_dst,src) \
        do { (p_dst)[0]=(UINT8)(src); (p_dst)[1]=(UINT8)((src) >> 8); } while (0)
#define SET24(p_dst,src) \
        do { (p_dst)[0]=(UINT8)(src); (p_dst)[1]=(UINT8)((src) >> 8); \
             (p_dst)[2]=(UINT8)((src) >> 16); } while (0)
#define SET32(p_dst,src) \
        do { (p_dst)[0]=(UINT8)(src); (p_dst)[1]=(UINT8)((src) >> 8); \
             (p_dst)[2]=(UINT8)((src) >> 16); (p_dst)[3]=(UINT8)((src) >> 24); } while (0)

#ifdef TARGET_LITTLE_ENDIAN
#define GET16_ALIGNED(p_src)        (*((UINT16 *)(p_src)))
#define GET32_ALIGNED(p_src)        (*((UINT32 *)(p_src)))
#define SET16_ALIGNED(p_dst,src)    do { *((UINT16 *)(p_dst)) = (UINT16)(src); } while (0)
#define SET32_ALIGNED(p_dst,src)    do { *((UINT32 *)(p_dst)) = (UINT32)(src); } while (0)
#else
#define GET16_ALIGNED(p_src)        GET16(p_src)
#define GET32_ALIGNED(p_src)        GET32(p_src)
#define SET16_ALIGNED(p_dst,src)    SET16(p_dst, src)
#define SET32_ALIGNED(p_dst,src)    SET32(p_dst, src)
#endif

/* DMSG() macro - Debugging */

#ifdef DEBUG_FFS
#define PRINT_FAT()                 print_fat(0)
#define PR_DENTRY_T(X)              print_dir_entry(X)
#define PRINT_TAB()                 print_tab()
#define PRINT_FAT_CACHE(X)          print_fat_cache(X)

#else // DEBUG_FFS
#define PRINT_FAT()
#define PR_DENTRY_T(X)
#define PRINT_TAB()
#define PRINT_FAT_CACHE(X)
#endif

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

typedef struct {
    UINT8       name[DOS_NAME_LENGTH];
    UINT8       attr;
    UINT8       sysid;
    UINT8       create_time[3];
    UINT8       create_date[2];             // aligned
    UINT8       access_date[2];             // aligned
    UINT8       start_clu_hi[2];            // aligned
    UINT8       modify_time[2];             // aligned
    UINT8       modify_date[2];             // aligned
    UINT8       start_clu[2];               // aligned
    UINT8       size[4];                    // aligned
} DENTRY_T;             // MS-DOS FAT Dir Entry Definition      // 32 bytes

typedef struct {
    UINT8       entry_offset;
    UINT8       unicode_0_4[10];
    UINT8       attr;
    UINT8       sysid;
    UINT8       checksum;
    UINT8       unicode_5_10[12];
    UINT8       start_clu[2];               // aligned
    UINT8       unicode_11_12[4];
} EXTENTRY_T;           // MS-DOS FAT Extended Dir Entry Definition for long file name  // 32 bytes

typedef struct {
    UINT8       used;
    INT32       drv;
    UINT8       attr;
    UINT32      size;
    UINT32      dir;
    INT32       entry;
    UINT32      start_clu;
    INT32       modified;
    INT32       opencnt;
    INT32       hint_last_off;
    UINT32      hint_last_clu;
} FILE_HDR;

typedef struct {
    UINT8       used;
    UINT8       type;
    UINT32      mode;
    UINT32      rwoffset;
    FILE_HDR    *fh;
} OFILE_T;

/* master boot record */
typedef struct {
    UINT8       def_boot;
    UINT8       bgn_head;
    UINT8       bgn_sector;
    UINT8       bgn_cylinder;
    UINT8       sys_type;
    UINT8       end_head;
    UINT8       end_sector;
    UINT8       end_cylinder;
    UINT8       start_sector[4];
    UINT8       num_sectors[4];
} PART_ENTRY_T;

typedef struct {
    UINT8       boot_code[446];
    UINT8       partition[64];
    UINT8       signature[2];
} MBR_SECTOR_T;

/* partition boot record */
typedef struct {
    UINT8       sector_size[2];
    UINT8       sectors_per_clu;
    UINT8       num_reserved[2];            // aligned
    UINT8       num_fats;
    UINT8       num_root_entries[2];
    UINT8       num_sectors[2];
    UINT8       media_type;
    UINT8       num_fat_sectors[2];         // aligned
    UINT8       sectors_in_track[2];        // aligned
    UINT8       num_heads[2];               // aligned
    UINT8       num_hidden_sectors[4];      // aligned
    UINT8       num_huge_sectors[4];        // aligned

    UINT8       num_fat32_sectors[4];       // aligned
    UINT8       ext_flags[2];               // aligned
    UINT8       version[2];                 // aligned
    UINT8       root_cluster[4];            // aligned
    UINT8       fsinfo_sector[2];           // aligned
    UINT8       backup_sector[2];           // aligned
    UINT8       reserved[12];
} BPB_T;

typedef struct {
    UINT8       phy_drv_no;
    UINT8       reserved;
    UINT8       ext_signature;
    UINT8       vol_serial[4];
    UINT8       vol_label[11];
    UINT8       vol_type[8];
} EXTBPB_T;

typedef struct {
    UINT8   jmp_boot[3];
    UINT8   oem_name[8];
    UINT8   bpb[25];
    union {
        struct {
            UINT8   ext_bpb[26];
            UINT8   boot_code[446];
        } fat16;
        struct {
            UINT8   bpb[28];
            UINT8   ext_bpb[26];
            UINT8   boot_code[418];
        } fat32;
    } u;
    UINT8   boot_code[2];
    UINT8   signature[2];
} PBR_SECTOR_T;

typedef struct {
    UINT8       signature1[4];              // aligned
    UINT8       reserved1[480];
    UINT8       signature2[4];              // aligned
    UINT8       free_cluster[4];            // aligned
    UINT8       next_cluster[4];            // aligned
    UINT8       reserved2[14];
    UINT8       signature3[2];
} FSI_SECTOR_T;

typedef struct {
    INT32       dev;
    INT32       vol;
    UINT32      vol_type;

    UINT32      sectors_per_clu;
    UINT32      sectors_per_blk;
    UINT32      sectors_per_page;
    UINT32      cluster_size;
    UINT32      cluster_size_bits;
    UINT32      num_clusters;

#ifdef ANTI_FRAG_CLU_ALLOC
    UINT32      clu_per_block;
    INT32       clean_blk_exist;
#endif

    UINT32      PBR_sector;             /* PBR sector */
    UINT32      FAT1_start_sector;      /* FAT start sector */
    UINT32      FAT2_start_sector;
    UINT32      root_start_sector;      /* root start sector */
    UINT32      data_start_sector;      /* data start sector */
    UINT32      num_FAT_sectors;        /* number of sectors per FAT */
    UINT32      num_root_sectors;       /* number of sectors in root_dir */
    UINT32      root_dir;               /* root dir cluster */

    UINT32      cur_dir;
    UINT32      cur_path_len;
    UINT32      dentries_in_root;
    UINT32      dentries_per_clu;
    UINT32      clu_srch_ptr_for_file;
    UINT32      clu_srch_ptr_for_dir;
    UINT32      used_clusters;

    struct FAT_CACHE_T *fat_hint_bp;
    struct DIR_CACHE_T *dir_hint_bp;
    
    LOG_T       log;
    UINT32      log_start_sector;
    UINT32      log_num_sectors;
    UINT32      log_sector_off;
    
    UINT32      dev_ejected;
    
#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
    INT32       hint_dir;
    INT32       hint_ent;
    UINT32      hint_d_start_clu;
    UINT32      hint_d_last_clu;
    INT32       hint_d_last_off;
#endif
} FS_STRUCT_T;

extern FS_STRUCT_T      fs_struct[];
extern FS_STRUCT_T      *p_fs;
extern INT32            fs_curdrv;

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
extern INT32            fs_curdev;
extern INT32            fs_curvol;
#endif

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)

#include <linux/mm.h>
#include <linux/types.h>
//#include <linux/iobuf.h>
#include <linux/rfs_fs_i.h>

/* Vol Functions */
int rfs_set_partition(int dev, int num_vol, int *vol_off, int *vol_size);
int rfs_get_partition(int dev, int *num_vol, int *vol_off, int *vol_size);
int rfs_get_dev_size(int dev, int *dev_size);
int rfs_format_vol(int dev, int vol, int fat_type, int clu_size);
int rfs_mount_vol(int dev, int vol);
int rfs_umount_vol(int dev, int vol);
int rfs_check_vol(int dev, int vol);
int rfs_read_super(int dev, int vol, rfs_sb_info_t *info);
int rfs_set_drv(int dev, int vol);
int rfs_get_drv(int *curdev, int *curvol);
int rfs_sync_vol(int dev, int vol);
int rfs_write_inode(rfs_file_id_t *fid, time_t mtime);

/* DIR Functions */
int rfs_mkdir(rfs_file_id_t *fid, int mode);
int rfs_readdir(rfs_file_id_t *fid, rfs_dir_entry_t *dir_entry, loff_t *ppos);
int rfs_rmdir(rfs_file_id_t *fid);

/* File Functions */
int rfs_create(rfs_file_id_t *fid, int mode);
ssize_t rfs_file_read(rfs_file_id_t *fid, char *buf, size_t cnt, loff_t *ppos);
ssize_t rfs_file_write(rfs_file_id_t *fid, char *buf, size_t cnt, loff_t *ppos);
#if 0
ssize_t rfs_file_read_direct(rfs_file_id_t *fid, struct kiobuf *iobuf, loff_t *ppos);
ssize_t rfs_file_write_direct(rfs_file_id_t *fid, struct kiobuf *iobuf, loff_t *ppos);
#endif
int rfs_unlink(rfs_file_id_t *fid);
int rfs_rename(rfs_file_id_t *oldfid, rfs_file_id_t *newfid);
int rfs_truncate(rfs_file_id_t *fid, loff_t *size);
int rfs_lookup(rfs_file_id_t *fid, int *is_dosname);
int rfs_read_inode(rfs_file_id_t *fid, rfs_inode_t *info);
int rfs_read_inode_with_name(rfs_file_id_t *fid, rfs_inode_t *info, char *path);
int rfs_get_attr(rfs_file_id_t *fid, char *attr);
int rfs_set_attr(rfs_file_id_t *fid, char attr);
int rfs_lock_all(rfs_file_id_t *fid, char *pattern, int flag);
int rfs_unlock_all(rfs_file_id_t *fid, char *pattern, int flag);
int rfs_unlink_all(rfs_file_id_t *fid, char *pattern, int flag);
int rfs_get_size(rfs_file_id_t *fd);
#ifdef CONFIG_RFS_NLS
int rfs_set_codepage (int codepage);
#endif


#else /* !defined(ZFLASH_LINUX) || !defined(ZFLASH_KERNEL) */

/* Vol Functions */
INT32 rfsSetPartition(INT32 dev, INT32 num_vol, INT32 *vol_size);
//INT32 rfsGetPartition(INT32 dev, INT32 vol, PART_INFO *info);
INT32 rfsFormatVol(INT32 dev, INT32 vol, INT32 fat_type, INT32 clu_size);
INT32 rfsMountVol(INT32 dev, INT32 vol);
INT32 rfsUmountVol(INT32 dev, INT32 vol);
INT32 rfsCheckVol(INT32 dev, INT32 vol);
INT32 rfsGetVolInfo(INT32 dev, INT32 vol, VOL_INFO *info);
INT32 rfsSetDrv(INT32 dev, INT32 vol);
INT32 rfsGetDrv(INT32 *curdev, INT32 *curvol);
INT32 rfsSyncVol(void);

/* DIR Functions */
INT32 rfsCreateDir(INT8 *path, UINT32 mode);
INT32 rfsOpenDir(UINT32 *did, INT8 *name);
INT32 rfsCloseDir(UINT32 did);
INT32 rfsReadDir(UINT32 did, RFS_DIR_ENTRY_T *dir_ent);
INT32 rfsRewindDir(UINT32 did);
INT32 rfsRemoveDir(INT8 *path);
INT32 rfsChangeDir(INT8 *path);
INT32 rfsPwd(INT8 *pathbuf);

#ifdef ZFLASH_DVS
INT32 rfsRemoveAllFiles(INT8 *path, INT8 *wildcard, BOOL flag);
#endif

/* File Functions */
INT32 rfsCreateFile(UINT32 *fid, INT8 *name, UINT32 mode);
INT32 rfsOpenFile(UINT32 *fid, INT8 *name, UINT32 mode);
INT32 rfsCloseFile(UINT32 fid);
INT32 rfsReadFile(UINT32 fid, void *buffer, UINT32 bcount, UINT32 *tcount);
INT32 rfsWriteFile(UINT32 fid, void *buffer, UINT32 bcount, UINT32 *tcount);
INT32 rfsRemoveFile(INT8 *name);
INT32 rfsMoveFile(INT8 *oldname, INT8 *newname);
INT32 rfsSeekFile(UINT32 fid, UINT32 position, INT32 offset, UINT32 *new_ptr);
INT32 rfsTruncateFile(UINT32 fid, UINT32 newsize);
INT32 rfsReadStat(INT8 *name, RFS_DIR_ENTRY_T *info);
INT32 rfsGetAttr(INT8 *name, UINT8 *attr);
INT32 rfsSetAttr(INT8 *name, UINT8 attr);

#endif /* defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL) */

/*----------------------------------------------------------------------*/
/*  External Function Declarations (NOT TO UPPER LAYER)                 */
/*----------------------------------------------------------------------*/

/* log management functions */
INT32  make_log_file(void);

/* fs management functions */
void   fs_set_drv(INT32 new_drv);
INT32  fs_set_dev(INT32 new_dev, INT32 new_vol);
void   sync_vol_internal(void);

/* fs checking functions */
INT32  fs_check_internal(void);
INT32  check_dir(UINT32 dir);
INT32  check_dup_files(UINT32 dir);
INT32  check_file(UINT32 dir, INT32 offset);
INT32  check_clusters(UINT32 start_clu, INT32 size);
BOOL   is_FAT_range_OK(UINT32 clu);

#ifdef CRASH_TEST
void   fs_crash(void);
#endif

/* cluster management functions */
void   clear_cluster(UINT32 cluster);
INT32  alloc_cluster(UINT32 *new_clu, INT32 num_clu, INT32 alloc_type, UINT32 hint_clu);

#ifdef CONFIG_FREE_CLUSTER_MANAGEMENT
extern INT32 init_free_extent_list(INT32 drv);
extern INT32 set_free_extent_list(INT32 drv);
extern INT32 free_cluster(off_t clu_num);
extern INT32 sync_free_clusters(void);
extern INT32 alloc_cluster_in_extent(UINT32 *clu_array, size_t clu_count);
extern INT32 remove_free_extent_list(INT32 drv);
extern UINT32 check_space(void);
extern void rfs_alloc_exit(void);
#endif

void   add_new_cluster(UINT32 chain, UINT32 new_clu);
void   free_chain(UINT32 chain);
void   free_chain_no_log(UINT32 chain);
void   free_last_cluster(UINT32 chain);
UINT32 find_last_cluster(UINT32 chain);
INT32  count_num_clusters(UINT32 dir);
INT32  count_used_clusters(void);

#ifdef ANTI_FRAG_CLU_ALLOC
INT32  check_clu_info_bmap(void);
INT32  is_dup_alloc(UINT32 clu, UINT32 *clu_list, INT32 check_size);
void   build_clu_info_bmap(void);
void   set_clu_info_bmap(UINT32 clu);
void   clear_clu_info_bmap(UINT32 clu);
#endif

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
void   undo_pre_clu_alloc(UINT32 dir, INT32 entry);
#else
void   undo_pre_clu_alloc(OFILE_T * of, UINT32 start_clu);
#endif

/* dir entry management functions */
UINT8  ENTRY_TYPE(DENTRY_T *ep);
void   SET_ENTRY_TYPE(DENTRY_T *ep, UINT8 type);
UINT32 ENTRY_TIME(UINT16 time, UINT16 date);
void   SET_ENTRY_TIME(DENTRY_T *ep, time_t curtime);
void   init_dir_entry(DENTRY_T *ep, UINT8 type, UINT8 sysid, UINT32 first_clu, INT8 *name, UINT8 *dos_name);
void   init_ext_dir_entry(EXTENTRY_T *ep, UINT8 type, INT32 entry, UINT16 *unicode_name, UINT8 checksum);
DENTRY_T *get_entry_with_sector(UINT32 sector, INT32 offset);
DENTRY_T *get_entry_in_dir(UINT32 dir, INT32 entry, UINT32 *sector);
DENTRY_T *get_short_name_entry_in_dir(UINT32 dir, INT32 count, INT32 *entry, UINT32 *sector);
void   delete_entries(UINT32 dir, INT32 entry, INT32 num_entries, INT32 direction);
INT32  search_deleted_or_unused_entries(UINT32 dir, INT32 num_entries);
INT32  find_empty_entries(UINT32 dir, INT32 num_entries);
INT32  find_entry_with_name(UINT32 dir, INT8 *name, UINT32 type, INT32 hint_ent);
INT32  find_entry_in_par_dir(UINT32 pdir, UINT32 dir);
void   find_location(UINT32 dir, INT32 entry, UINT32 *sector, INT32 *offset);
BOOL   is_dir_empty(UINT32 dir);
INT32  count_short_name_entries(UINT32 dir);
INT32  count_ext_entries(UINT32 dir, INT32 entry, UINT8 checksum);

/* name conversion functions */
INT32  get_dos_name_and_unicode_name(UINT32 dir, INT8 *name, INT32 *entries, UINT8 *sysid, UINT8 *dos_name, UINT16 *unicode_name);
void   get_unicode_name_from_extend_entries(UINT32 dir, INT32 entry, UINT16 *unicode_name);
void   extract_unicode_name_from_ext_entries(EXTENTRY_T *extp, UINT16 *unicode, INT32 is_last);
void   attach_count_to_dos_name(UINT8 *dos_name, INT32 count);
INT32  calc_num_dentries_for_name(UINT16 *unicode_name);
//INT32  calc_num_dentries_for_name(INT8 *c_string_name);
UINT8  calc_checksum_with_dos_name(UINT8 *dos_name);
#ifdef CONFIG_RFS_FAT32
int add_numeric_tail(unsigned int dir, unsigned char *dos_name);
#endif

BOOL   PATTERN_MATCH(INT8 *pattern, INT8 *name);
BOOL   DCF_CHECK(UINT8 *dosname, BOOL flag);

/* name resolution functions */
INT32  path_resolve(INT8 *path, UINT32 *dir, INT8 **lastname);
INT32  resolve_name(INT8 *name, INT8 **arg);
INT32  reverse_strcat(INT8 *to, INT8 *from);

/* ofile manipulating functions */
void   init_ofile(void);
UINT32 alloc_ofile(UINT32 dir, INT32 entry);
void   free_ofile(INT32 i);
void   free_all_ofile(INT32 drv);
INT32  count_ofile(void);

/* file_hdr manipulating functions */
FILE_HDR *alloc_hdr(void);
FILE_HDR *find_hdr(UINT32 dir, INT32 entry);
void   free_hdr(FILE_HDR *fh);

/* file operation functions */
INT32  open_f_internal(UINT32 *fid, UINT32 dir, INT32 entry, UINT32 mode);
INT32  change_name(UINT32 dir, INT32 old_entry, INT8 *new_name, INT32 *fid_entry);
INT32  move_to_dir(UINT32 old_dir, INT32 old_entry, UINT32 new_dir, INT8 *new_name, INT32 *fid_entry);
UINT32 remove_f_internal(UINT32 dir, INT32 entry, INT32 flag_clu_free);

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
INT32  truncate_f_internal(UINT32 dir, INT32 entry, loff_t *new_size, UINT32 *clu_to_free);
#else
INT32  truncate_f_internal(UINT32 fid, UINT32 new_size, UINT32 *clu_to_free);
#endif

/* sector read/write functions */
void   Sector_Read(INT32 drv, UINT32 sector, UINT8 *buf);
void   Sector_Write(INT32 drv, UINT32 sector, UINT8 *buf);
void   Multiple_Sectors_Read(INT32 drv, UINT32 sector, UINT8 *buf, INT32 num_sectors);
void   Multiple_Sectors_Write(INT32 drv, UINT32 sector, UINT8 *buf, INT32 num_sectors);
void   Sync(INT32 drv);

/* debugging functions */
#ifdef DEBUG_FFS
INT32  print_tab(void);
void   print_fat(INT8 force_flag);
void   print_fat_cache(UINT8 *fat);
void   print_dir_entry(DENTRY_T *ep);
#endif

#endif /* _RFS_H */

/* end of rfs.h */
