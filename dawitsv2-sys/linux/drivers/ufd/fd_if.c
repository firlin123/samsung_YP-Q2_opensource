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
/*  FILE    : fd_if.c                                                   */
/*  PURPOSE : Code for Flash Device Common Interface                    */
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

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/compatmac.h>

#include <asm/arch/stmp37xx_usbevent.h>
#include <asm/arch/stmp37xx_pm.h>

#endif

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

#define USE_BUFFERING_FOR_UMS       1       /* 1 : use buffering for UMS
                                               0 : do not use
                                               (default = 1) */
                                               
/* UFD level locking (mutual exclusion) mechanism */

#if (1) && defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)

    #define USE_SEMAPHORE           1
    #define USE_INTERRUPTIBLE_SEMA  0
    
    #if (USE_SEMAPHORE == 1)
        struct semaphore ufd_lock;
        
        #define UFD_LOCK_INIT       sema_init(&ufd_lock, 1)

        #if (USE_INTERRUPTIBLE_SEMA == 1)
        #define UFD_LOCK            do {                                    \
                                        if (down_interruptible(&ufd_lock))  \
                                            return -ERESTARTSYS;            \
                                    } while (0)
        #else
        #define UFD_LOCK            down(&ufd_lock)
        #endif
        
        #define UFD_UNLOCK          up(&ufd_lock)
    #else
        spinlock_t ufd_lock;
    
        #define UFD_LOCK_INIT       spin_lock_init(&ufd_lock)
        #define UFD_LOCK            spin_lock(&ufd_lock)
        #define UFD_UNLOCK          spin_unlock(&ufd_lock)
    #endif

#else
    #define UFD_LOCK_INIT
    #define UFD_LOCK
    #define UFD_UNLOCK
#endif

/* because both the RFS and UFD work basically in a blocking mode,
   there is a possibility that other real-time threads might miss their
   deadlines if RFS has a lot of requests to process (e.g. writing huge
   data); this problem is known as kernel locking, and can be avoided
   by letting the RFS thread relinquish the CPU if it has consumed too
   much CPU time; the following macro can be used for this purpose */

#if (0) && defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
#define MAX_JIFFIES_TO_USE      5       /* 1 jiffy = 10 msec in general */

#define AVOID_MONOPOLIZATION(ldev)                                      \
        do {                                                            \
            if (current->pid == current_task_pid &&                     \
                FD_UserSequence == current_user_seq) {                  \
                if (jiffies >= start_jiffies + MAX_JIFFIES_TO_USE) {    \
                                                                        \
                    /* looks like the current task is monopolizing */   \
                    /* the CPU; perform the sync operation first   */   \
                    err = FD_Sync((ldev)->DevID);                       \
                                                                        \
                    /* relinquish the CPU now */                        \
                    if (!err) {                                         \
                        current_user_seq = FD_UserSequence - 1;         \
                        schedule();                                     \
                    }                                                   \
                }                                                       \
            }                                                           \
            else {                                                      \
                current_task_pid = current->pid;                        \
                current_user_seq = FD_UserSequence;                     \
                start_jiffies = jiffies;                                \
            }                                                           \
        } while (0)
#else
#define AVOID_MONOPOLIZATION(ldev)
#endif

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  External Variable Definitions                                       */
/*----------------------------------------------------------------------*/

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
UINT32 FD_UserSequence = 0;         /* a UFD user (e.g. RFS) increases this
                                       variable whenever it is invoked by
                                       the system call; this value is used
                                       by the 'AVOID_MONOPOLIZATION' macro
                                       for a reference value */
#endif

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

static FLASH_PART  EntireChipSpace[MAX_FLASH_CHIPS];

#if defined(ZFLASH_LINUX) && defined(MAX_JIFFIES_TO_USE)
static pid_t  current_task_pid;
static UINT32 current_user_seq = 0xFFFF;
static UINT32 start_jiffies;
#endif

#if USE_DEFAULT_PARTITION
FLASH_PARTTAB DefaultPartTable[MAX_FLASH_CHIPS] =
{
    /* for flash memory chip-0  */
    {
        /* NumPartitions        */      4,
        /* Part[]               */
        {
            /* for partition-0  */      // for boot loader (vivi)
            {
                /* StartBlock   */      6,
                /* NumBlocks    */      2,      // 256KB
                /* DevClass     */      LFD_BLK_DEVICE_RAW,
                /* InDevTable   */      1,
                /* DevIndex     */      0,
                /* Protected    */      0,
                /* ECCMode      */      0
            },
            /* for partition-1  */      // for kernel image
            {
                /* StartBlock   */      8,
                /* NumBlocks    */      8,      // 2MB
                /* DevClass     */      LFD_BLK_DEVICE_RAW,
                /* InDevTable   */      1,
                /* DevIndex     */      1,
                /* Protected    */      0,
                /* ECCMode      */      0
            },

            /* for partition-3  */
            {
                /* StartBlock   */      16,
                /* NumBlocks    */      64,
                /* DevClass     */      LFD_BLK_DEVICE_FTL,
                /* InDevTable   */      1,
                /* DevIndex     */      0,
                /* Protected    */      0,
                /* ECCMode      */      0
            },
            /* for partition-4  */
            {
                /* StartBlock   */      80,
                /* NumBlocks    */      7908,
                /* DevClass     */      LFD_BLK_DEVICE_FTL,
                /* InDevTable   */      1,
                /* DevIndex     */      1,
                /* Protected    */      0,
                /* ECCMode      */      0
            },
            /* and so on...     */
        }
    },
    
    /* for flash memory chip-1  */

    {
        /* NumPartitions        */      4,
        /* Part[]               */
        {
            /* for partition-0  */      // for boot loader (vivi)
            {
                /* StartBlock   */      6,
                /* NumBlocks    */      2,      // 256KB
                /* DevClass     */      LFD_BLK_DEVICE_RAW,
                /* InDevTable   */      1,
                /* DevIndex     */      0,
                /* Protected    */      0,
                /* ECCMode      */      0
            },
            /* for partition-1  */      // for kernel image
            {
                /* StartBlock   */      8,
                /* NumBlocks    */      8,      // 2MB
                /* DevClass     */      LFD_BLK_DEVICE_RAW,
                /* InDevTable   */      1,
                /* DevIndex     */      1,
                /* Protected    */      0,
                /* ECCMode      */      0
            },

            /* for partition-3  */
            {
                /* StartBlock   */      16,
                /* NumBlocks    */      64,
                /* DevClass     */      LFD_BLK_DEVICE_FTL,
                /* InDevTable   */      1,
                /* DevIndex     */      0,
                /* Protected    */      0,
                /* ECCMode      */      0
            },
            /* for partition-4  */
            {
                /* StartBlock   */      80,
                /* NumBlocks    */      3452, //7908,
                /* DevClass     */      LFD_BLK_DEVICE_FTL,
                /* InDevTable   */      1,
                /* DevIndex     */      1,
                /* Protected    */      0,
                /* ECCMode      */      0
            },
            /* and so on...     */
        }
    },
    
    /* for flash memory chip-1  */
    /* for flash memory chip-2  */
    /* and so on...             */
};
#endif

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

#if (USE_BUFFERING_FOR_UMS == 1)

BOOL usb_ftl_connected = 0;
EXPORT_SYMBOL(usb_ftl_connected);

static INT32 pm_usb_event_callback(ss_pm_request_t event);
#endif

static INT32 sw_copy_back(PFDEV *src_pdev, UINT32 src_block, UINT16 src_page, 
                          PFDEV *dest_pdev, UINT32 dest_block, UINT16 dest_page);


/*======================================================================*/
/*  External Function Definitions                                       */
/*======================================================================*/


extern INT32  
FD_Init(void)
{
    INT32  err;
    UINT16 i;
    FLASH_PARTTAB part_tab;    

    /*--------------------------------------------------------------*/
    /*  first, initialize flash device driver modules               */
    /*--------------------------------------------------------------*/

    /* initialize logical flash device interface layer */
    err = LFD_Init();
    if (err) return(err);
    
#if USE_DLBM
    /* initialize bad block management layer */
    err = BM_Init();
    if (err) return(err);
#endif
    
    /* initialize physical flash device interface layer;
       in this function call, physical flash devices are installed */
    err = PFD_Init();
    if (err) return(err);

    /*--------------------------------------------------------------*/
    /*  second, open the device & recognize logical device info      */
    /*--------------------------------------------------------------*/

    /* open each of the registered physical devices */
    for (i = 0; i < PDevCount; i++) {
        err = PFD_Open(i);
        if (err) { 
            printk("PFD_Open for chip(%d) failed...\n", i); 
            return(err);
        }
    }

#if USE_DLBM
    /* perform BM open for each physical device */
    for (i = 0; i < PDevCount; i++) {
        err = BM_Open(i);
        if (err) { 
            printk("BM_Open for chip(%d) failed...\n", i); 
            return(err);
        }
    }

    /* perform BM format for each physical device;
       it's safe -- format would be bypassed if it already done */
    for (i = 0; i < PDevCount; i++) {
        err = BM_Format(i, FALSE); 
        if (err) { 
            printk("BM_Format for chip(%d) failed\n", i); 
            return(err);
        }
    }
#endif

    /* read low-level flash partition information */
    for (i = 0; i < PDevCount; i++) {
	    err = LFD_ReadPartitionTable(i, &part_tab);
	    if (err) {
		    printk("%s: partition table read failed for chip %d\n", 
			   __FUNCTION__, i); 
		    //udelay(2000); 
		    printk("Try to create a defalut partition table\n"); 
		    part_tab.NumPartitions = 0; 
		    // return(err);
	    } 
#if 0 /* DEBUG */ 
	    else {
		    int k; 
		    for ( k = 0; k < part_tab.NumPartitions; k++ ) { 
			    printk("part[%d]: start=%d, nblk=%d\n", 
				   k, 
				   part_tab.Part[k].StartBlock,
				   part_tab.Part[k].NumBlocks); 
		    }
	    } 
#endif 

#if USE_DEFAULT_PARTITION
        //DefaultPartTable[i].Part[3].NumBlocks 
        //    = PDev[i].ActualNumBlocks - DefaultPartTable[i].Part[3].StartBlock;

        /* if there is no low-level partition table yet,
           use a default partition table here */
        if ((part_tab.NumPartitions == 0) ||
            (MEMCMP(&part_tab, &DefaultPartTable[i],
                    (UINT32)&part_tab.Part[part_tab.NumPartitions] -
                    (UINT32)&part_tab) != 0)) {

            //panic("%s: Default partition is'nt exist.. must re-install f/w\n", __FUNCTION__); 

            err = LFD_WritePartitionTable(i, &DefaultPartTable[i]);
            if (err) { 
                printk("LFD_WritePartitionTable for chip(%d) failed\n", i); 
                return(err);
            }
        }
#endif
    }
    
#if (USE_BUFFERING_FOR_UMS == 1)
    /* register callback function for USB event; 
       USB connectivity information is used in the FTL block device driver */
    ss_pm_register(SS_PM_USB_DEV, pm_usb_event_callback);
    
    usb_ftl_connected = is_USB_connected();
    if (usb_ftl_connected) 
    	printk("<UFD> %s : USB Connected!!(%d)\n", __FUNCTION__, usb_ftl_connected); 
    else
    	printk("<UFD> %s : USB Not Connected!!(%d)\n", __FUNCTION__, usb_ftl_connected);
    
#endif
    
    UFD_LOCK_INIT;

    printk("<UFD> FD_Init is successful!!!\n"); 
    
    return(FM_SUCCESS);
}


extern INT32  
FD_Shutdown(void)
{
    UINT16 i;
    
    /* close each of the registered physical devices */
    for (i = 0; i < PDevCount; i++) {
        PFD_Close(i);
    }

    return(FM_SUCCESS);
}


/*----------------------------------------------------------------------*/
/*  Flash Memory Device Driver APIs                                     */
/*----------------------------------------------------------------------*/

extern INT32  
FD_Open(UINT32 dev_id)
{
    INT32  err, i;
    LFDEV  *ldev;
    UINT16 dev_class, dev_index, serial, chip_id, part_id;
    BOOL   all_same_size = TRUE; 

    /* check if it is already open */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev != NULL) {
        ldev->UsageCount++;
        return(FM_SUCCESS);
    }

    UFD_LOCK;

    /* allocate a logical device structure */
    ldev = LFD_AllocLogicalDevice(dev_id);
    if (ldev == NULL) {
	UFD_UNLOCK;
	return(FM_OPEN_FAIL);
    }

    /* get device class & serial number out of device ID */
    dev_class = GET_DEV_CLASS(dev_id);
    serial = GET_DEV_SERIAL(dev_id);
    
    switch (dev_class) {
    case LFD_BLK_DEVICE_RAW:
        /* extract chip ID and partition ID */
        chip_id = GET_RAWDEV_CHIP_ID(serial);
        part_id = GET_RAWDEV_PART_ID(serial);
        
        if (chip_id != RAW_BLKDEV_TABLE) {
            /* accessing a raw block device using the chip ID & part ID pair */
            if ((chip_id >= PDevCount) || 
                (part_id != ENTIRE_PARTITION && 
                 part_id >= PartTable[chip_id].NumPartitions)) {
                err = FM_BAD_DEVICE_ID;
                goto error;
            }
            
            /* set up the partition information for this logical device */
            ldev->PartInfo.NumPartitions = 1;
            ldev->PartInfo.Parts[0].PDev = &PDev[chip_id];
            if (part_id != ENTIRE_PARTITION) {
                ldev->PartInfo.Parts[0].Part = &PartTable[chip_id].Part[part_id];
            }
            else {
                ldev->PartInfo.Parts[0].Part = &EntireChipSpace[chip_id];
    
                /* using the 'LFD_BLK_DEVICE_RAW' class and 'ENTIRE_PARTITION'
                   partition ID, the entire chip space can be accessed; the 
                   only area that cannot be accessed is the DLBM area and the
                   partition table block */
                EntireChipSpace[chip_id].StartBlock = 0;
                EntireChipSpace[chip_id].NumBlocks = PDev[chip_id].ActualNumBlocks;
            }
        }
        else {
            /* accessing a raw block device using the raw block device table */
            dev_index = part_id;
            if (dev_index >= MAX_FLASH_VPARTITIONS ||
                RAWDevTable[dev_index].NumPartitions == 0) {
                err = FM_BAD_DEVICE_ID;
                goto error;
            }
            
            /* get the partition information for this logical device */
            MEMCPY(&ldev->PartInfo, &RAWDevTable[dev_index], sizeof(FLASH_VPART));
        }
        break;

    case LFD_BLK_DEVICE_FTL:
        /* check if the given device ID is valid */
        dev_index = GET_FTLDEV_INDEX(serial);
        if (dev_index >= MAX_FLASH_VPARTITIONS || 
            FTLDevTable[dev_index].NumPartitions == 0) {
            err = FM_BAD_DEVICE_ID;
            goto error;
        }
        
        /* get the partition information for this logical device */
        MEMCPY(&ldev->PartInfo, &FTLDevTable[dev_index], sizeof(FLASH_VPART));
        break;
        
    default:
        /* not supported device ID */
        err = FM_BAD_DEVICE_ID;
        goto error;
    }

    /* set up the device specification */
    MEMCPY(&ldev->DevSpec, 
           &ldev->PartInfo.Parts[0].PDev->DevSpec, sizeof(FLASH_SPEC));
    ldev->DevSpec.NumBlocks = 0;
    ldev->DevSpec.MaxNumBadBlocks = 0;
    for (i = 0; i < ldev->PartInfo.NumPartitions; i++) {
        ldev->DevSpec.NumBlocks += ldev->PartInfo.Parts[i].Part->NumBlocks;
#if !USE_DLBM
        ldev->DevSpec.MaxNumBadBlocks += 
        ldev->PartInfo.Parts[i].PDev->DevSpec.MaxNumBadBlocks;
#endif

        /* check if all of the constituent partitions have the same size */
        if (ldev->PartInfo.Parts[i].Part->NumBlocks !=
            ldev->PartInfo.Parts[0].Part->NumBlocks) {
            all_same_size = FALSE;
        }
    }

    /* determine the level of interleaving for this device */
    ldev->InterleavingLevel = 1;
    if (all_same_size && (ldev->PartInfo.NumPartitions == 2 ||
                          ldev->PartInfo.NumPartitions == 4 ||
                          ldev->PartInfo.NumPartitions == 8)) {
        ldev->InterleavingLevel = MIN(MAX_INTERLEAVED_CHIPS,
                                      ldev->PartInfo.NumPartitions);
        ldev->DevSpec.NumPlanes *= ldev->InterleavingLevel;
    }

    ldev->UsageCount++;
    UFD_UNLOCK; 

    return(FM_SUCCESS);
    
error:
    LFD_FreeLogicalDevice(dev_id);
    UFD_UNLOCK; 

    return(err);
}


extern INT32
FD_Close(UINT32 dev_id)
{
    LFDEV *ldev;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);

    UFD_LOCK;

    /* close the logical device referenced by dev_id */
    ldev->UsageCount--;
    if (ldev->UsageCount == 0) {
        LFD_FreeLogicalDevice(dev_id);
    }
    UFD_UNLOCK;
    
    return(FM_SUCCESS);
}


extern INT32  
FD_ReadPage(UINT32 dev_id, UINT32 block, UINT16 page,
            UINT16 sector_offset, UINT16 num_sectors,
            UINT8 *dbuf, UINT8 *sbuf)
{
    INT32 err;
    LFDEV *ldev;
    PFDEV *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);
    
    AVOID_MONOPOLIZATION(ldev);

    UFD_LOCK;

#if USE_CUSTOM_DEV_SPEC
    /* if a custom device spec is used, 
       call the corresponding logical device API */
    if (ldev->CustomDevSpecType != DEV_SPEC_ORIGINAL) {
	int ret;
        ret = LFD_ReadPage(ldev, block, page, sector_offset, num_sectors,
                           dbuf, sbuf);
	UFD_UNLOCK; 
	return ret;
    }
#endif

#if STRICT_CHECK
    /* check if the block and page numbers are within the valid ranges */
    if (block >= ldev->DevSpec.NumBlocks) return(FM_ILLEGAL_ACCESS);
    if (page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
    if ((sector_offset + num_sectors) > ldev->DevSpec.SectorsPerPage) 
        return(FM_ILLEGAL_ACCESS);
#endif
    
    /* get the partition information for this logical device; 
       the block number is also adjusted as a relative value 
       in the corresponding partition */
    part_info = LFD_GetPartitionInfo(ldev, &block);
    pdev = part_info->PDev;
    part = part_info->Part;

    /* get the block number in the chip-wide address space */
    block += part->StartBlock;
    
    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_ReadPage(pdev, block, page, sector_offset, num_sectors, 
                           dbuf, sbuf);
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

    UFD_UNLOCK;
    return(err);
}


extern INT32  
FD_ReadPageGroup(UINT32 dev_id, UINT32 group, UINT16 page,
                 UINT8 *dbuf_group[], UINT8 *sbuf_group[], INT32 *flag)
{
    INT32  err = FM_SUCCESS, *flag2;
    UINT32 block, org_block, b[MAX_PHYSICAL_PLANES];
    UINT16 i, j, planes_per_part, plane_offset;
    UINT8  **dbuf_group2, **sbuf_group2;

    LFDEV  *ldev;
    PFDEV  *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);

    AVOID_MONOPOLIZATION(ldev);

    UFD_LOCK;

#if USE_CUSTOM_DEV_SPEC
    /* if a custom device spec is used, 
       call the corresponding logical device API */
    if (ldev->CustomDevSpecType != DEV_SPEC_ORIGINAL) {
	int ret;
        ret = LFD_ReadPageGroup(ldev, group, page, 
                                dbuf_group, sbuf_group, flag);
	UFD_UNLOCK;
	return ret;
    }
#endif

#if STRICT_CHECK
    /* check if the group and page numbers are within the valid ranges */
    if (group >= (ldev->DevSpec.NumBlocks / ldev->DevSpec.NumPlanes)) 
        return(FM_ILLEGAL_ACCESS);
    if (page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
#endif

    /* calculate the first block number in this group */
    org_block = group << BITS[ldev->DevSpec.NumPlanes];
    
    /* calculate some important constants */
    planes_per_part = ldev->DevSpec.NumPlanes >> BITS[ldev->InterleavingLevel];

    /* perform the corresponding group operation for each partition */
    for (i = 0; i < ldev->InterleavingLevel; i++) {

        /* calculate the block number in each partition */
        plane_offset = i << BITS[planes_per_part];
        block = org_block + plane_offset;

        /* get the information about the partition where this block exists;
           the block number is also adjusted as a relative value 
           in the corresponding partition */
        part_info = LFD_GetPartitionInfo(ldev, &block);
        pdev = part_info->PDev;
        part = part_info->Part;

        /* set temporary function call parameters */
        flag2 = &flag[plane_offset];
        dbuf_group2 = &dbuf_group[plane_offset];
        sbuf_group2 = &sbuf_group[plane_offset];
        
        /* ok, perform the multi-plane operation */
        do {
            /* calculate the block numbers in the chip-wide address space;
               because the block numbers can be modified in the physical layer,
               we should re-calculate those numbers for every loop iteration */
            b[0] = block + part->StartBlock;
            for (j = 1; j < planes_per_part; j++) {
                b[j] = b[0] + j;
            }
            
            /* read page group */
            err = PFD_ReadPageGroup(pdev, b, page, dbuf_group2, sbuf_group2,
                                    flag2);
        } while (err == FM_TRY_AGAIN);

        /* if an error occurs (ex. ECC_ERROR), return immediately */
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
#endif /* CFG_HWECC_PIPELINE */

    UFD_UNLOCK;
    return(err);
}


extern INT32  
FD_WritePage(UINT32 dev_id, UINT32 block, UINT16 page,
             UINT16 sector_offset, UINT16 num_sectors,
             UINT8 *dbuf, UINT8 *sbuf, BOOL is_last)
{
    INT32 err;
    LFDEV *ldev;
    PFDEV *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);
    
    AVOID_MONOPOLIZATION(ldev);

    UFD_LOCK;

#if USE_CUSTOM_DEV_SPEC
    /* if a custom device spec is used, 
       call the corresponding logical device API */
    if (ldev->CustomDevSpecType != DEV_SPEC_ORIGINAL) {
	int ret;
        ret = LFD_WritePage(ldev, block, page, sector_offset, num_sectors,
                            dbuf, sbuf, is_last);
	UFD_UNLOCK; 
	return ret;
    }
#endif

#if STRICT_CHECK
    /* check if the block and page numbers are within the valid ranges */
    if (block >= ldev->DevSpec.NumBlocks) return(FM_ILLEGAL_ACCESS);
    if (page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
    if ((sector_offset + num_sectors) > ldev->DevSpec.SectorsPerPage) 
        return(FM_ILLEGAL_ACCESS);
#endif

    /* get the partition information for this logical device; 
       the block number is also adjusted as a relative value 
       in the corresponding partition */
    part_info = LFD_GetPartitionInfo(ldev, &block);
    pdev = part_info->PDev;
    part = part_info->Part;

    /* check if this logical device is write protected */
    if (part->Protected) {
	UFD_UNLOCK;
	return(FM_PROTECT_ERROR);
    }

    /* get the block number in the chip-wide address space */
    block += part->StartBlock;

    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_WritePage(pdev, block, page, sector_offset, num_sectors,
                            dbuf, sbuf, is_last);
    } while (err == FM_TRY_AGAIN);

    UFD_UNLOCK;
    return(err);
}


extern INT32  
FD_WritePageGroup(UINT32 dev_id, UINT32 group, UINT16 page,
                  UINT8 *dbuf_group[], UINT8 *sbuf_group[], 
                  INT32 *flag, BOOL is_last)
{
    INT32  err = FM_SUCCESS, *flag2;
    UINT32 block, org_block, b[MAX_PHYSICAL_PLANES];
    UINT16 i, j, planes_per_part, plane_offset;
    UINT8  **dbuf_group2, **sbuf_group2;

    LFDEV  *ldev;
    PFDEV  *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);

    AVOID_MONOPOLIZATION(ldev);

    UFD_LOCK;

#if USE_CUSTOM_DEV_SPEC
    /* if a custom device spec is used, 
       call the corresponding logical device API */
    if (ldev->CustomDevSpecType != DEV_SPEC_ORIGINAL) {
	int ret;
        ret = LFD_WritePageGroup(ldev, group, page, 
                                 dbuf_group, sbuf_group, flag, is_last);
	UFD_UNLOCK;
	return ret;
    }
#endif

#if STRICT_CHECK
    /* check if the group and page numbers are within the valid ranges */
    if (group >= (ldev->DevSpec.NumBlocks / ldev->DevSpec.NumPlanes)) 
        return(FM_ILLEGAL_ACCESS);
    if (page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
#endif

    /* calculate the first block number in this group */
    org_block = group << BITS[ldev->DevSpec.NumPlanes];
    
    /* calculate some important constants */
    planes_per_part = ldev->DevSpec.NumPlanes >> BITS[ldev->InterleavingLevel];

    /* perform the corresponding group operation for each partition */
    for (i = 0; i < ldev->InterleavingLevel; i++) {

        /* calculate the block number in each partition */
        plane_offset = i << BITS[planes_per_part];
        block = org_block + plane_offset;

        /* get the information about the partition where this block exists;
           the block number is also adjusted as a relative value 
           in the corresponding partition */
        part_info = LFD_GetPartitionInfo(ldev, &block);
        pdev = part_info->PDev;
        part = part_info->Part;

        /* check if this logical device is write protected */
        if (part->Protected) {
	    UFD_UNLOCK;
	    return(FM_PROTECT_ERROR);
	}

        /* set temporary function call parameters */
        flag2 = &flag[plane_offset];
        dbuf_group2 = &dbuf_group[plane_offset];
        sbuf_group2 = &sbuf_group[plane_offset];
        
        /* ok, perform the multi-plane operation */
        do {
            /* calculate the block numbers in the chip-wide address space;
               because the block numbers can be modified in the physical layer,
               we should re-calculate those numbers for every loop iteration */
            b[0] = block + part->StartBlock;
            for (j = 1; j < planes_per_part; j++) {
                b[j] = b[0] + j;
            }
            
            /* write page group */
            err = PFD_WritePageGroup(pdev, b, page, dbuf_group2, sbuf_group2,
                                     flag2, is_last);
        } while (err == FM_TRY_AGAIN);
    }

    UFD_UNLOCK;
    return(err);
}


extern INT32  
FD_CopyBack(UINT32 dev_id, UINT32 src_block, UINT16 src_page, 
            UINT32 dest_block, UINT16 dest_page)
{
    INT32 err;
    LFDEV *ldev;
    PFDEV *src_pdev, *dest_pdev;
    FLASH_PART  *src_part, *dest_part;
    FLASH_RPART *part_info;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);
    
    AVOID_MONOPOLIZATION(ldev);

    UFD_LOCK;

#if USE_CUSTOM_DEV_SPEC
    /* if a custom device spec is used, 
       call the corresponding logical device API */
    if (ldev->CustomDevSpecType != DEV_SPEC_ORIGINAL) {
	int ret;
        ret = LFD_CopyBack(ldev, src_block, src_page, 
                           dest_block, dest_page);
	UFD_UNLOCK;
	return ret;
    }
#endif

#if STRICT_CHECK
    /* check if the block and page numbers are within the valid ranges */
    if (src_block >= ldev->DevSpec.NumBlocks) return(FM_ILLEGAL_ACCESS);
    if (dest_block >= ldev->DevSpec.NumBlocks) return(FM_ILLEGAL_ACCESS);
    if (src_page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
    if (dest_page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
#endif

    /* get the partition information for this logical device; 
       the block number is also adjusted as a relative value 
       in the corresponding partition */
    part_info = LFD_GetPartitionInfo(ldev, &src_block);
    src_pdev  = part_info->PDev;
    src_part  = part_info->Part;    
    
    part_info = LFD_GetPartitionInfo(ldev, &dest_block);
    dest_pdev  = part_info->PDev;
    dest_part  = part_info->Part;

    /* check if the target partition is write protected */
    if (dest_part->Protected) {
	UFD_UNLOCK;
	return(FM_PROTECT_ERROR);
    }

    /* get the block numbers in the chip-wide address space */
    src_block += src_part->StartBlock;
    dest_block += dest_part->StartBlock;

    if (src_pdev == dest_pdev) {
        /* source & destination blocks are on the same chip;
           therefore, the copy-back operation can be used */
        do {
            err = PFD_CopyBack(src_pdev, src_block, src_page, 
                               dest_block, dest_page);
        } while (err == FM_TRY_AGAIN);
    }
    else {
        /* source & destination blocks are on different chips;
           implement 'copy-back' using read & write */
        err = sw_copy_back(src_pdev, src_block, src_page, 
                           dest_pdev, dest_block, dest_page);
    }

    UFD_UNLOCK;
    return(err);
}

     
extern INT32  
FD_CopyBackGroup(UINT32 dev_id, UINT32 src_group, UINT16 src_page, 
                 UINT32 dest_group, UINT16 dest_page, INT32 *flag)
{
    INT32 err = FM_SUCCESS, *flag2;
    UINT16 planes_per_part, plane_offset, s_page[MAX_PHYSICAL_PLANES];
    UINT32 i, j;
    UINT32 src_block, dest_block;
    UINT32 org_src_block, org_dest_block;
    UINT32 s_block[MAX_PHYSICAL_PLANES], d_block[MAX_PHYSICAL_PLANES];

    LFDEV *ldev;
    PFDEV *src_pdev, *dest_pdev;
    FLASH_PART  *src_part, *dest_part;
    FLASH_RPART *part_info;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);
    
    AVOID_MONOPOLIZATION(ldev);

    UFD_LOCK;

#if USE_CUSTOM_DEV_SPEC
    /* if a custom device spec is used, 
       call the corresponding logical device API */
    if (ldev->CustomDevSpecType != DEV_SPEC_ORIGINAL) {
	int ret;
        ret = LFD_CopyBackGroup(ldev, src_group, src_page, 
                                dest_group, dest_page, flag);
	UFD_UNLOCK;
	return ret; 
    }
#endif

#if STRICT_CHECK
    /* check if the block and page numbers are within the valid ranges */
    if (src_group >= (ldev->DevSpec.NumBlocks / ldev->DevSpec.NumPlanes)) 
        return(FM_ILLEGAL_ACCESS);
    if (dest_group >= (ldev->DevSpec.NumBlocks / ldev->DevSpec.NumPlanes)) 
        return(FM_ILLEGAL_ACCESS);
    if (src_page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
    if (dest_page >= ldev->DevSpec.PagesPerBlock) return(FM_ILLEGAL_ACCESS);
#endif

    /* calculate the first block number in each of the src & dst groups */
    org_src_block = src_group << BITS[ldev->DevSpec.NumPlanes];
    org_dest_block = dest_group << BITS[ldev->DevSpec.NumPlanes];

    /* calculate some important constants */
    planes_per_part = ldev->DevSpec.NumPlanes >> BITS[ldev->InterleavingLevel];

    /* perform the corresponding group operation for each partition */
    for (i = 0; i < ldev->InterleavingLevel; i++) {

        /* calculate the block numbers in each partition */
        plane_offset = i << BITS[planes_per_part];
        src_block = org_src_block + plane_offset;
        dest_block = org_dest_block + plane_offset;

        /* get the information about the partitions where these blocks exist;
           the block numbers are also adjusted as a relative value 
           in the corresponding partition */
        part_info = LFD_GetPartitionInfo(ldev, &src_block);
        src_pdev  = part_info->PDev;
        src_part  = part_info->Part;
                
        part_info = LFD_GetPartitionInfo(ldev, &dest_block);
        dest_pdev  = part_info->PDev;
        dest_part  = part_info->Part;
        
        /* check if the target partition is write protected */
        if (dest_part->Protected) {
	    UFD_UNLOCK;
	    return(FM_PROTECT_ERROR);
	}

        /* set temporary function call parameters */
        flag2 = &flag[plane_offset];

        /* check if source & destination blocks are on the same chip */
        if (src_pdev == dest_pdev) {

            /* ok, the physical copy-back operation can be performed */
            do {
                /* calculate the block numbers in the chip-wide address space;
                   because the block numbers can be modified in the physical 
                   layer, we should re-calculate those numbers for every loop 
                   iteration */
                for (j = 0; j < planes_per_part; j++) {
                    s_block[j] = src_block + src_part->StartBlock + j;
                    d_block[j] = dest_block + dest_part->StartBlock + j;
                    s_page[j]  = src_page;
                }
            
                /* copy-back group */
                err = PFD_CopyBackGroup(src_pdev, s_block, s_page, 
                                        d_block, dest_page, flag2);
            } while (err == FM_TRY_AGAIN);
        }
        else {
            /* source & destination blocks are on different chips;
               implement 'copy-back' using read & write */
            for (j = 0; j < planes_per_part; j++) {
                if (flag2[j]) {

                    /* get the block numbers in the chip-wide address space */
                    s_block[j] = src_block + src_part->StartBlock + j;
                    d_block[j] = dest_block + dest_part->StartBlock + j;

                    /* perform S/W copy-back */                    
                    flag2[j] = sw_copy_back(src_pdev, s_block[j], src_page, 
                                            dest_pdev, d_block[j], dest_page);
                    if (flag2[j]) err = flag2[j];
                }
            }
        }
    }

    UFD_UNLOCK;
    return(err);
}


extern INT32  
FD_Erase(UINT32 dev_id, UINT32 block)
{
    INT32 err;
    LFDEV *ldev;
    PFDEV *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);
    
    AVOID_MONOPOLIZATION(ldev);

    UFD_LOCK;

#if USE_CUSTOM_DEV_SPEC
    /* if a custom device spec is used, 
       call the corresponding logical device API */
    if (ldev->CustomDevSpecType != DEV_SPEC_ORIGINAL) {
	int ret;
        ret = LFD_Erase(ldev, block);
	UFD_UNLOCK;
	return ret; 
    }
#endif

#if STRICT_CHECK
    /* check if the block number is within the valid range */
    if (block >= ldev->DevSpec.NumBlocks) return(FM_ILLEGAL_ACCESS);
#endif

    /* get the partition information for this logical device; 
       the block number is also adjusted as a relative value 
       in the corresponding partition */
    part_info = LFD_GetPartitionInfo(ldev, &block);
    pdev = part_info->PDev;
    part = part_info->Part;

    /* check if this logical device is write protected */
    if (part->Protected) {
	UFD_UNLOCK;
	return(FM_PROTECT_ERROR);
    }

    /* get the block number in the chip-wide address space */
    block += part->StartBlock;

    /* perform the physical device operation until it succeeds or fails */
    do {
        err = PFD_Erase(pdev, block);
    } while (err == FM_TRY_AGAIN);
    
    UFD_UNLOCK;
    return(err);
}


extern INT32  
FD_EraseGroup(UINT32 dev_id, UINT32 group, INT32 *flag)
{
    INT32  err = FM_SUCCESS, *flag2;
    UINT32 block, org_block, b[MAX_PHYSICAL_PLANES];
    UINT16 i, j, planes_per_part, plane_offset;

    LFDEV  *ldev;
    PFDEV  *pdev;
    FLASH_PART  *part;
    FLASH_RPART *part_info;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);

    AVOID_MONOPOLIZATION(ldev);

    UFD_LOCK;

#if USE_CUSTOM_DEV_SPEC
    /* if a custom device spec is used, 
       call the corresponding logical device API */
    if (ldev->CustomDevSpecType != DEV_SPEC_ORIGINAL) {
	int ret;
        ret = LFD_EraseGroup(ldev, group, flag);
	UFD_UNLOCK;
	return ret;
    }
#endif

#if STRICT_CHECK
    /* check if the group number is within the valid range */
    if (group >= (ldev->DevSpec.NumBlocks / ldev->DevSpec.NumPlanes)) 
        return(FM_ILLEGAL_ACCESS);
#endif

    /* calculate the first block number in this group */
    org_block = group << BITS[ldev->DevSpec.NumPlanes];
    
    /* calculate some important constants */
    planes_per_part = ldev->DevSpec.NumPlanes >> BITS[ldev->InterleavingLevel];

    /* perform the corresponding group operation for each partition */
    for (i = 0; i < ldev->InterleavingLevel; i++) {

        /* calculate the block number in each partition */
        plane_offset = i << BITS[planes_per_part];
        block = org_block + plane_offset;

        /* get the information about the partition where this block exists;
           the block number is also adjusted as a relative value 
           in the corresponding partition */
        part_info = LFD_GetPartitionInfo(ldev, &block);
        pdev = part_info->PDev;
        part = part_info->Part;

        /* check if this logical device is write protected */
        if (part->Protected) {
            UFD_UNLOCK;
            return(FM_PROTECT_ERROR);
        }

        /* set temporary function call parameters */
        flag2 = &flag[plane_offset];
        
        /* ok, perform the multi-plane operation */
        do {
            /* calculate the block numbers in the chip-wide address space;
               because the block numbers can be modified in the physical layer,
               we should re-calculate those numbers for every loop iteration */
            b[0] = block + part->StartBlock;
            for (j = 1; j < planes_per_part; j++) {
                b[j] = b[0] + j;
            }
            
            /* erase block group */
            err = PFD_EraseGroup(pdev, b, flag2);
        } while (err == FM_TRY_AGAIN);
    }

    UFD_UNLOCK;
    return(err);
}


extern INT32  
FD_Sync(UINT32 dev_id)
{
    INT32 err = FM_SUCCESS;
    UINT16 i;
    LFDEV *ldev;
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);
    
    UFD_LOCK;

    /* perform the sync operation for each constituent physical devices */
    for (i = 0; i < ldev->PartInfo.NumPartitions; i++) {
        do {
            err = PFD_Sync(ldev->PartInfo.Parts[i].PDev);
        } while (err == FM_TRY_AGAIN);
        if (err) break;
    }
    
    UFD_UNLOCK;
    return(err);
}


extern INT32  
FD_GetDeviceInfo(UINT32 dev_id, FLASH_SPEC *dev_info)
{
    LFDEV *ldev;

    /* check if parameters are valid */
    if (dev_info == NULL) return(FM_ERROR);
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);
    
    /* get the original flash spec. of the designated logical device */
    MEMCPY(dev_info, &ldev->DevSpec, sizeof(FLASH_SPEC));
    
    return(FM_SUCCESS);
}


extern INT32  
FD_SetCustomDeviceInfo(UINT32 dev_id, FLASH_SPEC *dev_info)
{
#if USE_CUSTOM_DEV_SPEC
    LFDEV *ldev;
    
    /* check if parameters are valid */
    if (dev_info == NULL) return(FM_ERROR);
    
    /* get the designated logical device */
    ldev = LFD_GetLogicalDevice(dev_id);
    if (ldev == NULL) return(FM_DEVICE_NOT_OPEN);
    
    /* if the given flash spec. differs from the original,
       set the custom flash spec. of the designated logical device */
    if (MEMCMP(&ldev->DevSpec, dev_info, sizeof(FLASH_SPEC))) {
        MEMCPY(&ldev->CustomDevSpec, dev_info, sizeof(FLASH_SPEC));
        
        /* determine the type of the custom device spec. */
        if (dev_info->NumPlanes > ldev->DevSpec.NumPlanes) {
            ldev->CustomDevSpecType = DEV_SPEC_EMUL_SMALL_MULTI;
        }
        else {
            ldev->CustomDevSpecType = DEV_SPEC_GENERIC_CUSTOM;
        }
    }
    else {
        ldev->CustomDevSpecType = DEV_SPEC_ORIGINAL;
    }
    
    return(FM_SUCCESS);
#else
    return(FM_ERROR);
#endif
}


/*----------------------------------------------------------------------*/
/*  Additional APIs for Flash Memory Volume Management                  */
/*----------------------------------------------------------------------*/

extern UINT16
FD_GetNumberOfChips(void)
{
    return(PFD_GetNumberOfChips());
}


extern INT32 
FD_ReadChipDeviceCode(UINT16 chip_id, UINT8 *maker, UINT8 *dev_code)
{
    return(PFD_ReadChipDeviceCode(chip_id, maker, dev_code));
}


extern INT32 
FD_GetChipDeviceInfo(UINT16 chip_id, FLASH_SPEC *dev_info)
{
    return(PFD_GetChipDeviceInfo(chip_id, dev_info));
}


extern INT32
FD_Format(UINT16 chip_id)
{
#if USE_DLBM
    return(BM_Format(chip_id, TRUE));
#else
    return(FM_SUCCESS);
#endif
}


extern INT32 
FD_ReadPartitionTable(UINT16 chip_id, FLASH_PARTTAB *part_tab)
{
    int ret;
    UFD_LOCK;
    ret = LFD_ReadPartitionTable(chip_id, part_tab);
    UFD_UNLOCK;
    return ret;
}


extern INT32 
FD_WritePartitionTable(UINT16 chip_id, FLASH_PARTTAB *part_tab)
{
    int ret;
    UFD_LOCK;
    ret = LFD_WritePartitionTable(chip_id, part_tab);
    UFD_UNLOCK;
    return ret;
}


extern INT32  
FD_GetNumBadBlocks(UINT16 chip_id)
{
#if USE_DLBM
    return(BM_GetNumBadBlocks(chip_id));
#else
    return(-1);
#endif
}


extern INT32  
FD_EraseAll(UINT16 chip_id)
{
    int ret;
    UFD_LOCK;
    ret = PFD_EraseAll(chip_id);
    UFD_UNLOCK;
    return ret;
}


extern INT32  
FD_ErasePart(UINT16 chip_id, UINT16 part_no)
{
    int ret;
    UFD_LOCK;
    ret = LFD_ErasePart(chip_id, part_no);
    UFD_UNLOCK;
    return ret;
}

/*
extern int is_recovery_mode(void);
EXPORT_SYMBOL(is_recovery_mode);
*/

#if defined(ZFLASH_LINUX) && defined(ZFLASH_KERNEL)
EXPORT_SYMBOL(FD_Init);
EXPORT_SYMBOL(FD_Open);
EXPORT_SYMBOL(FD_Close);
EXPORT_SYMBOL(FD_ReadPage);
EXPORT_SYMBOL(FD_ReadPageGroup);
EXPORT_SYMBOL(FD_WritePage);
EXPORT_SYMBOL(FD_WritePageGroup);
EXPORT_SYMBOL(FD_CopyBack);
EXPORT_SYMBOL(FD_CopyBackGroup);
EXPORT_SYMBOL(FD_Erase);
EXPORT_SYMBOL(FD_EraseGroup);
EXPORT_SYMBOL(FD_Sync);
EXPORT_SYMBOL(FD_GetDeviceInfo);
EXPORT_SYMBOL(FD_SetCustomDeviceInfo);
EXPORT_SYMBOL(FD_GetNumberOfChips);
EXPORT_SYMBOL(FD_ReadChipDeviceCode);
EXPORT_SYMBOL(FD_GetChipDeviceInfo);
EXPORT_SYMBOL(FD_Format);
EXPORT_SYMBOL(FD_ReadPartitionTable);
EXPORT_SYMBOL(FD_WritePartitionTable);
EXPORT_SYMBOL(FD_GetNumBadBlocks);
EXPORT_SYMBOL(FD_EraseAll);
EXPORT_SYMBOL(FD_ErasePart);
EXPORT_SYMBOL(FD_ECC_Corrected);
EXPORT_SYMBOL(FD_UserSequence);
#endif


/*----------------------------------------------------------------------*/
/*  USB Event Call-back Function                                        */
/*----------------------------------------------------------------------*/

#if (USE_BUFFERING_FOR_UMS == 1)

static INT32 
pm_usb_event_callback(ss_pm_request_t event)
{
	switch (event) {
	case SS_PM_USB_INSERTED:
        usb_ftl_connected = 1;
		break;

	case SS_PM_USB_REMOVED:
        usb_ftl_connected = 0;
		break;
	}

	return 0;
}

#endif


/*======================================================================*/
/*  Local Function Definitions                                          */
/*======================================================================*/

static INT32
sw_copy_back(PFDEV *src_pdev, UINT32 src_block, UINT16 src_page, 
             PFDEV *dest_pdev, UINT32 dest_block, UINT16 dest_page)
{
    INT32 err;
    UINT8 *pbuf;

    /* get the current I/O buffer */
    pbuf = PFD_GetBuffer(dest_pdev, OP_READ);

    /* read the source page */
    do {
        err = PFD_ReadPage(src_pdev, src_block, src_page, 0,
                           src_pdev->DevSpec.SectorsPerPage,
                           pbuf, pbuf + src_pdev->DevSpec.DataSize);
    } while (err == FM_TRY_AGAIN);
    
    if (err) goto end;

    /* write data in the source page into the destination page */
    do {
        err = PFD_WritePage(dest_pdev, dest_block, dest_page, 0,
                            dest_pdev->DevSpec.SectorsPerPage,
                            pbuf, pbuf + dest_pdev->DevSpec.DataSize, 
                            TRUE);
    } while (err == FM_TRY_AGAIN);
    
end:
    return(err);
}
