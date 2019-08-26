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
/*  This file implements the Flash Device Common Interface.             */
/*                                                                      */
/*  @author  Hak-Yong Lee                                               */
/*  @author  Sung-Kwan Kim                                              */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : Linux RFS (Robust FAT File System) for NAND Flash Memory  */
/*  FILE    : fd_if.h                                                   */
/*  PURPOSE : Header file for Flash Device Common Interface             */
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

#ifndef _FD_IF_H
#define _FD_IF_H

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Configurable)                         */
/*----------------------------------------------------------------------*/

#define MAX_FLASH_CHIPS         4       /* max # of physical devices 
                                           (max 15) */

#define MAX_FLASH_PARTITIONS    8       /* max # of partitions in each 
                                           physical device (max 15) */

#define MAX_FLASH_VPARTITIONS   4       /* max # of virtual partitions 
                                           (max 15) */

#define MAX_FLASH_DEVICES       8       /* max # of logical flash devices
                                           that can be opened simultaneously */

#define MAX_PHYSICAL_PLANES     4       /* max # of physical planes; this 
                                           constant depends on the specific 
                                           flash memory devices used */

#define MAX_SECTORS_PER_PAGE    8       /* hcyun=8 max # of sectors per page; this
                                           constant depends on the specific 
                                           flash memory devices used */

#define MAX_INTERLEAVED_CHIPS   2       /* max # of interleaved chips; the 
                                           valid value range for this is
                                           from 1 to MAX_FLASH_CHIPS, and
                                           it must be a number of 2^n */

#define USE_DEFAULT_PARTITION   0       /* 1: use default partition (default)
                                           0: don't use default partition */
                                           
#define STRICT_CHECK            0       /* 1: enable strict parameter check
                                           0: disable strict check (default) */

/* for linux, the following logical device class code corresponds to
   the major device number of a block device that corresponds to each 
   logical flash device; depending on the target system environment,
   you may have to customize the following code assignments */

enum LOGICAL_FLASH_DEVICE_CLASS {
    LFD_BLK_DEVICE_RAW          = 137,  /* used as a RAW block device */
    LFD_BLK_DEVICE_FTL          = 138   /* used as a FTL block device */
};

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

//#define MAX_LOGICAL_PLANES      (MAX_PHYSICAL_PLANES * MAX_INTERLEAVED_CHIPS)
#define MAX_LOGICAL_PLANES      (MAX_PHYSICAL_PLANES * 4)

#define MAX_PAGE_SIZE           (528 * MAX_SECTORS_PER_PAGE)
#define MAX_DATA_SIZE           (512 * MAX_SECTORS_PER_PAGE)
#define MAX_SPARE_SIZE          (16  * MAX_SECTORS_PER_PAGE)

#define RAW_BLKDEV_TABLE        0xF     /* special ID used to indicate that a
                                           raw block device will be referenced
                                           by using an index value to the raw
                                           block device table instead of using
                                           the chip number & partition number
                                           pairs */

#define ENTIRE_PARTITION        0xF     /* special partition ID for the entire
                                           chip space (excluding the DLBM area
                                           and the partition table block) */

/* return values of flash memory device driver API functions */

enum DRIVER_API_RETURN_VALUES {
    FM_SUCCESS                  = 0,
    FM_BAD_DEVICE_ID            = 1,
    FM_OPEN_FAIL                = 2,
    FM_INIT_FAIL                = 3,
    FM_CLOSE_FAIL               = 4,
    FM_NOT_FORMATTED            = 5,
    FM_ILLEGAL_ACCESS           = 6,
    FM_DEVICE_NOT_OPEN          = 7,
    FM_ECC_ERROR                = 8,
    FM_TRY_AGAIN                = 9,
    FM_ERROR                    = 10,
    
    FM_READ_ERROR               = 11,
    FM_WRITE_ERROR              = 12,
    FM_ERASE_ERROR              = 13,
    FM_PROTECT_ERROR            = 14,

    /* previous error return values should 
       have values greater than 15 (= 0xF) */

    FM_PREV_WRITE_ERROR         = 16,
    FM_PREV_ERASE_ERROR         = 17,
    FM_PREV_PROTECT_ERROR       = 18,
    
    FM_PREV_WRITE_GROUP_ERROR   = 19,
    FM_PREV_ERASE_GROUP_ERROR   = 20
};

#define FM_PREV_ERROR_MASK      0xF0

/* macros for decomposing device ID */

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
#define DEV_SERIAL_BITS         20
#define DEV_SERIAL_MASK         0xFFFFF
#else
#define DEV_SERIAL_BITS         16
#define DEV_SERIAL_MASK         0xFFFF
#endif

#define GET_DEV_CLASS(dev_id)   ((UINT16)((dev_id) >> DEV_SERIAL_BITS))
#define GET_DEV_SERIAL(dev_id)  ((UINT16)((dev_id) & DEV_SERIAL_MASK))

/* definition for logical device ID used for 'RAW block device'

            +-----------+-----------+-----------+-----------+
   dev_id:  |   Byte3   |   Byte2   |   Byte1   |   Byte0   |
            +-----------+-----------+-----------+-----------+
             `---- device class ---' `--- serial number ---'

            +----------------------------------------------------------------+
            | device class  = LFD_BLK_DEVICE_RAW                             |
            |                                                                |
            | serial number = not used (Byte1) +                             |
            |                 chip number (Byte0, upper 4 bits) +            |
            |                 partition number (Byte0, lower 4 bits)         |
            |                                                                |
            |                 or                                             |
            |                                                                |
            |               = not used (Byte1) +                             |
            |                 RAW_BLKDEV_TABLE (Byte0, upper 4 bits) +       |
            |                 raw block device index (Byte0, lower 4 bits)   |
            +----------------------------------------------------------------+

            * special partition number for entire device space = 0xF

            * if a special number 'RAW_BLKDEV_TABLE' (= 0xF) is given in the 
              place of chip number, a raw block device can be referenced by 
              using the raw block device table and the index to the table; 
              this mechanism is required because there might be a raw block 
              device that is comprised of multiple partitions on multiple chips

            * examples for RAW block device's serial number:
              - chip-0, partition-0  = 0x0000
              - chip-1, partition-2  = 0x0012
              - chip-2, entire space = 0x002F
              - 1st device in the RAW block device table = 0x00F0
              - 2nd device in the RAW block device table = 0x00F1
*/

#define GET_RAWDEV_CHIP_ID(serial)  ((UINT16)((serial) >> 4))
#define GET_RAWDEV_PART_ID(serial)  ((UINT16)((serial) & 0xF))
#define GET_RAWDEV_INDEX(serial)    ((UINT16)((serial) & 0xF))

/* definition for logical device ID used for 'FTL block device'

            +-----------+-----------+-----------+-----------+
   dev_id:  |   Byte3   |   Byte2   |   Byte1   |   Byte0   |
            +-----------+-----------+-----------+-----------+
             `---- device class ---' `--- serial number ---'

            +----------------------------------------------------------------+
            | device class  = LFD_BLK_DEVICE_FTL                             |
            |                                                                |
            | serial number = not used (Byte1) +                             |
            |                 FTL block device index (Byte0, upper 4 bits) + |
            |                 reserved (Byte0, lower 4 bits)                 |
            +----------------------------------------------------------------+

            * 'FTL block device index' is an index to the FTL block device 
              table, which maintains information about the constituent 
              partitions for each raw block device

            * 'reserved' bits of Byte0 is currently used by the file system 
              to store high-level (filesystem-level) partition information

            * examples for FTL block device's serial number:
              - 1st FTL block device = 0x0000
              - 2nd FTL block device = 0x0010
              - 3rd FTL block device = 0x0020
*/

#define GET_FTLDEV_INDEX(serial)  ((UINT16)((serial) >> 4))

/* macros for composing device ID;
   you can make a 'dev_id' using one of the following macros:
   
   - dev_id for a RAW block device using (chip_id, part_id):
     SET_DEV_ID(LFD_BLK_DEVICE_RAW, SET_RAWDEV_SERIAL(chip_id, part_id))
   
   - dev_id for a RAW block device using the RAW_BLKDEV_TABLE index:
     SET_DEV_ID(LFD_BLK_DEVICE_RAW, 
                SET_RAWDEV_SERIAL(RAW_BLKDEV_TABLE, rawdev_index))
     
   - dev_id for an FTL block device:
     SET_DEV_ID(LFD_BLK_DEVICE_FTL, SET_FTLDEV_SERIAL(ftldev_index))
*/

#define SET_DEV_ID(dev_class, serial)       \
        ((((UINT32)(dev_class))<<DEV_SERIAL_BITS) | ((serial)&DEV_SERIAL_MASK))

#define SET_RAWDEV_SERIAL(chip_id, part_id) \
        ((UINT16)((((chip_id) & 0xF) << 4) | ((part_id) & 0xF)))

#define SET_FTLDEV_SERIAL(ftldev_index) \
        ((UINT16)(((ftldev_index) & 0xF) << 4))

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/* data structure for dimensional flash memory specification */

typedef struct _FLASH_SPEC {
    UINT16      NumDiesPerCE;       /* number of dies per chip enable */
    UINT16      NumPlanes;          /* number of planes */
    UINT16      PageSize;           /* page size (bytes) */
    UINT16      DataSize;           /* page main area size (bytes) */
    UINT16      SpareSize;          /* page spare area size (bytes) */
    UINT16      SectorsPerPage;     /* number of sectors per page */
    UINT16      PagesPerBlock;      /* number of pages per block */
    UINT32      BlockSize;          /* block size (bytes) */
    UINT32      NumBlocks;          /* total number of blocks */
    UINT32      MaxNumBadBlocks;    /* maximum number of bad blocks */
} FLASH_SPEC;

/* data structure for each flash partition */

typedef struct _FLASH_PART {
    UINT32      StartBlock;         /* starting block # of partition */
    UINT32      NumBlocks;          /* number of blocks in partition */
    UINT32      DevClass;           /* logical flash device class; 
                                       for linux, DevClass corresponds
                                       to the major device number */
    BOOL        InDevTable;         /* indicates whether this partition is
                                       registered in one of the RAW and FTL
                                       block device tables; for an FTL block
                                       device partition, this value should
                                       be TRUE */
    UINT8       DevIndex;           /* device index for the RAW block device
                                       table or the FTL block device table
                                       (device tables are indexed separately);
                                       if multiple RAW (or FTL) block device 
                                       partitions share the same 'DevIndex' 
                                       value, they'll be merged into a single
                                       virtual partition */
    BOOL        Protected;          /* write/erase protection flag */
    UINT8       ECCMode;            /* ECC mode: None, H/W, S/W, ... */
} FLASH_PART;

/* data structure for flash partition table */

typedef struct _FLASH_PARTTAB {
    UINT16      NumPartitions;      /* # of partitions in a chip */
    FLASH_PART  Part[MAX_FLASH_PARTITIONS];
} FLASH_PARTTAB;

/*----------------------------------------------------------------------*/
/*  External Variable Declarations                                      */
/*----------------------------------------------------------------------*/

extern BOOL   FD_ECC_Corrected;     /* defined in fd_physical.c */

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
extern UINT32 FD_UserSequence;      /* a UFD user (e.g. RFS) increases this
                                       variable whenever it is invoked by
                                       the system call; this value is used
                                       by the 'AVOID_MONOPOLIZATION' macro
                                       for a reference value */
#endif

/*----------------------------------------------------------------------*/
/*  External Function Declarations                                      */
/*----------------------------------------------------------------------*/

/* init function (initializes all of the device driver layers) */

extern INT32  FD_Init               (void);
extern INT32  FD_Shutdown           (void);

/* flash memory device driver APIs; 
   upper layer software (e.g. FTL) calls these functions 
   (dev_id = logical device ID) */

extern INT32  FD_Open               (UINT32 dev_id);
extern INT32  FD_Close              (UINT32 dev_id);
extern INT32  FD_ReadPage           (UINT32 dev_id, UINT32 block, UINT16 page,
                                     UINT16 sector_offset, UINT16 num_sectors,
                                     UINT8 *dbuf, UINT8 *sbuf);
extern INT32  FD_ReadPageGroup      (UINT32 dev_id, UINT32 group, UINT16 page,
                                     UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                     INT32 *flag);
extern INT32  FD_WritePage          (UINT32 dev_id, UINT32 block, UINT16 page,
                                     UINT16 sector_offset, UINT16 num_sectors,
                                     UINT8 *dbuf, UINT8 *sbuf, BOOL is_last);
extern INT32  FD_WritePageGroup     (UINT32 dev_id, UINT32 group, UINT16 page,
                                     UINT8 *dbuf_group[], UINT8 *sbuf_group[],
                                     INT32 *flag, BOOL is_last);
extern INT32  FD_CopyBack           (UINT32 dev_id, 
                                     UINT32 src_block, UINT16 src_page, 
                                     UINT32 dest_block, UINT16 dest_page);
extern INT32  FD_CopyBackGroup      (UINT32 dev_id, 
                                     UINT32 src_group, UINT16 src_page, 
                                     UINT32 dest_group, UINT16 dest_page, 
                                     INT32 *flag);
extern INT32  FD_Erase              (UINT32 dev_id, UINT32 block);
extern INT32  FD_EraseGroup         (UINT32 dev_id, UINT32 group, 
                                     INT32 *flag);
extern INT32  FD_Sync               (UINT32 dev_id);

extern INT32  FD_GetDeviceInfo      (UINT32 dev_id, FLASH_SPEC *dev_info);
extern INT32  FD_SetCustomDeviceInfo(UINT32 dev_id, FLASH_SPEC *dev_info);
                                     
/* additional APIs for flash memory volume management
   (chip_id = physical device ID) */

extern UINT16 FD_GetNumberOfChips   (void);
extern INT32  FD_ReadChipDeviceCode (UINT16 chip_id, UINT8 *maker, 
                                     UINT8 *dev_code);
extern INT32  FD_GetChipDeviceInfo  (UINT16 chip_id, FLASH_SPEC *dev_info);
extern INT32  FD_Format             (UINT16 chip_id);
extern INT32  FD_ReadPartitionTable (UINT16 chip_id, FLASH_PARTTAB *part_tab);
extern INT32  FD_WritePartitionTable(UINT16 chip_id, FLASH_PARTTAB *part_tab);
extern INT32  FD_GetNumBadBlocks    (UINT16 chip_id);
extern INT32  FD_EraseAll           (UINT16 chip_id);
extern INT32  FD_ErasePart          (UINT16 chip_id, UINT16 part_no);

#endif /* _FD_IF_H */
