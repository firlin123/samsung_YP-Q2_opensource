/*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation; either version 2 of the License, or (at your option) any later
* version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
/*
 * Driver for the STMP3xXX onchip DIGCTL (TA3)
 *
 * 2008 Sigmatel Korea
 *
 * 2005 (c) Samsung Electronics
 *          Heechul Yun <heechul.yun@samsung.com>
 *
 */ 
 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>       /* everything... */
#include <linux/mman.h>
#include <linux/init.h>
#include <linux/rwsem.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/pgalloc.h>

#include <asm/arch/hardware.h> 


#define DIGCTL_MINOR  66

static unsigned long digctl_base = 0;
static unsigned long digctl_size = 0;

static spinlock_t digctl_spinlock;
static volatile int digctl_locked = 0;

#define DIGCTL_PHYS 0x8001c000
#define DIGCTL_BASE 0xe001c000
#define DIGCTL_SIZE 0x00000400

int digctl_access_obtain(unsigned long *pmem, unsigned long *psize) {

	unsigned long flags;
	
	spin_lock_irqsave(&digctl_spinlock, flags);
	
	if (digctl_locked != 0) 	{
		spin_unlock_irqrestore(&digctl_spinlock, flags);
		return -EAGAIN;
	}

	digctl_locked = 0;

	spin_unlock_irqrestore(&digctl_spinlock, flags);
	
	*pmem = DIGCTL_BASE;
	*psize = DIGCTL_SIZE;
	
	return 0;
}


int digctl_access_release(unsigned long *pmem, unsigned long *psize)
{

	if ((*pmem != DIGCTL_BASE) ||
	    (*psize != DIGCTL_SIZE )) {
		return -EINVAL;
	}
	
	*pmem = 0;
	*psize = 0;

	digctl_locked = 0;

	return 0;
}

int digctl_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long off = vma->vm_pgoff << PAGE_SHIFT;

	if (((file->f_flags & O_WRONLY) != 0) ||
	    ((file->f_flags & O_RDWR) != 0)) 	{
		vma->vm_page_prot = (pgprot_t)PAGE_SHARED;
	} else {
		vma->vm_page_prot = (pgprot_t)PAGE_READONLY;
	}
	
	/* Do not cache DIGCTL memory if O_SYNC flag is set */
	pgprot_val(vma->vm_page_prot) &= ~L_PTE_CACHEABLE;

	/* Don't try to swap out physical pages.. */
	vma->vm_flags |= VM_RESERVED | VM_LOCKED;
	
	off += DIGCTL_PHYS;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	
	if (remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
	    vma->vm_end - vma->vm_start, vma->vm_page_prot))
	{
		return -EAGAIN;
	}
	return 0;
}

static int digctl_open(struct inode * inode, struct file * filp) {
	return digctl_access_obtain(&digctl_base, &digctl_size);
}

static int digctl_release(struct inode * inode, struct file * filp) {
	return digctl_access_release(&digctl_base, &digctl_size);
}


static struct file_operations digctl_fops = {
	mmap:		digctl_mmap,
	open:		digctl_open,
	release:	digctl_release,
};

static struct miscdevice digctl_misc = {
	minor : DIGCTL_MINOR,
	name  : "misc/digctl",
	fops  : &digctl_fops,
};


/*
 * Module housekeeping.
 */
static int __init digctl_init(void)
{
	//printk("[DRIVER] DIGCTL device driver\n"); 

	if (misc_register(&digctl_misc) != 0)
	{
		printk(KERN_ERR "Cannot register device /dev/%s\n",
		       digctl_misc.name);
		return -EFAULT;
	}
	
	return 0;
}


static void __exit digctl_cleanup(void)
{
	misc_deregister(&digctl_misc);
}



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heechul Yun <heechul.yun@samsung.com>");

module_init(digctl_init);
module_exit(digctl_cleanup);
