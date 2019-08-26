/*
* Copyright (C) 2007 SigmaTel, Inc., Shaun Myhill <smyhill@sigmatel.com>
*
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

////////////////////////////////////////////////////////////////////////////////
//   Includes and external references
////////////////////////////////////////////////////////////////////////////////

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/irq.h>

#include <linux/poll.h>
#include <asm/cacheflush.h>
#include <asm/arch/dma.h>
#include <asm/arch/irqs.h>
#include <linux/dma-mapping.h>

#include <asm/arch-stmp37xx/ocram.h>

#ifndef OCRAM_MAJOR
#define OCRAM_MAJOR 250 
#endif

#define DRIVER_NAME "ocram"
#define DRIVER_DESC "SigmaTel ocram driver"
#define DRIVER_AUTH "Copyright (C) 2008 SigmaTel, Inc."

/* Driver functons */

/* Internal functions */

/* The driver structure */

struct SharedMemory {
	unsigned long pVirtUser;
	unsigned long pVirt;
	unsigned long pPhys;
	unsigned long len;
};

static struct SharedMemory mem_shared[2];

void *get_shared_virt (char __user *buf)
{
	int i; 
	unsigned long user = (unsigned long)buf;

	for (i = 0; i < ARRAY_SIZE(mem_shared); i++) {
		if (user >= mem_shared[i].pVirtUser && 
			user <  mem_shared[i].pVirtUser + mem_shared[i].len) {
			return (void*)(mem_shared[i].pVirt + user-mem_shared[i].pVirtUser);
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_shared_virt);

int ocram_mmap (struct file *file, struct vm_area_struct *vma)
{
	static unsigned long* pCodeVirt = (unsigned long*)NULL;
	static unsigned long pCodePhys = 0;

	unsigned long off;
	unsigned long start;

	/* physical sram audio start pointer start = 0x0000F000 */
	start = OCRAM_CODEC_START & PAGE_MASK;
	off = (vma->vm_pgoff << PAGE_SHIFT) + start;

	vma->vm_pgoff = off >> PAGE_SHIFT;
	/* Don't try to swap out physical pages.. */
	vma->vm_flags |= VM_RESERVED | VM_LOCKED;

	/* SDRAM mapping for WMA code */
	if (vma->vm_start == WMA_INIT_CODE_VIRT) {
		if (!pCodeVirt) {
			pCodeVirt = (unsigned long*)kmalloc(SDRAM_MAX_ALLOC_SIZE, GFP_KERNEL);
			pCodePhys = virt_to_phys(pCodeVirt);
		}
		off = pCodePhys;
	}
	/* OCRAM shared mem mapping for MTP */
	else if (vma->vm_start >= SHARED_DATA_VIRT && vma->vm_start <= (SHARED_DATA_VIRT + SHARED_DATA_SIZE)) {
		/* disable cache for shared mem */
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		/* */
		if (off == 0) {
			if (!mem_shared[0].pVirt) {
				mem_shared[0].pVirtUser = vma->vm_start;
				mem_shared[0].len = vma->vm_end - vma->vm_start;
				mem_shared[0].pVirt = (unsigned long)OCRAM_CODEC_START_VIRT;
				mem_shared[0].pPhys = (unsigned long)OCRAM_CODEC_START;
			}
			off = mem_shared[0].pPhys;
		}
		else {
			if (!mem_shared[1].pVirt) {
				mem_shared[1].pVirtUser = vma->vm_start;
				mem_shared[1].len = vma->vm_end - vma->vm_start;
				mem_shared[1].pVirt = (unsigned long)OCRAM_CODEC_START_VIRT + off;
				mem_shared[1].pPhys = (unsigned long)OCRAM_CODEC_START + off;
			}
			off = mem_shared[1].pPhys;
		}
	}
	/* SDRAM/OCRAM mapping for other codec (mp3, wma, ogg) */
	else {
		/* ogg code section will be loaded SDRAM */
		if (vma->vm_end - vma->vm_start == OGG_CODE_SIZE) {
			if (!pCodeVirt) {
				pCodeVirt = (unsigned long*)kmalloc(SDRAM_MAX_ALLOC_SIZE, GFP_KERNEL);
				pCodePhys = virt_to_phys(pCodeVirt);
			}
			off = pCodePhys;
		}
		else {
			/* for codec(mp3, wma, ogg) run code & data section */
		}
	}

	if (remap_pfn_range(vma, vma->vm_start, off>>PAGE_SHIFT,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

static int ocram_open (struct inode *inode, struct file *fd)
{
	return 0;
}

static int ocram_release (struct inode *inode, struct file *fd)
{
	int i; 

	/* free OCRAM shared memory for MTP */
	for (i = 0; i < ARRAY_SIZE(mem_shared); i++) {
		if (mem_shared[i].pVirt) {
			memset(&mem_shared[i], 0, sizeof(mem_shared[i]));
		}
	}
	return 0;
}

/* Driver functions init */
static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open =	ocram_open,
	.release = ocram_release,
	.mmap = ocram_mmap
};

static int __init device_init (void)
{
	int ret = 0;
	
	ret = register_chrdev(OCRAM_MAJOR, DRIVER_NAME, &fops);
	return ret;
}

static void __exit device_exit (void)
{
	unregister_chrdev(OCRAM_MAJOR, DRIVER_NAME);
}

module_init(device_init);
module_exit(device_exit);

MODULE_AUTHOR(DRIVER_AUTH);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

