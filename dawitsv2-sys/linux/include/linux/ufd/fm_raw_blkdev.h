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
/*  This file implements the flash raw block device driver.             */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fm_raw_blkdev.h                                           */
/*  PURPOSE : Header file for Flash Raw Block Device Driver             */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - The main purpose of this layer is to provide a raw block device   */
/*    interface to user applications (e.g. fpart).                      */
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

#ifndef _FM_RAW_BLKDEV_H
#define _FM_RAW_BLKDEV_H

#ifdef __KERNEL__
#include "fd_if.h"
#else
#define MAX_FLASH_PARTITIONS 8
#endif

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

/* ioctl commands */
#define UFD_GET_DEV_INFO     0x8A21 
#define UFD_GET_PARTITION    0x8A22
#define UFD_SET_PARTITION    0x8A23
#define UFD_FORMAT           0x8A24
#define UFD_ERASE_ALL        0x8A25
#define UFD_ERASE_PARTITION  0x8A26
#define UFD_GET_OOB_INFO     0x8A27
#define UFD_RESTORE          0x8A28
#define UFD_UNLOCK_ALL       0x8A29
#define UFD_SET_RW_AREA      0x8A2A
#define UFD_READ_BLOCK       0x8A2B

/* device class */
#define UFD_BLK_DEVICE_RAW   137
#define UFD_BLK_DEVICE_FTL   138

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

typedef struct {
    int   phy_blk_size;  /* in bytes */
    int   num_blocks;
} UFD_DEVINFO_T;

typedef struct {
    int   num_parts;
    int   part_size[MAX_FLASH_PARTITIONS];      /* in number of blocks */
    int   part_class[MAX_FLASH_PARTITIONS];     /* device class */
    int   in_dev_table[MAX_FLASH_PARTITIONS];   /* registered 
                                                   in the device table? */
    int   dev_index[MAX_FLASH_PARTITIONS];      /* device table index */
} UFD_PARTTAB_T;

typedef struct {
    int             include_spare;              /* include the spare area? */
    unsigned int    block;                      /* block number in a logical
                                                   flash device (LFD) */
    unsigned char   *buf;                       /* buffer for block data;
                                                   if the spare area included,
                                                   the block data is stored in
                                                   this buffer in the following
                                                   sequence:
                                                   - main  data for page 0,
                                                   - spare data for page 0,
                                                   - main  data for page 1,
                                                   - spare data for page 1,
                                                     ... ... ...
                                                */
} UFD_BLOCK_IO_T;

#endif /* _FM_RAW_BLKDEV_H */

/* end of fm_raw_blkdev.h */
