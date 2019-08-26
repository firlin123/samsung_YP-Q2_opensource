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
/*  This file implements the RFS FAT file system core (caching).        */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : rfs_cache.h                                               */
/*  PURPOSE : Header file for Linux Robust FAT Flash File System Core   */
/*            (especially for FAT caching & buffer caching)             */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - FAT Read/Write and FAT Cache Functions                            */
/*  - Buffer Cache Manipulating Functions                               */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*  - 01/11/2003 [Joosun Hahn]  : added FAT32 support                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _RFS_CACHE_H
#define _RFS_CACHE_H

#include "fm_global.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

#define FAT_CACHE_SIZE              32  // num of cache blocks
#define DIR_CACHE_SIZE              32  // num of cache blocks
#define BUF_CACHE_SIZE              32  // num of cache blocks

#define FAT_CACHE_HASH_SIZE         32
#define DIR_CACHE_HASH_SIZE         32
#define BUF_CACHE_HASH_SIZE         32

#define LOCKBIT                     0x01
#define DIRTYBIT                    0x02
#define OVERWRITEBIT                0x04

#define DENTRY_CACHE_SIZE           4

#define DCACHE_FREE	                0x0000
#define DCACHE_USED	                0x0001

#define FAST_SEARCH1                0       // dentry caching for fast search
                                            // samsung version (hcyun)

#define FAST_SEARCH2                0       // dentry caching for fast search
                                            // zeen version

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

struct FAT_CACHE_T {        // FAT_CACHE
    struct FAT_CACHE_T   *next;
    struct FAT_CACHE_T   *prev;
    struct FAT_CACHE_T   *hash_next;
    struct FAT_CACHE_T   *hash_prev;
    INT32                drv;
    INT32                secno;
    INT32                dirty;
    UINT8                *fat_sector;
};

struct DIR_CACHE_T {
    struct DIR_CACHE_T   *next;
    struct DIR_CACHE_T   *prev;
    struct DIR_CACHE_T   *hash_next;
    struct DIR_CACHE_T   *hash_prev;
    INT32                drv;
    INT32                secno;
    INT32                flag;
    UINT8                *dir_sector;
};

struct BUF_CACHE_T {
    struct BUF_CACHE_T   *next;
    struct BUF_CACHE_T   *prev;
    struct BUF_CACHE_T   *hash_next;
    struct BUF_CACHE_T   *hash_prev;
    INT32                drv;
    INT32                secno;
    INT32                flag;
    UINT8                *buf_sector;
};

struct DIRTY_LIST_T {
    INT32                start_sector;
    INT32                num_sectors;
    UINT8                *buf;
};

#if FAST_SEARCH1
struct DENTRY_CACHE_T {
	UINT8       type; // hcyun. TYPE_FILE, TYPE_SYMLINK, ... 
	UINT32      pdir;
	INT8        filename[(256*2)]; // same as RFS_FNAME_LENGTH. defined in the rfs_fs_i.h 
	UINT32      dentry;
	INT16       status;
	UINT16      age;
};
#endif

#if FAST_SEARCH2
struct DENTRY_CACHE_T {
	BOOL        valid;
	INT32       drv;            // key used to look up the cache
	UINT32      dir;            // key used to look up the cache
	INT32       entry;          // key used to look up the cache
	INT8        name[26];       // key used to verify the matched cache entry
	UINT32      secno;          // cached value
	UINT8       *buf;           // cached value
};
#endif

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

void   FAT_cache_init(void);
void   FAT_write(UINT32 loc, UINT32 content);
UINT32 FAT_read(UINT32 loc);
void   FAT_delete_all(INT32 drv);
void   FAT_sync(void);

void   dir_cache_init(void);
UINT8 *dir_getblk(INT32 secno, INT32 read);
void   dir_getblk_bypass_cache(INT32 secno, UINT8 *buf);
UINT8 *dir_findblk(INT32 secno);
void   dir_lock(INT32 secno);
void   dir_unlock(INT32 secno);
void   dir_delete(INT32 secno, INT32 write);
void   dir_delete_all(INT32 drvno);
void   dir_modified(INT32 secno);
void   dir_sync(void);

void   buf_cache_init(void);
UINT8 *buf_getblk(INT32 secno, INT32 read);
void   buf_getblk_bypass_cache(INT32 secno, UINT8 *buf);
UINT8 *buf_findblk(INT32 secno);
void   buf_lock(INT32 secno);
void   buf_unlock(INT32 secno);
void   buf_delete(INT32 secno, INT32 write);
void   buf_delete_all(INT32 drvno);
void   buf_modified(INT32 secno);
void   buf_sync(void);

#if FAST_SEARCH1
void   dentry_cache_init(void);
void   dentry_cache_fetch(UINT32 pdir, INT8 *filename, UINT8 type, UINT32 dentry);
INT32  dentry_cache_lookup(UINT32 pdir, INT8 *filename, UINT8 type);
void   dentry_cache_invalidate(UINT32 pdir, UINT32 dentry);
#endif

#if FAST_SEARCH2
void   dentry_cache_init(void);
void   dentry_cache_fetch(UINT32 dir, INT32 entry, UINT32 secno, UINT8 *buf);
struct DENTRY_CACHE_T *dentry_cache_lookup(UINT32 dir, INT32 entry);
void   dentry_cache_invalidate(INT32 drv, UINT32 secno, UINT32 num_secs);
#endif

#endif /* _RFS_CACHE_H */

/* end of rfs_cache.h */
