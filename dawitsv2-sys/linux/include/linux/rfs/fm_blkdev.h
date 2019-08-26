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
/*  This file implements the flash block device driver.                 */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fm_blkdev.h                                               */
/*  PURPOSE : Header file for Flash Block Device Driver                 */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - This layer resides between the RFS file system and FTL.           */
/*  - The main purpose of this layer is to provide a block device       */
/*    interface to user applications (e.g. fdisk).                      */
/*  - For block reads & writes, the file system directly calls FTL      */
/*    jumping over this layer to enhance the performance.               */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 1.0)                                          */
/*                                                                      */
/*  - 01/12/2003 [Joosun Hahn] : First writing                          */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _FM_BLKDEV_H
#define _FM_BLKDEV_H

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

/* ioctl commands */
#define RFS_LOW_FORMAT       0x8A01
#define RFS_GET_DEV_INFO     0x8A02 
#define RFS_GET_PARTITION    0x8A03
#define RFS_SET_PARTITION    0x8A04
#define RFS_FORMAT           0x8A05
#define RFS_CHECK            0x8A06
#define RFS_LOCK_ALL         0x8A07
#define RFS_UNLOCK_ALL       0x8A08
#define RFS_REMOVE_ALL       0x8A09
#define RFS_RAW_WRITE        0x8A0A

#ifdef __KERNEL__

#define RFS_EVENT_USB_VOL_MOUNT     0x8A0B
#define RFS_EVENT_USB_VOL_UMOUNT    0x8A0C

#define FTL_GET_DEV_INFO    0x8A11  /* FTL stat       */
#define FTL_FORMAT          0x8A12  /* FTL format     */
#define FTL_SYNC            0x8A13  /* FTL sync       */
#define FTL_MAPDESTROY      0x8A14  /* FTL mapdestroy */

#endif

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

typedef struct {
    int    num_parts;
    int    part_offset[4];  /* in sector number */
    int    part_size[4];    /* in number of sectors */
} RFS_PARTTAB_T;

typedef struct {
    int    fat_type;
    int    clu_size;        /* in number of sectors */
} RFS_FORMAT_T;

typedef struct {
    char   pattern[260];
    int    flag;            /* DCF rule check flag */
} RFS_PATTERN_T;

typedef struct {
    unsigned short  page;
    unsigned char  *buf;
    unsigned int    buf_size;
} RFS_RAWIO_T;

#ifdef __KERNEL__

#include "fm_global.h"

typedef struct {
    UINT32 cylinders;
    UINT32 heads;
    UINT32 sectors;
    UINT32 ftl_num_sectors;
    UINT32 ftl_block_size;
    UINT32 ftl_page_size;
} FTL_DEVINFO_T;

typedef struct {
    INT32   (*set_part)(int dev, int num_vol, int *vol_off, int *vol_size);
    INT32   (*get_part)(int dev, int *num_vol, int *vol_off, int *vol_size);
    INT32   (*format)(int dev, int vol, int fat_type, int clu_size);
    INT32   (*mount)(int dev, int vol);
    INT32   (*umount)(int dev, int vol);
    INT32   (*check)(int dev, int vol);
    INT32   (*sync)(int dev, int vol);
} RFS_VOL_OPS_T;

#endif

/*----------------------------------------------------------------------*/
/*  Global Function Declarations                                        */
/*----------------------------------------------------------------------*/

#ifdef __KERNEL__

int ufd_init(void);
void ufd_exit(void);
INT32 ufd_sync(INT32 kdev);

INT32 bdev_open(INT32 kdev);
INT32 bdev_close(INT32 kdev);
INT32 bdev_read(INT32 kdev, UINT32 sector, UINT8 *buf, UINT32 num_sectors);
INT32 bdev_write(INT32 kdev, UINT32 sector, UINT8 *buf, UINT32 num_sectors);
INT32 bdev_ioctl(INT32 kdev, UINT32 cmd, void *arg);

#endif
	
#endif /* _FM_BLKDEV_H */

/* end of fm_blkdev.h */
