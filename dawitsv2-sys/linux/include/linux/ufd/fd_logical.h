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
/*  This file implements the Flash Device Logical Interface Layer.      */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Sung-Kwan Kim                                              */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fd_logical.h                                              */
/*  PURPOSE : Header file for Flash Device Logical Interface Layer      */
/*            (LFD)                                                     */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 2.0)                                          */
/*                                                                      */
/*  - 01/12/2003 [Sung-Kwan Kim] : First writing                        */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  TECHNICAL SUPPORT                                                   */
/*                                                                      */
/*  - Samsung Flash Planning Group (flashsw@samsung.com)                */
/*                                                                      */
/************************************************************************/

#ifndef _FD_LOGICAL_H
#define _FD_LOGICAL_H

#include "fd_if.h"
#include "fd_physical.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Configurable)                         */
/*----------------------------------------------------------------------*/

#define USE_CUSTOM_DEV_SPEC     1   /* 1: use customized device spec (default)
                                       0: don't use customized device spec */

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

#ifdef ZFLASH_BLEX
#undef  USE_CUSTOM_DEV_SPEC
#define USE_CUSTOM_DEV_SPEC     0
#endif

enum CUSTOM_DEV_SPEC_TYPES {
    DEV_SPEC_ORIGINAL           = 0x00,
    DEV_SPEC_EMUL_SMALL_MULTI   = 0x01,
    DEV_SPEC_GENERIC_CUSTOM     = 0xFF
};

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/* data structure for a real flash partition */

typedef struct _FLASH_REAL_PARTITION {
    PFDEV       *PDev;              /* pointer to a physical device */
    FLASH_PART  *Part;              /* pointer to a flash partition info 
                                       structure in the designated physical
                                       device */
} FLASH_RPART;

/* data structure for a virtual flash partition (a group of partitions) */

typedef struct _FLASH_VIRTUAL_PARTITION {
    UINT16      NumPartitions;      /* # of the constituent partitions */
    FLASH_RPART Parts[MAX_FLASH_CHIPS];  /* info about the partitions */
} FLASH_VPART;

/* data structure for a logical flash device */

typedef struct _LOGICAL_FLASH_DEVICE {
    UINT32      DevID;              /* unique dev ID used by upper layers */
    FLASH_VPART PartInfo;           /* partition information */
    FLASH_SPEC  DevSpec;            /* device specification */
    UINT16      InterleavingLevel;  /* level of interleaving */
    UINT8       UsageCount;         /* usage count */

#if USE_CUSTOM_DEV_SPEC
    UINT8       CustomDevSpecType;  /* type of CustomDevSpec */
    FLASH_SPEC  CustomDevSpec;      /* user-defined device specification */
#endif
} LFDEV;

/*----------------------------------------------------------------------*/
/*  External Variable Declarations                                      */
/*----------------------------------------------------------------------*/

extern LFDEV    LDev[MAX_FLASH_DEVICES];
extern UINT16   LDevCount;

extern FLASH_PARTTAB   PartTable[MAX_FLASH_CHIPS];
extern FLASH_VPART     RAWDevTable[MAX_FLASH_VPARTITIONS];
extern FLASH_VPART     FTLDevTable[MAX_FLASH_VPARTITIONS];

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

extern INT32  LFD_Init(void);

/* functions for flash memory low-level partition management */

extern INT32  LFD_ReadPartitionTable    (UINT16 chip_id, 
                                         FLASH_PARTTAB *part_tab);
extern INT32  LFD_WritePartitionTable   (UINT16 chip_id, 
                                         FLASH_PARTTAB *part_tab);
extern INT32  LFD_ErasePart             (UINT16 chip_id, UINT16 part_no);

/* functions for logical device management */

extern LFDEV *LFD_AllocLogicalDevice    (UINT32 dev_id);
extern void   LFD_FreeLogicalDevice     (UINT32 dev_id);
extern LFDEV *LFD_GetLogicalDevice      (UINT32 dev_id);

/* functions for customized device APIs */

#if USE_CUSTOM_DEV_SPEC

extern INT32  LFD_ReadPage          (LFDEV *ldev, UINT32 block, UINT16 page,
                                     UINT16 sector_offset, UINT16 num_sectors,
                                     UINT8 *dbuf, UINT8 *sbuf);
extern INT32  LFD_ReadPageGroup     (LFDEV *ldev, UINT32 group, UINT16 page,
                                     UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                     INT32 *flag);
extern INT32  LFD_WritePage         (LFDEV *ldev, UINT32 block, UINT16 page,
                                     UINT16 sector_offset, UINT16 num_sectors,
                                     UINT8 *dbuf, UINT8 *sbuf, BOOL is_last);
extern INT32  LFD_WritePageGroup    (LFDEV *ldev, UINT32 group, UINT16 page,
                                     UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                     INT32 *flag, BOOL is_last);
extern INT32  LFD_CopyBack          (LFDEV *ldev, 
                                     UINT32 src_block, UINT16 src_page, 
                                     UINT32 dest_block, UINT16 dest_page);
extern INT32  LFD_CopyBackGroup     (LFDEV *ldev, 
                                     UINT32 src_group, UINT16 src_page, 
                                     UINT32 dest_group, UINT16 dest_page, 
                                     INT32 *flag);
extern INT32  LFD_Erase             (LFDEV *ldev, UINT32 block);
extern INT32  LFD_EraseGroup        (LFDEV *ldev, UINT32 group, INT32 *flag);

#endif /* USE_CUSTOM_DEV_SPEC */

/* miscellaneous functions */

extern FLASH_RPART *LFD_GetPartitionInfo (LFDEV *ldev, UINT32 *block);

#endif /* _FD_LOGICAL_H */
