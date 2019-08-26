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
/*  This file implements the RFS FAT file system core (logging).        */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : rfs_log.h                                                 */
/*  PURPOSE : Header file for Linux Robust FAT File System Core         */
/*            (especially for FAT file system logging)                  */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - Log file manipulating functions.                                  */
/*  - Log file is used for the error recovery.                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*  - 01/10/2003 [Joosun Hahn]  : added FAT32 support                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _RFS_LOG_H
#define _RFS_LOG_H

#include "fm_global.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

/* if you don't want to install the error recovery mechanism 
   based on the log file, then comment out the following definition */

// #define USE_FILE_LOG            

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

/* log types */

#define LOG_NULL                0
#define LOG_CREATE_FILE         1
#define LOG_CREATE_DIR          2
#define LOG_MOVE                3
#define LOG_REMOVE              4
#define LOG_WRITE_FILE          5
#define LOG_RENAME              6
#define LOG_CLUSTER_FREE        7
#define LOG_TRUNCATE            8
#define LOG_DIR_EXPAND          9

#define LOG_NOTCHANGED          0
#define LOG_CHANGED             1

/* definition of the endian used in the log file */

#define LOG_LITTLE_ENDIAN       (0x55555555)
#define LOG_BIG_ENDIAN          (~LOG_LITTLE_ENDIAN)

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

typedef struct {
    UINT32      clu, data;
} FAT_LOG_T;

typedef struct {
    UINT64      seq;
    UINT32      endian;

    INT32       type;
 
    INT32       num_fat_log;
    UINT8       data[SECTOR_SIZE-12-8];
} LOG_T;        // General Log Type : 512 bytes

#define LOG_CREATE_NAME_SIZE    (SECTOR_SIZE - 8*1 - 4*7 - 8*1)

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      dir;
    INT32       entry;
    INT32       num_entries;
    INT32       mode;
    FAT_LOG_T   fat_log[1];
    INT8        name[LOG_CREATE_NAME_SIZE];
} LOG_CREATE_FILE_T;

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      dir;
    INT32       entry;
    INT32       num_entries;
    INT32       mode;
    FAT_LOG_T   fat_log[1];
    INT8        name[LOG_CREATE_NAME_SIZE];
} LOG_CREATE_DIR_T;

#define LOG_MOVE_NAME_SIZE      (SECTOR_SIZE - 4*9 - 8*1)

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      olddir;
    INT32       oldentry;
    INT32       num_old_entries;
    UINT32      newdir;
    INT32       newentry;
    INT32       num_new_entries;
    INT8        name[LOG_MOVE_NAME_SIZE];
} LOG_MOVE_T;

#define LOG_RENAME_NAME_SIZE    (SECTOR_SIZE - 4*8 - 8*1)

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      dir;
    INT32       oldentry;
    INT32       num_old_entries;
    INT32       newentry;
    INT32       num_new_entries;
    INT8        name[LOG_RENAME_NAME_SIZE];
} LOG_RENAME_T; // Rename file name just in its original dir entry;

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      dir;
    INT32       entry;
    INT32       num_entries;
    UINT32      clu_to_free;
    UINT8       dummy[SECTOR_SIZE -4*7 - 8*1];
} LOG_REMOVE_T;

#define LOG_WRITE_FILE_FAT_LOG_SIZE     ((SECTOR_SIZE - 4*8 - 8*1) / sizeof(UINT32))

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      dir;
    INT32       entry;
    UINT32      size;
    UINT32      start_clu;
    UINT32      attr;
    UINT32      fat_log[LOG_WRITE_FILE_FAT_LOG_SIZE];
} LOG_WRITE_FILE_T;

#define LOG_CLUSTER_FREE_FAT_ENTRY_NUM  ((SECTOR_SIZE - 4*4 - 8*1) / sizeof(FAT_LOG_T))

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      clu_to_free;
    FAT_LOG_T   fat_log[LOG_CLUSTER_FREE_FAT_ENTRY_NUM];
} LOG_CLUSTER_FREE_T;

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      dir;
    INT32       entry;
    UINT32      size;
    UINT32      clu_to_free;
    FAT_LOG_T   fat_log[1];
    UINT8       dummy[SECTOR_SIZE - 4*7 - 1*8 - 8*1];
} LOG_TRUNCATE_T;

typedef struct {
    UINT64      seq;
    UINT32      endian;
    INT32       type;
    INT32       num_fat_log;
    UINT32      dir;
    FAT_LOG_T   fat_log[2];
    UINT8       dummy[SECTOR_SIZE - 4*4 - 2*8 - 8*1];
} LOG_DIR_EXPAND_T;

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

#ifdef USE_FILE_LOG

void  Log_Read(void);
#ifdef ZFLASH_WINDOWS
void  Log_Log(void);
#endif
void  Log_Write(void);
void  Log_Replay(void);
void  Log_Endian_Change(LOG_T *log_p);

void  endian_change_8(char a[]);
void  endian_change_4(char a[]);

void  Log_Begin(INT32 type, UINT32 start_cluster);
INT32 Log_DirExpand_Prepare(INT32 type, UINT32 dir);
void  Log_DirExpand_Replay(LOG_DIR_EXPAND_T *log_p);
INT32 Log_Truncate_Prepare(INT32 type, UINT32 dir, INT32 entry, UINT32 size, UINT32 clu_to_free);
void  Log_Truncate_Replay(LOG_TRUNCATE_T *log_p);
INT32 Log_ClusterFree_Prepare(INT32 type, UINT32 clu_to_free);
void  Log_ClusterFree_Replay(LOG_CLUSTER_FREE_T *log_p);
INT32 Log_Remove_Prepare(INT32 type, UINT32 dir, INT32 entry, INT32 num_entries, UINT32 clu_to_free);
void  Log_Remove_Replay(LOG_REMOVE_T *log_p);
INT32 Log_Rename_Prepare(INT32 type, UINT32 dir, INT32 oldentry, INT32 num_old_entries, INT32 newentry, INT32 num_new_entries, INT8 *newname);
void  Log_Rename_Replay(LOG_RENAME_T *log_p);
INT32 Log_Move_Prepare(INT32 type, UINT32 olddir, INT32 oldentry, INT32 num_old_entries, UINT32 newdir, INT32 newentry, INT32 num_new_entries, INT8 *name);
void  Log_Move_Replay(LOG_MOVE_T *log_p);
INT32 Log_CreateDir_Prepare(INT32 type, UINT32 dir, INT32 entry, INT32 num_entries, INT32 mode, INT8 *name);
void  Log_CreateDir_Replay(LOG_CREATE_DIR_T *log_p);
INT32 Log_CreateFile_Prepare(INT32 type, UINT32 dir, INT32 entry, INT32 num_entries, INT32 mode, INT8 *name);
void  Log_CreateFile_Replay(LOG_CREATE_FILE_T *log_p);
INT32 Log_Write_Prepare(INT32 type, UINT32 dir, INT32 entry, UINT32 size, UINT32 start_cluster, UINT32 attr);
void  Log_Write_Replay(LOG_WRITE_FILE_T *log_p);
void  Log_Write_FAT_Chain(UINT32 clu);
void  Log_FAT_Append(UINT32 clu, UINT32 data);
INT32 Log_Check_Align(void);
/* EOS RFS */
UINT32 Log_Get_Op(void);

#else

#define Log_Read()
#ifdef ZFLASH_WINDOWS
#define Log_Log()
#endif
#define Log_Write()
#define Log_Replay()
#define Log_Endian_Change(log_p)

#define endian_change_8(a)
#define endian_change_4(a)

#define Log_Begin(type,start_cluster)
#define Log_DirExpand_Prepare(type,dir)                                    LOG_NOTCHANGED
#define Log_DirExpand_Replay(log_p)
#define Log_Truncate_Prepare(type,dir,entry,size,clu_to_free)              LOG_NOTCHANGED
#define Log_Truncate_Replay(log_p)
#define Log_ClusterFree_Prepare(type,clu_to_free)                          LOG_NOTCHANGED
#define Log_ClusterFree_Replay(log_p)
#define Log_Remove_Prepare(type,dir,entry,num_entries,clu_to_free)         LOG_NOTCHANGED
#define Log_Remove_Replay(log_p)
#define Log_Rename_Prepare(type,dir,olde,num_olde,newe,num_newe,name)      LOG_NOTCHANGED
#define Log_Rename_Replay(log_p)
#define Log_Move_Prepare(type,oldd,olde,num_olde,newd,newe,num_newe,name)  LOG_NOTCHANGED
#define Log_Move_Replay(log_p)
#define Log_CreateDir_Prepare(type,dir,entry,num_entries,mode,name)        LOG_NOTCHANGED
#define Log_CreateDir_Replay(log_p)
#define Log_CreateFile_Prepare(type,dir,entry,num_entries,more,name)       LOG_NOTCHANGED
#define Log_CreateFile_Replay(log_p)
#define Log_Write_Prepare(type,dir,entry,size,start_cluster,attr)          LOG_NOTCHANGED
#define Log_Write_Replay(log_p)
#define Log_Write_FAT_Chain(clu)
#define Log_FAT_Append(clu,data)
#define Log_Check_Align()                                                  FFS_SUCCESS
/* EOS RFS */
#define Log_Get_Op() LOG_NULL

#endif /* USE_FILE_LOG */

#endif /* _RFS_LOG_H */

/* end of rfs_log.h */
