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
/*  This file declares the RFS FAT file system APIs.                    */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Dong-Hee Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : rfs_api.h                                                 */
/*  PURPOSE : Header file for Linux Robust FAT File System APIs         */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - High level APIs are provided.                                     */
/*    (user or programmer friendly interface rather than system)        */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Dong-Hee Lee] : first writing                         */
/*  - 01/10/2003 [Joosun Hahn]  : style modified & restructured         */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _FFS_API_H
#define _FFS_API_H

#include "fm_global.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

#define RFS_LOG_FILE_NAME       "$RFS_LOG.LO$"

#if 0
#define MAX_NAME_LENGTH         256     // max len of file name including NULL
#define MAX_PATH_LENGTH         260     // max len of path name including NULL
#else /* 2007/03/14 - hcyun */ 
#  define MAX_NAME_LENGTH         (256*4)     // max len of file name including NULL
#  define MAX_PATH_LENGTH         (MAX_NAME_LENGTH + 4) 
#endif 

#define DOS_NAME_LENGTH         11      // MS-DOS FAT Compatible
#define DOS_PATH_LENGTH         80      // MS-DOS FAT Compatible

/* type of directory entry (file type) */
#define TYPE_UNUSED             ((UINT8) 0x00)
#define TYPE_DELETED            ((UINT8) 0x01)
#define TYPE_FILE               ((UINT8) 0x02)
#define TYPE_DIR                ((UINT8) 0x03)
#define TYPE_EXTEND             ((UINT8) 0x04)
#define TYPE_ALL                ((UINT8) 0x05)
#define TYPE_UNKNOWN            ((UINT8) 0x06)
#define TYPE_SYMLINK            ((UINT8) 0x42)  // TYPE_FILE | FM_SYMLINK
#define TYPE_SOCKET             ((UINT8) 0x82)  // TYPE_FILE | FM_SOCKET

/* file open and create mode */
#define FM_CREATE               0x01
#define FM_READ                 0x02
#define FM_WRITE                0x04
#define FM_OVERWRITE            0x08
#define FM_SYMLINK              0x40    // ATTR_SYMLINK
#define FM_SOCKET               0x80    // ATTR_SOCKET

#ifdef CRASH_TEST
/* definitions for test of crash condition of test specification */
#define CRASH_ON_OPEN           1
#define CRASH_AFTER_OPEN        2
#define CRASH_ON_READ           3
#define CRASH_AFTER_READ        4
#define CRASH_ON_WRITE          5
#define CRASH_AFTER_WRITE       6
#define CRASH_AFTER_SYNC        7
#define CRASH_ON_CLOSE          8
#define CRASH_AFTER_CLOSE       9
#define CRASH_AFTER_REMOVE      10
#endif

/* return values of FFS APIs */
#define FFS_SUCCESS             0
#define FFS_MEDIAERR            1
#define FFS_FORMATERR           2
#define FFS_MOUNTED             3
#define FFS_NOTMOUNTED          4
#define FFS_ALIGNMENTERR        5
#define FFS_INVALIDPATH         6
#define FFS_INVALIDFID          7
#define FFS_NOTFOUND            8
#define FFS_FILEEXIST           9
#define FFS_PERMISSIONERR       10
#define FFS_NOTOPENED           11
#define FFS_MAXOPEN             12
#define FFS_FULL                13
#define FFS_EOF                 14
#define FFS_DIRBUSY             15
#define FFS_MEMORYERR           16
#define FFS_ERROR               17      // generic error code

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

#define ATTR_NORMAL           ((UINT8) 0x00)
#define ATTR_READONLY         ((UINT8) 0x01)
#define ATTR_HIDDEN           ((UINT8) 0x02)
#define ATTR_SYSTEM           ((UINT8) 0x04)
#define ATTR_VOLUME           ((UINT8) 0x08)
#define ATTR_SUBDIR           ((UINT8) 0x10)
#define ATTR_ARCHIVE          ((UINT8) 0x20)
#define ATTR_EXTEND           ((UINT8) 0x0F)
#define ATTR_SYMLINK          ((UINT8) 0x40)
#define ATTR_SOCKET           ((UINT8) 0x80)

typedef struct {
    INT8        name[MAX_NAME_LENGTH];
    INT8        short_name[DOS_NAME_LENGTH+2];
    UINT8       attr;
    UINT8       sysid;
    UINT32      create_time;
    UINT16      create_date;
    UINT16      last_access_date;
    UINT16      last_modify_time;
    UINT16      last_modify_date;
    UINT16      start_cluster;
    UINT32      size;
} RFS_DIR_ENTRY_T;      // MS-DOS FAT Dir Entry Definition

typedef struct {
    UINT32      ClusterSize;
    UINT32      NumClusters;
    UINT32      FreeClusters;
    UINT32      UsedClusters;
    UINT32      NumOpened;
} VOL_INFO;

#ifdef ZFLASH_DVS
/*----------------------------------------------------------------------*/
/*  External Function Declarations (APIs for Samsung DVS Target)        */
/*----------------------------------------------------------------------*/

/* Vol Functions */
int ffsFormatVol(unsigned long DrvNo, unsigned char Mode);
int ffsMountVol(unsigned long DrvNo);
int ffsUmountVol(unsigned long DrvNo);
int ffsGetVolInfo(unsigned long DrvNo, VOL_INFO *Info);
int ffsSetDrv(unsigned long DrvNo);
int ffsGetDrv(unsigned long *CurDrv);

/* DIR Functions */
int ffsCreateDir(char *Path);
int ffsRemoveDir(char *Path);
int ffsChangeDir(char *Path);
int ffsPwd(char *Path);
int ffsReadDir(char *Path, RFS_DIR_ENTRY_T *dEntry);
int ffsRemoveAllFiles(char *Path, unsigned char DCF_RuleCheck);

/* File Functions */
int ffsCreateFile(char *Path, unsigned char Attr, unsigned long *fid);
int ffsOpenFile(char *Path, unsigned char Flag, unsigned long *fid);
int ffsCloseFile(unsigned long fid);
int ffsReadFile(unsigned long fid, void *Buffer, unsigned long bLen, unsigned long *rLen);
int ffsWriteFile(unsigned long fid, void *Buffer, unsigned long bLen, unsigned long *wLen);
int ffsCopyFile(char *SrcPath, char *DestPath);
int ffsRemoveFile(char *Path);
int ffsRenameFile(char *OldPath, char *NewPath);
int ffsSeekFile(unsigned long fid, unsigned long Offset, unsigned char Position, unsigned long *NewPtr);
int ffsGetFattr(char *Path, unsigned char *Attr);
int ffsSetFattr(char *Path, unsigned char Attr);

#else
/*----------------------------------------------------------------------*/
/*  External Function Declarations (General RFS APIs; cf. 'rfs.h')      */
/*----------------------------------------------------------------------*/

/* Vol Functions */
#define ffsSetPartition         rfsSetPartition
#define ffsGetPartition         rfsGetPartition
#define ffsFormatVol            rfsFormatVol
#define ffsMountVol             rfsMountVol
#define ffsUmountVol            rfsUmountVol
#define ffsCheckVol             rfsCheckVol
#define ffsGetVolInfo           rfsGetVolInfo
#define ffsSetDrv               rfsSetDrv
#define ffsGetDrv               rfsGetDrv
#define ffsSyncVol              rfsSyncVol

/* DIR Functions */
#define ffsCreateDir            rfsCreateDir
#define ffsOpenDir              rfsOpenDir
#define ffsCloseDir             rfsCloseDir
#define ffsReadDir              rfsReadDir
#define ffsRewindDir            rfsRewindDir
#define ffsRemoveDir            rfsRemoveDir
#define ffsChangeDir            rfsChangeDir
#define ffsPwd                  rfsPwd

/* File Functions */
#define ffsCreateFile           rfsCreateFile
#define ffsOpenFile             rfsOpenFile
#define ffsCloseFile            rfsCloseFile
#define ffsReadFile             rfsReadFile
#define ffsWriteFile            rfsWriteFile
#define ffsRemoveFile           rfsRemoveFile
#define ffsMoveFile             rfsMoveFile
#define ffsSeekFile             rfsSeekFile
#define ffsTruncateFile         rfsTruncateFile
#define ffsReadStat             rfsReadStat
#define ffsGetAttr              rfsGetAttr
#define ffsSetAttr              rfsSetAttr

#endif /* ZFLASH_DVS */

#endif /* _FFS_API_H */

/* end of ffs_api.h */
