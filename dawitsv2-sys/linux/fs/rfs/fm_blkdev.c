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
/*  This file implements the flash block device driver.                 */
/*                                                                      */
/*  @author  Joosun Hahn                                                */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : RFS Flash File System for NAND Flash Memory               */
/*  FILE    : fm_blkdev.c                                               */
/*  PURPOSE : Code for Flash Block Device Driver                        */
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
/*  REVISION HISTORY (Ver 2.0)                                          */
/*                                                                      */
/*  - 01/12/2003 [Joosun Hahn]  : First writing                         */
/*  - 02/25/2007 [Joosun Hahn]  : Revised to fit into Linux Kernel 2.6  */
/*                                                                      */
/************************************************************************/

#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>
#include <linux/backing-dev.h>
#include <linux/highmem.h>
#include <asm/errno.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/arch-stmp37xx/ocram.h>

#include "fm_global.h"
#include "fm_blkdev.h"
#include "fd_if.h"
#include "ftl.h"
#include "rfs_api.h"
#include "rfs.h"

#include <asm/uaccess.h>
#include <asm/unistd.h>

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

#define FTL_BLKDEV_CONFIG_REQ_FUNCTION  0   /* 1 : use request function
                                               0 : use make_request function
                                               (default = 1) */

#define USE_BUFFERING_FOR_USB           1   /* 1 : use buffering for USB
                                               0 : do not use
                                               (default = 1) */
#define USE_OCRAM_BUF_FOR_USB           1
#define OCRAM_RFS2_START_VIRT           0xf1011800
#define USB2BDEV_BUF_SIZE               (32*1024)
#define USB2BDEV_BUF_SIZE_SECS          (USB2BDEV_BUF_SIZE >> SECTOR_BITS)
#define LLD_DESC_BUF_SIZE               (8*1024)

#define MAJOR_NR                        LFD_BLK_DEVICE_FTL
#define DEVICE_NAME                     "ufd"
#define DEVICE_MAX_PARTITIONS           16

#define RFS_AVAILABLE                   (get_fs_type("rfs") != NULL)

#define USE_SEMAPHORE                   1
#define USE_INTERRUPTIBLE_SEMA          0

#if (USE_SEMAPHORE == 1)
    #if (USE_INTERRUPTIBLE_SEMA == 1)
    #define FTL_LOCK            do {                                          \
                                    if (down_interruptible(&ftl_sema_lock))   \
                                        return -ERESTARTSYS;                  \
                                } while (0)
    #else
    #define FTL_LOCK            down(&ftl_sema_lock)
    #endif
    #define FTL_UNLOCK          up(&ftl_sema_lock)

    #if (USE_INTERRUPTIBLE_SEMA == 1)
    #define BDEV_LOCK           do {                                          \
                                    if (down_interruptible(&bdev_sema_lock))  \
                                        return -ERESTARTSYS;                  \
                                } while (0)
    #else
    #define BDEV_LOCK           down(&bdev_sema_lock)
    #endif
    #define BDEV_UNLOCK         up(&bdev_sema_lock)
    
    #if (USE_INTERRUPTIBLE_SEMA == 1)
    #define REQ_LOCK            do {                                          \
                                    if (down_interruptible(&req_sema_lock))   \
                                        return -ERESTARTSYS;                  \
                                } while (0)
    #else
    #define REQ_LOCK            down(&req_sema_lock)
    #endif
    #define REQ_UNLOCK          up(&req_sema_lock)
#else
    #define FTL_LOCK            spin_lock(&ftl_lock)
    #define FTL_UNLOCK          spin_unlock(&ftl_lock)
    #define BDEV_LOCK           spin_lock(&bdev_lock)
    #define BDEV_UNLOCK         spin_unlock(&bdev_lock)
    #define REQ_LOCK            spin_lock(&req_lock)
    #define REQ_UNLOCK          spin_unlock(&req_lock)
#endif

/* DMSG() macro - for debugging */
#if DEBUG_FTL
#define DMSG(DebugLevel, fmt_and_args)  \
        __DMSG(DMSG_FTL, DebugLevel, fmt_and_args)
#else
#define DMSG(DebugLevel, fmt_and_args)
#endif

/*----------------------------------------------------------------------*/
/*  Type Definitions                                                    */
/*----------------------------------------------------------------------*/

typedef struct __FTL_BLOCK_DEVICE {
    unsigned int            dev_id;         /* device ID */
    unsigned int            size;           /* size in # of sectors */
    unsigned int            sectors_in_pagegroup;   /* # of sectors in the
                                                       logical page group */
    unsigned int            usage_count;
    unsigned int            ftl_opened;

    RFS_PARTTAB_T           rfs_parttab;    /* RFS partition info */

    spinlock_t              lock;
    struct request_queue    *queue;
    struct gendisk          *gd;
} FTL_BLKDEV;

/*----------------------------------------------------------------------*/
/*  Global Variable Definitions                                         */
/*----------------------------------------------------------------------*/

RFS_VOL_OPS_T rfs_vol_fops;

EXPORT_SYMBOL(rfs_vol_fops);

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

static FTL_BLKDEV   ftl_blkdev[MAX_FTL_DEVICE];

#if (USE_BUFFERING_FOR_USB == 1)
static UINT8       *usb2bdev_buf = NULL;
static UINT8       *__usb2bdev_buf[2];
static UINT16       usb2bdev_buf_idx;
static UINT32       usb2bdev_buf_secno;
static UINT32       usb2bdev_buf_num_secs = 0;

extern BOOL         usb_ftl_connected;
static BOOL         usb_vol_mounted = 0;

extern BOOL         usb_in_msc;
extern BOOL         usb_in_mtp;
#endif

#if USE_SEMAPHORE
struct semaphore    ftl_sema_lock;
struct semaphore    bdev_sema_lock;
struct semaphore    req_sema_lock;
#else
static spinlock_t   ftl_lock;
static spinlock_t   bdev_lock;
static spinlock_t   req_lock;
#endif

static struct proc_dir_entry *proc_ftl_dir;
static struct proc_dir_entry *ftl_format_blkmap_proc;
static struct proc_dir_entry *ftl_scan_logblock_proc;

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

static int identify_ftl_blkdev(void);

#if (USE_BUFFERING_FOR_USB == 1)
static INT32 usb2bdev_read(INT32 dev, UINT32 secno, UINT8 *buf, UINT32 num_secs);
static INT32 usb2bdev_write(INT32 dev, UINT32 secno, UINT8 *buf, UINT32 num_secs);
static INT32 usb2bdev_sync(INT32 dev);
static INT32 usb2bdev_flush(INT32 dev);
#endif


/*======================================================================*/
/*  Dedicated Block Device Driver Functions for RFS                     */
/*======================================================================*/

INT32 bdev_open(INT32 dev)
{
    if (GET_DEV_CLASS(dev) == MAJOR_NR) {
        INT32  err, minor = GET_DEV_SERIAL(dev);
        FTL_BLKDEV *bdev  = &ftl_blkdev[GET_FTLDEV_INDEX(minor)];
        
        FTL_LOCK;
        err = ftl_open(bdev->dev_id);
        FTL_UNLOCK;
        return err;
    }
    else {
        struct block_device *bdev;

        bdev = open_by_devnum(dev, FMODE_READ|FMODE_WRITE);
        if (IS_ERR(bdev)) return -ENODEV;

#if USE_SEMAPHORE
        sema_init(&bdev_sema_lock, 1);
#else
        spin_lock_init(&bdev_lock);
#endif
    
        return 0;
    }
}


INT32 bdev_close(INT32 dev)
{
    if (GET_DEV_CLASS(dev) == MAJOR_NR) {
        INT32  err, minor = GET_DEV_SERIAL(dev);
        FTL_BLKDEV *bdev  = &ftl_blkdev[GET_FTLDEV_INDEX(minor)];
        
        FTL_LOCK;
        err = ftl_close(bdev->dev_id);
        FTL_UNLOCK;
        return err;
    }
    else {
        struct block_device *bdev;

        bdev = bdget(dev);
        if (!bdev) return -ENODEV;
    
        bd_release(bdev);
    
        return(blkdev_put(bdev));
    }
}


INT32 bdev_read(INT32 dev, UINT32 secno, UINT8 *buf, UINT32 num_secs)
{
    if (GET_DEV_CLASS(dev) == MAJOR_NR) {
        INT32  err, minor = GET_DEV_SERIAL(dev);
        FTL_BLKDEV *bdev  = &ftl_blkdev[GET_FTLDEV_INDEX(minor)];

#if (USE_BUFFERING_FOR_USB == 1)
        if (usb_ftl_connected &&
            (usb_in_mtp || (usb_in_msc && !usb_vol_mounted))) {
            return usb2bdev_read(bdev->dev_id, secno, buf, num_secs);
        }
        else {
            if (usb2bdev_buf_num_secs > 0) {
                /* previously buffered requests are remaining in usb2bdev_buf;
                   flush them first */
                err = usb2bdev_flush(bdev->dev_id);
                if (err) return err;
            }
        }
#endif
        FTL_LOCK;
        err = ftl_read_sectors(bdev->dev_id, secno, buf, num_secs);
        FTL_UNLOCK;
        return err;
    }
    else {
        struct block_device *bdev;
        struct super_block *sb;
        struct buffer_head *bh;
        int blkno, num_blks, secs_per_blk, i;
        int sec_off_in_blk, num_secs_in_blk;
    
        bdev = bdget(dev);
        if (!bdev) return -ENODEV;
        
        BDEV_LOCK;
        sb = (struct super_block *) bdev->bd_private;
    
        blkno = secno >> (sb->s_blocksize_bits - SECTOR_BITS);
        secs_per_blk = sb->s_blocksize >> SECTOR_BITS;
        sec_off_in_blk = secno & (secs_per_blk - 1);
        num_blks = (sec_off_in_blk + num_secs + (secs_per_blk - 1)) >> (sb->s_blocksize_bits - SECTOR_BITS);
    
        for (i = 0; i < num_blks; i++) {
    
            bh = sb_bread(sb, blkno + i);
            if (!bh) {
                BDEV_UNLOCK;
                return -1;
            }
    
            if (i > 0) {
                sec_off_in_blk = 0;
            }
    
            if (i == (num_blks - 1)) {
                num_secs_in_blk = num_secs;
            }
            else {
                num_secs_in_blk = secs_per_blk;
            }
            if (num_blks > 1) {
                num_secs_in_blk -= sec_off_in_blk;
            }
    
            lock_buffer(bh);
    
            MEMCPY(buf, bh->b_data + (sec_off_in_blk << SECTOR_BITS), 
                   (num_secs_in_blk << SECTOR_BITS));
    
            buf += (num_secs_in_blk << SECTOR_BITS);
            num_secs -= num_secs_in_blk;
    
            unlock_buffer(bh);
    
            __brelse(bh);
        }
    
        BDEV_UNLOCK;
        return 0;
    }
}


INT32 bdev_write(INT32 dev, UINT32 secno, UINT8 *buf, UINT32 num_secs)
{
    if (GET_DEV_CLASS(dev) == MAJOR_NR) {
        INT32  err, minor = GET_DEV_SERIAL(dev);
        FTL_BLKDEV *bdev  = &ftl_blkdev[GET_FTLDEV_INDEX(minor)];

#if (USE_BUFFERING_FOR_USB == 1)
        if (usb_ftl_connected &&
            (usb_in_mtp || (usb_in_msc && !usb_vol_mounted))) {
            return usb2bdev_write(bdev->dev_id, secno, buf, num_secs);
        }
        else {
            if (usb2bdev_buf_num_secs > 0) {
                /* previously buffered requests are remaining in usb2bdev_buf;
                   flush them first */
                err = usb2bdev_flush(bdev->dev_id);
                if (err) return err;
            }
        }
#endif
        FTL_LOCK;
        err = ftl_write_sectors(bdev->dev_id, secno, buf, num_secs);
        FTL_UNLOCK;
        return err;
    }
    else {
        struct block_device *bdev;
        struct super_block *sb;
        struct buffer_head *bh;
        int blkno, num_blks, secs_per_blk, i;
        int sec_off_in_blk, num_secs_in_blk;
    
        bdev = bdget(dev);
        if (!bdev) return -ENODEV;
    
        BDEV_LOCK;
        sb = (struct super_block *) bdev->bd_private;
    
        blkno = secno >> (sb->s_blocksize_bits - SECTOR_BITS);
        secs_per_blk = sb->s_blocksize >> SECTOR_BITS;
        sec_off_in_blk = secno & (secs_per_blk - 1);
        num_blks = (sec_off_in_blk + num_secs + (secs_per_blk - 1)) >> (sb->s_blocksize_bits - SECTOR_BITS);
    
        for (i = 0; i < num_blks; i++) {
            bh = __getblk(bdev, blkno + i, sb->s_blocksize);
            
            if (i > 0) {
                sec_off_in_blk = 0;
            }
    
            if (i == (num_blks - 1)) {
                num_secs_in_blk = num_secs;
            }
            else {
                num_secs_in_blk = secs_per_blk;
            }
            if (num_blks > 1) {
                num_secs_in_blk -= sec_off_in_blk;
            }
            
            lock_buffer(bh);
    
            MEMCPY(bh->b_data + (sec_off_in_blk << SECTOR_BITS), buf, 
                   (num_secs_in_blk << SECTOR_BITS));
    
            buf += (num_secs_in_blk << SECTOR_BITS);
            num_secs -= num_secs_in_blk;
    
            mark_buffer_dirty(bh);
            set_buffer_uptodate(bh);
            unlock_buffer(bh);
    
            __brelse(bh);
        }
    
        BDEV_UNLOCK;
        return 0;
    }
}


INT32 bdev_sync(INT32 dev)
{
    if (GET_DEV_CLASS(dev) == MAJOR_NR) {
        INT32  err, minor = GET_DEV_SERIAL(dev);
        FTL_BLKDEV *bdev  = &ftl_blkdev[GET_FTLDEV_INDEX(minor)];

#if (USE_BUFFERING_FOR_USB == 1)
        if (usb_ftl_connected &&
            (usb_in_mtp || (usb_in_msc && !usb_vol_mounted))) {
            return usb2bdev_sync(bdev->dev_id);
        }
        else {
            if (usb2bdev_buf_num_secs > 0) {
                /* previously buffered requests are remaining in usb2bdev_buf;
                   flush them first */
                err = usb2bdev_flush(bdev->dev_id);
                if (err) return err;
            }
        }
#endif
        FTL_LOCK;
        err = ftl_sync(bdev->dev_id);
        FTL_UNLOCK;
        return err;
    }
    else {
        struct block_device *bdev;
    
        bdev = bdget(dev);
        if (!bdev) return -ENODEV;
    
        return sync_blockdev(bdev);
    }
}


INT32 bdev_ioctl(INT32 dev, UINT32 cmd, void *arg)
{
    if (GET_DEV_CLASS(dev) == MAJOR_NR) {
        INT32  err, minor = GET_DEV_SERIAL(dev);
        FTL_BLKDEV *bdev  = &ftl_blkdev[GET_FTLDEV_INDEX(minor)];

        if (minor & 0xf) return -EACCES;

        switch (cmd) {

            case FTL_GET_DEV_INFO:
            {
                int ftl_opened = FALSE;
                FTL_DEVINFO_T *binfo = (FTL_DEVINFO_T *) arg;
                FTL_STAT_T stat;
                
                FTL_LOCK;
                if (bdev->ftl_opened == FALSE) {
                    if ((err = ftl_open(bdev->dev_id))) {
                        FTL_UNLOCK;
                        DMSG(DL1, ("ftl_open failed: %d\n", err));
                        return -ENODEV;
                    }

                    ftl_opened = TRUE;
                }

                if ((err = ftl_stat(bdev->dev_id, &stat)) != FTL_SUCCESS) {
                    FTL_UNLOCK;
                    DMSG(DL1, ("ftl_stat failed: %d\n", err));
                    return -ENODEV;
                }

                if (ftl_opened) {
                    ftl_close(bdev->dev_id);
                }
                FTL_UNLOCK;

                binfo->heads = stat.BlockSize / stat.PageSize;
                binfo->sectors = (stat.BlockSize >> SECTOR_BITS) / binfo->heads;
                binfo->cylinders = (stat.NumSector << SECTOR_BITS) / stat.BlockSize;
                binfo->ftl_num_sectors = stat.NumSector;
                binfo->ftl_block_size = stat.BlockSize;
                binfo->ftl_page_size = stat.PageSize;
        
                while (binfo->cylinders >= 1024) {
                    if (binfo->heads <= 128) {
                        binfo->heads <<= 1;
                        binfo->cylinders >>= 1;
                    }
                    else if (binfo->sectors < 32) {
                        binfo->sectors <<= 1;
                        binfo->cylinders >>= 1;
                    }
                    else {
                        break;
                    }
                }
        
                if (binfo->heads == 256)
                    binfo->heads--;
     
                while (1) {
                    if (stat.NumSector < ((binfo->cylinders+1) *
                                           binfo->heads *
                                           binfo->sectors))
                        break;
        
                    binfo->cylinders++;
                }
        
                return 0;
            }

            case FTL_FORMAT:
            {
                FTL_DEVINFO_T binfo;

                /* close the corresponding FTL device first */
                FTL_LOCK;
                if (bdev->ftl_opened == TRUE) {
                    ftl_close(bdev->dev_id);
                    bdev->ftl_opened = FALSE;
                }

                /* format the logical flash device for FTL */
                if ((err = ftl_format(bdev->dev_id))) {
                    FTL_UNLOCK;
                    DMSG(DL1, ("ftl_format failed: %d\n", err));
                    return -EIO;
                }

                /* open the FTL device again */
                if ((err = ftl_open(bdev->dev_id))) {
                    FTL_UNLOCK;
                    DMSG(DL1, ("ftl_open (after format) failed: %d\n", err));
                    return -EIO;
                }
                bdev->ftl_opened = TRUE;
                FTL_UNLOCK;

                /* re-calculate the device size */
                if ((err = bdev_ioctl(bdev->dev_id, FTL_GET_DEV_INFO, &binfo)) < 0) {
                    DMSG(DL1, ("bdev_ioctl(FTL_GET_DEV_INFO) failed: %d\n", err));
                    return -ENODEV;
                }

                FTL_LOCK;
                bdev->size = binfo.cylinders * binfo.heads * binfo.sectors;
                set_capacity(bdev->gd, bdev->size);
                FTL_UNLOCK;

                return 0;
            }

            case FTL_SYNC:
            {
                if ((err = bdev_sync(bdev->dev_id))) {
                    FTL_UNLOCK;
                    DMSG(DL1, ("ftl_sync failed: %d\n", err));
                    return -EIO;
                }

                return 0;
            }
            
            case FTL_MAPDESTROY:
            {
                return 0;
            }

            case RFS_EVENT_USB_VOL_MOUNT:
            {
#if (USE_BUFFERING_FOR_USB == 1)
                usb_vol_mounted = 1;
#endif
                return 0;
            }
        
            case RFS_EVENT_USB_VOL_UMOUNT:
            {
#if (USE_BUFFERING_FOR_USB == 1)
                usb_vol_mounted = 0;
#endif
                return 0;
            }

            default:
                return -ENOTTY;
        }
    }
    
    else {
        struct block_device *bdev;
    
        bdev = bdget(dev);
        if (!bdev) return -ENODEV;
    
        return 0;
    }
}


/*======================================================================*/
/*  General FTL Block Device Driver Functions                           */
/*======================================================================*/

static int
ufd_open(struct inode *inode, struct file *filp)
{
    int err;
    unsigned int minor = MINOR(inode->i_rdev);
    unsigned int idx   = GET_FTLDEV_INDEX(minor);
    FTL_BLKDEV  *bdev;

    struct backing_dev_info *bdi = blk_get_backing_dev_info(inode->i_bdev);

    if (idx >= MAX_FTL_DEVICE) return -ENODEV;
    bdev = &ftl_blkdev[idx];

    FTL_LOCK;

    if (bdev->ftl_opened == FALSE) {
        err = ftl_open(bdev->dev_id);

        if (!err) {
            bdev->ftl_opened = TRUE;
        }
        else if (minor & 0xF) {
            FTL_UNLOCK;
            DMSG(DL1, ("ftl_open failed: %d\n", err));
            return -ENODEV;
        }
    }

    if (bdi != NULL) bdi->ra_pages = 0;     // no read-ahead !!

    bdev->usage_count++;
    FTL_UNLOCK;

    if (bdev->ftl_opened == TRUE && RFS_AVAILABLE) {
        err = rfs_vol_fops.get_part(bdev->dev_id, 
                                    &bdev->rfs_parttab.num_parts, 
                                    bdev->rfs_parttab.part_offset, 
                                    bdev->rfs_parttab.part_size);
        if (err) {
            MEMSET(&bdev->rfs_parttab, 0, sizeof(RFS_PARTTAB_T));
        }
    }

    return 0;
}


static int
ufd_release(struct inode *inode, struct file *filp)
{
    unsigned int minor = MINOR(inode->i_rdev);
    unsigned int idx   = GET_FTLDEV_INDEX(minor);
    FTL_BLKDEV  *bdev;
    
    if (idx >= MAX_FTL_DEVICE) return -ENODEV;
    bdev = &ftl_blkdev[idx];

    FTL_LOCK;
    ftl_sync(bdev->dev_id);
    bdev->usage_count--;
    if (bdev->usage_count == 0 && bdev->ftl_opened == TRUE) {
        ftl_close(bdev->dev_id);
        bdev->ftl_opened = FALSE;
    }
    FTL_UNLOCK;

    return 0;
}


static int
ufd_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
    unsigned int minor = MINOR(dev->bd_dev);
    FTL_BLKDEV  *bdev  = &ftl_blkdev[GET_FTLDEV_INDEX(minor)];
    FTL_DEVINFO_T binfo;

    if (bdev_ioctl(bdev->dev_id, FTL_GET_DEV_INFO, &binfo) < 0)
        return -ENODEV;

    geo->cylinders = binfo.cylinders;
    geo->heads     = binfo.heads;
    geo->sectors   = binfo.sectors;
    geo->start     = 0;

    return 0;
}


static int
ufd_ioctl(struct inode *inode, struct file *filp, 
          unsigned int cmd, unsigned long arg)
{
    int err;
    unsigned int minor = MINOR(inode->i_rdev);
    FTL_BLKDEV  *bdev  = &ftl_blkdev[GET_FTLDEV_INDEX(minor)];

    switch(cmd) {
        case BLKFLSBUF:
        {
            /* the core code does the work, we have nothing to do */
            return bdev_sync(bdev->dev_id);
        }
        
        case HDIO_GETGEO:
        {
            struct hd_geometry geo;

            if (ufd_getgeo(inode->i_bdev, &geo) < 0)
                return -ENODEV;

            return copy_to_user((char *) arg, (char *) &geo, sizeof(geo));
        }

        case RFS_LOW_FORMAT:
        {
            if (minor & 0xf) return -EACCES;

            return bdev_ioctl(bdev->dev_id, FTL_FORMAT, NULL);
        }

        case RFS_GET_DEV_INFO:
        {
            int dev_size;
            FTL_DEVINFO_T binfo;

            if (minor & 0xf) return -EACCES;
            
            if (identify_ftl_blkdev() < 0)
                return -ENODEV;

            if (bdev_ioctl(bdev->dev_id, FTL_GET_DEV_INFO, &binfo) < 0)
                return -ENODEV;

            dev_size = binfo.cylinders * binfo.heads * binfo.sectors;

            return put_user(dev_size, (int *) arg);
        }

        case RFS_GET_PARTITION:
        {
            RFS_PARTTAB_T tab;
 
            if (minor & 0xf) return -EACCES;
            if (! RFS_AVAILABLE) return -EFAULT;

            if ((err = rfs_vol_fops.get_part(bdev->dev_id, 
                                             &tab.num_parts, 
                                             tab.part_offset, 
                                             tab.part_size)) < 0) {
                DMSG(DL1, ("rfs_vol_fops.get_part failed: %d\n", err));
                return -EIO;
            }

            return copy_to_user((char *) arg, (char *) &tab, sizeof(tab));
        }

        case RFS_SET_PARTITION:
        {
            RFS_PARTTAB_T tab;

            if (minor & 0xf) return -EACCES;
            if (! RFS_AVAILABLE) return -EFAULT;

            if (copy_from_user((char *) &tab, (char *) arg, sizeof(tab)))
                return -EFAULT;

            if ((err = rfs_vol_fops.set_part(bdev->dev_id, 
                                             tab.num_parts,
                                             tab.part_offset, 
                                             tab.part_size)) < 0) {
                DMSG(DL1, ("rfs_vol_fops.set_part failed: %d\n", err));
                return -EIO;
            }

            FTL_LOCK;
            MEMCPY(&bdev->rfs_parttab, &tab, sizeof(RFS_PARTTAB_T));
            inode->i_bdev->bd_invalidated = 1;
            FTL_UNLOCK;

            return err;
        }

        case RFS_FORMAT:
        {
            int vol = (minor & 0xf) - 1;
            RFS_FORMAT_T fmt;

            if (vol == -1) return -EACCES;
            if (! RFS_AVAILABLE) return -EFAULT;

            if (copy_from_user((char *) &fmt, (char *) arg, sizeof(fmt)))
                return -EFAULT;

            if ((err = rfs_vol_fops.format(bdev->dev_id, vol, 
                                           fmt.fat_type, 
                                           fmt.clu_size)) < 0) {
                DMSG(DL1, ("rfs_vol_fops.format failed: %d\n", err));
                if (err == -FFS_MOUNTED)
                    return -EBUSY;
                if (err == -FFS_FULL)
                    return -ENOSPC;
                else
                    return -EIO;
            }

            return 0;
        }

        case RFS_CHECK:
        {
            int vol = (minor & 0xf) - 1;

            if (vol == -1) return -EACCES;
            if (! RFS_AVAILABLE) return -EFAULT;

            if ((err = rfs_vol_fops.mount(bdev->dev_id, vol)) < 0) {
                DMSG(DL1, ("rfs_vol_fops.mount failed: %d\n", err));
                return -EIO;
            }

            rfs_vol_fops.check(bdev->dev_id, vol);

            if ((err = rfs_vol_fops.umount(bdev->dev_id, vol)) < 0) {
                DMSG(DL1, ("rfs_vol_fops.umount failed: %d\n", err));
                return -EIO;
            }

            return 0;
        }
    }
    
    return -ENOTTY;
}


static struct block_device_operations ufd_fops = {
    .owner      = THIS_MODULE,
    .open       = ufd_open,
    .release    = ufd_release,
    .ioctl      = ufd_ioctl,
    .getgeo     = ufd_getgeo
};


/*----------------------------------------------------------------------*/
/*  Block I/O Fuctions (request processing, data transfer, etc.)        */
/*----------------------------------------------------------------------*/

static int
ufd_xfer_bio(FTL_BLKDEV *bdev, struct bio *bio)
{
    unsigned int minor = MINOR(bio->bi_bdev->bd_dev);
    unsigned int idx   = GET_FTLDEV_INDEX(minor); 
    int vol = (minor & 0xf) - 1; 
    int part_offset, part_size;
    
    int i, err = -1;
    struct bio_vec *bvec;
    sector_t sector = bio->bi_sector;
    
    //printk("%s: in_atomic=%d\n", __FUNCTION__, in_atomic());
    
    if ((idx >= MAX_FTL_DEVICE) || (vol >= MAX_DRIVE)) {
        PRINT(KERN_WARNING "ufd: request for unknown device\n");
        return -1;
    }
    
    if (vol == -1) {
        part_offset = 0;
        part_size = bdev->size;
    }
    else {
        part_offset = bdev->rfs_parttab.part_offset[vol];
        part_size = bdev->rfs_parttab.part_size[vol]; 
    }
    
    if ((bio->bi_sector + (bio->bi_size >> SECTOR_BITS)) > part_size) {
        PRINT(KERN_WARNING "ufd: request past end of device\n");
        return -1;
    }
    
    bio_for_each_segment(bvec, bio, i) {

        char *pbuf = kmap(bvec->bv_page) + bvec->bv_offset;
        //char *pbuf = __bio_kmap_atomic(bio, i, KM_USER0);
        
        switch (bio_data_dir(bio)) {
    
        case READ:
        case READA:
            err = bdev_read(bdev->dev_id, 
                            sector + part_offset, 
                            pbuf, 
                            bio_cur_sectors(bio));
            if (err) goto error;
            break;

        case WRITE:
            err = bdev_write(bdev->dev_id, 
                             sector + part_offset,
                             pbuf, 
                             bio_cur_sectors(bio));
            if (err) goto error;
            break;
        }

        sector += bio_cur_sectors(bio);
    
        kunmap(bvec->bv_page);
        //__bio_kunmap_atomic(bio, KM_USER0);
    }

#if 1
    if (!err && bio_data_dir(bio) == WRITE) {
        bdev_sync(bdev->dev_id);
    }
#endif

    //bio_endio(bio, 0);
    return 0;

error:
    //bio_io_error(bio);
    return 0;
}


#if (FTL_BLKDEV_CONFIG_REQ_FUNCTION == 1)

static int
ufd_xfer_request(FTL_BLKDEV *bdev, struct request *req)
{
    struct bio *bio;
    int nsect = 0;
    
    __rq_for_each_bio(bio, req) {
        REQ_LOCK;
        ufd_xfer_bio(bdev, bio);
        REQ_UNLOCK;
        nsect += (bio->bi_size >> SECTOR_BITS);
    }
    
    return nsect;
}


static void 
ufd_request(request_queue_t *q)
{
    struct request *req; 
    int sectors_xferred;
    FTL_BLKDEV *bdev = q->queuedata;
    
    //printk("%s: pid=%d\n", __FUNCTION__, current->pid);
    
    while ((req = elv_next_request(q)) != NULL) {
        if (! blk_fs_request(req)) {
            PRINT(KERN_NOTICE "skip non-fs request\n");
            end_request(req, 0);
            continue;
        }
        
        spin_unlock_irq(q->queue_lock);
        sectors_xferred = ufd_xfer_request(bdev, req);
        spin_lock_irq(q->queue_lock);

        if (! end_that_request_first(req, 1, sectors_xferred)) {
            blkdev_dequeue_request(req);
            end_that_request_last(req,1);
        }
    }
}


#else /* (FTL_BLKDEV_CONFIG_REQ_FUNCTION == 0) */

static int 
ufd_make_request(request_queue_t *q, struct bio *bio)
{
    FTL_BLKDEV *bdev = q->queuedata;
    int status;

    status = ufd_xfer_bio(bdev, bio);
    bio_endio(bio, status);
    return 0;
}

#endif /* (FTL_BLKDEV_CONFIG_REQ_FUNCTION == 0) */


/*----------------------------------------------------------------------*/
/*  Read/Write Buffering Functions for USB Data Transfer (UMS & MTP)    */
/*----------------------------------------------------------------------*/

#if (USE_BUFFERING_FOR_USB == 1)

static INT32 
usb2bdev_read(INT32 dev, UINT32 secno, UINT8 *buf, UINT32 num_secs)
{
    INT32 err;
    UINT32 s1 = secno;                  // starting sector
    UINT32 s2 = secno + num_secs - 1;   // last sector
    
    /* check if there are buffered data in usb2bdev_buf and 
       if the sector range of the current read request overlaps with 
       the buffered data; if so, flush the buffered data first */
#if 1
    if ((usb2bdev_buf_num_secs > 0) &&
        ((s1 >= usb2bdev_buf_secno && s1 < usb2bdev_buf_secno + usb2bdev_buf_num_secs) ||
         (s2 >= usb2bdev_buf_secno && s2 < usb2bdev_buf_secno + usb2bdev_buf_num_secs))) {
#else
    if (usb2bdev_buf_num_secs > 0) {
#endif
        err = usb2bdev_flush(dev);
        if (err) return(err);
    }

    FTL_LOCK;    
    err = ftl_read_sectors(dev, secno, buf, num_secs);
    FTL_UNLOCK;

    return(err);
}


static INT32 
usb2bdev_write(INT32 dev, UINT32 secno, UINT8 *buf, UINT32 num_secs)
{
    INT32  err;
    UINT32 n, sectors_in_pagegroup = ftl_blkdev[0].sectors_in_pagegroup;

    while (num_secs > 0) {
    /* usb2bdev_buf is empty now? */
    if (usb2bdev_buf_num_secs == 0) {
        usb2bdev_buf_secno = secno;
    }
    
    /* check if the current request can be buffered in usb2bdev_buf */
        if (usb2bdev_buf_secno + usb2bdev_buf_num_secs == secno) {
#if 1
        /* check if the buffered request is misaligned with respect to
           logical page group in FTL; if misaligned, try to correct alignment
           by flushing the current buffer first if the currently received
           request is correctly aligned (NOTE: we assume that the requests
           sent from USB never span over two logical page groups) */
            if ((usb2bdev_buf_num_secs > 0) &&
                (usb2bdev_buf_secno & (sectors_in_pagegroup - 1))) {
            if (!(secno & (sectors_in_pagegroup - 1))) {
                goto flush;
            }
        }
#endif
            /* buffer the current request */
            n = MIN(num_secs, USB2BDEV_BUF_SIZE_SECS - usb2bdev_buf_num_secs);
        MEMCPY(usb2bdev_buf + (usb2bdev_buf_num_secs << SECTOR_BITS), 
                   buf, n << SECTOR_BITS);
            usb2bdev_buf_num_secs += n;
            secno += n;
            num_secs -= n;
            buf += (n << SECTOR_BITS);

            /* if the buffer is not full, it means that the current request
               has been totally buffered; processing completed */
            if (usb2bdev_buf_num_secs < USB2BDEV_BUF_SIZE_SECS) {
                return 0;
        }
    }

flush:
        /* flush the current usb2bdev_buf and 
           continue to process the current request */
    err = usb2bdev_flush(dev);
    if (err) return(err);
    }
    
    return 0;
}


static INT32 
usb2bdev_sync(INT32 dev)
{
    /* for USB to BDEV connection, do not sync at the end of each request
       for better performance; so, just return here */
    return 0;
}


static INT32 
usb2bdev_flush(INT32 dev)
{
    INT32 err;
    
    FTL_LOCK;
    
    /* flush buffered data in the current usb2bdev_buf */
    //printk("%s: secno=%d, num_secs=%d\n", __FUNCTION__, usb2bdev_buf_secno, usb2bdev_buf_num_secs);
    err = ftl_write_sectors(dev, usb2bdev_buf_secno, usb2bdev_buf, usb2bdev_buf_num_secs);
    if (err) {
        FTL_UNLOCK;
        return(err);
    }
    
    err = ftl_flush(dev);
    if (err) {
        FTL_UNLOCK;
        return(err);
    }
    
    usb2bdev_buf_idx = 1 - usb2bdev_buf_idx;
    usb2bdev_buf = __usb2bdev_buf[usb2bdev_buf_idx];
    usb2bdev_buf_num_secs = 0;

    FTL_UNLOCK;
    
    return(err);
}

#endif /* (USE_BUFFERING_FOR_USB == 1) */


extern INT32
ufd_sync(INT32 dev)
{
    INT32 err;
    
#if (USE_BUFFERING_FOR_USB == 1)
    if (usb2bdev_buf_num_secs > 0) {
        err = usb2bdev_flush(dev);
        if (err) return(err);
    }
#endif

    FTL_LOCK;
    err = ftl_sync(dev);
    FTL_UNLOCK;

    return(err);
}


/*======================================================================*/
/*  /proc Fuctions                                                      */
/*======================================================================*/

static ssize_t 
ftl_format_blkmap_write_proc(struct file *file, const char *buf, unsigned long count, void *data)
{
    INT32 err;
    UINT32 dev, dev_index;
    
    sscanf(buf, "%d", &dev_index);
    dev = SET_DEV_ID(LFD_BLK_DEVICE_FTL, SET_FTLDEV_SERIAL(dev_index));
    
    FTL_LOCK;
    
    err = ftl_open(dev);
    if (err) goto error;
    
    err = ftl_format_blkmap(dev);
    if (err) {
        ftl_close(dev);
        goto error;
    }
    
    ftl_close(dev);
    
    FTL_UNLOCK;
    printk("ftl_format_blkmap() succeeded.\n");
    return count;
    
error:
    FTL_UNLOCK;
    printk("ftl_format_blkmap() failed.\n");
    return count;
}


static ssize_t 
ftl_scan_logblock_write_proc(struct file *file, const char *buf, unsigned long count, void *data)
{
    INT32 err;
    UINT32 dev, dev_index;
    
    sscanf(buf, "%d", &dev_index);
    dev = SET_DEV_ID(LFD_BLK_DEVICE_FTL, SET_FTLDEV_SERIAL(dev_index));
    
    FTL_LOCK;
    
    err = ftl_open(dev);
    if (err) goto error;
    
    err = ftl_scan_logblock(dev);
    if (err) {
        ftl_close(dev);
        goto error;
    }
    
    ftl_close(dev);
    
    FTL_UNLOCK;
    //printk("ftl_scan_logblock() succeeded.\n");
    return count;
    
error:
    FTL_UNLOCK;
    printk("ftl_scan_logblock() failed.\n");
    return count;
}


static void 
ufd_proc_init(void)
{
    proc_ftl_dir = proc_mkdir("ftl", 0);
    
    ftl_format_blkmap_proc = create_proc_entry("format_blkmap", 0, proc_ftl_dir);
    ftl_format_blkmap_proc->write_proc = ftl_format_blkmap_write_proc;
    
    ftl_scan_logblock_proc = create_proc_entry("scan_logblock", 0, proc_ftl_dir);
    ftl_scan_logblock_proc->write_proc = ftl_scan_logblock_write_proc;
}


/*======================================================================*/
/*  Other Local Fuctions                                                */
/*======================================================================*/

static int 
identify_ftl_blkdev(void)
{
    int err, idx, i, j;
    int ftl_blkdev_found[MAX_FTL_DEVICE];

    FTL_BLKDEV    *bdev;
    FTL_DEVINFO_T  binfo;
    FLASH_PARTTAB  tab, *t = &tab;
    FLASH_SPEC     ldev_info;

    struct block_device *dev;
    
    for (i = 0; i < MAX_FTL_DEVICE; i++) ftl_blkdev_found[i] = FALSE;

    for (i = 0; i < FD_GetNumberOfChips(); i++) {
        err = FD_ReadPartitionTable(i, t);    
        if (err) {
            DMSG(DL1, ("FD_ReadPartitionTable() failed: %d\n", err));
            return -1;
        }

        for (j = 0; j < t->NumPartitions; j++) {
            
            if (t->Part[j].DevClass != MAJOR_NR || 
                t->Part[j].InDevTable == FALSE) continue;

            idx = t->Part[j].DevIndex; 
            if (idx >= MAX_FTL_DEVICE || idx >= MAX_FLASH_VPARTITIONS) {
                return -1;
            }
            
            if (ftl_blkdev_found[idx] == FALSE) {
                ftl_blkdev_found[idx] = TRUE;
                bdev = &ftl_blkdev[idx];
                FD_Open(bdev->dev_id);
                FD_GetDeviceInfo(bdev->dev_id, &ldev_info);
                FD_Close(bdev->dev_id);
again:
                if (bdev_ioctl(bdev->dev_id, FTL_GET_DEV_INFO, &binfo) < 0) {
                    printk("RFS-FTL: ftl_open() failed. Should be re-formatted.\n");
#if 0
                    if (bdev_ioctl(bdev->dev_id, FTL_FORMAT, NULL) == 0) goto again;
#endif
                    continue;
                }
                
                FTL_LOCK;
                bdev->size = binfo.cylinders * binfo.heads * binfo.sectors;
                bdev->sectors_in_pagegroup = ldev_info.SectorsPerPage * ldev_info.NumPlanes;
                set_capacity(bdev->gd, bdev->size);
                FTL_UNLOCK;
                
                dev = bdget(bdev->dev_id);
                dev->bd_invalidated = 1;
            }
        }
    }

    return 0;
}


/*======================================================================*/
/*  Module Init & Exit Fuctions                                         */
/*======================================================================*/

int __init ufd_init(void)
{
    int i;
    extern int ftl_buf_init(void); 
    
    ftl_buf_init();

#if 0
    /* initialize the UFD (Uniform Flash Device Driver) module */
    if (FD_Init()) {
        PRINT("FD_Init failed.\n");
        return -1;
    }
#endif

#if (USE_BUFFERING_FOR_USB == 1)
#if (USE_OCRAM_BUF_FOR_USB == 1)
    __usb2bdev_buf[0] = OCRAM_RFS_START_VIRT + LLD_DESC_BUF_SIZE;
    __usb2bdev_buf[1] = OCRAM_RFS2_START_VIRT;
#else
    __usb2bdev_buf[0] = (UINT8 *) MALLOC(USB2BDEV_BUF_SIZE);
    if (__usb2bdev_buf[0] == NULL) {
        PRINT("%s: ERROR - failed to allocate memory for usb2bdev_buf.\n", __FUNCTION__);
        return -1;
    }
    
    __usb2bdev_buf[1] = (UINT8 *) MALLOC(USB2BDEV_BUF_SIZE);
    if (__usb2bdev_buf[1] == NULL) {
        PRINT("%s: ERROR - failed to allocate memory for usb2bdev_buf.\n", __FUNCTION__);
        FREE(__usb2bdev_buf[0]);
        return -1;
    }
#endif

    usb2bdev_buf_idx = 0;
    usb2bdev_buf = __usb2bdev_buf[usb2bdev_buf_idx];
#endif

    /* register the FTL block device driver */
    if (register_blkdev(MAJOR_NR, DEVICE_NAME) < 0) {
        PRINT("FTL_BLKDEV: reserved major number cannot be used.\n");
        PRINT("FTL_BLKDEV: operation aborted.\n");
        return -1;
    }

    /* initialize each FTL block device structure */
    for (i = 0; i < MAX_FTL_DEVICE; i++) {
        ftl_blkdev[i].dev_id = SET_DEV_ID(MAJOR_NR, SET_FTLDEV_SERIAL(i));
        ftl_blkdev[i].size   = 0;
        ftl_blkdev[i].usage_count = 0;
        ftl_blkdev[i].ftl_opened  = FALSE;
        MEMSET(&ftl_blkdev[i].rfs_parttab, 0, sizeof(RFS_PARTTAB_T));
        spin_lock_init(&ftl_blkdev[i].lock);
    }

#if USE_SEMAPHORE
    sema_init(&ftl_sema_lock, 1);
    sema_init(&req_sema_lock, 1);
#else
    spin_lock_init(&ftl_lock);
    spin_lock_init(&req_lock);
#endif

    /* initialize each FTL block device structure (continued) */
    for (i = 0; i < MAX_FTL_DEVICE; i++) {

        /* initialize the request queue */
#if (FTL_BLKDEV_CONFIG_REQ_FUNCTION == 1)
        ftl_blkdev[i].queue = blk_init_queue(ufd_request, &ftl_blkdev[i].lock);
#else
        ftl_blkdev[i].queue = blk_alloc_queue(GFP_KERNEL);
        blk_queue_make_request(ftl_blkdev[i].queue, ufd_make_request);
#endif
        ftl_blkdev[i].queue->queuedata = &ftl_blkdev[i];
        
        /* initialize the gendisk structure */
        ftl_blkdev[i].gd               = alloc_disk(DEVICE_MAX_PARTITIONS);
        ftl_blkdev[i].gd->major        = MAJOR_NR;
        ftl_blkdev[i].gd->first_minor  = i * DEVICE_MAX_PARTITIONS;
        ftl_blkdev[i].gd->minors       = DEVICE_MAX_PARTITIONS;
        ftl_blkdev[i].gd->fops         = &ufd_fops;
        ftl_blkdev[i].gd->queue        = ftl_blkdev[i].queue;
        ftl_blkdev[i].gd->private_data = &ftl_blkdev[i];

        sprintf(ftl_blkdev[i].gd->disk_name, "%s%c", DEVICE_NAME, 'a'+i);
        set_capacity(ftl_blkdev[i].gd, ftl_blkdev[i].size);

        add_disk(ftl_blkdev[i].gd);
    }

    /* do ftl_open() here to maintain FTL in the open state always,
       in order to prevent repeated ftl_open() overhead */
    for (i = 0; i < MAX_FTL_DEVICE; i++) {
        ftl_open(ftl_blkdev[i].dev_id);
    }
    
    /* read flash memory and identify FTL block devices */
    if (identify_ftl_blkdev() < 0) {
        for (i = 0; i < MAX_FTL_DEVICE; i++) {
            del_gendisk(ftl_blkdev[i].gd);
            put_disk(ftl_blkdev[i].gd);
            blk_cleanup_queue(ftl_blkdev[i].queue);
            ftl_close(ftl_blkdev[i].dev_id);
        }
        
        unregister_blkdev(MAJOR_NR, DEVICE_NAME);
        return -1;
    }

    ufd_proc_init();

    return 0;
}


void __exit ufd_exit(void)
{
    int i;
    
    for (i = 0; i < MAX_FTL_DEVICE; i++) {
        del_gendisk(ftl_blkdev[i].gd);
        put_disk(ftl_blkdev[i].gd);
        
        blk_cleanup_queue(ftl_blkdev[i].queue);
    }
    
    unregister_blkdev(MAJOR_NR, DEVICE_NAME);
    
    for (i = 0; i < MAX_FTL_DEVICE; i++) {
        ftl_close(ftl_blkdev[i].dev_id);
    }
}


//module_init(ufd_init);
//module_exit(ufd_exit);

EXPORT_SYMBOL(bdev_open);
EXPORT_SYMBOL(bdev_close);
EXPORT_SYMBOL(bdev_read);
EXPORT_SYMBOL(bdev_write);
EXPORT_SYMBOL(bdev_sync);
EXPORT_SYMBOL(bdev_ioctl);

//MODULE_LICENSE("Zeen InfoTech");

/* end of fm_blkdev.c */
