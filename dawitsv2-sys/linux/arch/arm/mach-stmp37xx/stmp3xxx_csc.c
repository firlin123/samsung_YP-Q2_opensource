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

#include "stmp3xxx_csc.h"

#define WIDTH		320
#define HEIGHT		240

#define CSCMEMMAX			4
#define CSCMEMSIZE			0x26000 //(WIDTH*HEIGHT*2)


static void *csc_memory[CSCMEMMAX];
static dma_addr_t csc_phys[CSCMEMMAX];


/* Driver functons */

/* Internal functions */

/* The driver structure */
static CSC_Dev csc_dev;

static unsigned long get_physical_address(void* virtual_address)
{
    unsigned long offset_vma;
    unsigned long physical_address;
    struct vm_area_struct* vma;

	vma = find_vma(current->mm, (unsigned long)virtual_address);
	offset_vma = (unsigned long)virtual_address - vma->vm_start;
    physical_address = offset_vma + vma->vm_pgoff * PAGE_SIZE;
	return physical_address;
}

static int convert (convertor_data_t* convertor_data)
{
    unsigned vertical_offset = 0;
    unsigned horizontal_offset = 0;

	u32 upsample	 = 1;
	u32 inputformat  = 0;							// YCbCr 420 input data
	u32 rotate		 = 1;							// Don't rotate image
	u32 scale		 = 1;							// no scaling
	u32 outputmode	 = 0;							// output format is 24-big RGB
	u32 width = convertor_data->memory_stride;
	u32 height = convertor_data->height;

    BUG_ON(csc_dev.s_doing_csc);
/*
    if ((!access_ok(VERIFY_READ, convertor_data->y_buffer, convertor_data->y_buffer_size)) || 
          (!access_ok(VERIFY_READ, convertor_data->u_buffer, convertor_data->u_buffer_size)) ||
          (!access_ok(VERIFY_READ, convertor_data->v_buffer, convertor_data->v_buffer_size)) ||
          (!access_ok(VERIFY_WRITE, convertor_data->rgb_buffer, convertor_data->rgb_buffer_size)))
    {
        printk("stmp36xxfb: Invalid yuv data\n");;
        return -EINVAL;
    }
 */  
#if 0
    /* Clean cache on input buffers */
    arm926_dma_clean_range((unsigned long)convertor_data->y_buffer, 
			   (unsigned long)(convertor_data->y_buffer + convertor_data->y_buffer_size));
    arm926_dma_clean_range((unsigned long)convertor_data->u_buffer, 
			   (unsigned long)(convertor_data->u_buffer + convertor_data->u_buffer_size));
    arm926_dma_clean_range((unsigned long)convertor_data->v_buffer, 
			   (unsigned long)(convertor_data->v_buffer + convertor_data->v_buffer_size));
#endif

    /* Calculate offsets - should divide by 2 but RGB buffer is 16 bit */
    if (HEIGHT >= convertor_data->height)
    {
        vertical_offset = (HEIGHT - convertor_data->height);
    }
    
    if (WIDTH >= convertor_data->memory_stride)
    {
        horizontal_offset = (WIDTH - convertor_data->memory_stride);
    }
/*
	printk("%p %p %p %p, %d %d, %d, %d %d\n",
			convertor_data->y_buffer,convertor_data->u_buffer,convertor_data->v_buffer,convertor_data->rgb_buffer,
			convertor_data->width ,convertor_data->height, convertor_data->memory_stride, vertical_offset, horizontal_offset);
*/

/* added by jinho.lim : to support HW csc even not 320x240 resolution */
#if 1
	/* if 320*240 : no scale */
	if(height == HEIGHT && width == WIDTH)
	{
		//printk(" 320x240 : %d, %d, %d %d\n", width, height, vertical_offset, horizontal_offset);
		scale = 0;
		/* Take out of reset */
		HW_DCP_CTRL_CLR(BM_DCP_CTRL_SFTRST | BM_DCP_CTRL_CLKGATE);

		HW_DCP_CSCOUTBUFPARAM_WR((HEIGHT<<12) | WIDTH);
		HW_DCP_CSCINBUFPARAM_WR(WIDTH);
		
		HW_DCP_CSCRGB_WR(get_physical_address(convertor_data->rgb_buffer));
		HW_DCP_CSCLUMA_WR(get_physical_address(convertor_data->y_buffer));
		HW_DCP_CSCCHROMAU_WR(get_physical_address(convertor_data->u_buffer));
		HW_DCP_CSCCHROMAV_WR(get_physical_address(convertor_data->v_buffer));
		
		HW_DCP_CSCCTRL0_WR ((upsample<<14) | (scale<<13) | (rotate<<12) | (outputmode<<8) | (inputformat<<4) | 1);
	}
	else
	{
		//printk(" not 320x240 : %d, %d, %d %d\n", width, height, vertical_offset, horizontal_offset);
		scale = 1;
		/* Take out of reset */
		HW_DCP_CTRL_CLR(BM_DCP_CTRL_SFTRST | BM_DCP_CTRL_CLKGATE);
		
		HW_DCP_CSCOUTBUFPARAM_WR((height<<12) | width);
		HW_DCP_CSCINBUFPARAM_WR(width);
		
		BW_DCP_CSCXSCALE_INT(1);
		BW_DCP_CSCXSCALE_FRAC(0);
		BW_DCP_CSCXSCALE_WIDTH(width);
		BW_DCP_CSCYSCALE_INT(1);
		BW_DCP_CSCYSCALE_FRAC(0);
		BW_DCP_CSCYSCALE_HEIGHT(HEIGHT);

		//HW_DCP_CSCRGB_WR(get_physical_address(convertor_data->rgb_buffer));	
		//HW_DCP_CSCRGB_WR(get_physical_address(convertor_data->rgb_buffer) + vertical_offset);
		HW_DCP_CSCRGB_WR(get_physical_address(convertor_data->rgb_buffer) + (vertical_offset + horizontal_offset*HEIGHT));
		HW_DCP_CSCLUMA_WR(get_physical_address(convertor_data->y_buffer));
		HW_DCP_CSCCHROMAU_WR(get_physical_address(convertor_data->u_buffer));
		HW_DCP_CSCCHROMAV_WR(get_physical_address(convertor_data->v_buffer));
		
		HW_DCP_CSCCTRL0_WR ((upsample<<14) | (scale<<13) | (rotate<<12) | (outputmode<<8) | (inputformat<<4) | 1);
	}
#else
	/* Set up pointers */ 
	HW_DCP_CSCLUMA_WR(get_physical_address(convertor_data->y_buffer));
	HW_DCP_CSCCHROMAU_WR(get_physical_address(convertor_data->u_buffer));
	HW_DCP_CSCCHROMAV_WR(get_physical_address(convertor_data->v_buffer));
	HW_DCP_CSCINBUFPARAM_WR(convertor_data->memory_stride);

	HW_DCP_CSCRGB_WR(get_physical_address(convertor_data->rgb_buffer) + 
								vertical_offset + horizontal_offset*WIDTH);
	HW_DCP_CSCOUTBUFPARAM_WR((240<<12) | WIDTH);
#endif

/*
    printk("%x %x, %x %x, %x %x %x %x, %x %x %x\n", HW_DCP_CSCCTRL0_RD(), HW_DCP_CSCSTAT_RD(), HW_DCP_CSCOUTBUFPARAM_RD(),
            HW_DCP_CSCINBUFPARAM_RD(), HW_DCP_CSCRGB_RD(), HW_DCP_CSCLUMA_RD(), HW_DCP_CSCCHROMAU_RD(), HW_DCP_CSCCHROMAV_RD(),
            HW_DCP_CSCCOEFF0_RD(), HW_DCP_CSCCOEFF1_RD(), HW_DCP_CSCCOEFF2_RD());
*/
	/* Do conversion and wait until it's done */
    HW_DCP_CSCCTRL0_SET(1);
    //while (!HW_DCP_CSCSTAT.B.COMPLETE);
    csc_dev.s_doing_csc = 1;
    wait_event_interruptible(csc_dev.wait_q, csc_dev.s_doing_csc == 0);
#if 0
    /* Invalidate cache on output buffers */
    dmac_inv_range((unsigned long)convertor_data->rgb_buffer,
				   (unsigned long)(convertor_data->rgb_buffer + convertor_data->rgb_buffer_size));
#endif
	return 0;
}

static irqreturn_t device_csc_handler (int irq_num, void* dev_idp)
{
    BUG_ON(HW_DCP_CSCSTAT.B.COMPLETE == 0);

	HW_DCP_STAT_CLR(BM_DCP_STAT_CSCIRQ);
    
    /* Wake up calling task */
    csc_dev.s_doing_csc = 0;
    wake_up_interruptible(&csc_dev.wait_q);
    
    return IRQ_HANDLED;
}

static int convert_init (void)
{
    u32 upsample     = 1;
    u32 inputformat  = 0;                           // YCbCr 420 input data
    u32 rotate       = 1;//0;                           // Don't rotate image
    u32 scale        = 0;                           // no scaling
    u32 outputmode   = 0;                           // output format is 24-big RGB
    u32 width        = WIDTH;                   // original image width
    u32 height       = HEIGHT;                  // original image height
    
    /* Take out of reset */
    HW_DCP_CTRL_CLR(BM_DCP_CTRL_SFTRST | BM_DCP_CTRL_CLKGATE);

    // The output buffer is defined by the image size.
    HW_DCP_CSCOUTBUFPARAM_WR ((height<<12) | width);
    
    // The input buffer width is determined by the original image. The height is determined by
    // how many lines of video we want processed into the output buffer. Using a stride of 1.5
    //HW_DCP_CSCINBUFPARAM_WR((width + width  / 2));
    
    // Constants for Colour-Space Conversion
    BW_DCP_CSCCOEFF0_Y_OFFSET(0);
    BW_DCP_CSCCOEFF0_C0(0x100);
    BW_DCP_CSCCOEFF1_C1(0x123);
    BW_DCP_CSCCOEFF2_C2(0x065);
    BW_DCP_CSCCOEFF2_C3(0x094);
    BW_DCP_CSCCOEFF1_C4(0x208);

    // Set parameters 
#if 1
    HW_DCP_CSCCTRL0_WR ((upsample<<14) | (scale<<13) | (rotate<<12) | (outputmode<<8) | (inputformat<<4));
#else
	HW_DCP_CSCCTRL0_WR ((upsample<<14) | (1<<13) | (rotate<<12) | (outputmode<<8) | (inputformat<<4));
	BW_DCP_CSCXSCALE_INT(2);
	BW_DCP_CSCXSCALE_FRAC(0);
	BW_DCP_CSCXSCALE_WIDTH(320/2);
	BW_DCP_CSCYSCALE_INT(2);
	BW_DCP_CSCYSCALE_FRAC(0);
	BW_DCP_CSCYSCALE_HEIGHT(240/2);
#endif

    /* register irq handler with kernel */
    if (request_irq(IRQ_DCP, device_csc_handler, 0, DRIVER_NAME, NULL) != 0)
    {
        printk(DRIVER_NAME ": Could not register CSC IRQ\n");
		return -1;
    }
    
    HW_DCP_CTRL_SET(BM_DCP_CTRL_CSC_INTERRUPT_ENABLE);
	return 0;
}

static int device_ioctl (struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -ENOTTY;

	convertor_data_t convertor_data;

	switch (cmd)
	{
		case STMP3XXX_CSC_IOCS_CONVERT:
			if (copy_from_user(&convertor_data, (void*)arg, sizeof (convertor_data)))
			{
				printk(DRIVER_NAME ": CSC could not copy_from_user\n");
				return -EINVAL;
			}
			ret = convert(&convertor_data);
			break;
	}

    return ret;
}

static int device_mmap (struct file *file, struct vm_area_struct *vma)
{
	unsigned long off;
	unsigned long start;
	int which = (vma->vm_pgoff<<PAGE_SHIFT)/CSCMEMSIZE;

//	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
//		return -EINVAL;


	off = 0;

//	lock_kernel();
	/* yuv buffer memory */
	start = csc_phys[which];
	
//	unlock_kernel();
	start &= PAGE_MASK;

	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_RESERVED;
	
	if (remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
	    vma->vm_end - vma->vm_start, vma->vm_page_prot))
	{
		return -EAGAIN;
	}
	return 0;
}

static int device_open (struct inode *inodep, struct file *filp)
{
    int ret = 0;

    PDEBUG("CSC open:\n");

	spin_lock(&csc_dev.sp_lock);
	/* We're a read user -- only 1 reader allowed */
	if (csc_dev.num_open > 0)
	{
		PDEBUG(" failed. Already open\n");
		ret = -EBUSY;
	}
	else
	{
		csc_dev.num_open++;
		ret = convert_init();
	}
	spin_unlock(&csc_dev.sp_lock);

	return ret;
}

static int device_release (struct inode *inodep, struct file *filp)
{
    PDEBUG("device_release:\n");

    spin_lock(&csc_dev.sp_lock);

    csc_dev.num_open--;
	HW_DCP_CTRL_SET(BM_DCP_CTRL_SFTRST | BM_DCP_CTRL_CLKGATE);
	free_irq(IRQ_DCP, NULL);

    spin_unlock(&csc_dev.sp_lock);

	return 0;
}

/* Driver functions init */
struct file_operations csc_fops =
{
	.owner = THIS_MODULE,
	.ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,
	.mmap = device_mmap
};

static int __init device_init (void)
{
	int i;
    int ret = 0;

    PDEBUG( KERN_DEBUG DRIVER_DESC "\n");
    PDEBUG( KERN_DEBUG DRIVER_AUTH "\n");
    PDEBUG(" device_init\n");


	for (i = 0; i < CSCMEMMAX; i++) {
		csc_memory[i] = dma_alloc_writecombine(NULL, CSCMEMSIZE, &csc_phys[i], GFP_KERNEL);
		if (!csc_memory[i])
		{
			printk(DRIVER_NAME ": could not alloc memory\n");
			return -ENOMEM;
		}
	}

    /* Init device structure */
    csc_dev.num_open = 0;
	csc_dev.s_doing_csc	= 0;

	spin_lock_init(&csc_dev.sp_lock);
    init_waitqueue_head(&csc_dev.wait_q);

	ret = register_chrdev(CSC_DRIVER_MAJOR, DRIVER_NAME, &csc_fops);

	return ret;
}

static void __exit device_exit (void)
{
    // module is being removed, so cleanup
    PDEBUG(" device_exit()\n");

	unregister_chrdev(CSC_DRIVER_MAJOR, DRIVER_NAME);
}

module_init(device_init);
module_exit(device_exit);

MODULE_AUTHOR(DRIVER_AUTH);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

