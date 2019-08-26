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
/*  This file implements the RFS FAT file system VFS interface.         */
/*                                                                      */
/*  @author  Joosun Hahn                                                */
/*  @author  Sungjoon Ahn                                               */
/************************************************************************/
/************************************************************************/
/*                                                                      */
/*  PROJECT : RFS Flash File System for NAND Flash Memory               */
/*  FILE    : super.c                                                   */
/*  PURPOSE : RFS Robust FAT Flash File System VFS Interface            */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  REVISION HISTORY (Ver 2.0)                                          */
/*                                                                      */
/*  - 01/07/2003 [Sungjoon Ahn] : first writing                         */
/*  - 01/10/2003 [Joosun Hahn]  : added FAT32 support                   */
/*  - 01/10/2003 [Sungjoon Ahn] : added symbolic link support           */
/*  - 01/11/2003 [Sungjoon Ahn] : added socket support                  */
/*                                                                      */
/************************************************************************/
 
#include <linux/sched.h>
#include <linux/unistd.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/rfs_fs_i.h>
#include <linux/smp_lock.h>
#include <linux/ioctl.h>
#include <linux/statfs.h>
#include <linux/blkdev.h>
#include <linux/quotaops.h>
#include <linux/ctype.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <asm/semaphore.h>
#include <asm/byteorder.h>
#include <asm/uaccess.h>
#include <asm/fcntl.h>
#include <asm/statfs.h>

#include "fm_global.h"
#include "fm_blkdev.h"

#include "rfs_api.h"
#include "rfs.h"
#include "rfs_code_convert.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

#define RFS_CONFIG_PAGE_CACHE   1
//#define RFS_CONFIG_PAGE_CACHE   0  // Page cache test
#define RFS_CONFIG_AUTO_FORMAT  0

#if 0
static struct semaphore rfs_vfs_lock;
#define RFS_VFS_LOCK_INIT       sema_init(&rfs_vfs_lock, 1)
#define RFS_VFS_LOCK            down(&rfs_vfs_lock)
#define RFS_VFS_UNLOCK          up(&rfs_vfs_lock)
#else
static spinlock_t rfs_vfs_lock;
#define RFS_VFS_LOCK_INIT       spin_lock_init(&rfs_vfs_lock)
#define RFS_VFS_LOCK            spin_lock(&rfs_vfs_lock)
#define RFS_VFS_UNLOCK          spin_unlock(&rfs_vfs_lock)
#endif

/* DMSG() macro - for debugging */
#if (FFS_CONFIG_DEBUG == 1)
#define DMSG(DebugLevel, fmt_and_args)  \
        __DMSG(DMSG_FFS, DebugLevel, fmt_and_args)
#else
#define DMSG(DebugLevel, fmt_and_args)
#endif

#define MALLOC_SIZE             128*1024        /* in case of 64MB RAM */

#if (RFS_CONFIG_AUTO_FORMAT == 1)
#define DEFAULT_CLUSIZE         8               /* 4KB */
#endif

#ifdef CONFIG_RFS_NLS
#define DEFAULT_CODEPAGE        949

#define set_codepage() \
    do {                                                                \
        int cp, codepage;                                               \
        char *p, iocharset[50];                                         \
                                                                        \
        if (!rfs_parse_options((char *) data, &codepage, iocharset))    \
            goto rfs_sb_err2;                                           \
                                                                        \
        cp = codepage? codepage : DEFAULT_CODEPAGE;                     \
        DMSG(DL1, (KERN_NOTICE "RFS: Using codepage %d\n", cp));        \
                                                                        \
        p = iocharset[0]? iocharset : CONFIG_NLS_DEFAULT;               \
        DMSG(DL1, (KERN_NOTICE "RFS: Using IO charset %s\n", p));       \
                                                                        \
        if ((err = rfs_set_codepage(dev, vol, p, cp)) < 0) {            \
            DMSG(DL1, (KERN_NOTICE "%s: rfs_set_codepage failed: %d\n", \
                                                __FUNCTION__, err));    \
            goto rfs_sb_err2;                                           \
        }                                                               \
    } while (0)

#define put_codepage() \
    do {                                                                \
        if ((err = rfs_put_codepage(dev, vol)) < 0) {                   \
            DMSG(DL1, (KERN_NOTICE "%s: rfs_put_codepage failed: %d\n", \
                                                __FUNCTION__, err));    \
        }                                                               \
    } while (0)

#else /* CONFIG_RFS_NLS */
#define set_codepage()
#define put_codepage()
#endif

/* unique inode number in RFS */
#define RFS_INO(a, b, c)           ((UINT32)((a)<<16) | (UINT32)(((b)&0xff)<<8) | (UINT32)((c)&0xff))

inline int GET_DEV(dev_t kdev) 
{
    return (kdev & ~((UINT32)0xf));
}
 
/* inline function to get volume ID (kdev: Linux device number) */
inline int GET_VOL(dev_t kdev) 
{
    return ((MINOR(kdev) & 0xf) - 1);
}

/*----------------------------------------------------------------------*/
/*  Global Variable Definitions                                         */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

static char zero_buf[512];

static struct super_operations rfs_ops;
static struct file_operations rfs_file_operations;
static struct inode_operations rfs_file_inode_operations;
static struct file_operations rfs_dir_operations;
static struct inode_operations rfs_dir_inode_operations;
static struct address_space_operations rfs_address_operations;
static struct inode_operations rfs_symlink_inode_operations;
static struct dentry_operations rfs_dentry_operations;

/*----------------------------------------------------------------------*/
/*  Local Function Declarations                                         */
/*----------------------------------------------------------------------*/

/* super block operations */
static void rfs_vfs_read_inode(struct inode *inode);
static int  rfs_vfs_write_inode(struct inode *inode, int wait);
static void rfs_vfs_clear_inode(struct inode *inode);
static void rfs_vfs_put_super(struct super_block *sb);
static void rfs_vfs_write_super(struct super_block *sb);
static int  rfs_vfs_statfs(struct dentry *dentry, struct kstatfs *stat);
static int  rfs_vfs_sync(struct super_block *sb, int wait);

/* inode operations */
static struct dentry *rfs_vfs_lookup(struct inode *dir, struct dentry *dentry,
                                      struct nameidata *nd);
static int  rfs_vfs_mkdir(struct inode *dir, struct dentry *dentry, int mode);
static int  rfs_vfs_rmdir(struct inode *dir, struct dentry *dentry);
static int  rfs_vfs_create(struct inode *dir, struct dentry *dentry, 
                           int mode, struct nameidata *nd);
static int  rfs_vfs_symlink(struct inode *dir, struct dentry *dentry, 
                            const char *target);
static int  rfs_vfs_mknod(struct inode *dir, struct dentry *dentry, int mode, 
                          dev_t rdev);
static int  rfs_vfs_unlink(struct inode *dir, struct dentry *dentry);
static int  rfs_vfs_rename(struct inode *old_dir, struct dentry *old_dentry,
                           struct inode *new_dir, struct dentry *new_dentry);
static void rfs_vfs_truncate(struct inode *inode);
static int  rfs_vfs_permission(struct inode *inode, int mode, 
                               struct nameidata *nd);
static int  rfs_vfs_setattr(struct dentry *dentry, struct iattr *ia);

static int  rfs_vfs_readlink(struct dentry *dentry, char *buffer, int buflen);
static void rfs_vfs_follow_link(struct dentry *dentry, struct nameidata *nd);

/* directory file operations */
static int  rfs_vfs_readdir(struct file *filp, void *dir_entry, 
                            filldir_t filldir);
                            
/* file operations */
#if (RFS_CONFIG_PAGE_CACHE == 0)
static ssize_t rfs_vfs_file_read(struct file *filp, char *buffer,
                                  size_t count, loff_t * ppos);
static ssize_t rfs_vfs_file_write(struct file *filp, const char *bufifer,
                                   size_t count, loff_t * ppos);
#else
static ssize_t rfs_do_sync_read(struct file *filp, char __user *buf, 
                                size_t len, loff_t *ppos);
#endif

static int  rfs_vfs_file_sync(struct file *filp, struct dentry *dentry, 
                              int datasync);

static int  rfs_vfs_do_readpage(struct file *file, struct page *page);
static int  rfs_vfs_readpage(struct file *file, struct page *page);
static int  rfs_vfs_writepage(struct page *page, struct writeback_control *wbc);
static ssize_t rfs_vfs_prepare_write(struct file *filp, struct page *page,
                                      unsigned int from, unsigned int to);
static ssize_t rfs_vfs_commit_write(struct file *filp, struct page *page,
                                     unsigned int from, unsigned int to);
#ifdef DIO
static int  rfs_vfs_direct_IO(int rw, struct inode *inode, struct kiobuf *iobuf,
                              unsigned long blovknr, int blocksize);
#endif

/* dentry operations */
static int  rfs_vfs_dentry_delete(struct dentry *dentry);

/* other utility functions */
#ifdef CONFIG_RFS_NLS
static int  rfs_parse_options(char *options, int *codepage, char *iocharset);
#endif
static struct inode *rfs_new_inode(struct super_block *sb);
static int  rfs_iset(struct inode *inode, void *data);
static int  rfs_find_actor(struct inode *inode, void *data);
                           
/* filesystem initialization fuctions */
static int  rfs_fill_super(struct super_block *sb, void *data, int silent);
#if 1
static struct super_block *rfs_get_sb(struct file_system_type *fs_type, 
                                int flags, const char *dev_name, void *data,
                                struct vfsmount *mnt);
#else
static struct super_block *rfs_get_sb(struct file_system_type *fs_type, 
                                int flags, const char *dev_name, void *data);
#endif



/*======================================================================*/
/*  Function Definitions                                                */
/*======================================================================*/

/*----------------------------------------------------------------------*/
/*  Super Block Operations                                              */
/*----------------------------------------------------------------------*/

static struct super_operations rfs_ops = {
    read_inode:     rfs_vfs_read_inode,
    write_inode:    rfs_vfs_write_inode,
    clear_inode:    rfs_vfs_clear_inode,
    put_super:      rfs_vfs_put_super,
    write_super:    rfs_vfs_write_super,
    statfs:         rfs_vfs_statfs,
    sync_fs:        rfs_vfs_sync
};

static void rfs_vfs_read_inode(struct inode *inode)
{
    rfs_inode_info_t *rfs_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &rfs_i->fid;
    rfs_inode_t fi;
    
    RFS_VFS_LOCK;

    MEMSET(&fi, 0, sizeof(rfs_inode_t));

    rfs_read_inode(fid, &fi);

    if (fi.type == TYPE_DIR) 
        inode->i_mode = S_IFDIR;
    else if (fi.type == TYPE_SYMLINK) 
        inode->i_mode = S_IFLNK;
    else if (fi.type == TYPE_SOCKET) 
        inode->i_mode = S_IFSOCK;
    else 
        inode->i_mode = S_IFREG;

    if (S_ISDIR(inode->i_mode)) {
        fid->first_cluster = fi.first_cluster;
    }
    fid->hint_last_off = -1;
    fid->hint_last_clu = 1;

    inode->i_mode |= 0777;
    if (fi.attr & ATTR_READONLY) 
        inode->i_mode &= ~0222;

    inode->i_size = fi.size;
    inode->i_blkbits = inode->i_sb->s_blocksize_bits;
    inode->i_blocks = (inode->i_size + 511) >> 9;
    inode->i_mtime.tv_sec = (time_t) fi.mtime;

    if (S_ISREG(inode->i_mode)) {
        inode->i_op = &rfs_file_inode_operations;
        inode->i_fop = &rfs_file_operations;
        inode->i_mapping->a_ops = &rfs_address_operations;
        inode->i_mapping->nrpages = 0;
    }
    else if (S_ISDIR(inode->i_mode)) {
        inode->i_op = &rfs_dir_inode_operations;
        inode->i_fop = &rfs_dir_operations;
    }
    else if (S_ISLNK(inode->i_mode)) {
        inode->i_op = &rfs_symlink_inode_operations;
    }
    else if (S_ISSOCK(inode->i_mode)) {
        init_special_inode(inode, inode->i_mode, 0);
    }
    
    RFS_VFS_UNLOCK;
} /* end of rfs_vfs_read_inode */

static int rfs_vfs_write_inode(struct inode *inode, int wait)
{
    rfs_sb_info_t *rfs_sb = (rfs_sb_info_t *)inode->i_sb->s_fs_info;
    rfs_inode_info_t *rfs_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &rfs_i->fid;
    time_t mtime = inode->i_mtime.tv_sec;
    int err;
    //unsigned int size = i_size_read(inode); //inode->i_size;

    RFS_VFS_LOCK;
    
    if (current->flags & PF_MEMALLOC) {
        RFS_VFS_UNLOCK;
        return -EIO;
    }

    if (inode->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EPERM;
    }

    /* check if current directory is root directory */
    if (fid->first_cluster == rfs_sb->root_cluster) {
        RFS_VFS_UNLOCK;
        return -EPERM;
    }

    if ((err = rfs_write_inode(fid, mtime)) < 0) {
        DMSG(DL1, (KERN_NOTICE "%s: rfs_write_inode failed: %d\n", 
                                                __FUNCTION__, err));
        RFS_VFS_UNLOCK;
        return -EIO;
    }
    
    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_write_inode */

static void rfs_vfs_clear_inode(struct inode *inode)
{
    RFS_VFS_LOCK;
    
    FREE(inode->i_private);
    inode->i_private = NULL;
    
    RFS_VFS_UNLOCK;
} /* end of rfs_vfs_clear_inode */

static void rfs_vfs_put_super(struct super_block *sb)
{
    int dev, vol, err;

    RFS_VFS_LOCK;
    
    dev = GET_DEV(sb->s_dev);
    vol = GET_VOL(sb->s_dev);

    put_codepage();

    if ((err = rfs_umount_vol(dev, vol)) < 0) {
        DMSG(DL1, (KERN_NOTICE "%s: rfs_umount_vol failed: %d\n", 
                                                __FUNCTION__, err));
    }
    sb->s_dirt = 0;

    FREE(sb->s_fs_info);
    sb->s_fs_info = NULL;
    
    RFS_VFS_UNLOCK;
} /* end of rfs_vfs_put_super */

static void rfs_vfs_write_super(struct super_block *sb)
{
    int dev, vol;

    RFS_VFS_LOCK;
    
    dev = GET_DEV(sb->s_dev);
    vol = GET_VOL(sb->s_dev);

    rfs_sync_vol(dev, vol);
    sb->s_dirt = 0;
    
    RFS_VFS_UNLOCK;
} /* rfs_vfs_write_super */

static int rfs_vfs_statfs(struct dentry *dentry, struct kstatfs *stat)
{
    rfs_sb_info_t tmp_sb;
    int dev, vol, err;

    RFS_VFS_LOCK;

    dev = GET_DEV(dentry->d_sb->s_dev);
    vol = GET_VOL(dentry->d_sb->s_dev);

    if ((err = rfs_read_super(dev, vol, &tmp_sb)) < 0) {
        DMSG(DL1, (KERN_NOTICE "%s: rfs_read_super failed: %d\n", 
                                                __FUNCTION__, err));
        RFS_VFS_UNLOCK;
        return -EIO;
    }

    stat->f_type    = RFS_MAGIC;
    stat->f_bsize   = tmp_sb.cluster_size;
    stat->f_blocks  = tmp_sb.num_clusters;
    stat->f_bfree   = tmp_sb.free_clusters;
    stat->f_bavail  = tmp_sb.free_clusters;
    stat->f_namelen = RFS_FNAME_LENGTH;

    RFS_VFS_UNLOCK;
    return 0;
} /* rfs_vfs_statfs */

static int rfs_vfs_sync(struct super_block *sb, int wait)
{
    int dev, vol;

    RFS_VFS_LOCK;
    
    dev = GET_DEV(sb->s_dev);
    vol = GET_VOL(sb->s_dev);

    rfs_sync_vol(dev, vol);
    ufd_sync(dev);
    
    RFS_VFS_UNLOCK;
    
    return 0;
} /* rfs_vfs_sync */


/*----------------------------------------------------------------------*/
/*  Inode Operations                                                    */
/*----------------------------------------------------------------------*/

static struct inode_operations rfs_dir_inode_operations = {
    create:     rfs_vfs_create,
    lookup:     rfs_vfs_lookup,
    unlink:     rfs_vfs_unlink,
    symlink:    rfs_vfs_symlink,
    mkdir:      rfs_vfs_mkdir,
    rmdir:      rfs_vfs_rmdir,
    mknod:      rfs_vfs_mknod,
    rename:     rfs_vfs_rename,
    permission: rfs_vfs_permission,
    setattr:    rfs_vfs_setattr,
};

static struct inode_operations rfs_file_inode_operations = {
    truncate:   rfs_vfs_truncate,
    permission: rfs_vfs_permission,
    setattr:    rfs_vfs_setattr,
};

static struct inode_operations rfs_symlink_inode_operations = {
    readlink:    rfs_vfs_readlink,
    follow_link: rfs_vfs_follow_link,
};

static struct dentry *rfs_vfs_lookup(struct inode *dir, struct dentry *dentry,
                                     struct nameidata *nd)
{
    struct inode *inode = NULL;
    struct dentry *alias = NULL;
    rfs_file_id_t fid;
    rfs_inode_info_t *dir_i = (rfs_inode_info_t *)dir->i_private;
    int is_dosname = FALSE;
    int err;
    
    RFS_VFS_LOCK;

    if (dentry->d_name.len >= RFS_FNAME_LENGTH) {
        RFS_VFS_UNLOCK;
        return ERR_PTR(-ENAMETOOLONG);
    }

    fid.dev = GET_DEV(dir->i_rdev);
    fid.vol = GET_VOL(dir->i_rdev);
    fid.parent_first_cluster = dir_i->fid.first_cluster;
    strcpy(fid.filename, dentry->d_name.name);
    fid.fn_length = dentry->d_name.len;
    fid.dentry = -2;
    
    if ((err = rfs_lookup(&fid, &is_dosname)) == 0) {
        
        inode = iget5_locked(dir->i_sb, 
                             RFS_INO(fid.parent_first_cluster, fid.filename[0], fid.filename[fid.fn_length-1]), //fid.parent_first_cluster, 
                             rfs_find_actor, rfs_iset, (void *)&fid);

        if (!inode) {
            RFS_VFS_UNLOCK;
            return ERR_PTR(-EACCES);
        }
        
        if (inode->i_state & I_NEW) {
            inode->i_sb = dir->i_sb;
            inode->i_ino = RFS_INO(fid.parent_first_cluster, fid.filename[0], fid.filename[fid.fn_length-1]);
            unlock_new_inode(inode);
        }

        /* code to switch dentry */
        alias = d_find_alias(inode);
        if (alias) {
            if (d_invalidate(alias) == 0) {
                dput(alias);
            }
            else {
                iput(inode);
                RFS_VFS_UNLOCK;
                return alias;
            }
        }
        if (is_dosname == TRUE)
            dentry->d_op = &rfs_dentry_operations;
    }
    else if (err != -FFS_NOTFOUND) {
        DMSG(DL1, (KERN_NOTICE "%s: rfs_lookup failed: %d\n", 
                                                __FUNCTION__, err));
        RFS_VFS_UNLOCK;
        return ERR_PTR(-EIO); 
    }

    d_add(dentry, inode);
    
    RFS_VFS_UNLOCK;
    return NULL;
} /* end of rfs_vfs_lookup */

static int rfs_vfs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
    struct inode *inode = NULL;
    rfs_file_id_t fid;
    rfs_inode_info_t *dir_i = (rfs_inode_info_t *)dir->i_private;
    rfs_inode_info_t *new_i;
    rfs_sb_info_t *dir_sb = (rfs_sb_info_t *)dir->i_sb->s_fs_info;
    int err;

    __u8 flag = 0;

    RFS_VFS_LOCK;

    if (dentry->d_name.len >= RFS_FNAME_LENGTH) {
        RFS_VFS_UNLOCK;
        return -ENAMETOOLONG;
    }

    if (dir->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EROFS;
    }

    fid.dev = GET_DEV(dir->i_rdev);
    fid.vol = GET_VOL(dir->i_rdev);
    fid.parent_first_cluster = dir_i->fid.first_cluster;
    strcpy(fid.filename, dentry->d_name.name);
    fid.fn_length = dentry->d_name.len;
    fid.dentry = -2;
    fid.hint_last_off = -1;
    fid.hint_last_clu = 1;

    if ((err = rfs_mkdir(&fid, flag)) < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: rfs_mkdir failed: %d\n", 
                                                __FUNCTION__, err));
        if (err == -FFS_INVALIDPATH)
            return -EINVAL;
        if (err == -FFS_FILEEXIST)
            return -EEXIST;
        if (err == -FFS_FULL) 
            return -ENOSPC;
        else 
            return -EIO;
    }

    if ((inode = rfs_new_inode(dir->i_sb)) == NULL) {
        RFS_VFS_UNLOCK;
        return -EIO;
    }

    new_i = (rfs_inode_info_t *)inode->i_private;
    MEMCPY(&new_i->fid, &fid, sizeof(rfs_file_id_t));

    dentry->d_op = &rfs_dentry_operations;

    inode->i_sb = dir->i_sb;
    inode->i_ino = RFS_INO(fid.parent_first_cluster, fid.filename[0], fid.filename[fid.fn_length-1]); //fid.parent_first_cluster;
    inode->i_mode = S_IFDIR | 0777;
    inode->i_nlink = 1;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_rdev = inode->i_sb->s_dev;
    inode->i_bdev = inode->i_sb->s_bdev;
    inode->i_size += (unsigned int)(dir_sb->cluster_size);
    inode->i_ctime = inode->i_mtime = inode->i_atime = CURRENT_TIME;
    inode->i_blkbits = inode->i_sb->s_blocksize_bits;
    inode->i_blocks = 0;
    inode->i_version = 0;

    inode->i_op = &rfs_dir_inode_operations;
    inode->i_fop = &rfs_dir_operations;

    insert_inode_hash(inode);
    
    d_instantiate(dentry, inode);
    
    dir->i_mtime = dir->i_atime = CURRENT_TIME;
    mark_inode_dirty(dir);
    
    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_mkdir */

static int rfs_vfs_rmdir(struct inode *dir, struct dentry *dentry)
{
    struct inode *inode = dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    int err;
    
    RFS_VFS_LOCK;

    if (dir->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EROFS;
    }

    if ((err = rfs_rmdir(fid)) < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: rfs_rmdir failed: %d\n", 
                                                __FUNCTION__, err));
        if (err == -FFS_INVALIDPATH)
            return -EINVAL;
        if (err == -FFS_FILEEXIST) 
            return -ENOTEMPTY;
        if (err == -FFS_NOTFOUND) 
            return -ENOENT;
        if (err == -FFS_DIRBUSY)
            return -EBUSY;
        else 
            return -EIO;
    }

    inode->i_nlink = 0;
    mark_inode_dirty(inode);
    
    dir->i_mtime = dir->i_atime = CURRENT_TIME;
    mark_inode_dirty(dir);

    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_rmdir */

static int rfs_vfs_create(struct inode *dir, struct dentry *dentry, 
                          int mode, struct nameidata *nd)
{
    struct inode *inode = NULL;
    rfs_file_id_t fid;
    rfs_inode_info_t *dir_i = (rfs_inode_info_t *)dir->i_private;
    rfs_inode_info_t *new_i;
    int err;

    __u8 flag = 0;
    
    RFS_VFS_LOCK;

    if (dentry->d_name.len >= RFS_FNAME_LENGTH) {
        RFS_VFS_UNLOCK;
        return -ENAMETOOLONG;
    }

    if (dir->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EROFS;
    }

    fid.dev = GET_DEV(dir->i_rdev);
    fid.vol = GET_VOL(dir->i_rdev);
    fid.parent_first_cluster = dir_i->fid.first_cluster;
    strcpy(fid.filename, dentry->d_name.name);
    fid.fn_length = dentry->d_name.len;
    fid.dentry = -2;
    fid.hint_last_off = -1;
    fid.hint_last_clu = 1;
    
    if ((err = rfs_create(&fid, flag)) < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: rfs_create failed: %d\n", 
                                                __FUNCTION__, err));
        if (err == -FFS_INVALIDPATH)
            return -EINVAL;
        if (err == -FFS_FILEEXIST)
            return -EEXIST;
        if (err == -FFS_FULL)
            return -ENOSPC;
        else
            return -EIO;
    }

    if ((inode = rfs_new_inode(dir->i_sb)) == NULL) {
        RFS_VFS_UNLOCK;
        return -EIO;
    }

    new_i = (rfs_inode_info_t *)inode->i_private;
    MEMCPY(&new_i->fid, &fid, sizeof(rfs_file_id_t));

    dentry->d_op = &rfs_dentry_operations;

    inode->i_sb = dir->i_sb;
    inode->i_ino = RFS_INO(fid.parent_first_cluster, fid.filename[0], fid.filename[fid.fn_length-1]); //fid.parent_first_cluster;
    inode->i_mode = S_IFREG | 0777;
    inode->i_nlink = 1;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_rdev = inode->i_sb->s_dev;
    inode->i_bdev = inode->i_sb->s_bdev;
    inode->i_size = 0;
    inode->i_ctime = inode->i_mtime = inode->i_atime = CURRENT_TIME;
    inode->i_blkbits = inode->i_sb->s_blocksize_bits;
    inode->i_blocks = 0;
    inode->i_version = 0;

    inode->i_op = &rfs_file_inode_operations;
    inode->i_fop = &rfs_file_operations;
    inode->i_mapping->a_ops = &rfs_address_operations;
    inode->i_mapping->nrpages = 0;

    insert_inode_hash(inode);

    d_instantiate(dentry, inode);

    dir->i_mtime = dir->i_atime = CURRENT_TIME;
    mark_inode_dirty(dir);

    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_create */

static int rfs_vfs_symlink(struct inode *dir, struct dentry *dentry, 
                            const char *target)
{
    struct inode *inode = NULL;
    rfs_file_id_t fid;
    rfs_inode_info_t *dir_i = (rfs_inode_info_t *)dir->i_private;
    rfs_inode_info_t *new_i;
    loff_t pos = 0;
    ssize_t result = 0;
    int err;

    // create symlink file
    __u8 flag = FM_SYMLINK;
    
    RFS_VFS_LOCK;

    if (dentry->d_name.len >= RFS_FNAME_LENGTH) {
        RFS_VFS_UNLOCK;
        return -ENAMETOOLONG;
    }

    if (dir->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EROFS;
    }

    fid.dev = GET_DEV(dir->i_rdev);
    fid.vol = GET_VOL(dir->i_rdev);
    fid.parent_first_cluster = dir_i->fid.first_cluster;
    strcpy(fid.filename, dentry->d_name.name);
    fid.fn_length = dentry->d_name.len;
    fid.dentry = -2;
    fid.hint_last_off = -1;
    fid.hint_last_clu = 1;

    if ((err = rfs_create(&fid, flag)) < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: symbolic link create failed: %d\n", 
                                                __FUNCTION__, err));
        if (err == -FFS_INVALIDPATH)
            return -EINVAL;
        if (err == -FFS_FILEEXIST)
            return -EEXIST;
        if (err == -FFS_FULL)
            return -ENOSPC;
        else
            return -EIO;
    }

    if ((inode = rfs_new_inode(dir->i_sb)) == NULL) {
        RFS_VFS_UNLOCK;
        return -EIO;
    }

    new_i = (rfs_inode_info_t *)inode->i_private;
    MEMCPY(&new_i->fid, &fid, sizeof(rfs_file_id_t));

    dentry->d_op = &rfs_dentry_operations;

    inode->i_sb = dir->i_sb;
    inode->i_ino = RFS_INO(fid.parent_first_cluster, fid.filename[0], fid.filename[fid.fn_length-1]); //fid.parent_first_cluster;
    inode->i_mode = S_IFLNK | 0777;
    inode->i_nlink = 1;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_rdev = inode->i_sb->s_dev;
    inode->i_bdev = inode->i_sb->s_bdev;
    inode->i_size = strlen(target)+1;
    inode->i_ctime = inode->i_mtime = inode->i_atime = CURRENT_TIME;
    inode->i_blkbits = inode->i_sb->s_blocksize_bits;
    inode->i_blocks = 0;
    inode->i_version = 0;

    inode->i_op = &rfs_symlink_inode_operations;

    insert_inode_hash(inode);

    d_instantiate(dentry, inode);

    // write target path
    result = rfs_file_write(&fid, (char *) target, strlen(target)+1, &pos);

    if (result <= 0) {
        RFS_VFS_UNLOCK;
        if (result < 0) {
            DMSG(DL1, (KERN_NOTICE "%s: symbolic link write failed: %d\n", 
                                                __FUNCTION__, result));
            if (result == -FFS_INVALIDPATH)
                return -EINVAL;
            if (result == -FFS_NOTFOUND)
                return -ENOENT;
            else
                return -EIO;
        }
        else {
            DMSG(DL1, (KERN_NOTICE "%s: symbolic link write failed: %d\n", 
                                                __FUNCTION__, -FFS_FULL));
            return -ENOSPC;
        }
    }

    rfs_sync_vol(fid.dev, fid.vol);
    dir->i_sb->s_dirt = 0;

    dir->i_mtime = dir->i_atime = CURRENT_TIME;
    mark_inode_dirty(dir);

    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_symlink */

static int rfs_vfs_mknod(struct inode *dir, struct dentry *dentry, int mode, 
                          dev_t rdev)
{
    struct inode *inode = NULL;
    rfs_file_id_t fid;
    rfs_inode_info_t *dir_i = (rfs_inode_info_t *)dir->i_private;
    rfs_inode_info_t *new_i;
    int err;

    // create socket file
    __u8 flag = FM_SOCKET;
    
    RFS_VFS_LOCK;

    if (dentry->d_name.len >= RFS_FNAME_LENGTH) {
        RFS_VFS_UNLOCK;
        return -ENAMETOOLONG;
    }

    if (!S_ISSOCK(mode)) {   // support only socket type file
        RFS_VFS_UNLOCK;
        return -ENOSYS;
    }

    if (dir->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EROFS;
    }

    fid.dev = GET_DEV(dir->i_rdev);
    fid.vol = GET_VOL(dir->i_rdev);
    fid.parent_first_cluster = dir_i->fid.first_cluster;
    strcpy(fid.filename, dentry->d_name.name);
    fid.fn_length = dentry->d_name.len;
    fid.dentry = -2;
    fid.hint_last_off = -1;
    fid.hint_last_clu = 1;

    if ((err = rfs_create(&fid, flag)) < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: socket create failed: %d\n", 
                                                __FUNCTION__, err));
        if (err == -FFS_INVALIDPATH)
            return -EINVAL;
        if (err == -FFS_FILEEXIST)
            return -EEXIST;
        if (err == -FFS_FULL)
            return -ENOSPC;
        else
            return -EIO;
    }

    if ((inode = rfs_new_inode(dir->i_sb)) == NULL) {
        RFS_VFS_UNLOCK;
        return -EIO;
    }

    new_i = (rfs_inode_info_t *)inode->i_private;
    MEMCPY(&new_i->fid, &fid, sizeof(rfs_file_id_t));

    inode->i_sb = dir->i_sb;
    inode->i_ino = RFS_INO(fid.parent_first_cluster, fid.filename[0], fid.filename[fid.fn_length-1]); //fid.parent_first_cluster;
    inode->i_mode = S_IFSOCK | 0777;
    inode->i_nlink = 1;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_rdev = inode->i_sb->s_dev;
    inode->i_bdev = inode->i_sb->s_bdev;
    inode->i_size = 0;
    inode->i_ctime = inode->i_mtime = inode->i_atime = CURRENT_TIME;
    inode->i_blkbits = inode->i_sb->s_blocksize_bits;
    inode->i_blocks = 0;
    inode->i_version = 0;

    insert_inode_hash(inode);

    init_special_inode(inode, mode, 0);

    d_instantiate(dentry, inode);

    dir->i_mtime = dir->i_atime = CURRENT_TIME;
    mark_inode_dirty(dir);

    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_mknod */

static int rfs_vfs_unlink(struct inode *dir, struct dentry *dentry)
{
    struct inode *inode = dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    int err;
    
    RFS_VFS_LOCK;

    if (dir->i_sb->s_flags & MS_RDONLY) {
       RFS_VFS_UNLOCK;
       return -EROFS;
    }

    if ((err = rfs_unlink(fid)) < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: rfs_unlink failed: %d\n", 
                                                __FUNCTION__, err));
        if (err == -FFS_PERMISSIONERR)
            return -EPERM;
        if (err == -FFS_INVALIDPATH)
            return -EINVAL;
        if (err == -FFS_NOTFOUND)
            return -ENOENT;
        else
            return -EIO;
    }

    inode->i_nlink = 0;
    mark_inode_dirty(inode);

    dir->i_mtime = dir->i_atime = CURRENT_TIME;
    mark_inode_dirty(dir);

    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_unlink */

static int rfs_vfs_rename(struct inode *old_dir, struct dentry *old_dentry,
                           struct inode *new_dir, struct dentry *new_dentry)
{
    struct inode *old_inode = old_dentry->d_inode;
    struct inode *new_inode = new_dentry->d_inode;
    rfs_file_id_t *old_fidp, new_fid;
    rfs_inode_info_t *new_dir_i = (rfs_inode_info_t *)new_dir->i_private;
    rfs_inode_info_t *old_inode_i = (rfs_inode_info_t *)old_inode->i_private;
    int err;
    
    RFS_VFS_LOCK;

    if ((old_dentry->d_name.len >= RFS_FNAME_LENGTH) ||
        (new_dentry->d_name.len >= RFS_FNAME_LENGTH)) {
        RFS_VFS_UNLOCK;
        return -ENAMETOOLONG;
    }

    if (new_inode) {
        if (S_ISREG(new_inode->i_mode) || S_ISLNK(new_inode->i_mode)) {
            RFS_VFS_UNLOCK;
            rfs_vfs_unlink(new_inode, new_dentry);
            RFS_VFS_LOCK;
        }
        else if (S_ISDIR(new_inode->i_mode)) {
            RFS_VFS_UNLOCK;
            rfs_vfs_rmdir(new_inode, new_dentry);
            RFS_VFS_LOCK;
        }
    }

    if (new_dir->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EROFS;
    }

    old_fidp = &old_inode_i->fid;

    new_fid.dev = old_fidp->dev;
    new_fid.vol = old_fidp->vol;
    new_fid.parent_first_cluster = new_dir_i->fid.first_cluster;
    new_fid.first_cluster = old_fidp->first_cluster;
    strcpy(new_fid.filename, new_dentry->d_name.name);
    new_fid.fn_length = new_dentry->d_name.len;
    new_fid.hint_last_off = old_fidp->hint_last_off;
    new_fid.hint_last_clu = old_fidp->hint_last_clu;
    new_fid.dentry = -2;

    if ((err = rfs_rename(old_fidp, &new_fid)) < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: rfs_rename failed: %d\n", 
                                                __FUNCTION__, err));
        if (err == -FFS_PERMISSIONERR)
            return -EPERM;
        if (err == -FFS_INVALIDPATH)
            return -EINVAL;
        if (err == -FFS_FILEEXIST)
            return -EEXIST;
        if (err == -FFS_NOTFOUND)
            return -ENOENT;
        if (err == -FFS_FULL)
            return -ENOSPC;
        else
            return -EIO;
    }

    MEMCPY(old_fidp, &new_fid, sizeof(rfs_file_id_t));
    old_inode->i_ino = RFS_INO(new_fid.parent_first_cluster, new_fid.filename[0], new_fid.filename[new_fid.fn_length-1]); //new_fid.parent_first_cluster;

    remove_inode_hash(old_inode);
    insert_inode_hash(old_inode);

    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_rename */

static void rfs_vfs_truncate(struct inode *inode)
{
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    loff_t zero_pos;
    ssize_t result = 0;
    ssize_t rest, i;
    unsigned int cur_size = 0;
    int err;
    
    RFS_VFS_LOCK;

    /* check if size to be truncated is bigger than current size */
    if ((cur_size = rfs_get_size(fid)) < 0) {
        DMSG(DL1, (KERN_NOTICE "%s: rfs_get_size failed: %d\n", 
                                                __FUNCTION__, err));
        RFS_VFS_UNLOCK;
        return;
    }

    if (cur_size < inode->i_size) {
        /* truncate file to be bigger one */
        MEMSET(zero_buf, 0, 512);
        zero_pos = cur_size;
        rest = inode->i_size - cur_size;

        while (rest > 0) {
            i = min_t(unsigned int, 512, rest);

            result = rfs_file_write(fid, zero_buf, i, &zero_pos);
            if (result <= 0) break;

            rest -= result;
        }

        if (result <= 0) {
            if (result < 0) {
                DMSG(DL1, (KERNEL_NOTICE "%s: rfs_write_file failed: %d\n",
                                                __FUNCTION__, result));
            }
            else {
                DMSG(DL1, (KERNEL_NOTICE "%s: rfs_write_file failed: %d\n",
                                                __FUNCTION__, -FFS_FULL));
            }

            if ((cur_size = rfs_get_size(fid)) < 0) {
                DMSG(DL1, (KERN_NOTICE "%s: rfs_get_size failed: %d\n", 
                                                __FUNCTION__, err));
                RFS_VFS_UNLOCK;
                return;
            }

            inode->i_size = cur_size;
        }
    }
    else {
        /* truncate file to be smaller one */
        if ((err = rfs_truncate(fid, &inode->i_size)) < 0) {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_truncate failed: %d\n", 
                                                __FUNCTION__, err));

            if ((cur_size = rfs_get_size(fid)) < 0) {
                DMSG(DL1, (KERN_NOTICE "%s: rfs_get_size failed: %d\n", 
                                                __FUNCTION__, err));
                RFS_VFS_UNLOCK;
                return;
            }

            inode->i_size = cur_size;
        }
        
        fid->hint_last_off = -1;
        fid->hint_last_clu = 1;
    }

    inode->i_blocks = (inode->i_size + 511) >> 9;
    inode->i_mtime = inode->i_atime = CURRENT_TIME;
    mark_inode_dirty(inode);
    
    RFS_VFS_UNLOCK;
} /* end of rfs_vfs_truncate */

static int rfs_vfs_permission(struct inode *inode, int mode, 
                              struct nameidata *nd)
{
    return 0;    // all operation permitted
} /* end of rfs_vfs_permission */

static int rfs_vfs_setattr(struct dentry *dentry, struct iattr *ia)
{
    struct inode *inode = dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    int perm, sticky, err;
    char rfs_attr = 0;
    
    RFS_VFS_LOCK;

    if (ia->ia_valid & ATTR_MODE) {
        perm = ia->ia_mode & 0777;
        sticky = ia->ia_mode & 01000;

        if ((err = rfs_get_attr(fid, &rfs_attr)) < 0) {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_get_attr failed: %d\n", 
                                                __FUNCTION__, err));
            RFS_VFS_UNLOCK;
            return -EPERM;
        }
        if (perm & 0222)
            rfs_attr &= ~ATTR_READONLY;
        else
            rfs_attr |= ATTR_READONLY;

        if (sticky & 01000)
            rfs_attr |= ATTR_HIDDEN | ATTR_SYSTEM;
        else
            rfs_attr &= ~(ATTR_HIDDEN | ATTR_SYSTEM);

        if ((err = rfs_set_attr(fid, rfs_attr)) < 0) {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_set_file_attr failed: %d\n", 
                                                __FUNCTION__, err));
            RFS_VFS_UNLOCK;
            return -EPERM;
        }
    }

    RFS_VFS_UNLOCK;
    err = inode_setattr(inode, ia);
    
    return err;
} /* end of rfs_vfs_setattr */

static int rfs_vfs_readlink(struct dentry *dentry, char *buffer, int buflen)
{
    struct inode *inode = dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    loff_t pos = 0;
    ssize_t result;
    char *tmp_buf = zero_buf;
    int err;
    
    RFS_VFS_LOCK;

    result = rfs_file_read(fid, tmp_buf, 512, &pos);
    if (result < 0 || result > buflen) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: symbolic link read failed: %d\n", 
                                                __FUNCTION__, result));
        if (result == -FFS_INVALIDPATH)
            return -EINVAL;
        if (result == -FFS_NOTFOUND)
            return -ENOENT;
        else
            return -EIO;
    }
    
    err = vfs_readlink(dentry, buffer, buflen, tmp_buf);
    RFS_VFS_UNLOCK;

    return err;
} /* end of rfs_vfs_readlink */

static void rfs_vfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
    struct inode *inode = dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    loff_t pos = 0;
    ssize_t result;
    char *tmp_buf = zero_buf;
    
    RFS_VFS_LOCK;

    result = rfs_file_read(fid, tmp_buf, 512, &pos);
    if (result < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: symbolic link read failed: %d\n", 
                                                __FUNCTION__, result));
        return;
    }
    
    vfs_follow_link(nd, tmp_buf);
    RFS_VFS_UNLOCK;
} /* end of rfs_vfs_follow_link */


/*----------------------------------------------------------------------*/
/*  Directory File Operations                                           */
/*----------------------------------------------------------------------*/

static struct file_operations rfs_dir_operations = {
    read: generic_read_dir,
    readdir: rfs_vfs_readdir,
};

static int rfs_vfs_readdir(struct file *filp, void *dir_entry, 
                            filldir_t filldir)
{
    struct inode *inode = filp->f_dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    rfs_dir_entry_t rfs_dentry;
    unsigned int tmp_type;
    unsigned int tmp_ino;
    loff_t tmp_pos;
    int err;
    
    RFS_VFS_LOCK;

    while (1) {
        tmp_pos = filp->f_pos;

        err = rfs_readdir(fid, &rfs_dentry, &filp->f_pos);
        if (err < 0) {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_readdir failed: %d\n", 
                                                __FUNCTION__, err));
            break;
        }

        if (!rfs_dentry.name[0]) {
            break;
        }

        tmp_ino = fid->first_cluster;
        // Busybox App treats inode 0 exceptionally.
        // So we do the trick for FAT12/16 root_dir.
        if (tmp_ino == 0) tmp_ino++;

        if (rfs_dentry.type == TYPE_DIR) 
            tmp_type = DT_DIR;
        else 
            tmp_type = DT_REG;

        err = filldir(dir_entry, rfs_dentry.name, strlen(rfs_dentry.name),
                      tmp_pos, tmp_ino, tmp_type);
        if (err < 0) {
            filp->f_pos = tmp_pos;      // rollback
            break;
        }
    }

    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_readdir */


/*----------------------------------------------------------------------*/
/*  File Operations                                                     */
/*----------------------------------------------------------------------*/
 
#if (RFS_CONFIG_PAGE_CACHE == 1)
static struct file_operations rfs_file_operations = {
    //read:       do_sync_read,
    read:       rfs_do_sync_read,
    write:      do_sync_write,
    aio_read:   generic_file_aio_read,
    aio_write:  generic_file_aio_write,
    mmap:       generic_file_mmap,
    fsync:      rfs_vfs_file_sync,
};
#else
static struct file_operations rfs_file_operations = {
    read:       rfs_vfs_file_read,
    write:      rfs_vfs_file_write,
    aio_read:   generic_file_aio_read,
    aio_write:  generic_file_aio_write,
    mmap:       generic_file_mmap,
    fsync:      rfs_vfs_file_sync,
};
#endif

static struct address_space_operations rfs_address_operations = {
    readpage:       rfs_vfs_readpage,
    writepage:      rfs_vfs_writepage,
    prepare_write:  rfs_vfs_prepare_write,
    commit_write:   rfs_vfs_commit_write,
#ifdef DIO
    direct_IO:      rfs_vfs_direct_IO,
#endif
};


#if (RFS_CONFIG_PAGE_CACHE == 0)

static ssize_t rfs_vfs_file_read(struct file *filp, char *buffer,
                                  size_t count, loff_t *ppos)
{
    struct inode *inode = filp->f_dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    ssize_t result = 0, read_bytes = 0;
    ssize_t size, rest, i;
    char *tmp_buf = NULL;
    
    RFS_VFS_LOCK;

    /* do boundary check */
    if (*ppos > inode->i_size) {
        RFS_VFS_UNLOCK;
        return 0;
    }

    size = (count < MALLOC_SIZE) ? count : MALLOC_SIZE;
    rest = count;

    while (size > 0) {
        tmp_buf = (char *) MALLOC(size);
        if (tmp_buf) break;
        size >>= 1;
    }
    if (!tmp_buf) {
        DMSG(DL1, (KERN_NOTICE "%s: MALLOC failed: %d\n", 
                                                __FUNCTION__, -ENOMEM));
        RFS_VFS_UNLOCK;
        return -ENOMEM;
    }

    while (rest > 0) {
        i = min_t(unsigned int, size, rest);

        result = rfs_file_read(fid, tmp_buf, i, ppos);
        if (result < 0) break;

        if (copy_to_user(buffer+read_bytes, tmp_buf, result)) {
            result = -FFS_MEMORYERR;
            break;
        }

        read_bytes += result;
        if (result < i) break;

        rest -= result;
    }

    FREE(tmp_buf);

    if (result < 0) {
        RFS_VFS_UNLOCK;
        DMSG(DL1, (KERN_NOTICE "%s: rfs_file_read failed: %d\n", 
                                                __FUNCTION__, result));
        if (result == -FFS_INVALIDPATH)
            return -EINVAL;
        if (result == -FFS_MEMORYERR)
            return -EFAULT;
        if (result == -FFS_NOTFOUND)
            return -ENOENT;
        else
            return -EIO;
    }

    RFS_VFS_UNLOCK;
    return read_bytes;
} /* end of rfs_vfs_file_read */

static ssize_t rfs_vfs_file_write(struct file *filp, const char *buffer,
                                   size_t count, loff_t *ppos)
{
    struct inode *inode = filp->f_dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    ssize_t result = 0, written_bytes = 0;
    ssize_t size, rest, i;
    loff_t zero_pos;
    unsigned long limit = current->signal->rlim[RLIMIT_FSIZE].rlim_cur;
    char *tmp_buf = NULL;
    
    RFS_VFS_LOCK;

    /* check if it is read-only filesystem */
    if (inode->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EROFS;
    }

    /* check if file is opened as O_APPEND */
    if ((!S_ISBLK(inode->i_mode)) && (filp->f_flags & O_APPEND))
        *ppos = inode->i_size;

    /* check if ppos exceeds file limit */
    if ((!S_ISBLK(inode->i_mode)) && (limit != RLIM_INFINITY)) {
        if (*ppos >= limit) {
            send_sig(SIGXFSZ, current, 0);
            RFS_VFS_UNLOCK;
            return -EFBIG;
        }
        if ((*ppos > 0xFFFFFFFFULL) || (count > (limit - (u32) *ppos))) {
            count = limit - (u32) *ppos;
        }
    }

    if (*ppos > inode->i_size) {
        MEMSET(zero_buf, 0, 512);
        zero_pos = inode->i_size;
        rest = *ppos - inode->i_size;

        while (rest > 0) {
            i = min_t(unsigned int, 512,rest);

            result = rfs_file_write(fid, zero_buf, i, &zero_pos);
            if (result <= 0) break;

            written_bytes += result;
            rest -= result;
        }

        if (written_bytes > 0) {
            inode->i_size = max_t(loff_t, inode->i_size, zero_pos);
            inode->i_blocks = (inode->i_size + 511) >> 9;

            if (filp->f_flags & O_SYNC) {
                rfs_sync_vol(fid->dev, fid->vol);
                inode->i_sb->s_dirt = 0;
            }
            else {
                inode->i_sb->s_dirt = 1;
            }
        }

        if (result <= 0) {
            RFS_VFS_UNLOCK;
            if (result < 0) {
                DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, result));
                if (result == -FFS_INVALIDPATH)
                    return -EINVAL;
                if (result == -FFS_NOTFOUND)
                    return -ENOENT;
                else
                    return -EIO;
            }
            else {
                DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, -FFS_FULL));
                return -ENOSPC;
            }
        }
    }

    size = (count < MALLOC_SIZE) ? count : MALLOC_SIZE;
    rest = count;

    while (size > 0) {
        tmp_buf = (char *) MALLOC(size);
        if (tmp_buf) break;
        size >>= 1;
    }
    if (!tmp_buf) {
        DMSG(DL1, (KERN_NOTICE "%s: MALLOC failed: %d\n", 
                                                __FUNCTION__, -ENOMEM));
        RFS_VFS_UNLOCK;
        return -ENOMEM;
    }

    written_bytes = 0;

    while (rest > 0) {
        i = min_t(unsigned int, size, rest);

        if (copy_from_user(tmp_buf, buffer+written_bytes, i)) {
            result = -FFS_MEMORYERR;
            break;
        }

        result = rfs_file_write(fid, tmp_buf, i, ppos);
        if (result <= 0) break; 

        written_bytes += result;
        if (result < i) break;

        rest -= result;
    } 

    FREE(tmp_buf);

    if (written_bytes > 0) {
        inode->i_size = max_t(loff_t, inode->i_size, *ppos);
        inode->i_blocks = (inode->i_size + 511) >> 9;

        if (filp->f_flags & O_SYNC) {
            rfs_sync_vol(fid->dev, fid->vol);
            inode->i_sb->s_dirt = 0;
        }
        else {
            inode->i_sb->s_dirt = 1;
        }
    }

    if (result <= 0) {
        RFS_VFS_UNLOCK;
        if (result < 0) {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, result));
            if (result == -FFS_INVALIDPATH)
                return -EINVAL;
            if (result == -FFS_MEMORYERR)
                return -EFAULT;
            if (result == -FFS_NOTFOUND)
                return -ENOENT;
            else
                return -EIO;
        }
        else {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, -FFS_FULL));
            return -ENOSPC;
        }
    }

    mark_inode_dirty(inode);

    RFS_VFS_UNLOCK;
    return written_bytes;
} /* end of rfs_vfs_file_write */

#else

static ssize_t rfs_do_sync_read(struct file *filp, char __user *buf, 
                                size_t len, loff_t *ppos)
{
    ssize_t ret;
    struct inode *inode = filp->f_dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    struct address_space *mapping = inode->i_mapping;
    
    ret = do_sync_read(filp, buf, len, ppos);
    
    RFS_VFS_LOCK;
    if (fid->vol == 1) {
        invalidate_inode_pages(mapping);
    }
    RFS_VFS_UNLOCK;
    
    return(ret);
}  /* end of rfs_do_sync_read */

#endif

static int rfs_vfs_file_sync(struct file *filp, struct dentry *dentry, 
                              int datasync)
{
    return 0;
} /* end of rfs_vfs_file_sync */

static int rfs_vfs_do_readpage(struct file *file, struct page *page)
{
    struct inode *inode = file->f_dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    loff_t pos;
    ssize_t r, read_bytes;
    int result;
    void *tmp_buf;
    
    RFS_VFS_LOCK;

    get_page(page);
    tmp_buf = page_address(page);
    ClearPageUptodate(page);
    ClearPageError(page);

    read_bytes = result = 0;
    pos = page->index << PAGE_CACHE_SHIFT;

    if (pos < inode->i_size) {
        read_bytes = min_t(unsigned int, inode->i_size-pos, PAGE_SIZE);
        r = rfs_file_read(fid, tmp_buf, read_bytes, &pos);
        if (r != read_bytes) {
            RFS_VFS_UNLOCK;
            DMSG(DL1, (KERN_NOTICE "%s: rfs_file_read failed: %d\n", 
                                                __FUNCTION__, r));
            if (r == -FFS_INVALIDPATH)
                result = -EINVAL;
            if (r == -FFS_NOTFOUND)
                result = -ENOENT;
            else
                result = -EIO;
        }
    }

    if (read_bytes < PAGE_SIZE)
        MEMSET(tmp_buf+read_bytes, 0, PAGE_SIZE-read_bytes);

    if (result) 
        SetPageError(page);
    else 
        SetPageUptodate(page);       

    flush_dcache_page(page);

    put_page(page);

    RFS_VFS_UNLOCK;
    return result;
} /* end of rfs_vfs_do_readpage */

static int rfs_vfs_readpage(struct file *file, struct page *page)
{
    int ret = rfs_vfs_do_readpage(file, page);
    unlock_page(page);
    return ret;
} /* end of rfs_vfs_readpage */

static int rfs_vfs_writepage(struct page *page, struct writeback_control *wbc)
{
    struct inode *inode = page->mapping->host;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    loff_t pos = page->index << PAGE_CACHE_SHIFT;
    void *buf = page_address(page);
    unsigned long end_index = inode->i_size >> PAGE_CACHE_SHIFT;
    unsigned int written_bytes = (page->index < end_index) ? PAGE_SIZE : inode->i_size & (PAGE_SIZE-1);
    ssize_t result;
    
    RFS_VFS_LOCK;

    result = rfs_file_write(fid, buf, written_bytes, &pos);

    if (result <= 0) {
        RFS_VFS_UNLOCK;
        if (result < 0) {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, result));
            if (result == -FFS_INVALIDPATH)
                return -EINVAL;
            if (result == -FFS_NOTFOUND)
                return -ENOENT;
            else
                return -EIO;
        }
        else {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, -FFS_FULL));
            return -ENOSPC;
        }
    }

    rfs_sync_vol(fid->dev, fid->vol);
    
    inode->i_sb->s_dirt = 0;
    SetPageUptodate(page);
    unlock_page(page);
    
    RFS_VFS_UNLOCK;
    return 0;
} /* end of rfs_vfs_writepage */

static ssize_t rfs_vfs_prepare_write(struct file *filp, struct page *page,
                                      unsigned int from, unsigned int to)
{
    if (!PageUptodate(page) && (from || to < PAGE_CACHE_SIZE))
        return rfs_vfs_do_readpage(filp, page);

    return 0;
} /* end of rfs_vfs_prepare_write */

static ssize_t rfs_vfs_commit_write(struct file *filp, struct page *page,
                                     unsigned int from, unsigned int to)
{
    struct inode *inode = filp->f_dentry->d_inode;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    rfs_file_id_t *fid = &inode_i->fid;
    loff_t zero_pos, pos = (page->index << PAGE_CACHE_SHIFT) + from;
    void *buf = page_address(page) + from;
    ssize_t result = 0, written_bytes = 0;
    ssize_t rest, i;

    RFS_VFS_LOCK;
    
    if (inode->i_sb->s_flags & MS_RDONLY) {
        RFS_VFS_UNLOCK;
        return -EROFS;
    }

    if (pos > inode->i_size) {
        MEMSET(zero_buf, 0, 512);
        zero_pos = inode->i_size;
        rest = pos - inode->i_size;

        while (rest > 0) {
            i = min_t(unsigned int, 512, rest);

            result = rfs_file_write(fid, zero_buf, i, &zero_pos);
            if (result <= 0) break;

            written_bytes += result;
            rest -= result;
        }

        if (written_bytes > 0) {
            inode->i_size = max_t(loff_t, inode->i_size, zero_pos);
            inode->i_blocks = (inode->i_size + 511) >> 9;

            if (filp->f_flags & O_SYNC) {
                rfs_sync_vol(fid->dev, fid->vol);
                inode->i_sb->s_dirt = 0;
            }
            else {
                inode->i_sb->s_dirt = 1;
            }

            SetPageUptodate(page);       
        }

        if (result <= 0) {
            RFS_VFS_UNLOCK;
            if (result < 0) {
                DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, result));
                if (result == -FFS_INVALIDPATH)
                    return -EINVAL;
                if (result == -FFS_NOTFOUND)
                    return -ENOENT;
                else
                    return -EIO;
            }
            else {
                DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, -FFS_FULL));
                return -ENOSPC;
            }
        }
    }

    result = rfs_file_write(fid, buf, to-from, &pos);

    if (result <= 0) {
        SetPageError(page);
        RFS_VFS_UNLOCK;

        if (result < 0) {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, result));
            if (result == -FFS_INVALIDPATH)
                return -EINVAL;
            if (result == -FFS_NOTFOUND)
                return -ENOENT;
            else
                return -EIO;
        }
        else {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, -FFS_FULL));
            return -ENOSPC;
        }
    }

    inode->i_size = max_t(loff_t, inode->i_size, pos);
    inode->i_blocks = (inode->i_size + 511) >> 9;
    inode->i_mtime = inode->i_atime = CURRENT_TIME;

    inode->i_sb->s_dirt = 1;
    
    mark_inode_dirty(inode);
    SetPageUptodate(page);       

    RFS_VFS_UNLOCK;
    return result;
} /* end of rfs_vfs_commit_write */

#ifdef DIO
static int rfs_vfs_direct_IO(int rw, struct inode *inode, struct kiobuf *iobuf,
                              unsigned long blocknr, int blocksize)
{
    rfs_file_id_t *fid = &inode->u.rfs_i.fid;
    loff_t pos = blocknr * blocksize;
    loff_t tmp_pos, zero_pos;
    ssize_t result = 0;
    ssize_t rest, i;
    
    RFS_VFS_LOCK;

    /* do alignment and validity check */
    if ((iobuf->offset & (blocksize-1)) || (iobuf->length & (blocksize-1))) {
        RFS_VFS_UNLOCK;
        return -EINVAL;
    }

    if (!iobuf->nr_pages)
        panic("rfs_vfs_direct_IO: iobuf not initialized");

    if (rw == READ) {
        result = rfs_file_read_direct(fid, iobuf, &pos);

        if (result < 0) {
            RFS_VFS_UNLOCK;
            DMSG(DL1, (KERN_NOTICE "%s: direct IO read failed: %d\n", 
                                                __FUNCTION__, result));
            if (result == -FFS_INVALIDPATH)
                return -EINVAL;
            if (result == -FFS_NOTFOUND)
                return -ENOENT;
            else
                return -EIO;
        }
    }
    else {
        if (inode->i_sb->s_flags & MS_RDONLY) {
            RFS_VFS_UNLOCK;
            return -EROFS;
        }

        tmp_pos = blocknr << inode->i_blkbits;
        if (tmp_pos > inode->i_size) {
            MEMSET(zero_buf, 0, 512);
            zero_pos = inode->i_size;
            rest = tmp_pos - inode->i_size;

            while (rest > 0) {
                i = min_t(unsigned int, 512, rest);

                result = rfs_file_write(fid, zero_buf, i, &zero_pos);
                if (result <= 0) break;

                rest -= result;
            }

            if (result <= 0) {
                RFS_VFS_UNLOCK;
                if (result < 0) {
                    DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, result));
                    if (result == -FFS_INVALIDPATH)
                        return -EINVAL;
                    if (result == -FFS_NOTFOUND)
                        return -ENOENT;
                    else
                        return -EIO;
                }
                else {
                    DMSG(DL1, (KERN_NOTICE "%s: rfs_file_write failed: %d\n", 
                                                __FUNCTION__, -FFS_FULL));
                    return -ENOSPC;
                }
            }

            inode->i_size = max_t(loff_t, inode->i_size, zero_pos);
            inode->i_blocks = (inode->i_size + 511) >> 9;

            inode->i_sb->s_dirt = 1;
       }

        result = rfs_file_write_direct(fid, iobuf, &pos);

        if (result <= 0) {
            RFS_VFS_UNLOCK;
            if (result < 0) {
                DMSG(DL1, (KERN_NOTICE "%s: direct IO write failed: %d\n", 
                                                __FUNCTION__, result));
                if (result == -FFS_INVALIDPATH)
                    return -EINVAL;
                if (result == -FFS_NOTFOUND)
                    return -ENOENT;
                else
                    return -EIO;
            }
            else {
                DMSG(DL1, (KERN_NOTICE "%s: direct IO write failed: %d\n", 
                                                __FUNCTION__, -FFS_FULL));
                return -ENOSPC;
            }
        }

        inode->i_size = max_t(loff_t, inode->i_size, pos);
        inode->i_blocks = (inode->i_size + 511) >> 9;

        inode->i_sb->s_dirt = 1;
        
        mark_inode_dirty(inode);
    }

    RFS_VFS_UNLOCK;
    return result;
} /* end of rfs_vfs_direct_IO */
#endif


/*----------------------------------------------------------------------*/
/*  Dentry Operations                                                   */
/*----------------------------------------------------------------------*/

static struct dentry_operations rfs_dentry_operations = {
    d_delete: rfs_vfs_dentry_delete,
};

static int rfs_vfs_dentry_delete(struct dentry *dentry)
{
    return 1;
} /* end of rfs_vfs_dentry_delete */


/*----------------------------------------------------------------------*/
/*  Other Utility Functions                                             */
/*----------------------------------------------------------------------*/

#ifdef CONFIG_RFS_NLS
static int rfs_parse_options(char *options, int *codepage, char *iocharset)
{
    char *this_char, *value, save, *savep, *p;
    int ret = 1, len;

    *codepage = 0;
    *iocharset = '\0';

    if (!options)
        return 1;

    save = '\0';
    savep = NULL;

    for (this_char = strtok(options, "."); this_char; 
                                           this_char = strtok(NULL, ",")) {
        if ((value = strchr(this_char, '=')) != NULL) {
            save = *value;
            savep = value;
            *value++ = 0;
        }
        if (!strcmp(this_char, "codepage") && value) {
            *codepage = simple_strtoul(value, &value, 0);
            if (*value) ret = 0;
        }
        else if (!strcmp(this_char, "iocharset") && value) {
            p = value;
            while (*value && (*value != ','))
                value++;
            len = value - p;
            if (len) {
                if (len < 50) {
                    MEMCPY(iocharset, p, len);
                    iocharset[len] = '\0';
                }
                else {
                    ret = 0;
                }
            }
        }
        if (this_char != options) *(this_char-1) = ',';
        if (value) *savep = save;
        if (ret == 0) break;
    }

    return ret;
} /* end of rfs_parse_options */
#endif

static struct inode *rfs_new_inode(struct super_block *sb)
{
    struct inode *inode;

    inode = new_inode(sb);
    if (inode != NULL) {
        inode->i_private = MALLOC(sizeof(rfs_inode_info_t));
        if (inode->i_private == NULL) {
            iput(inode);
            return NULL;
        }
        inode->i_rdev = sb->s_dev;
        inode->i_bdev = sb->s_bdev;
    }

    return inode;
} /* end of rfs_new_inode */

static int rfs_iset(struct inode *inode, void *data)
{
    rfs_inode_info_t *rfs_i;
    rfs_file_id_t *fid = (rfs_file_id_t *)data;
    //rfs_inode_t fi;

    inode->i_rdev = inode->i_sb->s_dev;
    inode->i_bdev = inode->i_sb->s_bdev;

    rfs_i = MALLOC(sizeof(rfs_inode_info_t));
    if (rfs_i == NULL) return -1;

    MEMCPY(&rfs_i->fid, fid, sizeof(rfs_file_id_t));
    inode->i_private = (void *)rfs_i;
    
#if 1
    RFS_VFS_UNLOCK;
    rfs_vfs_read_inode(inode);
    RFS_VFS_LOCK;

#else
    MEMSET(&fi, 0, sizeof(rfs_inode_t));

    rfs_read_inode(fid, &fi);
    
    if (fi.type == TYPE_DIR) 
        inode->i_mode = S_IFDIR;
    else if (fi.type == TYPE_SYMLINK) 
        inode->i_mode = S_IFLNK;
    else if (fi.type == TYPE_SOCKET) 
        inode->i_mode = S_IFSOCK;
    else 
        inode->i_mode = S_IFREG;

    if (S_ISDIR(inode->i_mode)) {
        fid->first_cluster = fi.first_cluster;
    }
    fid->hint_last_off = -1;
    fid->hint_last_clu = 1;

    inode->i_mode |= 0777;
    if (fi.attr & ATTR_READONLY) 
        inode->i_mode &= ~0222;

    inode->i_size = fi.size;
    inode->i_blkbits = inode->i_sb->s_blocksize_bits;
    inode->i_blocks = (inode->i_size + 511) >> 9;
    inode->i_mtime.tv_sec = (time_t) fi.mtime;

    if (S_ISREG(inode->i_mode)) {
        inode->i_op = &rfs_file_inode_operations;
        inode->i_fop = &rfs_file_operations;
        inode->i_mapping->a_ops = &rfs_address_operations;
        inode->i_mapping->nrpages = 0;
    }
    else if (S_ISDIR(inode->i_mode)) {
        inode->i_op = &rfs_dir_inode_operations;
        inode->i_fop = &rfs_dir_operations;
    }
    else if (S_ISLNK(inode->i_mode)) {
        inode->i_op = &rfs_symlink_inode_operations;
    }
    else if (S_ISSOCK(inode->i_mode)) {
        init_special_inode(inode, inode->i_mode, 0);
    }
#endif

    return 0;
} /* rfs_iset */

static int rfs_find_actor(struct inode *inode, void *data)
{
    int i;
    int len1, len2;
    char *fn1, *fn2;
    rfs_file_id_t *fid = (rfs_file_id_t *)data;
    rfs_inode_info_t *inode_i = (rfs_inode_info_t *)inode->i_private;
    
    i = GET_DEV(inode->i_rdev);
    if (i != fid->dev)
        return 0;

    len1 = fid->fn_length;
    len2 = inode_i->fid.fn_length;

    if (len1 != len2)
        return 0;

    fn1 = fid->filename;
    fn2 = inode_i->fid.filename;

    for (i = 0; i < len1; i++) {
        if (toupper(fn1[i]) != toupper(fn2[i]))
            return 0;
    }

    return 1;
} /* end of rfs_find_actor */


/*----------------------------------------------------------------------*/
/*  Module Initialization Functions                                     */
/*----------------------------------------------------------------------*/

static struct file_system_type rfs_fs_type = {
    .owner      = THIS_MODULE,
    .name       = "rfs",
    .get_sb     = rfs_get_sb,
    .kill_sb    = kill_block_super,
    .fs_flags   = FS_REQUIRES_DEV,
};

static int rfs_fill_super(struct super_block *sb, void *data, int silent)
{
    rfs_sb_info_t *rfs_sb;
    rfs_file_id_t fid;
    struct inode *root_inode = NULL;
    int dev, vol, err;

#if (RFS_CONFIG_AUTO_FORMAT == 1)
    BDEV_INFO_T binfo;
    int vol_off, vol_size;
#endif

    RFS_VFS_LOCK_INIT;

    dev = GET_DEV(sb->s_dev);
    vol = GET_VOL(sb->s_dev);

    /* allocate memory for rfs private info */
    rfs_sb = MALLOC(sizeof(rfs_sb_info_t));
    if (rfs_sb == NULL) {
        goto rfs_sb_err1;
    }

    // setup superblock info
    sb_min_blocksize(sb, 512);

    sb->s_maxbytes = 0xFFFFFFFF;    // maximum file size

    sb->s_magic = RFS_MAGIC;
    sb->s_op = &rfs_ops;
    sb->s_dirt = 0;
    sb->s_bdev->bd_private = (unsigned long) sb;

    if ((err = rfs_mount_vol(dev, vol)) < 0) {

#if (RFS_CONFIG_AUTO_FORMAT == 1)
        if (MAJOR(dev) == LFD_BLK_DEVICE_FTL) {
            DMSG(DL1, (KERN_NOTICE "%s: rfs_mount_vol failed: %d\n", 
                                                __FUNCTION__, err));

            /* format the flash memory for FTL (low-level format) */
            if ((err = bdev_ioctl(dev, BDEV_FORMAT, NULL)) != 0) {
                DMSG(DL1, (KERN_NOTICE "%s: ftl_format failed: %d\n", 
                                                __FUNCTION__, err));
                goto rfs_sb_err1;
            }

            /* open the block device */
            if ((err = bdev_open(dev)) != 0) {
                DMSG(DL1, (KERN_NOTICE "%s: ftl_open failed: %d\n", 
                                                __FUNCTION__, err));
                goto rfs_sb_err1;
            }

            if ((err = bdev_ioctl(dev, BDEV_GET_DEV_INFO, &binfo)) != 0) {
                DMSG(DL1, (KERN_NOTICE "%s: ftl_stat failed: %d\n", 
                                                __FUNCTION__, err));
                goto rfs_sb_err1;
            }

            vol_off  = 1;
            vol_size = binfo.cylinders * binfo.heads * binfo.sectors - 1;

            /* close the block device */
            if ((err = bdev_close(dev)) != 0) {
                DMSG(DL1, (KERN_NOTICE "%s: ftl_close failed: %d\n", 
                                                __FUNCTION__, err));
                goto rfs_sb_err1;
            }
    
            if ((err = rfs_set_partition(dev, 1, &vol_off, &vol_size)) < 0) {
                DMSG(DL1, (KERN_NOTICE "%s: rfs_set_partition failed: %d\n", 
                                                __FUNCTION__, err));
                goto rfs_sb_err1;
            }

            if ((err = rfs_format_vol(dev, vol, 
                                      FAT32, DEFAULT_CLUSIZE)) < 0) {
                if ((err = rfs_format_vol(dev, vol, 
                                          FAT16, DEFAULT_CLUSIZE)) < 0) {
                    if ((err = rfs_format_vol(dev, vol, 
                                              FAT12, DEFAULT_CLUSIZE)) < 0) {
                        DMSG(DL1, (KERN_NOTICE "%s: rfs_format_vol failed: %d\n", 
                                                __FUNCTION__, err));
                        goto rfs_sb_err1;
                    }
                }
            }

            if ((err = rfs_mount_vol(dev, vol)) < 0){
                DMSG(DL1, (KERN_NOTICE "%s: rfs_mount_vol failed: %d\n", 
                                                __FUNCTION__, err));
                goto rfs_sb_err1;
            }
        } 
        else {
#endif
            DMSG(DL1, (KERN_NOTICE "%s: rfs_mount_vol failed: %d\n", 
                                                __FUNCTION__, err));
            goto rfs_sb_err1;
#if (RFS_CONFIG_AUTO_FORMAT == 1)
        }
#endif
    }

    if ((err = rfs_read_super(dev, vol, rfs_sb)) < 0) {
        DMSG(DL1, (KERN_NOTICE "%s: rfs_read_super failed: %d\n", 
                                                __FUNCTION__, err));
        goto rfs_sb_err2;
    }

    // set codepage
    set_codepage();
 
    // read root dir inode
    fid.dev = dev;
    fid.vol = vol;
    fid.parent_first_cluster = rfs_sb->root_cluster;
    fid.first_cluster = rfs_sb->root_cluster;
    strcpy(fid.filename, ".");
    fid.fn_length = 1;
    fid.dentry = -2;

    if ((err = rfs_lookup(&fid, NULL)) < 0) {
        DMSG(DL1, (KERN_NOTICE "%s: rfs_lookup failed: %d\n", 
                                                __FUNCTION__, err));
        goto rfs_sb_err3;
    }

    RFS_VFS_LOCK;
    root_inode = iget5_locked(sb, 
                              RFS_INO(fid.parent_first_cluster, fid.filename[0], fid.filename[fid.fn_length-1]), //rfs_sb->root_cluster, 
                              rfs_find_actor, rfs_iset, (void *)&fid);
    RFS_VFS_UNLOCK;
    if (!root_inode)
        goto rfs_sb_err3;

    if (root_inode->i_state & I_NEW) {
        root_inode->i_sb = sb;
        root_inode->i_ino = 2;
        unlock_new_inode(root_inode);
    }

    if (!(sb->s_root = d_alloc_root(root_inode)))
        goto rfs_sb_err4;

    sb->s_root->d_op = &rfs_dentry_operations;

    sb->s_fs_info = (void *)rfs_sb;
    return 0;

rfs_sb_err4:
    iput(root_inode);

rfs_sb_err3:
#ifdef CONFIG_RFS_NLS
    if (rfs_put_codepage(dev, vol) < 0) 
        PRINT(KERN_WARNING "RFS: failed to clear mount\n");
#endif

rfs_sb_err2:
    if (rfs_umount_vol(dev, vol) < 0) 
        PRINT(KERN_WARNING "RFS: failed to clear mount\n");
    FREE(rfs_sb);

rfs_sb_err1:
    PRINT(KERN_WARNING "RFS: failed to mount\n");
    return -1;
} /* end of rfs_fill_super */
#if 1
static struct super_block *rfs_get_sb(struct file_system_type *fs_type,
        int flags, const char *dev_name, void *data, struct vfsmount *mnt)
{
        return get_sb_bdev(fs_type, flags, dev_name, data, rfs_fill_super, mnt);
}
#else
static struct super_block *rfs_get_sb(struct file_system_type *fs_type,
        int flags, const char *dev_name, void *data)
{
        return get_sb_bdev(fs_type, flags, dev_name, data, rfs_fill_super);
}

#endif

int __init init_rfs_fs(void)
{
    extern RFS_VOL_OPS_T rfs_vol_fops;

    int err = ufd_init();
    if (err) return -1;

    rfs_vol_fops.set_part   = rfs_set_partition;
    rfs_vol_fops.get_part   = rfs_get_partition;
    rfs_vol_fops.format     = rfs_format_vol;
    rfs_vol_fops.mount      = rfs_mount_vol;
    rfs_vol_fops.umount     = rfs_umount_vol;
    rfs_vol_fops.check      = rfs_check_vol;
    rfs_vol_fops.sync       = rfs_sync_vol;

    return register_filesystem(&rfs_fs_type);
} /* end of init_rfs_fs */

void __exit exit_rfs_fs(void)
{
    ufd_exit();
    unregister_filesystem(&rfs_fs_type);
} /* end of exit_rfs_fs */

module_init(init_rfs_fs);
module_exit(exit_rfs_fs);

//MODULE_LICENSE("Zeen Information Technologies, Inc. Proprietary");
