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
/*  FILE    : fd_logical.c                                              */
/*  PURPOSE : Code for Flash Device Logical Interface Layer (LFD)       */
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

#include "fm_global.h"
#include "fd_if.h"
#include "fd_logical.h"
#include "fd_bm.h"
#include "fd_physical.h"

#if CFG_HWECC_PIPELINE == HWECC_PIPELINE_UFD
#include "lld/large/hwecc.h" /* HWECC_PIPELINEING */ 
#endif 

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Configurable)                         */
/*----------------------------------------------------------------------*/

#define COPY_TO_LINEAR_BUFFER   1           /* if delivered buffers are 
                                               disjoint, copy them into a
                                               linear buffer?
                                               1: yes (default), 0: no */

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions (Non-Configurable)                     */
/*----------------------------------------------------------------------*/

#define PARTTABLE_SIGNATURE     0xBAB0826A
#define INVALID_DEVICE_ID       0xFFFFFFFF  /* invalid device ID */

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/* data structure for flash memory page of partition table */

typedef struct _FLASH_PARTPAGE {
    UINT32          Signature;      /* partition table magic number */
    FLASH_PARTTAB   PartTable;      /* partition table */
    UINT16          CheckSum;       /* check sum for partition table */
} FLASH_PARTPAGE;

/*----------------------------------------------------------------------*/
/*  External Variable Definitions                                       */
/*----------------------------------------------------------------------*/

LFDEV         LDev[MAX_FLASH_DEVICES];      /* opened LFD device table */
UINT16        LDevCount;

FLASH_PARTTAB PartTable[MAX_FLASH_CHIPS];   /* partition table */
FLASH_VPART   RAWDevTable[MAX_FLASH_VPARTITIONS]; /* RAW block device table */
FLASH_VPART   FTLDevTable[MAX_FLASH_VPARTITIONS]; /* FTL block device table */

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

static INT32  read_sector_group(LFDEV *ldev, PFDEV *pdev, 
                                UINT32 block, UINT16 page, 
                                UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
                                INT32 *flag);
static INT32  write_sector_group(LFDEV *ldev, PFDEV *pdev, 
                                 UINT32 block, UINT16 page,
                                 UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
                                 INT32 *flag, BOOL is_last);
static BOOL   is_valid_partition_table(PFDEV *pdev, FLASH_PARTTAB *part_tab);
static INT32  make_blk_dev_table(FLASH_PARTTAB *part_table, 
                                 FLASH_VPART *dev_table, UINT16 dev_class);
static UINT16 calc_checksum(UINT16 *data, UINT16 size);


/*======================================================================*/
/*  External Function Definitions                                       */
/*======================================================================*/

extern INT32 
LFD_Init(void)
{
    int i;

    /* initialize the flash partition table */
    for (i = 0; i < MAX_FLASH_CHIPS; i++) {
        MEMSET((void *)&PartTable[i], 0, sizeof(FLASH_PARTTAB));
    }

    /* initialize the logical flash device table */
    for (i = 0; i < MAX_FLASH_DEVICES; i++) {
        MEMSET((void *)&LDev[i], 0, sizeof(LFDEV));
        LDev[i].DevID = INVALID_DEVICE_ID;
    }
    
    LDevCount = 0;
    return(FM_SUCCESS);
}


/*----------------------------------------------------------------------*/
/*  Functions for Flash Memory Low-Level Partition Management           */
/*----------------------------------------------------------------------*/

extern INT32    
LFD_ReadPartitionTable(UINT16 chip_id, FLASH_PARTTAB *part_tab)
{
    INT32  err;
    UINT32 part_block;
    UINT8 *pbuf;
    PFDEV *pdev;
    FLASH_PARTPAGE *p;

    /* existing chip id? */
    if (chip_id >= PDevCount) return(FM_BAD_DEVICE_ID);
    pdev = &PDev[chip_id];

#if USE_DLBM
    if (!BM_isFormatted(chip_id)) return(FM_NOT_FORMATTED);
#endif

    /* get the current I/O buffer */
    pbuf = PFD_GetBuffer(pdev, OP_READ);
    p = (FLASH_PARTPAGE *)pbuf;

    /* get the block where partition information is stored */
    part_block = pdev->PartTableBlock;

    /* read partition table from flash memory */
    do {
        err = PFD_ReadPage(pdev, part_block, 0, 0, 
                           pdev->DevSpec.SectorsPerPage, pbuf, NULL);
    } while (err == FM_TRY_AGAIN);

    if (err) return(err);

    /* verify the partition table using signature & checksum code */
    if (p->Signature != PARTTABLE_SIGNATURE ||
        p->CheckSum != calc_checksum((UINT16 *)&p->PartTable, 
                                     sizeof(FLASH_PARTTAB))) {
        PartTable[chip_id].NumPartitions = 0;
    }
    else {
        /* copy the partition table into the global data structure */
        MEMCPY(&PartTable[chip_id], &p->PartTable, sizeof(FLASH_PARTTAB));

	/* Ugly fix for bad written partition table. phase1-4GB, hcyun */ 
	is_valid_partition_table(pdev, &PartTable[chip_id]); 
        
        /* make the RAW block device tables based on the partition table */
        err = make_blk_dev_table(PartTable, RAWDevTable, LFD_BLK_DEVICE_RAW);
        if (err) return(err);
        
        /* make the FTL block device tables based on the partition table */
        err = make_blk_dev_table(PartTable, FTLDevTable, LFD_BLK_DEVICE_FTL);
        if (err) return(err);
    }
    
    /* also copy the partition table into the return parameter */
    if (part_tab != NULL) {
        MEMCPY(part_tab, &PartTable[chip_id], sizeof(FLASH_PARTTAB));
    }

    return(FM_SUCCESS);
}


extern INT32    
LFD_WritePartitionTable(UINT16 chip_id, FLASH_PARTTAB *part_tab)
{
    INT32  err;
    UINT32 part_block;
    UINT8 *pbuf;
    PFDEV *pdev;
    FLASH_PARTPAGE *p;

    /* check validity of parameters */
    if (part_tab == NULL) return(FM_ERROR);

    /* existing chip id? */
    if (chip_id >= PDevCount) return(FM_BAD_DEVICE_ID);
    pdev = &PDev[chip_id];

#if USE_DLBM
    if (!BM_isFormatted(chip_id)) return(FM_NOT_FORMATTED);
#endif

    /* if someone uses this physical device now, the partition table
       cannot be changed; by default, every physical device is opened
       at the UFD initialization time... so if someone uses this 
       physical device, the usage count will be larger than one */
#if 0
    /* currently, this checking is not used;
       if you want to enable this checking, you should insert the
       PFD_Open() function calls in the FD_Open() function */
    if (pdev->UsageCount > 2) return(FM_ERROR);
#endif

    /* check the validity of the given partition table */
    if (!is_valid_partition_table(pdev, part_tab)) return(FM_ERROR);

    /*--------------------------------------------------------------*/
    /*  first, erase the partition block                            */
    /*--------------------------------------------------------------*/

    /* get the block where partition information is stored */
    part_block = pdev->PartTableBlock;

    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_Erase(pdev, part_block);
    } while (err == FM_TRY_AGAIN);

    if (err) return(err);
    
    /*--------------------------------------------------------------*/
    /*  second, fill the partition page buffer                      */
    /*--------------------------------------------------------------*/

    /* get the current I/O buffer */
    pbuf = PFD_GetBuffer(pdev, OP_WRITE);
    p = (FLASH_PARTPAGE *)pbuf;

    /* clear the page buffer */
    MEMSET(pbuf, 0xff, pdev->DevSpec.PageSize);
    
    /* fill the partition table in the page buffer */
    p->Signature = PARTTABLE_SIGNATURE;
    MEMCPY(&p->PartTable, part_tab, sizeof(FLASH_PARTTAB));
    p->CheckSum = calc_checksum((UINT16 *)&p->PartTable, 
                                sizeof(FLASH_PARTTAB));

    /*--------------------------------------------------------------*/
    /*  third, write the partition page buffer in flash             */
    /*--------------------------------------------------------------*/

    /* get the block where partition information is stored */
    part_block = pdev->PartTableBlock;

    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_WritePage(pdev, part_block, 0, 0, 
                            pdev->DevSpec.SectorsPerPage, pbuf, NULL, TRUE);
    } while (err == FM_TRY_AGAIN);

    if (!err) {    
        /* copy the given partition table in the global data structure */
        MEMCPY(&PartTable[chip_id], part_tab, sizeof(FLASH_PARTTAB));
        
        /* make the RAW block device tables based on the partition table */
        make_blk_dev_table(PartTable, RAWDevTable, LFD_BLK_DEVICE_RAW);
        
        /* make the FTL block device tables based on the partition table */
        make_blk_dev_table(PartTable, FTLDevTable, LFD_BLK_DEVICE_FTL);
    }
    
    return(err);
}


extern INT32  
LFD_ErasePart(UINT16 chip_id, UINT16 part_no)
{
    PFDEV *pdev;
    FLASH_PARTTAB *part;
    UINT32 i, start_block, last_block, err = FM_SUCCESS;
    int chip;

    /* existing chip id? */
    if (chip_id >= PDevCount && chip_id != 0xf ) 
	    return(FM_BAD_DEVICE_ID);

    if ( chip_id == 0xf ) {
	    part = &PartTable[0];
    } else { 
	    part = &PartTable[chip_id];
    }
    
    if (part_no >= part->NumPartitions && part_no != ENTIRE_PARTITION)
	    return (FM_BAD_DEVICE_ID);

    /* determine the range of the blocks to be erased */
    if (part_no == ENTIRE_PARTITION) {
	    start_block = 0;
	    if (chip_id == 0xf) {
	        pdev = &PDev[0];
	    }
	    else {
	        pdev = &PDev[chip_id];
	    }
	    last_block  = pdev->PartTableBlock;
    }
    else {
	    start_block = part->Part[part_no].StartBlock;
	    last_block  = start_block + part->Part[part_no].NumBlocks;
    }
    
    /* erase all blocks in the given partition */
    for ( chip = 0; chip < PDevCount; chip++ ) { 
	    if ( !(chip_id == 0xf || chip_id == chip) ) 
		    continue; 
	    pdev = &PDev[chip];    
	    for (i = start_block; i < last_block; i++) {
		    do {
			    err = PFD_Erase(pdev, i);
		    } while (err == FM_TRY_AGAIN);
		    
		    if (err) break;
	    }
    }
    return(err);
}


/*----------------------------------------------------------------------*/
/*  Functions for Logical Device Management                             */
/*----------------------------------------------------------------------*/

extern LFDEV *
LFD_AllocLogicalDevice(UINT32 dev_id)
{
    UINT16 i;
    
    /* search for an unallocated logical device */
    for (i = 0; i < MAX_FLASH_DEVICES; i++) {
        if (LDev[i].DevID == INVALID_DEVICE_ID) break;
    }
    
    /* all logical devices are allocated? */
    if (i == MAX_FLASH_DEVICES) return(NULL);
    
    /* ok, allocate a logical device and initialize it */
    MEMSET((void *)&LDev[i], 0, sizeof(LFDEV));
    LDev[i].DevID = dev_id;
    LDevCount++;
    
    return(&LDev[i]);
}


extern void     
LFD_FreeLogicalDevice(UINT32 dev_id)
{
    UINT16 i;
    
    /* search for the logical device referenced by the given dev_id */
    for (i = 0; i < MAX_FLASH_DEVICES; i++) {
        if (LDev[i].DevID == dev_id) {
            LDev[i].DevID = INVALID_DEVICE_ID;
            LDevCount--;
            break;
        }
    }
}


extern LFDEV *
LFD_GetLogicalDevice(UINT32 dev_id)
{
    register UINT16 i;
    
    /* search for the logical device referenced by the given dev_id */
    for (i = 0; i < MAX_FLASH_DEVICES; i++) {
        if (LDev[i].DevID == dev_id) return(&LDev[i]);
    }
    
    return(NULL);
}


/*----------------------------------------------------------------------*/
/*  Functions for Customized Device APIs                                */
/*----------------------------------------------------------------------*/

#if USE_CUSTOM_DEV_SPEC

extern INT32  
LFD_ReadPage(LFDEV *ldev, UINT32 block, UINT16 page,
             UINT16 sector_offset, UINT16 num_sectors,
             UINT8 *dbuf, UINT8 *sbuf)
{
    INT32  err;
    UINT16 sectors_per_page;
    UINT32 org_block;

    PFDEV *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;

    if (ldev->CustomDevSpecType != DEV_SPEC_EMUL_SMALL_MULTI) {
        /* currently the only supported custom device is 
           the emulated multi-planed small block flash */
        return(FM_ERROR);
    }

    /* calculate the original block number */
    sectors_per_page = ldev->DevSpec.SectorsPerPage;
    org_block = block >> BITS[sectors_per_page];
    
    /* get the partition information for this logical device;
       the block number is also adjusted as a relative value 
       in the corresponding partition */
    part_info = LFD_GetPartitionInfo(ldev, &org_block);
    pdev = part_info->PDev;
    part = part_info->Part;
    
#if STRICT_CHECK
    /* check if the block and page numbers are within the valid ranges */
    if (org_block >= part->NumBlocks) return(FM_ILLEGAL_ACCESS);
    if (page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
    if ((sector_offset + num_sectors) > 1)
        return(FM_ILLEGAL_ACCESS);
#endif
    
    /* get the block number in the chip-wide address space */
    org_block += part->StartBlock;
    
    /* re-calculate the sector offset */
    sector_offset = (UINT16)(block & (sectors_per_page - 1));

    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_ReadPage(pdev, org_block, page, 
                           sector_offset, num_sectors, dbuf, sbuf);
    } while (err == FM_TRY_AGAIN);

#if (CFG_HWECC_PIPELINE == HWECC_PIPELINE_UFD)
    if ( !err && (err = hwecc_ecc_decode_sync()) ) { 
	    switch( err ) { 
	    case HWECC_DMA_ERROR:
		    printk("HWECC DMA Error\n"); 
	    case HWECC_UNCORR_ERROR:
		    printk("%s: Uncorrectable ECC Error\n", __FUNCTION__);
		    printk("chip(%d),block(%d),page(%d),offset(%d),nsect(%d)\n", 
			   prev_ecc.chip, prev_ecc.block, prev_ecc.page, 
			   prev_ecc.offset, prev_ecc.nsect); 
		    lld_stat.uncorr_ecc_err++;
		    err = FM_ECC_ERROR; 
		    break;
	    case HWECC_CORR_ERROR:
		    lld_stat.corr_ecc_err++;
	    default: 
		    err = FM_SUCCESS;
		    break;
	    }
    }
#endif /* CFG_HWECC_PIPELINE */ 

    return(err);
}


extern INT32 
LFD_ReadPageGroup(LFDEV *ldev, UINT32 group, UINT16 page,
                  UINT8 *dbuf_group[], UINT8 *sbuf_group[], INT32 *flag)
{
    INT32  err = FM_SUCCESS, *flag2, f[MAX_PHYSICAL_PLANES];
    UINT32 block, org_block, b[MAX_PHYSICAL_PLANES];
    UINT16 i, j, k, n;
    UINT16 planes_per_part, sectors_per_page, sector_offset;
    UINT8  **dbuf_group2, **sbuf_group2;
    UINT8  *d[MAX_PHYSICAL_PLANES], *s[MAX_PHYSICAL_PLANES];
    BOOL   multi_ok;

    PFDEV  *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;
    
    if (ldev->CustomDevSpecType != DEV_SPEC_EMUL_SMALL_MULTI) {
        /* currently the only supported custom device is 
           the emulated multi-planed small block flash */
        return(FM_ERROR);
    }

#if STRICT_CHECK
    /* check if the group and page numbers are within the valid ranges */
    if (group >= (ldev->CustomDevSpec.NumBlocks / ldev->CustomDevSpec.NumPlanes))
        return(FM_ILLEGAL_ACCESS);
    if (page >= ldev->CustomDevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
#endif

    /* calculate the first block number in this group */
    org_block = group << BITS[ldev->DevSpec.NumPlanes];
    
    /* calculate some important constants */
    planes_per_part = ldev->DevSpec.NumPlanes >> BITS[ldev->InterleavingLevel];
    sectors_per_page = ldev->DevSpec.SectorsPerPage;

    /* perform the corresponding group operation for each partition */
    for (i = 0; i < ldev->InterleavingLevel; i++) {

        /* calculate the block number in each partition */
        block = org_block + (i << BITS[planes_per_part]);

        /* get the information about the partition where this block exists;
           the block number is also adjusted as a relative value 
           in the corresponding partition */
        part_info = LFD_GetPartitionInfo(ldev, &block);
        pdev = part_info->PDev;
        part = part_info->Part;
        
        /* check if we can perform the corresponding multi-plane operation */
        multi_ok = TRUE;
        sector_offset = (i << BITS[planes_per_part]) << BITS[sectors_per_page];
        flag2 = &flag[sector_offset];
        dbuf_group2 = &dbuf_group[sector_offset];
        sbuf_group2 = &sbuf_group[sector_offset];
        

        for (j = 0, n = 0; j < planes_per_part; j++) {

            /* set up the buffer pointers for the multi-plane operation */
            d[j] = dbuf_group2[n];
            s[j] = sbuf_group2[n];
            f[j] = 0;

            for (k = 0; k < sectors_per_page; k++, n++) {

                /* all sectors are involved in the operation? */
                if (flag2[n]) f[j]++;
                
                /* the buffers are linear? */
                if (k > 0 && (dbuf_group2[n] - dbuf_group2[n-1]) != 512) {
                    multi_ok = FALSE;
                }
            }

            /* check if the multi-plane operation can be performed */
            if (f[j] == sectors_per_page) f[j] = 1;
            else {
                if (f[j] != 0) multi_ok = FALSE;
            }
        }

        if (multi_ok) {
            /* ok, perform the multi-plane operation */
            do {
                /* calculate the block numbers in the chip-wide address space;
                   because the block numbers can be modified in the physical 
                   layer, we should re-calculate those numbers for every loop 
                   iteration */
                b[0] = block + part->StartBlock;
                for (j = 1; j < planes_per_part; j++) {
                    b[j] = b[0] + j;
                }
                
                /* read page group */
                err = PFD_ReadPageGroup(pdev, b, page, d, s, f);
            } while (err == FM_TRY_AGAIN);
            
            /* save the results */
            for (j = 0, n = 0; j < planes_per_part; j++) {
                for (k = 0; k < sectors_per_page; k++, n++) {
                    if (flag2[n]) {
                        flag2[n] = f[j];
                    }
                }
            }
        }
        else {
            /* we should perform multiple single-plane operations */
            
            /* calculate the block number in the chip-wide address space */
            block += part->StartBlock;
            
            /* perform single-plane operations */
            for (j = 0; j < planes_per_part; j++) {
                if (f[j]) {
                    err = read_sector_group(ldev, pdev, block+j, page,
                              dbuf_group2 + (j << BITS[sectors_per_page]),
                              sbuf_group2 + (j << BITS[sectors_per_page]),
                              flag2 + (j << BITS[sectors_per_page]));
                    if (err) return(err);
                }
            }
        }
        if (err) break;
    }

#if (CFG_HWECC_PIPELINE == HWECC_PIPELINE_UFD)
    if ( !err && (err = hwecc_ecc_decode_sync()) ) { 
	    switch( err ) { 
	    case HWECC_DMA_ERROR:
		    printk("HWECC DMA Error\n"); 
	    case HWECC_UNCORR_ERROR:
		    printk("%s: Uncorrectable ECC Error\n", __FUNCTION__);
		    printk("chip(%d),block(%d),page(%d),offset(%d),nsect(%d)\n", 
			   prev_ecc.chip, prev_ecc.block, prev_ecc.page, 
			   prev_ecc.offset, prev_ecc.nsect); 
		    lld_stat.uncorr_ecc_err++;
		    err = FM_ECC_ERROR; 
		    break;
	    case HWECC_CORR_ERROR:
		    lld_stat.corr_ecc_err++;
	    default: 
		    err = FM_SUCCESS;
		    break;
	    }
    }
#endif 

    return(err);
}


extern INT32  
LFD_WritePage(LFDEV *ldev, UINT32 block, UINT16 page,
              UINT16 sector_offset, UINT16 num_sectors,
              UINT8 *dbuf, UINT8 *sbuf, BOOL is_last)
{
    INT32  err;
    UINT16 sectors_per_page;
    UINT32 org_block;

    PFDEV *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;

    if (ldev->CustomDevSpecType != DEV_SPEC_EMUL_SMALL_MULTI) {
        /* currently the only supported custom device is 
           the emulated multi-planed small block flash */
        return(FM_ERROR);
    }
    
    /* calculate the original block number */
    sectors_per_page = ldev->DevSpec.SectorsPerPage;
    org_block = block >> BITS[sectors_per_page];

    /* get the partition information for this logical device;
       the block number is also adjusted as a relative value 
       in the corresponding partition */
    part_info = LFD_GetPartitionInfo(ldev, &org_block);
    pdev = part_info->PDev;
    part = part_info->Part;

#if STRICT_CHECK
    /* check if the block and page numbers are within the valid ranges */
    if (org_block >= part->NumBlocks) return(FM_ILLEGAL_ACCESS);
    if (page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
    if ((sector_offset + num_sectors) > 1) 
        return(FM_ILLEGAL_ACCESS);
#endif

    /* check if this logical device is write protected */
    if (part->Protected) return(FM_PROTECT_ERROR);

    /* get the block number in chip-wide address space */
    org_block += part->StartBlock;

    /* re-calculate the sector offset */
    sector_offset = (UINT16)(block & (sectors_per_page - 1));

    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_WritePage(pdev, org_block, page, 
                            sector_offset, num_sectors, dbuf, sbuf, is_last);
    } while (err == FM_TRY_AGAIN);

    return(err);
}


extern INT32  
LFD_WritePageGroup(LFDEV *ldev, UINT32 group, UINT16 page,
                   UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
                   INT32 *flag, BOOL is_last)
{
    INT32  err = FM_SUCCESS, *flag2, f[MAX_PHYSICAL_PLANES];
    UINT32 block, org_block, b[MAX_PHYSICAL_PLANES];
    UINT16 i, j, k, n;
    UINT16 planes_per_part, sectors_per_page, sector_offset;
    UINT8  **dbuf_group2, **sbuf_group2;
    UINT8  *d[MAX_PHYSICAL_PLANES], *s[MAX_PHYSICAL_PLANES];
    BOOL   multi_ok;

    PFDEV  *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;

    if (ldev->CustomDevSpecType != DEV_SPEC_EMUL_SMALL_MULTI) {
        /* currently the only supported custom device is 
           the emulated multi-planed small block flash */
        return(FM_ERROR);
    }

#if STRICT_CHECK
    /* check if the group and page numbers are within the valid ranges */
    if (group >= (ldev->CustomDevSpec.NumBlocks / ldev->CustomDevSpec.NumPlanes)) 
        return(FM_ILLEGAL_ACCESS);
    if (page >= ldev->CustomDevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
#endif

    /* calculate the first block number in this group */
    org_block = group << BITS[ldev->DevSpec.NumPlanes];
    
    /* calculate some important constants */
    planes_per_part = ldev->DevSpec.NumPlanes >> BITS[ldev->InterleavingLevel];
    sectors_per_page = ldev->DevSpec.SectorsPerPage;

    /* perform the corresponding group operation for each partition */
    for (i = 0; i < ldev->InterleavingLevel; i++) {

        /* calculate the block number in each partition */
        block = org_block + (i << BITS[planes_per_part]);

        /* get the information about the partition where this block exists;
           the block number is also adjusted as a relative value 
           in the corresponding partition */
        part_info = LFD_GetPartitionInfo(ldev, &block);
        pdev = part_info->PDev;
        part = part_info->Part;
        
        /* check if this logical device is write protected */
        if (part->Protected) return(FM_PROTECT_ERROR);
        
        /* check if we can perform the corresponding multi-plane operation */
        multi_ok = TRUE;
        sector_offset = (i << BITS[planes_per_part]) << BITS[sectors_per_page];
        flag2 = &flag[sector_offset];
        dbuf_group2 = &dbuf_group[sector_offset];
        sbuf_group2 = &sbuf_group[sector_offset];
        
        for (j = 0, n = 0; j < planes_per_part; j++) {

            /* set up the buffer pointers for the multi-plane operation */
            d[j] = dbuf_group2[n];
            s[j] = sbuf_group2[n];
            f[j] = 0;

            for (k = 0; k < sectors_per_page; k++, n++) {

                /* all sectors are involved in the operation? */
                if (flag2[n]) f[j]++;
                
                /* the buffers are linear? */
                if (k > 0 && (dbuf_group2[n] - dbuf_group2[n-1]) != 512) {
                    multi_ok = FALSE;
                }
            }
            
            /* check if the multi-plane operation can be performed */
            if (f[j] == sectors_per_page) f[j] = 1;
            else {
                if (f[j] != 0) multi_ok = FALSE;
            }
        }

        if (multi_ok) {
            /* ok, perform the multi-plane operation */
            do {
                /* calculate the block numbers in the chip-wide address space;
                   because the block numbers can be modified in the physical 
                   layer, we should re-calculate those numbers for every loop 
                   iteration */
                b[0] = block + part->StartBlock;
                for (j = 1; j < planes_per_part; j++) {
                    b[j] = b[0] + j;
                }
                
                /* write page group */
                err = PFD_WritePageGroup(pdev, b, page, d, s, f, is_last);
            } while (err == FM_TRY_AGAIN);
            
            /* save the results */
            for (j = 0, n = 0; j < planes_per_part; j++) {
                for (k = 0; k < sectors_per_page; k++, n++) {
                    if (flag2[n]) {
                        flag2[n] = f[j];
                    }
                }
            }
        }
        else {
            /* we should perform multiple single-plane operations */
            
            /* calculate the block number in the chip-wide address space */
            block += part->StartBlock;
            
            /* perform single-plane operations */
            for (j = 0; j < planes_per_part; j++) {
                if (f[j]) {
                    err = write_sector_group(ldev, pdev, block+j, page,
                              dbuf_group2 + (j << BITS[sectors_per_page]),
                              sbuf_group2 + (j << BITS[sectors_per_page]),
                              flag2 + (j << BITS[sectors_per_page]), is_last);
                    if (err) return(err);
                }
            }
        }
    }

    return(err);
}


extern INT32  
LFD_CopyBack(LFDEV *ldev, UINT32 src_block, UINT16 src_page, 
             UINT32 dest_block, UINT16 dest_page)
{
    if (ldev->CustomDevSpecType != DEV_SPEC_EMUL_SMALL_MULTI) {
        /* currently the only supported custom device is 
           the emulated multi-planed small block flash */
        return(FM_ERROR);
    }
    
    /* copy-back in sector units is impossible */
    return(FM_ILLEGAL_ACCESS);
}


extern INT32  
LFD_CopyBackGroup(LFDEV *ldev, UINT32 src_group, UINT16 src_page, 
                  UINT32 dest_group, UINT16 dest_page, INT32 *flag)
{
    INT32  err = FM_SUCCESS, *flag2, f[MAX_PHYSICAL_PLANES];
    UINT8  *pbuf;
    UINT16 planes_per_part, sectors_per_page, sector_offset;
    UINT16 i, j, k, n, src_p[MAX_PHYSICAL_PLANES];
    UINT32 org_src_block, org_dest_block;
    UINT32 src_block, dest_block;
    UINT32 src_b[MAX_PHYSICAL_PLANES], dest_b[MAX_PHYSICAL_PLANES];
    BOOL   multi_ok, copyback_ok[MAX_PHYSICAL_PLANES];

    PFDEV       *src_pdev = NULL, *dest_pdev = NULL;
    FLASH_PART  *src_part, *dest_part;
    FLASH_RPART *part_info;

    if (ldev->CustomDevSpecType != DEV_SPEC_EMUL_SMALL_MULTI) {
        /* currently the only supported custom device is 
           the emulated multi-planed small block flash */
        return(FM_ERROR);
    }

#if STRICT_CHECK
    /* check if the group and page numbers are within the valid ranges */
    if (src_group >= (ldev->CustomDevSpec.NumBlocks / ldev->CustomDevSpec.NumPlanes)) 
        return(FM_ILLEGAL_ACCESS);
    if (dest_group >= (ldev->CustomDevSpec.NumBlocks / ldev->CustomDevSpec.NumPlanes)) 
        return(FM_ILLEGAL_ACCESS);
    if (src_page >= ldev->CustomDevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
    if (dest_page >= ldev->CustomDevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
#endif

    /* calculate the first block number in each group */
    org_src_block = src_group << BITS[ldev->DevSpec.NumPlanes];
    org_dest_block = dest_group << BITS[ldev->DevSpec.NumPlanes];

    /* calculate some important constants */
    planes_per_part = ldev->DevSpec.NumPlanes >> BITS[ldev->InterleavingLevel];
    sectors_per_page = ldev->DevSpec.SectorsPerPage;

    /* perform the corresponding group operation for each partition */
    for (i = 0; i < ldev->InterleavingLevel; i++) {

        /* calculate the block number in each partition */
        src_block = org_src_block + (i << BITS[planes_per_part]);
        dest_block = org_dest_block + (i << BITS[planes_per_part]);

        /* get the information about the partitions where these blocks exist;
           the block numbers are also adjusted as a relative value 
           in the corresponding partition */
        part_info = LFD_GetPartitionInfo(ldev, &src_block);
        src_pdev  = part_info->PDev;
        src_part  = part_info->Part;

        part_info = LFD_GetPartitionInfo(ldev, &dest_block);
        dest_pdev = part_info->PDev;
        dest_part = part_info->Part;

        /* check if this logical device is write protected */
        if (dest_part->Protected) return(FM_PROTECT_ERROR);

        /* determine how to implement the copy-back operation;
           1) multi-plane copy-back, or
           2) single-plane copy-back, or
           3) read & write */
        multi_ok = TRUE;
        sector_offset = (i << BITS[planes_per_part]) << BITS[sectors_per_page];
        flag2 = &flag[sector_offset];
        
        for (j = 0, n = 0; j < planes_per_part; j++) {
            f[j] = 0;
            copyback_ok[j] = TRUE;

            for (k = 0; k < sectors_per_page; k++, n++) {
                if (flag2[n]) f[j]++;
            }
            
            /* check if copy-back can be performed on each plane */
            if (f[j] == sectors_per_page) f[j] = 1;
            else {
                copyback_ok[j] = FALSE;
                if (f[j] != 0) multi_ok = FALSE;
            }

            /* check if the src & dest blocks are on the same chip */
            if (src_pdev != dest_pdev) {
                copyback_ok[j] = FALSE;
                multi_ok = FALSE;
            }

            src_p[j] = src_page;
        }

        if (multi_ok) {
            /* ok, perform the multi-plane operation */
            do {
                /* calculate the block numbers in the chip-wide address space;
                   because the block numbers can be modified in the physical 
                   layer, we should re-calculate those numbers for every loop 
                   iteration */
                src_b[0] = src_block + src_part->StartBlock;
                dest_b[0] = dest_block + dest_part->StartBlock;
                for (j = 1; j < planes_per_part; j++) {
                    src_b[j] = src_b[0] + j;
                    dest_b[j] = dest_b[0] + j;
                }
                
                /* read page group */
                err = PFD_CopyBackGroup(src_pdev, src_b, src_p, 
                                        dest_b, dest_page, f);
            } while (err == FM_TRY_AGAIN);
            
            /* save the results */
            for (j = 0, n = 0; j < planes_per_part; j++) {
                for (k = 0; k < sectors_per_page; k++, n++) {
                    if (flag2[n]) {
                        flag2[n] = f[j];
                    }
                }
            }
        }
        else {
            /* we should perform multiple single-plane operations */

            /* calculate the block number in the chip-wide address space */
            src_block += src_part->StartBlock;
            dest_block += dest_part->StartBlock;

            for (j=0, n=0; j < planes_per_part; j++, n+=sectors_per_page) {
                if (f[j] == 0) continue;

                if (copyback_ok[j]) {
                    do {
                        err = PFD_CopyBack(src_pdev, src_block+j, src_page,
                                           dest_block+j, dest_page);
                    } while (err == FM_TRY_AGAIN);
                }
                else {
                    /* copy-back operation cannot be performed;
                       instead, implement 'copy-back' using read & write */
            
                    /* get the current I/O buffer */
                    pbuf = PFD_GetBuffer(dest_pdev, OP_READ);
            
                    /* read the source page */
                    do {
                        err = PFD_ReadPage(src_pdev, src_block+j, src_page, 
                                           0, sectors_per_page, pbuf, 
                                           pbuf + ldev->DevSpec.DataSize);
                    } while (err == FM_TRY_AGAIN);
                    
                    if (err) goto end;
            
                    /* fill the sector buffers with 0xff that are not copied */
                    for (k = 0; k < sectors_per_page; k++) {
                        if (!flag2[n + k]) {
                            MEMSET(pbuf+512*k, 0xff, 512);
                            MEMSET(pbuf+ldev->DevSpec.DataSize+16*k, 0xff, 16);
                        }
                    }
            
                    /* write data into the destination page */
                    do {
                        err = PFD_WritePage(dest_pdev, dest_block+j, dest_page,
                                            0, sectors_per_page, 
                                            pbuf, pbuf+ldev->DevSpec.DataSize,
                                            TRUE);
                    } while (err == FM_TRY_AGAIN);
                }
            
end:
                for (k = 0; k < sectors_per_page; k++) {
                    if (flag2[n + k]) flag2[n + k] = err;
                }
            }
        }
    }

    return(err);
}


extern INT32  
LFD_Erase(LFDEV *ldev, UINT32 block)
{
    if (ldev->CustomDevSpecType != DEV_SPEC_EMUL_SMALL_MULTI) {
        /* currently the only supported custom device is 
           the emulated multi-planed small block flash */
        return(FM_ERROR);
    }
    
    /* erase in sub-block units is impossible */
    return(FM_ILLEGAL_ACCESS);
}


extern INT32  
LFD_EraseGroup(LFDEV *ldev, UINT32 group, INT32 *flag)
{
    INT32  err = FM_SUCCESS, *flag2, f[MAX_PHYSICAL_PLANES];
    UINT32 block, org_block, b[MAX_PHYSICAL_PLANES];
    UINT16 i, j, k, n, planes_per_part, sectors_per_page;
    BOOL   multi_ok;

    PFDEV  *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;

    if (ldev->CustomDevSpecType != DEV_SPEC_EMUL_SMALL_MULTI) {
        /* currently the only supported custom device is 
           the emulated multi-planed small block flash */
        return(FM_ERROR);
    }

#if STRICT_CHECK
    /* check if the group number is within the valid range */
    if (group >= (ldev->CustomDevSpec.NumBlocks / ldev->CustomDevSpec.NumPlanes))
        return(FM_ILLEGAL_ACCESS);
#endif

    /* calculate the first block number in this group */
    org_block = group << BITS[ldev->DevSpec.NumPlanes];
    
    /* calculate some important constants */
    planes_per_part = ldev->DevSpec.NumPlanes >> BITS[ldev->InterleavingLevel];
    sectors_per_page = ldev->DevSpec.SectorsPerPage;

    /* perform the corresponding group operation for each partition */
    for (i = 0; i < ldev->InterleavingLevel; i++) {

        /* calculate the block number in each partition */
        block = org_block + (i << BITS[planes_per_part]);

        /* get the information about the partition where this block exists;
           the block number is also adjusted as a relative value 
           in the corresponding partition */
        part_info = LFD_GetPartitionInfo(ldev, &block);
        pdev = part_info->PDev;
        part = part_info->Part;
        
        /* check if this logical device is write protected */
        if (part->Protected) return(FM_PROTECT_ERROR);
        
        /* check if we can perform the corresponding multi-plane operation */
        multi_ok = TRUE;
        flag2 = &flag[(i << BITS[planes_per_part]) << BITS[sectors_per_page]];
        
        for (j = 0, n = 0; j < planes_per_part; j++) {
            f[j] = 0;

            /* all blocks are involved in the operation? */
            for (k = 0; k < sectors_per_page; k++, n++) {
                if (flag2[n]) f[j]++;
            }

            /* check if the multi-plane operation can be performed */
            if (f[j] == sectors_per_page) f[j] = 1;
            else {
                if (f[j] != 0) multi_ok = FALSE;
            }
        }

        /* check if we can perform the multi-plane operation */
        if (multi_ok) {
            do {
                /* calculate the block numbers in the chip-wide address space;
                   because the block numbers can be modified in the physical 
                   layer, we should re-calculate those numbers for every loop 
                   iteration */
                b[0] = block + part->StartBlock;
                for (j = 1; j < planes_per_part; j++) {
                    b[j] = b[0] + j;
                }
                
                /* erase block group */
                err = PFD_EraseGroup(pdev, b, f);
            } while (err == FM_TRY_AGAIN);

            /* save the results */
            for (j = 0, n = 0; j < planes_per_part; j++) {
                for (k = 0; k < sectors_per_page; k++, n++) {
                    if (flag2[n]) {
                        flag2[n] = f[j];
                    }
                }
            }
        }
        else {
            /* we should perform multiple single-plane operations */

            /* calculate the block number in the chip-wide address space */
            block += part->StartBlock;
            
            /* perform single-plane operations */
            for (j = 0, n = 0; j < planes_per_part; j++) {
                if (f[j]) {
                    do {
                        err = PFD_Erase(pdev, block+j);
                    } while (err == FM_TRY_AGAIN);
                    
                    for (k = 0; k < sectors_per_page; k++, n++) {
                        if (flag2[n]) {
                            flag2[n] = err;
                        }
                    }
                }
            }
        }
    }

    return(err);
}

#endif /* USE_CUSTOM_DEV_SPEC */


/*----------------------------------------------------------------------*/
/*  Miscellaneous Functions                                             */
/*----------------------------------------------------------------------*/

extern FLASH_RPART *
LFD_GetPartitionInfo(LFDEV *ldev, UINT32 *block)
{
    UINT16 planes_per_part;
    UINT32 blocks_per_part, blocks_per_partgrp;
    UINT32 part_offset, block_offset;
    UINT32 i, org_block = *block, b = *block;
    FLASH_VPART *part_info = &ldev->PartInfo;
    FLASH_RPART *part = NULL;

    if (ldev->InterleavingLevel == 1) {
        /* non-interleaved case; 
           each partition may have a different number of blocks */

        for (i = 0; i < part_info->NumPartitions; i++) {
            if (part_info->Parts[i].Part->NumBlocks > b) {
                part = &part_info->Parts[i];
                *block = b;
                break;
            }
            b -= part_info->Parts[i].Part->NumBlocks;
        }
    }
    else {
        /* interleaved case;
           each partition must have the same number of blocks */

        /* calculate some constants */
        planes_per_part = part_info->Parts[0].PDev->DevSpec.NumPlanes;
        blocks_per_part = part_info->Parts[0].Part->NumBlocks;
        blocks_per_partgrp = blocks_per_part << BITS[ldev->InterleavingLevel];
        block_offset = 0;
    
        /* determine the interleaved group of partitions
           that the given block belongs to */
        for (part_offset = 0; ; part_offset += ldev->InterleavingLevel) {
            block_offset += blocks_per_partgrp;
            if (block_offset > b) break;
        }
        block_offset -= blocks_per_partgrp;
        
        /* in the found group of partitions, identify the exact partition */
        i = (b & (ldev->DevSpec.NumPlanes - 1)) >> BITS[planes_per_part];
        part = &part_info->Parts[part_offset + i];
    
        /* re-calculate the block number in the corresponding partition */
        b -= (block_offset + (i << BITS[planes_per_part]));
        i = (b & ~((UINT32)(planes_per_part - 1))) >> BITS[ldev->InterleavingLevel];
        *block = i + (b & (planes_per_part - 1));
    }

//#if STRICT_CHECK
#if 1
    if (part == NULL) {
        PRINT("\n");
        PRINT("%s : no proper partition for block %d\n", __FUNCTION__, org_block);
        PRINT("\n");
    }
#endif

    return(part);
}


/*======================================================================*/
/*  Local Function Definitions                                          */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/*  Multi-sector Flash Memory Access Functions                          */
/*----------------------------------------------------------------------*/

static INT32 
read_sector_group(LFDEV *ldev, PFDEV *pdev, UINT32 block, UINT16 page,
                  UINT8 *dbuf_group[], UINT8 *sbuf_group[], INT32 *flag)
{
    INT32   err = FM_SUCCESS;
    UINT16  i, j, sectors_per_page, sector_offset = 0;

#if COPY_TO_LINEAR_BUFFER
    BOOL    is_linear = TRUE;
    UINT8   *dbuf, *sbuf;
    UINT16  last_sector = 0, num_sectors = 0;
#else
    BOOL    read_flag = FALSE;
#endif

    /* number of sectors per page */
    sectors_per_page = ldev->DevSpec.SectorsPerPage;

    /* read sectors from the given page */
#if COPY_TO_LINEAR_BUFFER

    /* check the linearity of buffers */
    for (i = sectors_per_page - 1, j = 0xff; ; i--) {
        if (flag[i]) {
            num_sectors++;
            if (last_sector == 0) last_sector = i;
            if (j < 0xff) {
                if (j - i != 1) is_linear = FALSE;
                if ((dbuf_group[j] - dbuf_group[i]) != 512) is_linear = FALSE;
            }
            j = i;
        }
        if (i == 0) break;
    }
    sector_offset = j;

    /* buffers for each of the sectors are contiguous? */
    if (is_linear) {
        dbuf = dbuf_group[sector_offset];
        sbuf = sbuf_group[sector_offset];
    }
    else {
        /* buffers are not adjacent; 
           re-calculate the number of sectors to read */
        num_sectors = last_sector - sector_offset + 1;

        /* get the current I/O buffer */
        dbuf = PFD_GetBuffer(pdev, OP_READ);
        sbuf = dbuf + MAX_DATA_SIZE;
    }

    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_ReadPage(pdev, block, page, 
                           sector_offset, num_sectors,
                           dbuf, sbuf);
    } while (err == FM_TRY_AGAIN);
    
    for (i = sector_offset; i < sector_offset + num_sectors; i++) {
        if (flag[i]) {
            if (!is_linear && !err) {
                MEMCPY(dbuf_group[i], dbuf + 512*(i - sector_offset), 512);
                if (sbuf_group[i] != NULL) {
                    MEMCPY(sbuf_group[i], sbuf + 16*(i - sector_offset), 16);
                }
            }
            flag[i] = err;
        }
    }

#else /* COPY_TO_LINEAR_BUFFER == 0 */

    for (i = 0; i < sectors_per_page + 1; i++) {
        if (i < sectors_per_page && flag[i]) {
            if (!read_flag) {
                read_flag = TRUE;
                sector_offset = i;
            }
            else {
                if ((dbuf_group[i] - dbuf_group[i-1]) != 512) {
                    /* sector buffers are not continuous; read separately */
                    goto read_now;
                }
            }
        }
        else {
            if (read_flag) {
                read_flag = FALSE;
read_now:
                /* perform the physical device operation 
                   until it succeeds or fails */
                do {
                    err = PFD_ReadPage(pdev, block, page, sector_offset, 
                                       (UINT16)(i - sector_offset),
                                       dbuf_group[sector_offset],
                                       sbuf_group[sector_offset]);
                } while (err == FM_TRY_AGAIN);
                
                for (j = sector_offset; j < i; j++) flag[j] = err;
                if (err) return(err);
                
                /* read was committed due to buffer discontinuity? */
                if (read_flag) {
                    sector_offset = i;
                }
            }
        }
    }

#endif /* COPY_TO_LINEAR_BUFFER */

    return(err);
}


static INT32
write_sector_group(LFDEV *ldev, PFDEV *pdev, UINT32 block, UINT16 page,
                   UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
                   INT32 *flag, BOOL is_last)
{
    INT32   err = FM_SUCCESS;
    UINT16  i, j, sectors_per_page, sector_offset = 0;
    
#if COPY_TO_LINEAR_BUFFER
    BOOL    is_linear = TRUE;
    UINT8   *dbuf, *sbuf;
    UINT16  last_sector = 0, num_sectors = 0;
#else
    BOOL    write_flag = FALSE;
#endif

    /* number of sectors per page */
    sectors_per_page = ldev->DevSpec.SectorsPerPage;
    
    /* write sectors to the given page */
#if COPY_TO_LINEAR_BUFFER

    /* check the linearity of buffers */
    for (i = sectors_per_page - 1, j = 0xff; ; i--) {
        if (flag[i]) {
            num_sectors++;
            if (last_sector == 0) last_sector = i;
            if (j < 0xff) {
                if (j - i != 1) is_linear = FALSE;
                if ((dbuf_group[j] - dbuf_group[i]) != 512) is_linear = FALSE;
            }
            j = i;
        }
        if (i == 0) break;
    }
    sector_offset = j;

    /* buffers for each of four sectors are contiguous? */
    if (is_linear) {
        dbuf = dbuf_group[sector_offset];
        sbuf = sbuf_group[sector_offset];
    }
    else {
        /* buffers are not adjacent; 
           re-calculate the number of sectors to write */
        num_sectors = last_sector - sector_offset + 1;

        /* get the current I/O buffer */
        dbuf = PFD_GetBuffer(pdev, OP_WRITE);
        sbuf = dbuf + MAX_DATA_SIZE;

        for (i = sector_offset; i < sector_offset + num_sectors; i++) {
            if (flag[i]) {
                MEMCPY(dbuf + 512*(i - sector_offset), dbuf_group[i], 512);
                if (sbuf_group[i] != NULL) {
                    MEMCPY(sbuf + 16*(i - sector_offset), sbuf_group[i], 16);
                }
                else {
                    MEMSET(sbuf + 16*(i - sector_offset), 0xff, 16);
                }
            }
            else {
                MEMSET(dbuf + 512*(i - sector_offset), 0xff, 512);
                MEMSET(sbuf +  16*(i - sector_offset), 0xff,  16);
            }
        }
    }

    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_WritePage(pdev, block, page, sector_offset, num_sectors,
                            dbuf, sbuf, is_last);
    } while (err == FM_TRY_AGAIN);

    for (i = sector_offset; i < sector_offset + num_sectors; i++) {
        if (flag[i]) flag[i] = err;
    }

#else /* COPY_TO_LINEAR_BUFFER == 0 */

    for (i = 0; i < sectors_per_page + 1; i++) {
        if (i < sectors_per_page && flag[i]) {
            if (!write_flag) {
                write_flag = TRUE;
                sector_offset = i;
            }
            else {
                if ((dbuf_group[i] - dbuf_group[i-1]) != 512) {
                    /* sector buffers are not continuous; write separately */
                    goto write_now;
                }
            }
        }
        else {
            if (write_flag) {
                write_flag = FALSE;
write_now:
                /* perform the physical device operation 
                   until it succeeds or fails */
                do {
                    err = PFD_WritePage(pdev, block, page, sector_offset, 
                                        (UINT16)(i - sector_offset),
                                        dbuf_group[sector_offset], 
                                        sbuf_group[sector_offset], 
                                        is_last);
                } while (err == FM_TRY_AGAIN);

                for (j = sector_offset; j < i; j++) flag[j] = err;
                if (err) return(err);

                /* write was committed due to buffer discontinuity? */
                if (write_flag) {
                    sector_offset = i;
                }
            }
        }
    }

#endif /* COPY_TO_LINEAR_BUFFER */

    return(err);
}


/*----------------------------------------------------------------------*/
/*  Miscellaneous Local Functions                                       */
/*----------------------------------------------------------------------*/

static BOOL
is_valid_partition_table(PFDEV *pdev, FLASH_PARTTAB *part_tab)
{
    UINT16 i;
    UINT32 cur_block = 0;
    FLASH_PARTTAB part_table[MAX_FLASH_CHIPS];
    FLASH_VPART dev_table[MAX_FLASH_VPARTITIONS];
    
    if (part_tab->NumPartitions == 0 || 
        part_tab->NumPartitions > MAX_FLASH_PARTITIONS)
        return(FALSE);
    
    for (i = 0; i < part_tab->NumPartitions; i++) {
        
        /* a partition should contain at least one block */
        if (part_tab->Part[i].NumBlocks == 0) return(FALSE);
        
        /* partitions should not overlap with each other */
        if (part_tab->Part[i].StartBlock < cur_block) return(FALSE);
#if 0         
        /* check if first block of each partition is multi-plane aligned */
        cur_block = part_tab->Part[i].StartBlock;
        if ((cur_block % pdev->DevSpec.NumPlanes) != 0) return(FALSE);

        /* check if last block of each partition is multi-plane aligned */
        cur_block += part_tab->Part[i].NumBlocks;
        if (cur_block > pdev->ActualNumBlocks) return(FALSE);
        if ((cur_block % pdev->DevSpec.NumPlanes) != 0) return(FALSE);

#else /* ugly fix - hcyun */ 
	if ( (part_tab->Part[i].DevClass == LFD_BLK_DEVICE_FTL) && 
	     (part_tab->Part[i].StartBlock % pdev->DevSpec.NumPlanes) != 0 ) {
		printk("[WARNING] StartBlock(%d) must be multi-plane(%d) aligned\n", 
		       part_tab->Part[i].StartBlock, pdev->DevSpec.NumPlanes); 
		part_tab->Part[i].StartBlock = 
			((part_tab->Part[i].StartBlock / pdev->DevSpec.NumPlanes) + 1)
			* pdev->DevSpec.NumPlanes; 
		printk(" Corrected : Part[%d].StartBlock = %d\n", i, part_tab->Part[i].StartBlock); 
	}
	     
	if ( (part_tab->Part[i].NumBlocks % pdev->DevSpec.NumPlanes) != 0 ) { 
		printk("[WARNING] NumBlock(%d) must be multi-plane(%d) aligned\n", 
		       part_tab->Part[i].NumBlocks, pdev->DevSpec.NumPlanes);
		part_tab->Part[i].NumBlocks -= 
			(part_tab->Part[i].NumBlocks % pdev->DevSpec.NumPlanes);
		printk(" Corrected : Part[%d].NumBlocks = %d\n", i, part_tab->Part[i].NumBlocks); 
	}
#endif         

    }
    
    /* check if the info about the block device tables is correctly given */
    MEMCPY((void *)part_table, (void *)PartTable, sizeof(FLASH_PARTTAB) * MAX_FLASH_CHIPS);
    MEMCPY(&part_table[pdev->ChipID], part_tab, sizeof(FLASH_PARTTAB));
    if (make_blk_dev_table(part_table, dev_table, LFD_BLK_DEVICE_RAW) != FM_SUCCESS)
        return(FALSE);
    if (make_blk_dev_table(part_table, dev_table, LFD_BLK_DEVICE_FTL) != FM_SUCCESS)
        return(FALSE);
    
    return(TRUE);
}


static INT32
make_blk_dev_table(FLASH_PARTTAB *part_table, FLASH_VPART *dev_table, 
                   UINT16 dev_class)
{
    UINT8  dev_index;
    UINT16 i, j, chip_id;
    
    /* first, initialize the given block device table */
    MEMSET((void *)dev_table, 0, sizeof(FLASH_VPART) * MAX_FLASH_VPARTITIONS);
            
    /* make the device table based on the given partition table */
    for (chip_id = 0; chip_id < PDevCount; chip_id++) {
        for (i = 0; i < part_table[chip_id].NumPartitions; i++) {

            if (part_table[chip_id].Part[i].DevClass == dev_class &&
                part_table[chip_id].Part[i].InDevTable) {

                dev_index = part_table[chip_id].Part[i].DevIndex;
                if (dev_index >= MAX_FLASH_VPARTITIONS) return(FM_ERROR);
                
                j = dev_table[dev_index].NumPartitions;
                if (j >= MAX_FLASH_CHIPS) return(FM_ERROR);
                
                /* register this partition to the list of merged partitions */
                dev_table[dev_index].Parts[j].PDev = &PDev[chip_id];
                dev_table[dev_index].Parts[j].Part = &part_table[chip_id].Part[i];
                dev_table[dev_index].NumPartitions++;
                
                /* merged partitions must be from the same type of chips */
                if (dev_table[dev_index].Parts[j].PDev->DevType !=
                    dev_table[dev_index].Parts[0].PDev->DevType)
                    return(FM_ERROR);
            }
        }
    }
    
    return(FM_SUCCESS);
}


static UINT16 
calc_checksum(UINT16 *data, UINT16 size)
{
    UINT16 checksum = 0;

    for ( ; size > 0; size -= 2, data++) {
        checksum += *data;
    }
    
    return(checksum);
}
