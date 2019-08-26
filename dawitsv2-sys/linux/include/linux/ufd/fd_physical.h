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
/*  This file implements the Flash Device Physical Interface Layer.     */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Sung-Kwan Kim                                              */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fd_physical.h                                             */
/*  PURPOSE : Header file for Flash Device Physical Interface Layer     */
/*            (PFD)                                                     */
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

#ifndef _FD_PHYSICAL_H
#define _FD_PHYSICAL_H

#include "fd_if.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Configurable)                         */
/*----------------------------------------------------------------------*/

/* which flash memory devices to use?
   mark '1' for the flash memory types that you want to use
   (can be multiply marked) */

#define USE_ONENAND                 0

#define USE_SMALL_NAND              0
#define USE_SMALL_MULTI_NAND        0
#define USE_SMALL_DIRECT_NAND       0
#define USE_SMALL_DIRECT16_NAND     0

#define USE_LARGE_NAND              0
#define USE_LARGE_MULTI_NAND        0
#define USE_LARGE_MULTI_MLC_NAND    1

#define CONFIG_NAND_SECMLC_DDP      1

/* ecc methods (select one from the following methods) */

#define NO_ECC                  0       /* no ecc */
#define HW_ECC                  1       /* hardware ecc */
#define SW_ECC                  2       /* software ecc */

#define ECC_METHOD              NO_ECC

#define HWECC_PIPELINE_NONE     0 
#define HWECC_PIPELINE_LLD      1
#define HWECC_PIPELINE_UFD      2     
#define CFG_HWECC_PIPELINE      HWECC_PIPELINE_NONE

#if (ECC_METHOD != HW_ECC)
#undef  CFG_HWECC_PIPELINE
#define CFG_HWECC_PIPELINE      HWECC_PIPELINE_NONE
#endif

/* copy-back implementation (select one from the following methods) */

#define SW_COPYBACK             0       /* S/W copy-back, i.e. implementing 
                                           copy-back with read & write */
#define HW_COPYBACK             1       /* use device's copy-back function */
#define SAFE_HW_COPYBACK        2       /* use device's copy-back function
                                           only after ...
                                           1. read the source page
                                           2. if the page has no ECC error */ 

/* apply the copyback constraint that an even (odd) page could be copied
   only to another even (odd) page? */

extern int              g_use_odd_even_copyback;
extern int              g_copy_back_method; 


/* can LLD handle non-complete group operation?
   (non-complete group operation: a group operation whose flag parameter
   has some zero entries) */

#define USE_COMPLETE_GROUP_OP   1       /* 1 : cannot handle non-complete
                                               group operations (default)
                                           0 : can handle non-complete
                                               group operations */


/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

enum PHYSICAL_FLASH_DEVICE_TYPE {
    PFD_INVALID_NAND            = 0x00,
    PFD_SMALL_NAND              = 0x01,
    PFD_SMALL_MULTI_NAND        = 0x02,
    PFD_LARGE_NAND              = 0x03,
    PFD_LARGE_MULTI_NAND        = 0x04,
    PFD_ONENAND                 = 0x05
};

enum FLASH_OPERATION {
    OP_NONE                     = 0x00,
    OP_READ                     = 0x01,
    OP_WRITE                    = 0x02,
    OP_ERASE                    = 0x03,
    OP_COPYBACK                 = 0x04,
    OP_READ_GROUP               = 0x05,
    OP_WRITE_GROUP              = 0x06,
    OP_ERASE_GROUP              = 0x07,
    OP_COPYBACK_GROUP           = 0x08,

    OP_COPYBACK_READ            = 0x09,
    OP_COPYBACK_READ_GROUP      = 0x0A,
    OP_COPYBACK_WRITE           = 0x0B,
    OP_COPYBACK_WRITE_GROUP     = 0x0C
};

#define OP_SYNC_MASK            0x10    /* tells if an flash operation has
                                           already been synced or not; 
                                           this value is ORed with the above
                                           'FLASH_OPERATION' values */

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/* data structure for low-level flash device operations */

typedef struct _FLASH_OPS {
    INT32 (*Open)               (UINT16 chip_id);
    INT32 (*Close)              (UINT16 chip_id);
    INT32 (*ReadPage)           (UINT16 chip_id, UINT32 block, UINT16 page,
                                 UINT16 sector_offset, UINT16 num_sectors,
                                 UINT8 *dbuf, UINT8 *sbuf);
    INT32 (*ReadPageGroup)      (UINT16 chip_id, UINT32 *block, UINT16 page,
                                 UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                 INT32 *flag);
    INT32 (*WritePage)          (UINT16 chip_id, UINT32 block, UINT16 page,
                                 UINT16 sector_offset, UINT16 num_sectors,
                                 UINT8 *dbuf, UINT8 *sbuf, BOOL is_last);
    INT32 (*WritePageGroup)     (UINT16 chip_id, UINT32 *block, UINT16 page,
                                 UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                 INT32 *flag, BOOL is_last);
    INT32 (*CopyBack)           (UINT16 chip_id, 
                                 UINT32 src_block, UINT16 src_page, 
                                 UINT32 dest_block, UINT16 dest_page);
    INT32 (*CopyBackGroup)      (UINT16 chip_id, 
                                 UINT32 *src_block, UINT16 *src_page, 
                                 UINT32 *dest_block, UINT16 dest_page, 
                                 INT32 *flag);
    INT32 (*Erase)              (UINT16 chip_id, UINT32 block);
    INT32 (*EraseGroup)         (UINT16 chip_id, UINT32 *block, 
                                 INT32 *flag);
    INT32 (*Sync)               (UINT16 chip_id);
    BOOL  (*IsBadBlock)         (UINT16 chip_id, UINT32 block);
    BOOL  (*IsMultiOK)          (UINT16 chip_id, UINT32 *block, INT32 *flag);
    INT32 (*ReadDeviceCode)     (UINT16 chip_id, UINT8 *maker, 
                                 UINT8 *dev_code);
    INT32 (*SetRWArea)          (UINT16 chip_id, UINT32 start_block, 
                                 UINT32 num_blocks);
} FLASH_OPS;

/* data structure for describing flash operation */

typedef struct _OPERATION_DESC {
    UINT16          Command;
    INT32           Result;
    union {
        struct _READ_COMMAND_PARAM {
            UINT32  Block;
            UINT16  Page;
            UINT16  SectorOffset;
            UINT16  NumSectors;
            UINT8  *DBuf;
            UINT8  *SBuf;
        }           Read;
        
        struct _WRITE_COMMAND_PARAM {
            UINT32  Block;
            UINT16  Page;
            UINT16  SectorOffset;
            UINT16  NumSectors;
            UINT8  *DBuf;
            UINT8  *SBuf;
            BOOL    IsLast;
        }           Write;
        
        struct _COPYBACK_COMMAND_PARAM {
            UINT32  SrcBlock;
            UINT16  SrcPage;
            UINT32  DestBlock;
            UINT16  DestPage;
        }           CopyBack;
        
        struct _ERASE_COMMAND_PARAM {
            UINT32  Block;
        }           Erase;

#if (USE_SMALL_MULTI_NAND == 1) || (USE_LARGE_MULTI_NAND == 1) || (USE_LARGE_MULTI_MLC_NAND == 1)

        struct _READ_GROUP_COMMAND_PARAM {
            UINT32  Block[MAX_PHYSICAL_PLANES];
            UINT16  Page;
            UINT8  *DBuf[MAX_PHYSICAL_PLANES];
            UINT8  *SBuf[MAX_PHYSICAL_PLANES];
            INT32   Flag[MAX_PHYSICAL_PLANES];
        }           ReadGroup;
        
        struct _WRITE_GROUP_COMMAND_PARAM {
            UINT32  Block[MAX_PHYSICAL_PLANES];
            UINT16  Page;
            UINT8  *DBuf[MAX_PHYSICAL_PLANES];
            UINT8  *SBuf[MAX_PHYSICAL_PLANES];
            INT32   Flag[MAX_PHYSICAL_PLANES];
            BOOL    IsLast;
        }           WriteGroup;

        struct _COPYBACK_GROUP_COMMAND_PARAM {
            UINT32  SrcBlock[MAX_PHYSICAL_PLANES];
            UINT16  SrcPage[MAX_PHYSICAL_PLANES];
            UINT32  DestBlock[MAX_PHYSICAL_PLANES];
            UINT16  DestPage;
            INT32   Flag[MAX_PHYSICAL_PLANES];
        }           CopyBackGroup;

        struct _ERASE_GROUP_COMMAND_PARAM {
            UINT32  Block[MAX_PHYSICAL_PLANES];
            INT32   Flag[MAX_PHYSICAL_PLANES];
        }           EraseGroup;
#endif
    }               Param;
} OP_DESC;

/* data structure for physical flash device */

typedef struct _PHYSICAL_FLASH_DEVICE {
    UINT16      ChipID;             /* PFDEV array index for self reference */
    UINT16      DevType;            /* small/large block NAND, OneNAND, ... */
    UINT16      LocalChipID;        /* chip ID (0, 1, ...) in each class */
    UINT8       UsageCount;         /* usage count */

    UINT8       DevCode;            /* device code */
    FLASH_SPEC  DevSpec;            /* dimensional specification */
    FLASH_OPS   DevOps;             /* flash device operations */
    OP_DESC    *PrevOp;             /* previous operation information */
    
    UINT32      BmAreaStartBlock;   /* starting block number of BM area */
    UINT16      BmAreaNumBlocks;    /* number of blocks in BM area */
    UINT32      PartTableBlock;     /* block number of the partition table */
    UINT32      ActualNumBlocks;    /* equals (NumBlocks-BmAreaNumBlocks-1) */
    
	// 2006.06.25. in case of s/w copyback. max_physical_planes are used. 
    UINT8       Buf[MAX_PHYSICAL_PLANES][MAX_PAGE_SIZE];

    UINT8       BufIndex;           /* indicates what the current buffer is */
    BOOL        BufAllBusy;         /* TRUE if multi-plane S/W copy-back is 
                                       used and thus all buffers are busy */
} PFDEV;

/*----------------------------------------------------------------------*/
/*  External Variable Declarations                                      */
/*----------------------------------------------------------------------*/

extern PFDEV    PDev[MAX_FLASH_CHIPS];
extern UINT16   PDevCount;

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

extern INT32  PFD_Init                (void);
extern INT32  PFD_RegisterFlashDevice (UINT16       DevType, 
                                       UINT16       LocalChipID, 
                                       FLASH_SPEC  *DevSpec, 
                                       FLASH_OPS   *DevOps,
                                       OP_DESC     *PrevOp);
extern UINT16 PFD_GetNumberOfChips    (void);
extern PFDEV *PFD_GetPhysicalDevice   (UINT16       chip_id);

extern INT32  PFD_Open                (UINT16       ChipID);
extern INT32  PFD_Close               (UINT16       ChipID);

extern INT32  PFD_ReadChipDeviceCode  (UINT16       ChipID, 
                                       UINT8       *Maker, 
                                       UINT8       *DevCode);
extern INT32  PFD_GetChipDeviceInfo   (UINT16       ChipID, 
                                       FLASH_SPEC  *DevInfo);
extern INT32  PFD_EraseAll            (UINT16       ChipID);

/* wrapping functions for common physical I/O */

extern INT32  PFD_ReadPage          (PFDEV *pdev, UINT32 block, UINT16 page,
                                     UINT16 sector_offset, UINT16 num_sectors,
                                     UINT8 *dbuf, UINT8 *sbuf);
extern INT32  PFD_ReadPageGroup     (PFDEV *pdev, UINT32 *block, UINT16 page,
                                     UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                     INT32 *flag);
extern INT32  PFD_WritePage         (PFDEV *pdev, UINT32 block, UINT16 page,
                                     UINT16 sector_offset, UINT16 num_sectors,
                                     UINT8 *dbuf, UINT8 *sbuf, BOOL is_last);
extern INT32  PFD_WritePageGroup    (PFDEV *pdev, UINT32 *block, UINT16 page,
                                     UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                     INT32 *flag, BOOL is_last);
extern INT32  PFD_CopyBack          (PFDEV *pdev, 
                                     UINT32 src_block, UINT16 src_page, 
                                     UINT32 dest_block, UINT16 dest_page);
extern INT32  PFD_CopyBackGroup     (PFDEV *pdev, 
                                     UINT32 *src_block, UINT16 *src_page, 
                                     UINT32 *dest_block, UINT16 dest_page, 
                                     INT32 *flag);
extern INT32  PFD_Erase             (PFDEV *pdev, UINT32 block);
extern INT32  PFD_EraseGroup        (PFDEV *pdev, UINT32 *block, INT32 *flag);
extern INT32  PFD_Sync              (PFDEV *pdev);

/* miscellaneous functions */

extern UINT8 *PFD_GetBuffer         (PFDEV *pdev, UINT16 purpose);

#endif /* _FD_PHYSICAL_H */
