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
/*  FILE    : fd_physical.c                                             */
/*  PURPOSE : Code for Flash Device Physical Interface Layer (PFD)      */
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
#include "fd_bm.h"
#include "fd_physical.h"

#if USE_ONENAND
#include "lld/onenand/fm_driver_o.h"
#endif

#if USE_SMALL_MULTI_NAND
#include "lld/small/fm_driver_sm.h"
#endif

#if USE_LARGE_MULTI_NAND || USE_LARGE_MULTI_MLC_NAND
#include "lld/large/fm_driver_lm.h"
#include "lld/large/fm_driver_lm_mlc.h"
#include "lld/large/fm_driver_lm_mlc8g.h"
#endif

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

/* macro for calculating the plane number of a given block */

#define PFD_PLANE(block)        ((block) & (pdev->DevSpec.NumPlanes - 1))
#define PFD_GROUP(block)        ((block) >> BITS[pdev->DevSpec.NumPlanes])

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  External Variable Definitions                                       */
/*----------------------------------------------------------------------*/

PFDEV   PDev[MAX_FLASH_CHIPS];
UINT16  PDevCount;

BOOL    FD_ECC_Corrected;   /* this variable is assigned TRUE or FALSE value
                               in ecc.c and such value may be referenced

                               in ftl.c to detect 1-bit error correction */


volatile int     lld_debug = 0; 
lld_stat_t       lld_stat;
int              g_use_odd_even_copyback = 0; 
int              g_copy_back_method = HW_COPYBACK; 

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

static BOOL  is_multi_ok(PFDEV *pdev, UINT32 *block, INT32 *flag);
static BOOL  is_copyback_ok(PFDEV *pdev, UINT32 src_block, UINT16 src_page, 
                       UINT32 dest_block, UINT16 dest_page);

static INT32 copy_back(PFDEV *pdev, UINT32 src_block, UINT16 src_page, 
                       UINT32 dest_block, UINT16 dest_page, 
                       BOOL copyback_ok);

static INT32 multi_sw_copy_back(PFDEV *pdev, 
                       UINT32 *src_block, UINT16 *src_page,
                       UINT32 *dest_block, UINT16 dest_page, INT32 *flag);

static INT32 err_recover(PFDEV *pdev, UINT32 block, UINT16 page, INT32 err);

static INT32 recover_from_write_error(PFDEV *pdev, UINT32 block, UINT16 page,
                       UINT16 sector_offset, UINT16 num_sectors,
                       UINT8 *dbuf, UINT8 *sbuf);

static INT32 recover_from_copyback_error(PFDEV *pdev, 
                       UINT32 src_block, UINT16 src_page, 
                       UINT32 dest_block, UINT16 dest_page);

static INT32 recover_from_erase_error(PFDEV *pdev, UINT32 block);


/*======================================================================*/
/*  External Function Definitions                                       */
/*======================================================================*/

extern INT32 
PFD_Init(void)
{
    UINT16 i;
    INT32  err;

    /*--------------------------------------------------------------*/
    /*  first, initialize data structures                           */
    /*--------------------------------------------------------------*/
    
    /* initialize the physical flash device table */
    for (i = 0; i < MAX_FLASH_CHIPS; i++) {
        MEMSET((void *)&PDev[i], 0, sizeof(PFDEV));
        PDev[i].ChipID = i;
    }
    
    PDevCount = 0;
    
    /*--------------------------------------------------------------*/
    /*  second, install physical devices (CUSTOMIZE THIS PART!)     */
    /*--------------------------------------------------------------*/

#if USE_ONENAND
    /* install physical device driver for Samsung OneNAND (64MB) */
    err = FO_Init();
    if (err) return(err);
#endif

#if USE_SMALL_MULTI_NAND
    /* install physical device driver for Samsung K9F1208U0M (64MB) */
    err = FSM_Init();
    if (err) return(err);
#endif
    
#if USE_LARGE_MULTI_NAND || USE_LARGE_MULTI_MLC_NAND
    /* install physical device driver for Samsung K9WAG08U1M (2GB) */

#ifdef CONFIG_NAND_K9WAG08U1M 
#if VERBOSE
    printk("Try SLC 1/2/4GB\n"); 
#endif
    err = FLM_Init();
    if (err == FM_SUCCESS && PDevCount >= 1 ) return(err);
#endif /* CONFIG_NAND_K9WAG08U1M */

#ifdef CONFIG_NAND_K9HAG08U1M 
#if VERBOSE
    printk("<UFD> Try MLC 1/2/4GB (4Gb)\n"); 
#endif
    err = FLM_MLC_Init(); 
    if (err == FM_SUCCESS && PDevCount >= 1 ) return(err);
#endif /* CONFIG_NAND_K9HAG08U1M */

#ifdef CONFIG_NAND_K9LAG08U1M 
#if VERBOSE
    printk("<UFD> Try MLC 1GB (8Gb)\n"); 
#endif
    err = FLM_MLC8G_Init(); 
    if (err == FM_SUCCESS && PDevCount >= 1 ) return(err);
#endif /* CONFIG_NAND_K9LAG08U1M  */

#ifdef CONFIG_NAND_SECMLC_DDP
#if VERBOSE
    printk("<UFD> Try MLC 1/2/4GB (4Gb), 2/4/8GB (8Gb), 4/8/16GB (16Gb)\n"); 
#endif
    err = FLM_MLC8G_2_Init(); 
    if (err == FM_SUCCESS && PDevCount >= 1 ) return(err);
#endif /* CONFIG_NAND_K9HBG08U1M */

    if ( err ) return (err); 
#endif

    return(FM_SUCCESS);
}


extern INT32 
PFD_RegisterFlashDevice(UINT16 DevType, UINT16 LocalChipID, 
                        FLASH_SPEC *DevSpec, FLASH_OPS *DevOps, 
                        OP_DESC *PrevOp)
{
    PFDEV *pdev;
    
    if (DevType < PFD_SMALL_NAND && 
        DevType > PFD_ONENAND) return(FM_ERROR);
    if (PDevCount >= MAX_FLASH_CHIPS) return(FM_ERROR);
    if (DevSpec == NULL || DevOps == NULL) return(FM_ERROR);
    
    /* check the availability of mandatory driver functions */
    if (DevOps->Open == NULL ||
        DevOps->ReadPage == NULL ||
        DevOps->WritePage == NULL ||
#if 0 // !USE_LARGE_MULTI_MLC_NAND
        DevOps->CopyBack == NULL ||
#endif
        DevOps->Erase == NULL ||
        DevOps->IsBadBlock == NULL ||
        DevOps->ReadDeviceCode == NULL) {
        return(FM_ERROR);
    }

    /* register the given flash device in the table */
    pdev = &PDev[PDevCount++];
    pdev->DevType = DevType;
    pdev->LocalChipID = LocalChipID;
    pdev->PrevOp = PrevOp;
    MEMCPY((void *)&pdev->DevSpec, DevSpec, sizeof(FLASH_SPEC));
    MEMCPY((void *)&pdev->DevOps,  DevOps,  sizeof(FLASH_OPS));
    
    /* calculate other device parameters */
#if USE_DLBM
    pdev->BmAreaNumBlocks = BM_CalcBmAreaNumBlocks(pdev->ChipID);
#else
    pdev->BmAreaNumBlocks = 0;
#endif
    pdev->BmAreaStartBlock = pdev->DevSpec.NumBlocks - pdev->BmAreaNumBlocks;
    pdev->PartTableBlock = pdev->BmAreaStartBlock - 1;
    pdev->ActualNumBlocks = pdev->PartTableBlock;
    
    return(FM_SUCCESS);
}


extern UINT16    
PFD_GetNumberOfChips(void)
{
    return(PDevCount);
}


extern PFDEV *
PFD_GetPhysicalDevice(UINT16 chip_id)
{
    return(&PDev[chip_id]);
}


extern INT32    
PFD_Open(UINT16 ChipID)
{
    PFDEV *pdev;
    INT32 err;
    
    /* existing chip id? */
    if (ChipID >= PDevCount) return(FM_BAD_DEVICE_ID);
    pdev = &PDev[ChipID];
    
    /* is this first open? */
    if (pdev->UsageCount == 0) {
        err = pdev->DevOps.Open(pdev->LocalChipID);
    }
    else {
        err = FM_SUCCESS;
    }
    if (err == FM_SUCCESS) pdev->UsageCount++;
    
    return(err);
}


extern INT32    
PFD_Close(UINT16 ChipID)
{
    PFDEV *pdev;

    /* existing chip id? */
    if (ChipID >= PDevCount) return(FM_BAD_DEVICE_ID);
    pdev = &PDev[ChipID];
    
    /* open now? */
    if (pdev->UsageCount == 0) return(FM_DEVICE_NOT_OPEN);
    
    pdev->UsageCount--;
    if (pdev->UsageCount == 0) pdev->DevOps.Close(pdev->LocalChipID);
    
    return(FM_SUCCESS);
}


extern INT32 
PFD_ReadChipDeviceCode(UINT16 ChipID, UINT8 *Maker, UINT8 *DevCode)
{
    PFDEV *pdev;
    
    /* check validity of parameters */
    if (Maker == NULL || DevCode == NULL) return(FM_ERROR);
    
    /* existing chip id? */
    if (ChipID >= PDevCount) return(FM_BAD_DEVICE_ID);
    pdev = &PDev[ChipID];
    
    /* find out the device code by calling a low-level driver function */
    return(pdev->DevOps.ReadDeviceCode(pdev->LocalChipID, Maker, DevCode));
}


extern INT32 
PFD_GetChipDeviceInfo(UINT16 ChipID, FLASH_SPEC *DevInfo)
{
    PFDEV *pdev;
    
    /* check validity of parameters */
    if (DevInfo == NULL) return(FM_ERROR);
    
    /* existing chip id? */
    if (ChipID >= PDevCount) return(FM_BAD_DEVICE_ID);
    pdev = &PDev[ChipID];
    
    /* get chip device info; if DLBM (driver-level bad block management) 
       is used, the BM area should be excluded from the device size */
    MEMCPY(DevInfo, &pdev->DevSpec, sizeof(FLASH_SPEC));
    DevInfo->NumBlocks = pdev->ActualNumBlocks;
    
    return(FM_SUCCESS);    
}


extern INT32  
PFD_EraseAll(UINT16 ChipID)
{
    PFDEV *pdev;
    UINT32 i;
    
    /* existing chip id? */
    if (ChipID >= PDevCount) return(FM_BAD_DEVICE_ID);
    pdev = &PDev[ChipID];
    
    for (i = 0; i < pdev->DevSpec.NumBlocks; i++) {
	if (pdev->DevOps.IsBadBlock(pdev->LocalChipID, i) ) {
		printk("%d (Initial Bad Block) - TODO.. do not use actual erase\n", i); 
	} 
        if (pdev->DevOps.Erase(pdev->LocalChipID, i) == FM_SUCCESS) {
            if (pdev->DevOps.Sync == NULL || 
                pdev->DevOps.Sync(pdev->LocalChipID) == FM_SUCCESS) {
                continue;
            } 
            else {
                pdev->PrevOp->Command = OP_NONE; 
            }
        }

        /* erase failed; just go on to the next block */
    }
    
    return(FM_SUCCESS);
}


/*----------------------------------------------------------------------*/
/*  Wrapping Functions for Common Physical I/O                          */
/*----------------------------------------------------------------------*/

extern INT32  
PFD_ReadPage(PFDEV *pdev, UINT32 block, UINT16 page,
             UINT16 sector_offset, UINT16 num_sectors,
             UINT8 *dbuf, UINT8 *sbuf)
{
    INT32 err;
    
#if USE_DLBM
    /* get the replacement block if this block is a bad block */
    block = BM_GetSwappingBlock(pdev->ChipID, block);
#endif

    /* perform the corresponding physical device operation */
    err = pdev->DevOps.ReadPage(pdev->LocalChipID, block, page, 
                                sector_offset, num_sectors, dbuf, sbuf);
    
    /* perform error recovery if it is necessary and possible */
    if (err) {
        err = err_recover(pdev, block, page, err);
    }

    return(err);
}


extern INT32  
PFD_ReadPageGroup(PFDEV *pdev, UINT32 *block, UINT16 page,
                  UINT8 *dbuf_group[], UINT8 *sbuf_group[], INT32 *flag)
{
    INT32  err;
    UINT32 i;
    BOOL   multi_ok;

#if USE_DLBM    
    /* get the replacement blocks if there are bad blocks */
    for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
        if (flag[i]) {
            block[i] = BM_GetSwappingBlock(pdev->ChipID, block[i]);
        }
    }
#endif

    multi_ok = is_multi_ok(pdev, block, flag);
    
    /* check if the multi-plane operation can be performed */
    if (pdev->DevOps.ReadPageGroup != NULL && multi_ok) {
        err = pdev->DevOps.ReadPageGroup(pdev->LocalChipID, block, page,
                                         dbuf_group, sbuf_group, flag);
    }
    else {
        /* multi-plane operation cannot be performed;
           perform sigle-plane operations as iterating the loop */
        err = FM_SUCCESS;
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            if (flag[i]) {
                flag[i] = pdev->DevOps.ReadPage(pdev->LocalChipID, 
                                                block[i], page, 0, 
                                                pdev->DevSpec.SectorsPerPage,
                                                dbuf_group[i], 
                                                sbuf_group[i]);
                if (flag[i]) {
                    err = flag[i];
                    if (err & FM_PREV_ERROR_MASK) break;
                }
            }
        }
    }
    
    /* perform error recovery if it is necessary and possible */
    if (err & FM_PREV_ERROR_MASK) {
        
        /* error occurred in the previous request */
        err = err_recover(pdev, 0, 0, err);
    }
    else {
        /* error occurred in the current request */
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            if (flag[i]) {
                err = err_recover(pdev, block[i], page, flag[i]);
                if (err && err != FM_TRY_AGAIN) break;
            }
        }
    }

    return(err);
}


extern INT32  
PFD_WritePage(PFDEV *pdev, UINT32 block, UINT16 page,
              UINT16 sector_offset, UINT16 num_sectors,
              UINT8 *dbuf, UINT8 *sbuf, BOOL is_last)
{
    INT32 err;
    
#if USE_DLBM
    /* get the replacement block if this block is a bad block */
    block = BM_GetSwappingBlock(pdev->ChipID, block);
#endif

    /* perform the corresponding physical device operation */
    err = pdev->DevOps.WritePage(pdev->LocalChipID, block, page, 
                                 sector_offset, num_sectors, 
                                 dbuf, sbuf, is_last);
    
    /* perform error recovery if it is necessary and possible */
    if (err) {
        err = err_recover(pdev, block, page, err);
    }

    return(err);
}


extern INT32
PFD_WritePageGroup(PFDEV *pdev, UINT32 *block, UINT16 page,
                   UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
                   INT32 *flag, BOOL is_last)
{
    INT32  err;
    UINT32 i;
    BOOL   multi_ok;

#if USE_DLBM
    /* get the replacement blocks if there are bad blocks */
    for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
        if (flag[i]) {
            block[i] = BM_GetSwappingBlock(pdev->ChipID, block[i]);
        }
    }
#endif

    multi_ok = is_multi_ok(pdev, block, flag);

    /* check if the multi-plane operation can be performed */
    if (pdev->DevOps.WritePageGroup != NULL && multi_ok) {
        err = pdev->DevOps.WritePageGroup(pdev->LocalChipID, block, page,
                                          dbuf_group, sbuf_group, flag, 
                                          is_last);
    }
    else {
        /* multi-plane operation cannot be performed;
           perform sigle-plane operations as iterating the loop */
        err = FM_SUCCESS;
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            if (flag[i]) {
                flag[i] = pdev->DevOps.WritePage(pdev->LocalChipID, 
                                                 block[i], page, 0, 
                                                 pdev->DevSpec.SectorsPerPage,
                                                 dbuf_group[i], 
                                                 sbuf_group[i], 
                                                 is_last);
                if (flag[i]) {
                    err = flag[i];
                    if (err & FM_PREV_ERROR_MASK) break;
                }
            }
        }
    }

    /* perform error recovery if it is necessary and possible */
    if (err & FM_PREV_ERROR_MASK) {
        
        /* error occurred in the previous request */
        err = err_recover(pdev, 0, 0, err);
    }
    else {
        /* error occurred in the current request */
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            if (flag[i]) {
                err = err_recover(pdev, block[i], page, flag[i]);
                if (err && err != FM_TRY_AGAIN) break;
            }
        }
    }

    return(err);
}


extern INT32  
PFD_CopyBack(PFDEV *pdev, UINT32 src_block, UINT16 src_page, 
             UINT32 dest_block, UINT16 dest_page)
{
    INT32  err;
    BOOL   copyback_ok;

#if USE_DLBM
    /* get replacement blocks if some of these blocks are bad blocks */
    src_block  = BM_GetSwappingBlock(pdev->ChipID, src_block);
    dest_block = BM_GetSwappingBlock(pdev->ChipID, dest_block);
#endif    

    copyback_ok = is_copyback_ok(pdev, src_block, src_page, 
                                 dest_block, dest_page);

    /* perform copy-back operation */
    err = copy_back(pdev, src_block, src_page, 
                    dest_block, dest_page, copyback_ok);
    
    /* perform error recovery if it is necessary and possible */
    if (err) {
        err = err_recover(pdev, dest_block, dest_page, err);
    }
    
    return(err);
}


extern INT32  
PFD_CopyBackGroup(PFDEV *pdev, UINT32 *src_block, UINT16 *src_page, 
                  UINT32 *dest_block, UINT16 dest_page, INT32 *flag)
{
    INT32  err;
    UINT32 i;
    BOOL   copyback_ok[MAX_PHYSICAL_PLANES], multi_ok = TRUE;

    for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
        if (flag[i]) {
#if USE_DLBM
            /* get the replacement blocks if there are bad blocks */
            src_block[i]  = BM_GetSwappingBlock(pdev->ChipID, src_block[i]);
            dest_block[i] = BM_GetSwappingBlock(pdev->ChipID, dest_block[i]);
#endif
            copyback_ok[i] = is_copyback_ok(pdev, src_block[i], src_page[i],
                                            dest_block[i], dest_page);
            if (!copyback_ok[i]) multi_ok = FALSE;
        }
    }

    if (multi_ok) multi_ok = is_multi_ok(pdev, src_block, flag);
    if (multi_ok) multi_ok = is_multi_ok(pdev, dest_block, flag);
    
    /* check if the multi-plane operation can be performed */

    // 2006.06.25 - hcyun. 
    if ( g_copy_back_method == HW_COPYBACK && pdev->DevOps.CopyBackGroup != NULL && multi_ok) {
	    
        /* multi-plane H/W copy-back can be done */
        err = pdev->DevOps.CopyBackGroup(pdev->LocalChipID, 
                                         src_block, src_page,
                                         dest_block, dest_page, 
                                         flag);
    }

#if 1
    else if (multi_ok) {
        /* in this case multi-plane S/W copy-back can be done */
        err = multi_sw_copy_back(pdev, 
                                 src_block, src_page, 
                                 dest_block, dest_page, 
                                 flag);
    }
#endif

    else {
        /* multi-plane operation cannot be performed;
           perform sigle-plane operations as iterating the loop */
        err = FM_SUCCESS;
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            if (flag[i]) {
                
                /* perform copy-back operation */
                flag[i] = copy_back(pdev, src_block[i], src_page[i],
                                    dest_block[i], dest_page, 
                                    copyback_ok[i]);
                if (flag[i]) {
                    err = flag[i];
                    break;
                }
            }
        }
    }

    /* perform error recovery if it is necessary and possible */
    if (err & FM_PREV_ERROR_MASK) {
        
        /* error occurred in the previous request */
        err = err_recover(pdev, 0, 0, err);
    }
    else {
        /* error occurred in the current request */
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            if (flag[i]) {
                err = err_recover(pdev, dest_block[i], dest_page, flag[i]);
                if (err && err != FM_TRY_AGAIN) break;
            }
        }
    }
    
    return(err);
}


extern INT32  
PFD_Erase(PFDEV *pdev, UINT32 block)
{
    INT32 err;
    
#if USE_DLBM
    /* get the replacement block if this block is a bad block */
    block = BM_GetSwappingBlock(pdev->ChipID, block);
#endif
    
    /* perform the corresponding physical device operation */
    err = pdev->DevOps.Erase(pdev->LocalChipID, block);

    /* perform error recovery if it is necessary and possible */
    if (err) {
        err = err_recover(pdev, block, 0, err);
    }

    return(err);
}


extern INT32  
PFD_EraseGroup(PFDEV *pdev, UINT32 *block, INT32 *flag)
{
    INT32  err;
    UINT32 i;
    BOOL   multi_ok;

#if USE_DLBM
    /* get the replacement blocks if there are bad blocks */
    for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
        if (flag[i]) {
            block[i] = BM_GetSwappingBlock(pdev->ChipID, block[i]);
        }
    }
#endif

    multi_ok = is_multi_ok(pdev, block, flag);
    
    /* check if the multi-plane operation can be performed */
    if (pdev->DevOps.EraseGroup != NULL && multi_ok) {
        err = pdev->DevOps.EraseGroup(pdev->LocalChipID, block, flag);
    }
    else {
        /* multi-plane operation cannot be performed;
           perform sigle-plane operations as iterating the loop */
        err = FM_SUCCESS;
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            if (flag[i]) {
                flag[i] = pdev->DevOps.Erase(pdev->LocalChipID, block[i]);
                if (flag[i]) {
                    err = flag[i];
                    if (err & FM_PREV_ERROR_MASK) break;
                }
            }
        }
    }

    /* perform error recovery if it is necessary and possible */
    if (err & FM_PREV_ERROR_MASK) {
        
        /* error occurred in the previous request */
        err = err_recover(pdev, 0, 0, err);
    }
    else {
        /* error occurred in the current request */
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            if (flag[i]) {
                err = err_recover(pdev, block[i], 0, flag[i]);
                if (err && err != FM_TRY_AGAIN) break;
            }
        }
    }

    return(err);
}


extern INT32  
PFD_Sync(PFDEV *pdev)
{
    INT32 err = FM_SUCCESS;
    
    /* if possible, call the driver's sync function */
    if (pdev->DevOps.Sync != NULL) {
        err = pdev->DevOps.Sync(pdev->LocalChipID);
        if (err) {
            err = err_recover(pdev, 0, 0, err);
        }
    }
    
    return(err);
}


/*----------------------------------------------------------------------*/
/*  Miscellaneous Functions                                             */
/*----------------------------------------------------------------------*/

extern UINT8 *
PFD_GetBuffer(PFDEV *pdev, UINT16 purpose)
{
    if (purpose == OP_WRITE) {
        if (pdev->BufAllBusy) {
            PFD_Sync(pdev);
            pdev->BufAllBusy = FALSE;
        }
    }

    /* get the current I/O buffer */
    pdev->BufIndex = 1 - pdev->BufIndex;
    return(pdev->Buf[pdev->BufIndex]);
}


/*======================================================================*/
/*  Local Function Definitions                                          */
/*======================================================================*/

static BOOL  
is_multi_ok(PFDEV *pdev, UINT32 *block, INT32 *flag)
{
    BOOL multi_ok = TRUE;
    UINT32 i, group = 0xFFFFFFFF;
    
    if (pdev->DevOps.IsMultiOK != NULL) {
        return(pdev->DevOps.IsMultiOK(pdev->LocalChipID, block, flag));
    }
    
    /* default check routine */
    for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
        if (flag[i]) {
            if (PFD_PLANE(block[i]) != i) multi_ok = FALSE;
            if (group == 0xFFFFFFFF) group = PFD_GROUP(block[i]);
            else if (group != PFD_GROUP(block[i])) multi_ok = FALSE;
        }
#if USE_COMPLETE_GROUP_OP
        else {
            multi_ok = FALSE;
        }
#endif
    }
    if (i == 1) multi_ok = FALSE;

    return(multi_ok);
}


static BOOL  
is_copyback_ok(PFDEV *pdev, UINT32 src_block, UINT16 src_page, 
               UINT32 dest_block, UINT16 dest_page)
{
    BOOL copyback_ok = TRUE;
    UINT32 blocks_per_die, src_b, dest_b, src_die, dest_die;
    
    /* check if source & destination blocks are on the same plane */
    if (PFD_PLANE(src_block) != PFD_PLANE(dest_block)) copyback_ok = FALSE;
    
    /* if the physical device has more than one die but single chip 
       enable, there must be multiple groups of blocks across which 
       copy-backs cannot be performed */
    if (pdev->DevSpec.NumDiesPerCE > 1) {
        blocks_per_die = pdev->DevSpec.NumBlocks >> 
                         BITS[pdev->DevSpec.NumDiesPerCE];
        for (src_b = src_block, src_die = 0; 
             src_b >= blocks_per_die; 
             src_b -= blocks_per_die, src_die++);
        for (dest_b = dest_block, dest_die = 0; 
             dest_b >= blocks_per_die; 
             dest_b -= blocks_per_die, dest_die++);
        if (src_die != dest_die) copyback_ok = FALSE;
    }

    // for K9WAG08U1M -- hcyun 
    if ( g_use_odd_even_copyback ) { 
	    /* check if both of the source & destination pages are even or odd pages */
	    if ((src_page & 0x1) != (dest_page & 0x1)) copyback_ok = FALSE;
    }
    
    return(copyback_ok);
}


static INT32    
copy_back(PFDEV *pdev, UINT32 src_block, UINT16 src_page, 
          UINT32 dest_block, UINT16 dest_page, BOOL copyback_ok)
{
    INT32 err;
    UINT8 *pbuf;

    /* check if the copy-back operation can be performed */
    if (g_copy_back_method == HW_COPYBACK && copyback_ok) {

        /* perform the low-level device operation */
        err = pdev->DevOps.CopyBack(pdev->LocalChipID, src_block, src_page,
                                    dest_block, dest_page);
    }
    else {
        /* implement 'copy-back' using read & write */
        
        /* get the current I/O buffer */
        pbuf = PFD_GetBuffer(pdev, OP_READ);
        
        /* read the source page */
        err = pdev->DevOps.ReadPage(pdev->LocalChipID, src_block, src_page,
                                    0, pdev->DevSpec.SectorsPerPage,
                                    pbuf, pbuf + pdev->DevSpec.DataSize);
        if (err) return(err);
        
        /* write data in the source page into the destination page */
        err = pdev->DevOps.WritePage(pdev->LocalChipID, dest_block, dest_page,
                                     0, pdev->DevSpec.SectorsPerPage,
                                     pbuf, pbuf + pdev->DevSpec.DataSize, 
                                     TRUE);
    }
    
    return(err);
}


static INT32 
multi_sw_copy_back(PFDEV *pdev, 
                   UINT32 *src_block, UINT16 *src_page,
                   UINT32 *dest_block, UINT16 dest_page, INT32 *flag)
{
    INT32 err = FM_SUCCESS;
    INT32 i, flag2[MAX_PHYSICAL_PLANES];
    UINT8 *dbuf_group[MAX_PHYSICAL_PLANES];
    UINT8 *sbuf_group[MAX_PHYSICAL_PLANES];
    BOOL  same_src_page = TRUE;

    for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
        dbuf_group[i] = pdev->Buf[i];
        sbuf_group[i] = pdev->Buf[i] + pdev->DevSpec.DataSize;
        flag2[i] = flag[i];
        if (src_page[i] != src_page[0]) same_src_page = FALSE;
    }

    /* read the source page group */
    if (pdev->DevOps.ReadPageGroup != NULL && same_src_page) {
        err = pdev->DevOps.ReadPageGroup(pdev->LocalChipID, 
                                         src_block, src_page[0],
                                         dbuf_group, sbuf_group, flag2);
    }
    else {
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            err = pdev->DevOps.ReadPage(pdev->LocalChipID, 
                                        src_block[i], src_page[i],
                                        0, pdev->DevSpec.SectorsPerPage, 
                                        dbuf_group[i], sbuf_group[i]);
            if (err) break;
        }
    }
    if (err) return(err);

    /* write data in the source page group into the destination page group */
    if (pdev->DevOps.WritePageGroup != NULL) {
        err = pdev->DevOps.WritePageGroup(pdev->LocalChipID, 
                                          dest_block, dest_page,
                                          dbuf_group, sbuf_group, flag, TRUE);
    }
    else {
        for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
            flag[i] = pdev->DevOps.WritePage(pdev->LocalChipID, 
                                             dest_block[i], dest_page,
                                             0, pdev->DevSpec.SectorsPerPage,
                                             dbuf_group[i], sbuf_group[i], 
                                             TRUE);
            if (flag[i]) {
		err = flag[i];
		break;
	    }
        }
    }

    if (!err) pdev->BufAllBusy = TRUE;
    return(err);
}


/*----------------------------------------------------------------------*/
/*  Error Recovery Routines                                             */
/*----------------------------------------------------------------------*/
                       
static INT32 
err_recover(PFDEV *pdev, UINT32 block, UINT16 page, INT32 err)
{
#if USE_DLBM
    OP_DESC prev_op;

#if USE_SMALL_MULTI_NAND || USE_LARGE_MULTI_NAND || USE_LARGE_MULTI_MLC_NAND
    UINT16  i;
#endif
#endif

    if (pdev->PrevOp != NULL) {

#if USE_DLBM
        MEMCPY(&prev_op, pdev->PrevOp, sizeof(OP_DESC));
#endif
        pdev->PrevOp->Command = OP_NONE;
    }

    printk("err_recover: block=%d, page=%d, err=%d\n", block, page, err);

#if USE_DLBM
    switch (err) {
    case FM_WRITE_ERROR:
        /* bad block detected during current write operation */
        err = BM_SwapWriteBadBlock(pdev->ChipID, block, page);
        if (!err) err = FM_TRY_AGAIN;
        break;
        
    case FM_ERASE_ERROR:
        /* bad block detected during current erase operation */
        err = BM_SwapEraseBadBlock(pdev->ChipID, block);
        if (!err) err = FM_TRY_AGAIN;
        break;
        
    case FM_PREV_WRITE_ERROR:
        /* the previous operation should be 'write' or 'copyback' */
        if (prev_op.Command == OP_WRITE) {
            err = recover_from_write_error(pdev, 
                                    prev_op.Param.Write.Block,
                                    prev_op.Param.Write.Page,
                                    prev_op.Param.Write.SectorOffset,
                                    prev_op.Param.Write.NumSectors, 
                                    prev_op.Param.Write.DBuf,
                                    prev_op.Param.Write.SBuf);
        }
        else if (prev_op.Command == OP_COPYBACK) {
            err = recover_from_copyback_error(pdev, 
                                    prev_op.Param.CopyBack.SrcBlock,
                                    prev_op.Param.CopyBack.SrcPage,
                                    prev_op.Param.CopyBack.DestBlock,
                                    prev_op.Param.CopyBack.DestPage);
        }
        if (!err) err = FM_TRY_AGAIN;
        break;

    case FM_PREV_ERASE_ERROR:
        /* the previous operation should be 'erase' */
        if (prev_op.Command == OP_ERASE) {
            err = recover_from_erase_error(pdev, 
                                    prev_op.Param.Erase.Block);
        }
        if (!err) err = FM_TRY_AGAIN;
        break;

#if USE_SMALL_MULTI_NAND || USE_LARGE_MULTI_NAND || USE_LARGE_MULTI_MLC_NAND

    case FM_PREV_WRITE_GROUP_ERROR:
        /* the previous operation should be 'write_group' or 
           'copyback_group' */
        if (prev_op.Command == OP_WRITE_GROUP) {

            /* re-do the failed operation for each plane */
            for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
                if (prev_op.Param.WriteGroup.Flag[i]) {

                    /* try to perform the previous operation again here */
                    err = pdev->DevOps.WritePage(pdev->LocalChipID, 
                                    prev_op.Param.WriteGroup.Block[i],
                                    prev_op.Param.WriteGroup.Page,
                                    0, pdev->DevSpec.SectorsPerPage,
                                    prev_op.Param.WriteGroup.DBuf[i],
                                    prev_op.Param.WriteGroup.SBuf[i], 
                                    TRUE);
                    if (!err) {
                        if (pdev->DevOps.Sync == NULL) continue;
                        err = pdev->DevOps.Sync(pdev->LocalChipID);
                        if (!err) continue;
                    }
                    if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;

                    /* error occurred; perform error recovery */
                    err = recover_from_write_error(pdev, 
                                    prev_op.Param.WriteGroup.Block[i],
                                    prev_op.Param.WriteGroup.Page,
                                    0, pdev->DevSpec.SectorsPerPage,
                                    prev_op.Param.WriteGroup.DBuf[i],
                                    prev_op.Param.WriteGroup.SBuf[i]);
                    if (err) break;
                }
            }
        }
        else if (prev_op.Command == OP_COPYBACK_GROUP) {

            /* re-do the failed operation for each plane */
            for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
                if (prev_op.Param.CopyBackGroup.Flag[i]) {

                    /* try to perform the previous operation again here */
                    err = copy_back(pdev,
                                    prev_op.Param.CopyBackGroup.SrcBlock[i],
                                    prev_op.Param.CopyBackGroup.SrcPage[i],
                                    prev_op.Param.CopyBackGroup.DestBlock[i],
                                    prev_op.Param.CopyBackGroup.DestPage, 
                                    FALSE);
                    if (!err) {
                        if (pdev->DevOps.Sync == NULL) continue;
                        err = pdev->DevOps.Sync(pdev->LocalChipID);
                        if (!err) continue;
                    }
                    if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;

                    /* error occurred; perform error recovery */
                    err = recover_from_copyback_error(pdev, 
                                    prev_op.Param.CopyBackGroup.SrcBlock[i],
                                    prev_op.Param.CopyBackGroup.SrcPage[i],
                                    prev_op.Param.CopyBackGroup.DestBlock[i],
                                    prev_op.Param.CopyBackGroup.DestPage);
                    if (err) break;
                }
            }
        }
        if (!err) err = FM_TRY_AGAIN;
        break;
        
    case FM_PREV_ERASE_GROUP_ERROR:
        /* the previous operation should be 'erase_group' */
        if (prev_op.Command == OP_ERASE_GROUP) {

            /* re-do the failed operation for each plane */
            for (i = 0; i < pdev->DevSpec.NumPlanes; i++) {
                if (prev_op.Param.EraseGroup.Flag[i]) {

                    /* try to perform the previous operation again here */
                    err = pdev->DevOps.Erase(pdev->LocalChipID, 
                                    prev_op.Param.EraseGroup.Block[i]);
                    if (!err) {
                        if (pdev->DevOps.Sync == NULL) continue;
                        err = pdev->DevOps.Sync(pdev->LocalChipID);
                        if (!err) continue;
                    }
                    if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;

                    /* error occurred; perform error recovery */
                    err = recover_from_erase_error(pdev, 
                                    prev_op.Param.EraseGroup.Block[i]);
                    if (err) break;
                }
            }
        }
        if (!err) err = FM_TRY_AGAIN;
        break;

#endif /* USE_SMALL_MULTI_NAND || USE_LARGE_MULTI_NAND || USE_LARGE_MULTI_MLC_NAND */

    default:
        /* some upper layer software will handle the other errors */
        printk("%s: Unhandled error : %d\n", __FUNCTION__, err);
        break;
    }
#endif /* USE_DLBM */

    return(err);
}


#if USE_DLBM

static INT32
recover_from_write_error(PFDEV *pdev, UINT32 block, UINT16 page,
                         UINT16 sector_offset, UINT16 num_sectors,
                         UINT8 *dbuf, UINT8 *sbuf)
{
    INT32 err;
    UINT32 org_block, phy_block;

    /* if the given block is a replacement block (for a bad block),
       we need to find out the original block number */
    org_block = BM_GetOriginalBlock(pdev->ChipID, block);
    
    /* register this block as a bad block and replace it here */
    err = BM_SwapWriteBadBlock(pdev->ChipID, block, page);
    if (!err) {

        /* ok, bad block has been replaced successfully;
           perform the previous write operation again */
        while (1) {
            phy_block = BM_GetSwappingBlock(pdev->ChipID, org_block);
            err = pdev->DevOps.WritePage(pdev->LocalChipID, phy_block, page,
                                         sector_offset, num_sectors, 
                                         dbuf, sbuf, TRUE);
            if (!err) {
                if (pdev->DevOps.Sync == NULL) break;
                err = pdev->DevOps.Sync(pdev->LocalChipID);
                if (!err) break;
            }
            if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;

            /* error occurred again; perform recovery */
            if (err == FM_WRITE_ERROR) {
                err = BM_SwapWriteBadBlock(pdev->ChipID, phy_block, page);
            }
            if (err) break;
        }
    }
    
    return(err);
}


static INT32
recover_from_copyback_error(PFDEV *pdev, 
                            UINT32 src_block, UINT16 src_page, 
                            UINT32 dest_block, UINT16 dest_page)
{
    INT32 err;
    UINT32 org_block, phy_block;

    /* if the given block is a replacement block (for a bad block),
       we need to find out the original block number */
    org_block = BM_GetOriginalBlock(pdev->ChipID, dest_block);
    
    /* register this block as a bad one and replace it here */
    err = BM_SwapWriteBadBlock(pdev->ChipID, dest_block, dest_page);
    if (!err) {

        /* ok, bad block has been replaced successfully;
           perform the previous copy-back operation again */
        while (1) {
            phy_block = BM_GetSwappingBlock(pdev->ChipID, org_block);
            err = copy_back(pdev, src_block, src_page,
                            phy_block, dest_page, FALSE);
            if (!err) {
                if (pdev->DevOps.Sync == NULL) break;
                err = pdev->DevOps.Sync(pdev->LocalChipID);
                if (!err) break;
            }
            if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;

            /* error occurred again; perform recovery */
            if (err == FM_WRITE_ERROR) {
                err = BM_SwapWriteBadBlock(pdev->ChipID, phy_block, dest_page);
            }
            if (err) break;
        }

        /* watch out not to corrupt the existing data in 'pdev->Buf[2]'
           because the 'copy-back' function uses them in turn; the following
           code has an effect to let the function use the same buffer again 
           (thus leaving the other buffer untouched) */
        pdev->BufIndex = 1 - pdev->BufIndex;
    }

    return(err);
}


static INT32
recover_from_erase_error(PFDEV *pdev, UINT32 block)
{
    INT32 err;
    UINT32 org_block, phy_block;

    /* if the given block is a replacement block (for a bad block),
       we need to find out the original block number */
    org_block = BM_GetOriginalBlock(pdev->ChipID, block);
    
    /* register this block as a bad one and replace it here */
    err = BM_SwapEraseBadBlock(pdev->ChipID, block);
    if (!err) {

        /* ok, bad block has been replaced successfully;
           perform the previous erase operation again */
        while (1) {
            phy_block = BM_GetSwappingBlock(pdev->ChipID, org_block);
            err = pdev->DevOps.Erase(pdev->LocalChipID, phy_block);
            if (!err) {
                if (pdev->DevOps.Sync == NULL) break;
                err = pdev->DevOps.Sync(pdev->LocalChipID);
                if (!err) break;
            }
            if (pdev->PrevOp != NULL) pdev->PrevOp->Command = OP_NONE;

            /* error occurred again; perform recovery */
            if (err == FM_ERASE_ERROR) {
                err = BM_SwapEraseBadBlock(pdev->ChipID, phy_block);
            }
            if (err) break;
        }
    }
    
    return(err);
}

#endif /* USE_DLBM */
