/************************************************************************/
/*                                                                      */
/*  Copyright (c) 2003-2006 Zeen Information Technologies, Inc.         */
/*  All rights reserved.                                                */
/*                                                                      */
/*  This software is the confidential and proprietary information of    */
/*  Zeen Information Technologies, Inc. ("Confidential Information")    */
/*  You shall not disclose such confidential information and shall use  */
/*  it only in accordance with the terms of the license agreement you   */
/*  entered into the above copyright holder.                            */
/*                                                                      */
/************************************************************************/
/*  This file implements the flash raw block device driver.             */
/*                                                                      */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : RFS Flash File System for NAND Flash Memory               */
/*  FILE    : fm_raw_blkdev.c                                           */
/*  PURPOSE : Code for Flash Raw Block Device Driver                    */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/*  - This layer resides between the CRAMFS file system and UFD.        */
/*  - The main purpose of this layer is to provide a raw block device   */
/*    interface to CRAMFS.                                              */
/*  - Becasue there is no logical to physical addressing re-mapping     */
/*    translation, only read-only file system (e.g. CRAMFS) can work    */
/*    on top of this block device driver.                               */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 2.0)                                          */
/*                                                                      */
/*  - 01/12/2003 [Joosun Hahn]  : First writing                         */
/*                                                                      */
/************************************************************************/

#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/bio.h>
#include <asm/errno.h>
#include <asm/uaccess.h>

#include "fm_global.h"
#include "fm_raw_blkdev.h"
#include "fd_if.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

#define RAW_BLKDEV_CONFIG_REQ_FUNCTION  1   /* 1 : use request function
                                               0 : use make_request function
                                               (default = 1) */

#define MAJOR_NR                        LFD_BLK_DEVICE_RAW
#define DEVICE_NAME                     "ufdraw"
#define REAL_DEVICE_MAX_PARTITIONS      16
#define VIRTUAL_DEVICE_MAX_PARTITIONS   1

enum RAW_BLKDEV_TYPE {
    DEV_TYPE_REAL                       = 0,
    DEV_TYPE_VIRTUAL                    = 1
};

#define GET_IDS(chip_id, part_id, dev_id, minor)                            \
    do {                                                                    \
        chip_id = GET_RAWDEV_CHIP_ID(minor);                                \
        part_id = GET_RAWDEV_PART_ID(minor);                                \
        if (chip_id != RAW_BLKDEV_TABLE) {                                  \
            if (part_id == 0)                                               \
                part_id = ENTIRE_PARTITION;                                 \
            else                                                            \
                part_id--;                                                  \
        }                                                                   \
        dev_id = SET_DEV_ID(MAJOR_NR, SET_RAWDEV_SERIAL(chip_id, part_id)); \
    } while (0)

/* DMSG() macro - for debugging */

#if 0
#define DMSG(DebugLevel, fmt_and_args)  \
        _DMSG(DMSG_UFD, DebugLevel, fmt_and_args)
#else
#define DMSG(DebugLevel, fmt_and_args)
#endif

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

typedef struct __RAW_BLOCK_DEVICE {
    unsigned int            type;           /* real or virtual device */
    FLASH_SPEC              spec;           /* flash device specification */

    spinlock_t              lock;
    struct request_queue    *queue;
    struct gendisk          *gd;
} RAW_BLKDEV;

/*----------------------------------------------------------------------*/
/*  Global Variable Definitions                                         */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

static RAW_BLKDEV    r_rawdev[MAX_FLASH_CHIPS];
static RAW_BLKDEV    v_rawdev[MAX_FLASH_VPARTITIONS];
static FLASH_PARTTAB ufd_parttab[MAX_FLASH_CHIPS];

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

static RAW_BLKDEV *get_bdev(unsigned int minor);
static int identify_raw_blkdev(void);


/*======================================================================*/
/*  Raw Block Device Driver Functions                                   */
/*======================================================================*/

static int
ufdraw_open(struct inode *inode, struct file *file)
{
    int err;
    unsigned int minor = MINOR(inode->i_rdev);
    unsigned int chip_id, part_id, dev_id;
    RAW_BLKDEV *bdev;

    GET_IDS(chip_id, part_id, dev_id, minor);

    bdev = get_bdev(minor);
    if (bdev == NULL) return -ENODEV;

    //spin_lock(&bdev->lock);
    err = FD_Open(dev_id);
    //spin_unlock(&bdev->lock);

    if (err) {
        DMSG(DL1, ("FD_Open() error: %d\n", err));
        return -ENODEV;
    }

    return 0;
}


static int
ufdraw_release(struct inode *inode, struct file *file)
{
    unsigned int minor = MINOR(inode->i_rdev);
    unsigned int chip_id, part_id, dev_id;
    RAW_BLKDEV *bdev;

    GET_IDS(chip_id, part_id, dev_id, minor);

    bdev = get_bdev(minor);
    if (bdev == NULL) return -ENODEV;

    //spin_lock(&bdev->lock);
    FD_Close(dev_id);
    //spin_unlock(&bdev->lock);

    return 0;
}


static int
ufdraw_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
    unsigned int minor = MINOR(dev->bd_dev);
    unsigned int cylinders, heads, sectors;
    unsigned int chip_id, part_id, dev_id;
    FLASH_SPEC spec;
    
    GET_IDS(chip_id, part_id, dev_id, minor);

    if (chip_id != RAW_BLKDEV_TABLE && part_id != ENTIRE_PARTITION) 
        return -EACCES;

    if (FD_Open(dev_id)) return -EACCES;
    FD_GetDeviceInfo(dev_id, &spec);
    FD_Close(dev_id);

    cylinders = spec.NumBlocks;
    heads     = spec.PagesPerBlock;
    sectors   = spec.SectorsPerPage;
    
    while (cylinders >= 1024) {
        if (heads <= 128) {
            heads <<= 1;
            cylinders >>= 1;
        }
        else if (sectors < 32) {
            sectors <<= 1;
            cylinders >>= 1;
        }
        else {
            break;
        }
    }
    
    if (heads == 256)
        heads--;

    while (1) {
        if (((spec.BlockSize * spec.NumBlocks) >> SECTOR_BITS) < 
            ((cylinders + 1) * heads * sectors))
            break;
    
        cylinders++;
    }

    geo->cylinders = cylinders;
    geo->heads     = heads; 
    geo->sectors   = sectors;
    geo->start     = 0;

    return 0;
}


static int
ufdraw_ioctl(struct inode *inode, struct file *file, 
             unsigned int cmd, unsigned long arg)
{
    int err;
    unsigned int minor = MINOR(inode->i_rdev);
    unsigned int chip_id, part_id, dev_id;
    
    GET_IDS(chip_id, part_id, dev_id, minor);

    switch(cmd) {
        case BLKFLSBUF:
        {
            /* the core code does the work, we have nothing to do */
            return 0;
        }    

        case HDIO_GETGEO:
        {
            struct hd_geometry geo;

            if (ufdraw_getgeo(inode->i_bdev, &geo) < 0)
                return -ENODEV;

            return copy_to_user((char *) arg, (char *) &geo, sizeof(geo));
        }

        case UFD_GET_DEV_INFO:
        {
            FLASH_SPEC spec;
            UFD_DEVINFO_T info;

            if (part_id != ENTIRE_PARTITION) return -EACCES;

            if (FD_Open(dev_id)) return -EACCES;
            FD_GetDeviceInfo(dev_id, &spec);
            FD_Close(dev_id);
        
            info.phy_blk_size = spec.BlockSize;
            info.num_blocks = spec.NumBlocks;
    
            return copy_to_user((char *) arg, (char *) &info, sizeof(info));
        }

        case UFD_GET_PARTITION:
        {
            UFD_PARTTAB_T tab;
            int i;

            if (part_id != ENTIRE_PARTITION) return -EACCES;
            
            tab.num_parts = ufd_parttab[chip_id].NumPartitions;
            for (i = 0; i < tab.num_parts; i++) {
                tab.part_size[i]    = ufd_parttab[chip_id].Part[i].NumBlocks; 
                tab.part_class[i]   = ufd_parttab[chip_id].Part[i].DevClass; 
                tab.in_dev_table[i] = ufd_parttab[chip_id].Part[i].InDevTable;
                tab.dev_index[i]    = ufd_parttab[chip_id].Part[i].DevIndex;
            }

            return copy_to_user((char *) arg, (char *) &tab, sizeof(tab));
        }

        case UFD_SET_PARTITION:
        {
            UFD_PARTTAB_T tab;
            FLASH_PARTTAB tmp_tab;
            int i, sum_blks;
            
            if (part_id != ENTIRE_PARTITION) return -EACCES;

            if (copy_from_user((char *) &tab, (char *) arg, sizeof(tab)))
                return -EFAULT;

            tmp_tab.NumPartitions = tab.num_parts;
            for (i = 0, sum_blks = 0; i < tab.num_parts; i++) {
                tmp_tab.Part[i].StartBlock = sum_blks;
                tmp_tab.Part[i].NumBlocks  = tab.part_size[i]; 
                tmp_tab.Part[i].DevClass   = tab.part_class[i]; 
                tmp_tab.Part[i].InDevTable = tab.in_dev_table[i];
                tmp_tab.Part[i].DevIndex   = tab.dev_index[i];
                tmp_tab.Part[i].Protected  = 0;
                tmp_tab.Part[i].ECCMode    = 0;

                sum_blks += tmp_tab.Part[i].NumBlocks;
            }

            if ((err = FD_WritePartitionTable(chip_id, &tmp_tab)) != 0) {
                DMSG(DL1, ("FD_WritePartitionTable() error: %d\n", err));
                return -EIO;
            }

            if (identify_raw_blkdev() < 0) {
                return -EIO;
            }
            
            /* first, delete the existing partitions */
            for (i = 0; i < REAL_DEVICE_MAX_PARTITIONS; i++) {
                delete_partition(r_rawdev[chip_id].gd, i + 1);
            }
            
            /* add new partitions for this disk */
            for (i = 0; i < ufd_parttab[chip_id].NumPartitions; i++) {
                UINT32 start_sector, num_sectors;
                start_sector = (ufd_parttab[chip_id].Part[i].StartBlock * r_rawdev[chip_id].spec.BlockSize)
                                >> SECTOR_BITS;
                num_sectors  = (ufd_parttab[chip_id].Part[i].NumBlocks  * r_rawdev[chip_id].spec.BlockSize)
                                >> SECTOR_BITS;
                add_partition(r_rawdev[chip_id].gd, i + 1, (int)start_sector, (int)num_sectors, 0);
            }

            return 0;
        }

        case UFD_ERASE_PARTITION:
        { 
            if ((err = FD_ErasePart(chip_id, part_id)) != 0) {
                DMSG(DL1, ("FD_EraseAll() error: %d\n", err));
                return -EIO;
            }

            return 0;
        }

        case UFD_FORMAT:
        {
            if (part_id != ENTIRE_PARTITION) return -EACCES;

            if ((err = FD_Format(chip_id)) != 0) {
                DMSG(DL1, ("FD_Format() error: %d\n", err));
                return -EIO;
            }

            return 0;
        }

        case UFD_ERASE_ALL:
        {
            if (part_id != ENTIRE_PARTITION) return -EACCES;

            if ((err = FD_EraseAll(chip_id)) != 0) {
                DMSG(DL1, ("FD_EraseAll() error: %d\n", err));
                return -EIO;
            }

            return 0;
        }
    }

    return -ENOTTY;
}


static struct block_device_operations ufdraw_fops = {
    .owner      = THIS_MODULE,
    .open       = ufdraw_open,
    .release    = ufdraw_release,
    .ioctl      = ufdraw_ioctl,
    .getgeo     = ufdraw_getgeo
};


/*----------------------------------------------------------------------*/
/*  Block I/O Fuctions (request processing, data transfer, etc.)        */
/*----------------------------------------------------------------------*/

static int
ufdraw_read(RAW_BLKDEV *bdev, unsigned int dev_id, char *buf, 
            unsigned long pos, unsigned long len)
{
    unsigned int block, page;
    FLASH_SPEC *s = &bdev->spec;
    int num, off, res;
    int err, i = 0;

    block = pos / s->BlockSize;
    page = (pos % s->BlockSize) / s->DataSize;
    off = pos % s->DataSize;

    num = (off + len) / s->DataSize;
    res = (off + len) % s->DataSize;

    if (off > 0) {
        i = (len < s->DataSize-off) ? len : s->DataSize-off;

        err = FD_ReadPage(dev_id, block, page, off >> SECTOR_BITS, 
                          i >> SECTOR_BITS, buf, NULL);
        if (err) {
            DMSG(DL1, ("FD_ReadPage() error 1: %d\n", err));
            return -1;
        }

        page++;
        num--;
    }

    while (num > 0) {
        if (page == s->PagesPerBlock) {
            block++; page = 0;
        }

        err = FD_ReadPage(dev_id, block, page, 0, s->SectorsPerPage, 
                          buf+i, NULL);
        if (err) {
            DMSG(DL1, ("FD_ReadPage() error 2: %d\n", err));
            return -1;
        }
    
        i += s->DataSize;

        page++;
        num--;
    }

    //if (res > 0) {
    if (len-i > 0) {
        if (page == s->PagesPerBlock) {
            block++; page = 0;
        }

        err = FD_ReadPage(dev_id, block, page, 0, (len-i) >> SECTOR_BITS, 
                          buf+i, NULL);
        if (err) {
            DMSG(DL1, ("FD_ReadPage() error 3: %d\n", err));
            return -1;
        }
    }

    return 0;
}


static int 
ufdraw_write(RAW_BLKDEV *bdev, unsigned int dev_id, char *buf, 
             unsigned long pos, unsigned long len)
{
    unsigned int block, page;
    FLASH_SPEC *s = &bdev->spec;
    int num, off, res;
    int err, i = 0;

    block = pos / s->BlockSize;
    page = (pos % s->BlockSize) / s->DataSize;
    off = pos % s->DataSize;

    num = (off + len) / s->DataSize;
    res = (off + len) % s->DataSize;

    if (page == 0) {
        FD_Erase(dev_id, block);
    }

    if (off > 0) {
        i = (len < s->DataSize - off) ? len : s->DataSize - off;

        err = FD_WritePage(dev_id, block, page, off >> SECTOR_BITS, 
                           i >> SECTOR_BITS, buf, NULL, 0);
        if (err) {
            DMSG(DL1, ("FD_WritePage() error 1: %d\n", err));
            return -1;
        }

        page++;
        num--;
    }
    
    while (num > 0) {
        if (page == s->PagesPerBlock) {
            block++; page = 0;
            FD_Erase(dev_id, block);
        }

        err = FD_WritePage(dev_id, block, page, 0, s->SectorsPerPage, 
                           buf+i, NULL, 0);
        if (err) {
            DMSG(DL1, ("FD_WritePage() error 2: %d\n", err));
            return -1;
        }

        i += s->DataSize;

        page++;
        num--;
    }

    //if (res > 0) {
    if (len-i > 0) {
        if (page == s->PagesPerBlock) {
            block++; page = 0;
            FD_Erase(dev_id, block);
        }

        err = FD_WritePage(dev_id, block, page, 0, (len-i) >> SECTOR_BITS,
                           buf+i, NULL, 0);
        if (err) {
            DMSG(DL1, ("FD_WritePage() error 3: %d\n", err));
            return -1;
        }
    }

    return 0;
}


static int
ufdraw_xfer_bio(RAW_BLKDEV *bdev, struct bio *bio)
{
    unsigned int minor = MINOR(bio->bi_bdev->bd_dev);
    unsigned int chip_id, part_id, dev_id;
    FLASH_SPEC spec;
    
    int i, part_size, err = -1;
    struct bio_vec *bvec;
    sector_t sector = bio->bi_sector;

    GET_IDS(chip_id, part_id, dev_id, minor);
    
    if (bdev->type == DEV_TYPE_REAL) {
        if (part_id == ENTIRE_PARTITION) {
            FD_GetDeviceInfo(dev_id, &spec);
            part_size = (spec.NumBlocks * spec.BlockSize) >> SECTOR_BITS;
        }
        else {
            part_size = (ufd_parttab[chip_id].Part[part_id].NumBlocks
                         * bdev->spec.BlockSize) >> SECTOR_BITS;
        }
    }
    else {
        part_size = (bdev->spec.NumBlocks
                     * bdev->spec.BlockSize) >> SECTOR_BITS;
    }

    if ((bio->bi_sector + (bio->bi_size >> SECTOR_BITS)) > part_size) {
        PRINT(KERN_WARNING "ufdraw: request past end of device\n");
        return -1;
    }
    
    bio_for_each_segment(bvec, bio, i) {

        char *pbuf = kmap(bvec->bv_page) + bvec->bv_offset;
        //char *pbuf = __bio_kmap_atomic(bio, i, KM_USER0);
        
        switch (bio_data_dir(bio)) {
    
        case READ:
        case READA:
            err = ufdraw_read(bdev, dev_id, pbuf, 
                              sector << SECTOR_BITS,
                              bio_cur_sectors(bio) << SECTOR_BITS);
            if (err) goto error;
            break;

        case WRITE:
            err = ufdraw_write(bdev, dev_id, pbuf,
                               sector << SECTOR_BITS,
                               bio_cur_sectors(bio) << SECTOR_BITS);
            if (err) goto error;
            break;
        }

        sector += bio_cur_sectors(bio);

        kunmap(bvec->bv_page);
        //__bio_kunmap_atomic(bio, KM_USER0);
    }

#if 1
    if (!err && bio_data_dir(bio) == WRITE) {
        FD_Sync(dev_id);
    }
#endif

    //bio_endio(bio, 0);
    return 0;

error:
    //bio_io_error(bio);
    return 0;    
}


#if (RAW_BLKDEV_CONFIG_REQ_FUNCTION == 1)

static int
ufdraw_xfer_request(RAW_BLKDEV *bdev, struct request *req)
{
    struct bio *bio;
    int nsect = 0;
    
    __rq_for_each_bio(bio, req) {
        ufdraw_xfer_bio(bdev, bio);
        nsect += (bio->bi_size >> 9);
    }
    
    return nsect;
}


static void 
ufdraw_request(request_queue_t *q)
{
    struct request *req; 
    int sectors_xferred;
    RAW_BLKDEV *bdev = q->queuedata;

    while ((req = elv_next_request(q)) != NULL) {
        if (! blk_fs_request(req)) {
            PRINT(KERN_NOTICE "skip non-fs request\n");
            end_request(req, 0);
            continue;
        }

        spin_unlock_irq(q->queue_lock);
        sectors_xferred = ufdraw_xfer_request(bdev, req);
        spin_lock_irq(q->queue_lock);

        if (! end_that_request_first(req, 1, sectors_xferred)) {
            blkdev_dequeue_request(req);
            end_that_request_last(req,1);
        }
    }
}


#else /* (RAW_BLKDEV_CONFIG_REQ_FUNCTION == 0) */

static int 
ufdraw_make_request(request_queue_t *q, struct bio *bio)
{
    RAW_BLKDEV *bdev = q->queuedata;
    int status;

    status = ufdraw_xfer_bio(bdev, bio);
    bio_endio(bio, bio->bi_size, status);
    return 0;
}

#endif /* (RAW_BLKDEV_CONFIG_REQ_FUNCTION == 0) */


/*======================================================================*/
/*  Local Functions                                                     */
/*======================================================================*/

static RAW_BLKDEV *get_bdev(unsigned int minor)
{
    unsigned int chip_id = GET_RAWDEV_CHIP_ID(minor);
    unsigned int part_id = GET_RAWDEV_PART_ID(minor);
    RAW_BLKDEV *bdev = NULL;

    if (chip_id != RAW_BLKDEV_TABLE) {
        if (chip_id < MAX_FLASH_CHIPS)
            bdev = &r_rawdev[chip_id];
    }
    else {
        if (part_id < MAX_FLASH_VPARTITIONS)
            bdev = &v_rawdev[part_id];
    }

    return bdev;
}


static int 
identify_raw_blkdev(void)
{
    int err, idx, i, j;
    int v_rawdev_found[MAX_FLASH_VPARTITIONS];
    unsigned int minor, dev_id;
    FLASH_PARTTAB *t;

    for (i = 0; i < MAX_FLASH_VPARTITIONS; i++) v_rawdev_found[i] = FALSE;

    for (i = 0; i < FD_GetNumberOfChips(); i++) {
        err = FD_GetChipDeviceInfo(i, &r_rawdev[i].spec);
        if (err) {
            DMSG(DL1, ("FD_GetChipDeviceInfo() error: %d\n", err));
            return -1;
        }

        err = FD_ReadPartitionTable(i, t=ufd_parttab+i);    
        if (err) {
            DMSG(DL1, ("FD_ReadPartitionTable() error: %d\n", err));
            return -1;
        }
 
        for (j = 0; j < t->NumPartitions; j++) { 
            
            idx = t->Part[j].DevIndex;

            if ((t->Part[j].DevClass == MAJOR_NR) && 
                (t->Part[j].InDevTable == TRUE) &&
                (v_rawdev_found[idx] == FALSE)) {

                v_rawdev_found[idx] = TRUE;

                minor = SET_RAWDEV_SERIAL(RAW_BLKDEV_TABLE, idx);
                dev_id = SET_DEV_ID(MAJOR_NR, minor);

                if (FD_Open(dev_id)) return -1;

                FD_GetDeviceInfo(dev_id, &v_rawdev[idx].spec);
                FD_Close(dev_id);
            }
        }
    }

    return 0;
}


/*======================================================================*/
/*  Module Init & Exit Fuctions                                         */
/*======================================================================*/

int __init ufdraw_init(void)
{
    int err, i, j;

    /* register the raw block device driver */
    if (register_blkdev(MAJOR_NR, DEVICE_NAME) < 0) {
        PRINT("RAW_BLKDEV: reserved major number cannot be used.\n");
        PRINT("RAW_BLKDEV: operation aborted.\n");
        return -1;
    }

#if 1
    /* initialize the UFD (Uniform Flash Device Driver) module */
    err = FD_Init();
    if (err) {
        PRINT("FD_Init failed.\n");
        unregister_blkdev(MAJOR_NR, DEVICE_NAME);
        return -1;
    }
#endif
    
    /* initialize some global variables */
    MEMSET(ufd_parttab, 0, sizeof(ufd_parttab));
    
    /* initialize each 'real' raw block device structure */
    for (i = 0; i < MAX_FLASH_CHIPS; i++) {
        r_rawdev[i].type = DEV_TYPE_REAL;
        MEMSET(&r_rawdev[i].spec, 0, sizeof(FLASH_SPEC));
        spin_lock_init(&r_rawdev[i].lock);
    }

    /* initialize each 'virtual' raw block device structure */
    for (i = 0; i < MAX_FLASH_VPARTITIONS; i++) {
        v_rawdev[i].type = DEV_TYPE_VIRTUAL;
        MEMSET(&v_rawdev[i].spec, 0, sizeof(FLASH_SPEC));
        spin_lock_init(&v_rawdev[i].lock);
    }

    /* read flash memory and identify raw block devices */
    if (identify_raw_blkdev() < 0) {
        FD_Shutdown();
        unregister_blkdev(MAJOR_NR, DEVICE_NAME);
        return -1;
    }

    /* initialize each 'real' raw block device structure (continued) */
    for (i = 0; i < MAX_FLASH_CHIPS; i++) {

        /* initialize the request queue */
#if (RAW_BLKDEV_CONFIG_REQ_FUNCTION == 1)
        r_rawdev[i].queue = blk_init_queue(ufdraw_request, &r_rawdev[i].lock);
#else
        r_rawdev[i].queue = blk_alloc_queue(GFP_KERNEL);
        blk_queue_make_request(r_rawdev[i].queue, ufdraw_make_request);
#endif
        r_rawdev[i].queue->queuedata = &r_rawdev[i];
        
        /* initialize the gendisk structure */
        r_rawdev[i].gd               = alloc_disk(REAL_DEVICE_MAX_PARTITIONS);
        r_rawdev[i].gd->major        = MAJOR_NR;
        r_rawdev[i].gd->first_minor  = i * REAL_DEVICE_MAX_PARTITIONS;
        r_rawdev[i].gd->minors       = REAL_DEVICE_MAX_PARTITIONS;
        r_rawdev[i].gd->fops         = &ufdraw_fops;
        r_rawdev[i].gd->queue        = r_rawdev[i].queue;
        r_rawdev[i].gd->private_data = &r_rawdev[i];

        sprintf(r_rawdev[i].gd->disk_name, "%s%d", "ufd", i);
        set_capacity(r_rawdev[i].gd, 
            (r_rawdev[i].spec.NumBlocks * r_rawdev[i].spec.BlockSize)
             >> SECTOR_BITS);

        add_disk(r_rawdev[i].gd);
        
        /* add partitions for this disk */
        for (j = 0; j < ufd_parttab[i].NumPartitions; j++) {
            UINT32 start_sector, num_sectors;
            start_sector = (ufd_parttab[i].Part[j].StartBlock * r_rawdev[i].spec.BlockSize)
                            >> SECTOR_BITS;
            num_sectors  = (ufd_parttab[i].Part[j].NumBlocks  * r_rawdev[i].spec.BlockSize)
                            >> SECTOR_BITS;
            add_partition(r_rawdev[i].gd, j + 1, (int)start_sector, (int)num_sectors, 0);
        }
    }

    /* initialize each 'virtual' raw block device structure (continued) */
    for (i = 0; i < MAX_FLASH_VPARTITIONS; i++) {

        /* initialize the request queue */
#if (RAW_BLKDEV_CONFIG_REQ_FUNCTION == 1)
        v_rawdev[i].queue = blk_init_queue(ufdraw_request, &v_rawdev[i].lock);
#else
        v_rawdev[i].queue = blk_alloc_queue(GFP_KERNEL);
        blk_queue_make_request(v_rawdev[i].queue, ufdraw_make_request);
#endif
        v_rawdev[i].queue->queuedata = &v_rawdev[i];
        
        /* initialize the gendisk structure */
        v_rawdev[i].gd               = alloc_disk(VIRTUAL_DEVICE_MAX_PARTITIONS);
        v_rawdev[i].gd->major        = MAJOR_NR;
        v_rawdev[i].gd->first_minor  = 0xF0 + i * VIRTUAL_DEVICE_MAX_PARTITIONS;
        v_rawdev[i].gd->minors       = VIRTUAL_DEVICE_MAX_PARTITIONS;
        v_rawdev[i].gd->fops         = &ufdraw_fops;
        v_rawdev[i].gd->queue        = v_rawdev[i].queue;
        v_rawdev[i].gd->private_data = &v_rawdev[i];

        sprintf(v_rawdev[i].gd->disk_name, "%s%c", DEVICE_NAME, 'a'+i);
        set_capacity(v_rawdev[i].gd, 
            (v_rawdev[i].spec.NumBlocks * v_rawdev[i].spec.BlockSize) 
             >> SECTOR_BITS);

        add_disk(v_rawdev[i].gd);
    }

    return 0;
}


void __exit ufdraw_exit(void)
{
    int i;
    
    for (i = 0; i < MAX_FLASH_CHIPS; i++) {
        del_gendisk(r_rawdev[i].gd);
        put_disk(r_rawdev[i].gd);
        
        blk_cleanup_queue(r_rawdev[i].queue);
    }

    for (i = 0; i < MAX_FLASH_VPARTITIONS; i++) {
        del_gendisk(v_rawdev[i].gd);
        put_disk(v_rawdev[i].gd);
        
        blk_cleanup_queue(v_rawdev[i].queue);
    }

    unregister_blkdev(MAJOR_NR, DEVICE_NAME);
    FD_Shutdown();
}


module_init(ufdraw_init);
module_exit(ufdraw_exit);

MODULE_LICENSE("Zeen Information Technologies, Inc. Proprietary");
MODULE_AUTHOR("Joosun Hahn <jshan@zeen.snu.ac.kr>");
MODULE_DESCRIPTION("Raw Block Device for Nand Flash");
