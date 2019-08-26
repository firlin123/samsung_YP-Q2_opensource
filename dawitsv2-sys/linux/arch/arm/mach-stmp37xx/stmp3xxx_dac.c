/*
* Copyright (C) 2006 SigmaTel, Inc., David Weber <dweber@sigmatel.com>
*                                    Shaun Myhill <smyhill@sigmatel.com>
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

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

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
#include <linux/dma-mapping.h>
#include <linux/poll.h>
#include <linux/delay.h>

#include <asm/div64.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/arch/irqs.h>
#include <asm/arch/stmp3xxx_dac_ioctl.h>
#include <asm/arch/dma.h>

#define DAC_DRIVER_MAJOR	240

#ifndef bool
#define bool int
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#define TRACE_DAC  0

#undef PDEBUG             /* undef it, just in case */
#if TRACE_DAC
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "stmp3xxx_dac: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */


//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

#define DRIVER_NAME          "stmp3xxx_dac"
#define DRIVER_DESC          "SigmaTel 3xxx DAC driver"
#define DRIVER_AUTH          "Copyright (C) 2006 SigmaTel, Inc., David Weber"
#define BUS_ID               "dac0"
#define DMA_NUM_SAMPLES      800
#define CHAIN_LENGTH         32
#define DAC_DMA_CHANNEL      1
#define DAC_MIN_VOLUME       0x37
#define DAC_MAX_VOLUME       0xFE
#define HP_MAX_VOLUME        0x7F
#define HP_MIN_VOLUME        0
#define LO_MAX_VOLUME        0x1F
#define LO_MIN_VOLUME        0
#define DEFAULT_SAMPLE_RATE  44100
#define DEFAULT_SAMPLE_WIDTH 16

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------
typedef struct inode inode_t;
typedef struct file file_t;
typedef struct semaphore semaphore_t;
typedef struct cdev cdev_t;
typedef struct device device_t;
typedef struct file_operations driver_funcs_t;
typedef struct pt_regs pt_regs_t;
typedef struct poll_table_struct poll_table_t;
typedef uint16_t dma_buf_t[DMA_NUM_SAMPLES];

typedef struct
{
    reg32_t next;
    hw_apbx_chn_cmd_t cmd;
    reg32_t buf_ptr;
} dma_descriptor_t;

typedef struct
{
    bool inuse;
    dma_descriptor_t dd;     // dma descriptor
    dma_buf_t data;
} dma_block_t;

typedef struct
{
    dma_block_t* rd_ptr;
    dma_block_t* wr_ptr;
    dma_block_t* first_ptr;
    dma_block_t* last_ptr;
    uint16_t inuse_count;
    dma_block_t block[CHAIN_LENGTH];
} dma_chain_t;

typedef struct
{
    bool configured;
    uint8_t num_writers;
    uint32_t irq_dma_count;
    uint32_t irq_err_count;
    wait_queue_head_t wait_q;
    cdev_t cdev;
    dev_t dev;
    device_t* device;
//    unsigned int irq_dma;
//    unsigned int irq_err;
    spinlock_t lock;
    spinlock_t irq_lock;
    dma_addr_t real_addr;
    void* virt_addr;
    dma_chain_t* dma_chain;
    bool dac_running;
    unsigned long dac_sample_rate;
    unsigned long dac_sample_width;
    bool hp_on;
} dev_config_t;

typedef struct {
    unsigned status;
    unsigned samples_sent;
} dac_status_t;

enum DAC_STATUS_BITS {
    DAC_STATUS_UNDERRUN 	= 0x00000001,
    DAC_STATUS_DRAINED  	= 0x00000002,
    DAC_STATUS_SAMPLES_SENT 	= 0x00000004
};

#define MAKE_VOL_RANGE(max_vol, min_vol) ((max_vol << 16) | (min_vol & 0xFFFF))
#define GET_VOL_CTL_ID(volume_and_control) (volume_and_control >> 24)
#define GET_VOLUME(volume_and_control) (volume_and_control & 0x00FFFFFF)

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
static void __exit device_exit(void);
static int __init device_init(void);
static ssize_t device_read (file_t *filp, char __user *user_bufp, size_t size,
                            loff_t *offp);
static ssize_t device_write (file_t *filp, const char __user *user_bufp,
                             size_t size, loff_t *offp);
static int device_open (inode_t *inodep, file_t *filp);
static unsigned int device_poll(file_t *filp, poll_table_t *pollp);
static int device_ioctl (inode_t *inodep, file_t *filp, unsigned int cmd,
                         unsigned long arg);
static int device_close (inode_t *inodep, file_t *filp);
static int stmp3xxxdac_create(driver_funcs_t* funcs);
//static irqreturn_t device_dma_handler(int irq_num, void* dev_idp);
static void device_dma_handler(int id, unsigned int status, 
		void* dev_id);

//static irqreturn_t device_err_handler(int irq_num, void* dev_idp);
static void release(device_t* dev);
static void dma_chain_init(void);
static void dma_enable(void);
static void dac_init(unsigned long sample_rate, unsigned long bit_width_is_32);
static void get_dma_bytes_inuse(unsigned long *ret_total_left, unsigned long *ret_after_next_interrupt);
static void dac_send(uint16_t* data, size_t size);
static void dac_start(void);
static void dac_stop(void);
//static void dump_apbx_reg(void);
//static void dump_dac_reg(void);
static void dac_select_src(HP_SOURCE hp_source);
static void calculate_samplerate_params(unsigned long rate, unsigned long *ret_basemult, unsigned long *ret_src_hold, unsigned long *ret_src_int, unsigned long *ret_src_frac);
static int dac_sample_rate_set(unsigned long rate);
static void dac_ramp_to_vag(void);
static void dac_ramp_to_ground(void);
static void dac_vol_init(void);
static void dac_to_zero(void);
static void dac_trace_regs(void);
static void mute(VOL_CTL_ID vol_ctl_id);
static void unmute(VOL_CTL_ID vol_ctl_id);
static void volume_set(VOL_CTL_ID vol_ctl_id, uint8_t new_vol);
static void dac_power_lineout(bool up);
static int device_mmap (struct file *file, struct vm_area_struct *vma);

//------------------------------------------------------------------------------
// Static variables
//------------------------------------------------------------------------------
static driver_funcs_t driver_funcs =
{
    .owner   = THIS_MODULE,
    .read    = device_read,
    .write   = device_write,
    .ioctl   = device_ioctl,
    .open    = device_open,
	.mmap	 = device_mmap,
    .poll    = device_poll,
    .release = device_close
};

static device_t device =
{
    .parent  = NULL,
    .bus_id  = BUS_ID,
    .release = release,
    .coherent_dma_mask = ISA_DMA_THRESHOLD,
};

static dev_config_t dev_cfg =
{
    .configured       = false,
    .num_writers      = 0,
//    .irq_dma          = IRQ_DAC_DMA,
//    .irq_err          = IRQ_DAC_ERROR,
    .irq_dma_count    = 0,
    .irq_err_count    = 0,
    .device           = &device,
    .dac_running      = false,
    .dac_sample_rate  = DEFAULT_SAMPLE_RATE,
    .dac_sample_width = DEFAULT_SAMPLE_WIDTH,
    .hp_on	      = false
};

static bool expect_underrun = false;
static dac_status_t dac_status;

static unsigned session_activatedbytes = 0;
static unsigned interrupt_sentbytes = 0;
static unsigned session_sentbytes = 0;

//------------------------------------------------------------------------------
// Device functions called directly by kernel
//------------------------------------------------------------------------------

static int device_open (inode_t *inodep, file_t *filp)
{    
    int ret = 0;

    PDEBUG(" device_open()\n");

    if (1)//((filp->f_flags & O_RDWR) == O_RDWR)
    {
        //only 1 writer allowed
        if (dev_cfg.num_writers > 0)
            return -EBUSY;
        else
            dev_cfg.num_writers++;
    }
    else
    {
        ret = -EACCES;
    }

    return ret;
}

//------------------------------------------------------------------------------

static unsigned int device_poll(file_t *filp, poll_table_t *pollp)
{
    int ret = 0;
    PDEBUG(" device_poll()\n");

    poll_wait(filp, &dev_cfg.wait_q, pollp);

    spin_lock_irq(&dev_cfg.irq_lock);
    // Check if there's space in the DMA chain
    if (dev_cfg.dma_chain->inuse_count < CHAIN_LENGTH)
	ret |= (POLLOUT | POLLWRNORM);

    // Has our status changed?   
    if (dac_status.status)
	ret |= (POLLIN | POLLRDNORM);

    spin_unlock_irq(&dev_cfg.irq_lock);

    return ret;
}

//------------------------------------------------------------------------------

static ssize_t device_read (file_t *filp, char __user *user_bufp, size_t size,
                            loff_t *offp)
{
    int ret;
    
    PDEBUG(" device_read()\n");

    if (filp->f_flags & O_NONBLOCK)
    {
	// Non-blocking mode: exit if no status bits are set
	if (!dac_status.status)
	    return -EAGAIN;
    }

    // Otherwise... wait for a status bit change. Obviously.
    ret = wait_event_interruptible(dev_cfg.wait_q,
				   dac_status.status);

    // exit if signalled
    if (ret != 0)
	return ret;

    spin_lock_irq(&dev_cfg.irq_lock);

    if (size >= sizeof(dac_status))
	size = sizeof(dac_status);
    
    ret = copy_to_user(user_bufp, &dac_status, size);
    
    dac_status.status = 0;
    dac_status.samples_sent = 0;
    
    spin_unlock_irq(&dev_cfg.irq_lock);
       
    return size;
}

//------------------------------------------------------------------------------

static ssize_t device_write (file_t *filp, const char __user *user_bufp,
                             size_t size, loff_t *offp)
{
    int ret;
    PDEBUG(" device_write()\n");
    if (dev_cfg.configured == false)
    {
        // operation not permitted, because device is not fully configured
        return -EPERM;
    }

    PDEBUG(" device_write 1()\n");
    if (filp->f_flags & O_NONBLOCK)
    {
	// non-blocking mode: exit if the dma chain is full
	if (dev_cfg.dma_chain->inuse_count == CHAIN_LENGTH)
	    return -EAGAIN;
    }

    PDEBUG(" device_write 2()\n");
    // otherwise atomically wait until the dma chain is non-full
    ret = wait_event_interruptible(dev_cfg.wait_q,
				   dev_cfg.dma_chain->inuse_count < CHAIN_LENGTH);

    PDEBUG(" device_write() 3\n");
    // exit if signalled
    if (ret != 0)
	return ret;

    PDEBUG(" device_write() 4\n");
    // only allow writes that will fit in allocated buffer space
    if (size > sizeof(dma_buf_t))
    {
        size = sizeof(dma_buf_t);
    }

    PDEBUG(" device_write() 5\n");
    // copy the data to dma buffer
    ret = copy_from_user(dev_cfg.dma_chain->wr_ptr->data, user_bufp, size);

    PDEBUG(" device_write() 6\n");
    dac_send(dev_cfg.dma_chain->wr_ptr->data, size);

    return size;
}

//------------------------------------------------------------------------------

static void dac_send(uint16_t* data, size_t size)
{
    hw_apbx_chn_cmd_t dma_cmd;

    spin_lock(dev_cfg.lock);
        
    if (!dev_cfg.dac_running)
    {
        dac_start();
    }
    
    PDEBUG(" dac_send() - send from dma dd: %p : %d : %d\n",
           dev_cfg.dma_chain->wr_ptr,
           dev_cfg.dma_chain->inuse_count, size);

    // mark the dma block as in use, isr will make available after DAC dma
    // completes
    dev_cfg.dma_chain->wr_ptr->inuse = true;
    dev_cfg.dma_chain->inuse_count++;

    // set descriptor's data size
    dma_cmd.U = dev_cfg.dma_chain->wr_ptr->dd.cmd.U;
    dma_cmd.B.XFER_COUNT = size;
    dev_cfg.dma_chain->wr_ptr->dd.cmd = dma_cmd;

    // get setup for the next dma block, wrap write pointer if necessary
    if (dev_cfg.dma_chain->wr_ptr == dev_cfg.dma_chain->last_ptr)
    {
        dev_cfg.dma_chain->wr_ptr = dev_cfg.dma_chain->first_ptr;
    }
    else
    {
        dev_cfg.dma_chain->wr_ptr++;
    }

    // Renable the output
    //HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_DAC_ZERO_ENABLE);

    //kick dma (increment dma descriptor semaphore)
    BF_WRn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, INCREMENT_SEMA, 1);
    
    HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ);

    session_activatedbytes += size;

    spin_unlock(dev_cfg.lock);
}

//------------------------------------------------------------------------------

static int device_ioctl (inode_t *inodep, file_t *filp, unsigned int cmd,
                         unsigned long arg)
{
    int ret = 0;

    PDEBUG(" device_ioctl() - cmd: 0x%X, arg: 0x%X\n", cmd, (int)arg);

    switch(cmd)
    {
        case STMP3XXX_DAC_IOCT_MUTE:
            mute(arg);
            break;

        case STMP3XXX_DAC_IOCT_UNMUTE:
            unmute(arg);
            break;

        case STMP3XXX_DAC_IOCT_VOL_SET:
            volume_set(GET_VOL_CTL_ID(arg), GET_VOLUME(arg));
            break;

        case STMP3XXX_DAC_IOCT_HP_SOURCE:
            dac_select_src((HP_SOURCE)arg);
            break;

        case STMP3XXX_DAC_IOCT_SAMPLE_RATE:
            if (dac_sample_rate_set(arg) != 0)
            {
                ret = -EPERM;
            }
            break;


        case STMP3XXX_DAC_IOCQ_VOL_GET:
            switch (arg)
            {
                case VC_DAC:
                    ret = HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT;
                    break;

                case VC_HEADPHONE:
                    ret = -((int)HW_AUDIOOUT_HPVOL.B.VOL_LEFT - HP_MAX_VOLUME);
                    break;

                case VC_LINEOUT:
                    ret = -((int)HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT - LO_MAX_VOLUME);
                    break;

                default:
                    break;
            }
            break;

        case STMP3XXX_DAC_IOCQ_VOL_RANGE_GET:
            switch (arg)
            {
                case VC_DAC:
                    ret = MAKE_VOL_RANGE(DAC_MAX_VOLUME, DAC_MIN_VOLUME);
                    break;

                case VC_HEADPHONE:
                    ret = MAKE_VOL_RANGE(HP_MAX_VOLUME, HP_MIN_VOLUME);
                    break;

                case VC_LINEOUT:
                    ret = MAKE_VOL_RANGE(LO_MAX_VOLUME, LO_MIN_VOLUME);
                    break;

                default:
                    break;
            }
            break;

        case STMP3XXX_DAC_IOCQ_IS_MUTED:
            switch (arg)
            {
                case VC_DAC:
                    ret = HW_AUDIOOUT_DACVOLUME.B.MUTE_LEFT;
                    break;

                case VC_HEADPHONE:
                    ret = HW_AUDIOOUT_HPVOL.B.MUTE;
                    break;

                case VC_LINEOUT:
                    ret = HW_AUDIOOUT_LINEOUTCTRL.B.MUTE;
                    break;

                default:
                    break;
            }
            break;

        case STMP3XXX_DAC_IOCQ_HP_SOURCE_GET:
            ret = HW_AUDIOOUT_HPVOL.B.SELECT;
            break;

        case STMP3XXX_DAC_IOCQ_SAMPLE_RATE:
            ret = dev_cfg.dac_sample_rate;
            break;

	case STMP3XXX_DAC_IOCT_EXPECT_UNDERRUN:
	    spin_lock_irq(&dev_cfg.irq_lock);
	    expect_underrun = (bool)arg;
	    break;

	case STMP3XXX_DAC_IOCQ_ACTIVE:
	    spin_lock_irq(&dev_cfg.irq_lock);
	    ret = dev_cfg.dma_chain->inuse_count;
	    spin_unlock_irq(&dev_cfg.irq_lock);
	    break;
	    
	case STMP3XXX_DAC_IOCT_INIT:
	    if (!dev_cfg.dac_running)
	        dac_start();
	    break;

        case STMP3XXX_DAC_IOCS_QUEUE_STATE:
	{
	    stmp3xxx_dac_queue_state_t state;
	    state.total_bytes = sizeof(dma_buf_t) * CHAIN_LENGTH;
	    spin_lock_irq(&dev_cfg.irq_lock);
	    get_dma_bytes_inuse(&state.remaining_now_bytes,
				&state.remaining_next_bytes);
	    spin_unlock_irq(&dev_cfg.irq_lock);
	    ret = copy_to_user((void *) arg, &state, sizeof(state));
	    break;
	}   

        default:
            ret = -ENOTTY;
            break;
    }

    return ret;
}
 
static int device_mmap (struct file *file, struct vm_area_struct *vma)
{
	extern int ocram_mmap (struct file *file, struct vm_area_struct * vma);
	return ocram_mmap(file, vma);
}

//------------------------------------------------------------------------------

static int device_close (inode_t *inodep, file_t *filp)
{   
    int ret;
    PDEBUG(" device_close()\n");

    dev_cfg.num_writers = 0;
    
    // Wait for inuse_count to reach 0
    ret = wait_event_interruptible(dev_cfg.wait_q,
				   dev_cfg.dma_chain->inuse_count == 0);
    
    PDEBUG(" device_close() - dma chain has been drained\n");

    if(ret == 0)
    {
	// stop the dac
	dac_stop();
	return 0;
    }
    else
    {
	// got interrupted by a signal
	return ret;
    }
}

//------------------------------------------------------------------------------

static int __init device_init (void)
{
    int ret = 0;

    PDEBUG(DRIVER_DESC "\n");
    PDEBUG(DRIVER_AUTH "\n");
    PDEBUG(DRIVER_NAME " device_init() - installing module\n");

    ret = stmp3xxxdac_create(&driver_funcs);

    if (ret != 0)
    {
        PDEBUG(" device_init() - error: failed to create device\n");
        return ret;
    }

	// register irq handler with kernel
#if 0	
	ret = request_irq(dev_cfg.irq_dma, device_dma_handler, IRQF_DISABLED,
                      DRIVER_NAME, NULL);
#else
	ret = stmp37xx_request_dma(DMA_DAC, "stmp37xxdac", device_dma_handler, NULL);
#endif

  if (ret != 0)
    {
        PDEBUG(" device_init() - error: request_irq() returned error: %d\n", ret);
        return ret;
    }

#if 0
    // register irq handler with kernel
	ret = request_irq(dev_cfg.irq_dma, device_err_handler, 0, //IRQF_DISABLED,
                      DRIVER_NAME, NULL);
    if (ret != 0)
    {
        PDEBUG(" device_init() - error: request_irq() returned error: %d\n", ret);
        return ret;
    }
#endif
    // initialize dac
    dac_init(DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_WIDTH);

    // load dma chain into dac dma machine, enable dac dma interrupts
    dma_enable();

    // enable writes to succeed
    dev_cfg.configured = true;

    PDEBUG(" device_init() - module installed: major:%d, minor:%d\n",
           MAJOR(dev_cfg.dev), MINOR(dev_cfg.dev));

    return ret;
}

//------------------------------------------------------------------------------

static void __exit device_exit (void)
{
    PDEBUG(" device_exit() - removing module\n");

    // module is being removed, so cleanup
#if 0
	free_irq(dev_cfg.irq_dma, NULL);
	free_irq(dev_cfg.irq_err, NULL);
#else
	stmp37xx_free_dma(DMA_DAC);
#endif

#ifdef DAC_DRIVER_MAJOR
    unregister_chrdev(DAC_DRIVER_MAJOR, DRIVER_NAME);
#else
    unregister_chrdev_region(dev_cfg.dev, 1);
#endif
    device_unregister(dev_cfg.device);
}

//------------------------------------------------------------------------------

static void* map_phys_to_virt(unsigned real)
{
    return (void*)(real - dev_cfg.real_addr + (unsigned)dev_cfg.virt_addr);
}

//------------------------------------------------------------------------------

static void get_dma_bytes_inuse(unsigned long *ret_total_left, unsigned long *ret_after_next_interrupt)
{
    unsigned total_left;
    unsigned after_next_interrupt;
    unsigned loop_check;
    unsigned bytes;
    unsigned sem;
 
    bool had_interrupt_marker;

    dma_descriptor_t *first, *desc, *check_first;
    hw_apbh_chn_debug2_t debug;
 
    for (;;)
    {		
	total_left = 0;
	after_next_interrupt = 0;

	do
	{
	    first = (dma_descriptor_t *) map_phys_to_virt(HW_APBX_CHn_CURCMDAR_RD(DAC_DMA_CHANNEL));
	    sem = BF_RDn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, PHORE); 
	    
	}
	while (first != (dma_descriptor_t *)map_phys_to_virt(HW_APBX_CHn_CURCMDAR_RD(DAC_DMA_CHANNEL)));

	if (!first || sem == 0)
	{
	    // Empty chain 
	    break;
	}

	desc = first;
	had_interrupt_marker = false;
	
	loop_check = 10000;
	while (desc->cmd.B.CHAIN)
	{        
	    if (--loop_check == 0)
	    {
		break;
	    }
		
 	    if (desc->cmd.B.SEMAPHORE && --sem == 0)
 	    {
		break;
	    }
				
	    if (desc->cmd.B.IRQONCMPLT)
		had_interrupt_marker = true;

	    desc = (dma_descriptor_t*)map_phys_to_virt(desc->next);
	    	    
	    bytes = desc->cmd.B.XFER_COUNT;

	    if(had_interrupt_marker)
		after_next_interrupt += bytes;
		
	    total_left += bytes;
	}
	
	// Some amount of the current command will still be in transit
	debug = HW_APBH_CHn_DEBUG2(DAC_DMA_CHANNEL);
	total_left += debug.B.AHB_BYTES;

	check_first = (dma_descriptor_t *)map_phys_to_virt( HW_APBX_CHn_CURCMDAR_RD(DAC_DMA_CHANNEL));
	if(first == check_first)
	    break;
    }
    
    if(ret_total_left)
	*ret_total_left = total_left;
    if(ret_after_next_interrupt)
	*ret_after_next_interrupt = after_next_interrupt;
}

//------------------------------------------------------------------------------

//static irqreturn_t device_dma_handler(int irq_num, void* dev_idp)
static void device_dma_handler(int id, unsigned int status, 
		void* dev_id)
{
    const unsigned long sample_size = sizeof( short ); 
    const unsigned long channels_count = 2; /* stereo */
    
    unsigned long remaining_bytes, now_sentbytes, sent_bytes; 
    unsigned long samples_count;
    
    unsigned sema_dec;
    
    bool underrun = false;
 
    spin_lock_irq(&dev_cfg.irq_lock);
 
    get_dma_bytes_inuse( &remaining_bytes, NULL ); 
       
    now_sentbytes = session_activatedbytes - remaining_bytes;
    interrupt_sentbytes = now_sentbytes - session_sentbytes;
    session_sentbytes = now_sentbytes;
    
    sent_bytes = interrupt_sentbytes;
    samples_count = sent_bytes / ( sample_size * channels_count );

    dev_cfg.irq_dma_count++;
    
    // Check for underruns, handling the case where the flag state
    // changes during this call
    if (BF_RDn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, PHORE) == 0)
    {
	underrun = true;
    }
    else
    {
	if (HW_AUDIOOUT_CTRL.B.FIFO_UNDERFLOW_IRQ)
	{
	    underrun = true;
	    // Clear underrun flag
	    HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ);
	}
	else if (BF_RDn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, PHORE) == 0)
	    underrun = true;
    }

    // manage dma chain
    if (dev_cfg.dma_chain->inuse_count > 0)
    {
	sema_dec = dev_cfg.dma_chain->inuse_count - BF_RDn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, PHORE);
	dev_cfg.dma_chain->inuse_count -= sema_dec;
	
	while (sema_dec--)
	{
	    dev_cfg.dma_chain->rd_ptr->inuse = false;	
  
	    // get setup for the next dma block, wrap read pointer if necessary
	    if (dev_cfg.dma_chain->rd_ptr == dev_cfg.dma_chain->last_ptr)
	    {
		dev_cfg.dma_chain->rd_ptr = dev_cfg.dma_chain->first_ptr;
	    }
	    else
	    {
		// point at next dma block
		dev_cfg.dma_chain->rd_ptr++;
	    }
	}
	
	//ASSERT( ( sent_bytes % ( sample_size * channels_count ) ) == 0 ); // no remainder
	
	if (samples_count)
	{
	    dac_status.status |= DAC_STATUS_SAMPLES_SENT;
	    dac_status.samples_sent += samples_count;
	}
	
	if (!expect_underrun && underrun)
	{
            PDEBUG(" ********* UNDERRUN *********\n");
	    dac_status.status |= DAC_STATUS_UNDERRUN;
	}	
	else if (dev_cfg.dma_chain->inuse_count == 0)
	{   
	    if (expect_underrun)
	    {
                PDEBUG(" drained %d 0x%x\n", expect_underrun, HW_AUDIOOUT_CTRL.B.FIFO_UNDERFLOW_IRQ);
		dac_status.status |= DAC_STATUS_DRAINED;

		// Drained, so reset the byte counts
		session_activatedbytes = 0; session_sentbytes = 0; interrupt_sentbytes = 0;	

		dac_to_zero();

		expect_underrun = false;
	    }
	    else
                PDEBUG(" empty dma chain, but FIFO_UNDERFLOW_IRQ unset and not expecting underrun!\n");
	}

	// Push out zeros while the chain is empty
	//HW_AUDIOOUT_CTRL_SET(BM_AUDIOOUT_CTRL_DAC_ZERO_ENABLE);

	// wake up writing process, as dma block has become available
	wake_up_interruptible(&dev_cfg.wait_q);

    }
    else
    {
        PDEBUG(" device_dma_handler() - "
               "unexpected dma interrupt w/empty dma chain\n");
    }

    // clear interrupt status
    BF_CLR(APBX_CTRL1, CH1_CMDCMPLT_IRQ);
    //BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);

    if (HW_AUDIOOUT_CTRL.B.FIFO_OVERFLOW_IRQ)
	HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_FIFO_OVERFLOW_IRQ);
    
    spin_unlock_irq(&dev_cfg.irq_lock);
//    return IRQ_HANDLED;
}

//------------------------------------------------------------------------------

static irqreturn_t device_err_handler(int irq_num, void* dev_idp)
{
    spin_lock_irq(&dev_cfg.irq_lock);

    PDEBUG(" device_err_handler() - not implemented\n");

    dev_cfg.irq_err_count++;

    // clear interrupt status
    // TODO:

    spin_unlock_irq(&dev_cfg.irq_lock);

    return IRQ_HANDLED;
}

////////////////////////////////////////////////////////////////////////////////
// Device functions not called directly by kernel
////////////////////////////////////////////////////////////////////////////////
static int stmp3xxxdac_create(driver_funcs_t* funcs)
{
    int ret = 0;

    PDEBUG(" stmp3xxxdac_create()\n");

    // register the device, so we can get an initialized struct device * to
    // use with various system calls, specifically dma allocation functions
    ret = device_register(dev_cfg.device);

    if (ret != 0)
    {
        PDEBUG(" stmp3xxxdac_create() - unable to register driver\n");
        return ret;
    }

#ifdef DAC_DRIVER_MAJOR
    ret = register_chrdev(DAC_DRIVER_MAJOR, DRIVER_NAME, funcs);
#else
    // dynamically allocate driver major/minor numbers - the device file
    // must still be created (presumably by an init script)
    ret = alloc_chrdev_region(&dev_cfg.dev,  0, 1, DRIVER_NAME);
#endif

    if (ret != 0)
    {
        PDEBUG(" stmp3xxxdac_create() - unable to allocate device numbers\n");
        return ret;
    }

    // allocate some dma memory
    dev_cfg.virt_addr = dma_alloc_coherent(dev_cfg.device,
                                           sizeof(dma_block_t) * CHAIN_LENGTH,
                                           &(dev_cfg.real_addr),
                                           GFP_DMA);
    if (dev_cfg.virt_addr == NULL)
    {
        PDEBUG(" stmp3xxxdac_create() - unable to allocate dma memory\n");
        return -1;
    }

    PDEBUG(" stmp3xxxdac_create() - dma_alloc_coherent() - "
           "virt addr: %08X, real addr = %08X\n", (uint32_t)dev_cfg.virt_addr,
           (uint32_t)dev_cfg.real_addr);

    dma_chain_init();

    // initialize the wait queue so we can use to sleep on write to device
    // (when DAC is busy)
    init_waitqueue_head(&dev_cfg.wait_q);

    // register the device driver - associate driver funcs w/ device
    cdev_init(&dev_cfg.cdev, funcs);
    dev_cfg.cdev.ops = funcs;
    dev_cfg.cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev_cfg.cdev, dev_cfg.dev, 1);

    return ret;
}

//------------------------------------------------------------------------------

static void release(device_t* dev)
{
    PDEBUG(" release() - releasing device struct with bus_id: " BUS_ID "\n");

    dma_free_coherent(dev_cfg.device,
                      sizeof(dma_block_t) * CHAIN_LENGTH,
                      dev_cfg.virt_addr,
                      dev_cfg.real_addr);
}

//------------------------------------------------------------------------------

static void dma_chain_init(void)
{
    int i;

    // overlay the dma_chain structure on the dma memory (accessible via kernel
    // virtual address or real physical address)
    dma_chain_t* dma_chain = (dma_chain_t*)dev_cfg.real_addr;
    dev_cfg.dma_chain = (dma_chain_t*)dev_cfg.virt_addr;

    PDEBUG(" dma_chain_init()\n");

    // chain descriptors together
    for (i = 0; i < CHAIN_LENGTH; i++)
    {
        dev_cfg.dma_chain->block[i].inuse = false;

        // link current descriptor to next descriptor
        dev_cfg.dma_chain->block[i].dd.next = (reg32_t)&dma_chain->block[i + 1].dd;
        dev_cfg.dma_chain->block[i].dd.cmd = 
        (hw_apbx_chn_cmd_t)(
	    BF_APBX_CHn_CMD_XFER_COUNT(sizeof(dma_buf_t)) |
            BF_APBX_CHn_CMD_CHAIN(1) |
            BF_APBX_CHn_CMD_SEMAPHORE(1) |
            BF_APBX_CHn_CMD_IRQONCMPLT(1) |
            BV_FLD(APBX_CHn_CMD,COMMAND, DMA_READ)            
        );
        dev_cfg.dma_chain->block[i].dd.buf_ptr = (reg32_t)dma_chain->block[i].data;
    }
    // wrap last descriptor around to first
    dev_cfg.dma_chain->block[i - 1].dd.next = (reg32_t)&dma_chain->block[0].dd;

    dev_cfg.dma_chain->rd_ptr      = &dev_cfg.dma_chain->block[0];
    dev_cfg.dma_chain->wr_ptr      = &dev_cfg.dma_chain->block[0];
    dev_cfg.dma_chain->first_ptr   = &dev_cfg.dma_chain->block[0];
    dev_cfg.dma_chain->last_ptr    = &dev_cfg.dma_chain->block[CHAIN_LENGTH - 1];
    dev_cfg.dma_chain->inuse_count = 0;
}

//------------------------------------------------------------------------------

static void dma_enable(void)
{
    dma_chain_t* dma_chain = (dma_chain_t*)dev_cfg.real_addr;

    PDEBUG(" dma_enable()\n");

    BF_CLR(APBX_CTRL0, CLKGATE);
    BF_CLR(APBX_CTRL0, SFTRST);
    BF_WR(APBX_CTRL0, RESET_CHANNEL, (1 << DAC_DMA_CHANNEL));
    while (HW_APBX_CTRL0.B.RESET_CHANNEL & (1 << DAC_DMA_CHANNEL));

    // give APBX block the first DAC DMA descriptor
    BF_WRn(APBX_CHn_NXTCMDAR, DAC_DMA_CHANNEL, CMD_ADDR,
           (reg32_t)&dma_chain->block[0].dd);

    BF_CLR(APBX_CTRL1, CH1_CMDCMPLT_IRQ);
    BF_SET(APBX_CTRL1, CH1_CMDCMPLT_IRQ_EN);
}

//------------------------------------------------------------------------------

#define VDDD_BASE_MIN       1400 // minimum voltage requested (in mV)
#define VAG_BASE_VALUE_ADJ  ((((((VDDD_BASE_MIN*160)/(2*125)))-625)/25))   // Adjust for LW_REF (1.6/1.25)

static void dac_vol_init()
{
    hw_audioout_refctrl_t ref_ctrl;

    PDEBUG(" dac_vol_init\n");
    
    // Lower ADC and Vag reference voltage by ~22%
    BF_SET(AUDIOOUT_REFCTRL, LW_REF);
    // Pause 10 milliseconds
    msleep(10);

    // Force DAC to issue Quiet mode until we are ready to run.
    // And enable DAC circuitry
    BF_SET(AUDIOOUT_CTRL, DAC_ZERO_ENABLE);
    BF_CLR(AUDIOOUT_PWRDN, DAC);

    // Update DAC volume over zero-crossings
    // No! This increases power consumption by a LOT.
    //BF_SET(AUDIOOUT_DACVOLUME, EN_ZCD);
    //BF_SET(AUDIOOUT_ANACTRL,   EN_ZCD);

    // LCL:  Setup
    //
    // VBG_ADJ
    // DO NOT WRITE THIS VALUE UNLESS IT IS FOR THE LiIon BATTERY CHARGING CODE!!!
    //
    // ADJ_VAG
    // Set to one so that we can explicitly adjust the Vag (audio ground
    // voltage) by writing to the VAG_VAL field.
    //
    // VAG_VAL
    // Vdda is the Vdd for analog output. In the 3700, Vdda is directly
    // connected to Vddd, the Vdd for the digital section. So, if we lower
    // Vdd to reduce power consumption in the digital circuitry, we are also
    // lowering the high rail of the audio output.
    //
    // If we were using the entire voltage span from actual ground to Vdda,
    // then lowering Vddd would do two things: it would lower the volume,
    // and it would clip the highest voltages in the output signal. Since we
    // want power-conserving measures to have no effect on user experience,
    // we need to do something else.
    //
    // What we do here is to set Vag appropriately for the lowest possible
    // value of Vdda. That way, Vdda can fluctuate all it wants, and the
    // audio will remain undisturbed.
    //
    // The lowest voltage at which the digital hardware can function is
    // 1.35V. Vdda can never be lower. So, we can set Vag to
    //
    //     1.35V / 2  =  0.675V
    //
    // and be assured that the audio output will never be disturbed by
    // fluctuations in Vddd.
    //
    // VAG_VAL is a 4-bit field. A value of 0xf implies Vag = 1.00V. A value
    // of 0 implies a Vag = 0.625V. Each bit is a 25mV step.
    //
    //     0.675V - 0.625V  =  0.050V  =>  2 increments of 25mV each.
    //
    // NOTE: It is important to write the register in one step if LW_REF is
    // being set, because LW_REF will cause the VAG_VAL to be lowered by 1.25/1.6.
    //----------------------------------------------------------------------
    ref_ctrl.U = HW_AUDIOOUT_REFCTRL_RD();
    ref_ctrl.B.ADJ_VAG = 1;
    ref_ctrl.B.VAG_VAL = VAG_BASE_VALUE_ADJ;
    //ref_ctrl.B.VAG_VAL = 0;
    ref_ctrl.B.VBG_ADJ = 0x3;
    ref_ctrl.B.LW_REF = 1;
    HW_AUDIOOUT_REFCTRL_WR(ref_ctrl.U);

}

//------------------------------------------------------------------------------

// Bit-bang samples to the dac
static void dac_blat( unsigned short value, int num_samples )
{
    unsigned sample;
    
    //  ASSERT(HW_APBX_CHn_DEBUG1(1).B.STATEMACHINE == 0);

    sample = (unsigned)value;    // copy to other channel
    sample += (sample<<16);

    while ( num_samples > 0 )
    {
	if (HW_AUDIOOUT_DACDEBUG.B.FIFO_STATUS == 1)
	{
	    HW_AUDIOOUT_DATA_WR(sample);
	    num_samples--;
	}
    }
}

//------------------------------------------------------------------------------

static void dac_ramp_partial(short *u16DacValue, short target_value, short step)
{  
    if (step < 0)
    {
        while( *u16DacValue > target_value )
        {
                dac_blat( *u16DacValue, 1 );
                *u16DacValue += step;
        }    
    }
    else
    {
        while( *u16DacValue < target_value )
        {
                dac_blat( *u16DacValue, 1 );
                *u16DacValue += step;
        }    
    }
}

//------------------------------------------------------------------------------

static void dac_ramp_up(uint8_t newVagValue, uint8_t oldVagValue)
{   
    int16_t s16DacValue = 0x7FFF;       // set to lowest voltage    

    PDEBUG(" dac_ramp_up\n");

    dac_ramp_partial(&s16DacValue, 0x7000, -2);
    dac_ramp_partial(&s16DacValue, 0x6000, -3);
    dac_ramp_partial(&s16DacValue, 0x5000, -3);
    dac_ramp_partial(&s16DacValue, 0x4000, -4);

    // Slowly raise VAG back to set level
    while( newVagValue != oldVagValue )
    {
        if (newVagValue > oldVagValue)
        {
            BF_CLR(AUDIOOUT_REFCTRL,VAG_VAL);
            BF_SETV(AUDIOOUT_REFCTRL,VAG_VAL,--newVagValue);        
            dac_blat( s16DacValue, 512);
        }
        else
        {
            BF_CLR(AUDIOOUT_REFCTRL,VAG_VAL);
            BF_SETV(AUDIOOUT_REFCTRL,VAG_VAL,++newVagValue);
            dac_blat( s16DacValue, 512);
        }        
    }

    dac_ramp_partial(&s16DacValue, 0x3000, -4);
    dac_ramp_partial(&s16DacValue, 0x2000, -3);
    dac_ramp_partial(&s16DacValue, 0x1000, -2);
    dac_ramp_partial(&s16DacValue, 0x0000, -1);
}

//------------------------------------------------------------------------------

static void dac_ramp_to_vag(void)
{   
    int16_t     u16DacValue;
    uint8_t     oldVagValue;
    uint8_t     newVagValue;

    PDEBUG(" dac_ramp_to_vag\n");
    
    // Gather values from registers we are going to override
    oldVagValue = (uint8_t)BF_RD(AUDIOOUT_REFCTRL, VAG_VAL);
    
    // Setup Output for a clean signal
    BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, true);                // Prevent Digital Noise

    msleep(1);
    
    BF_CLR(AUDIOOUT_PWRDN,DAC);                                 // Power UP DAC   
    BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, false);                // Disable ClassAB    

    // Reduce VAG so that we can get closer to ground when starting 
    newVagValue = 0;
    BF_CLR(AUDIOOUT_REFCTRL,VAG_VAL);
    BF_SETV(AUDIOOUT_REFCTRL,VAG_VAL,newVagValue);    
    
    // Enabling DAC circuitry
    BF_CS2(AUDIOOUT_DACVOLUME, VOLUME_LEFT, DAC_MAX_VOLUME,     // Set max volume
                VOLUME_RIGHT, DAC_MAX_VOLUME);
    BF_CS2(AUDIOOUT_DACVOLUME,MUTE_LEFT,0,MUTE_RIGHT,0);        // Unmute DAC
    BF_CLR(AUDIOOUT_HPVOL,MUTE);                                // Unmute HP

    BF_CLR(AUDIOOUT_CTRL, DAC_ZERO_ENABLE);                     // Stop sending zeros

    BF_SET(AUDIOOUT_CTRL, RUN);                                 // Start DAC
    
    // Full deflection negative
    u16DacValue = 0x7fff;

    // Write DAC to ground for several milliseconds
    dac_blat( u16DacValue, 512 );

    // PowerUp Headphone, Unmute
    BF_CLR(AUDIOOUT_PWRDN,HEADPHONE);                           // Pwrup HP amp

    dac_blat( u16DacValue, 256 );

    BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, false);               // Release hold to GND
    dac_blat( u16DacValue, 256 );
    dac_ramp_up(newVagValue, oldVagValue);                      // Ramp to VAG

    // Need to wait here - block for the time being...       
   //msleep(1700);  
   BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, true);         // Enable ClassAB    
}

//------------------------------------------------------------------------------

static void dac_trace_regs()
{
    PDEBUG("HW_AUDIOOUT_CTRL     = 0x%08x\n", HW_AUDIOOUT_CTRL_RD()      );
    PDEBUG("HW_AUDIOOUT_STAT     = 0x%08x\n", HW_AUDIOOUT_STAT_RD()      );
    PDEBUG("HW_AUDIOOUT_DACSRR   = 0x%08x\n", HW_AUDIOOUT_DACSRR_RD()    );
    PDEBUG("HW_AUDIOOUT_DACVOLUME= 0x%08x\n", HW_AUDIOOUT_DACVOLUME_RD() );
    PDEBUG("HW_AUDIOOUT_DACDEBUG = 0x%08x\n", HW_AUDIOOUT_DACDEBUG_RD()  );
    PDEBUG("HW_AUDIOOUT_HPVOL    = 0x%08x\n", HW_AUDIOOUT_HPVOL_RD()     );
    PDEBUG("HW_AUDIOOUT_PWRDN    = 0x%08x\n", HW_AUDIOOUT_PWRDN_RD()     );
    PDEBUG("HW_AUDIOOUT_REFCTRL  = 0x%08x\n", HW_AUDIOOUT_REFCTRL_RD()   );
    PDEBUG("HW_AUDIOOUT_ANACTRL  = 0x%08x\n", HW_AUDIOOUT_ANACTRL_RD()   );
    PDEBUG("HW_AUDIOOUT_TEST     = 0x%08x\n", HW_AUDIOOUT_TEST_RD()      );
    PDEBUG("HW_AUDIOOUT_BISTCTRL = 0x%08x\n", HW_AUDIOOUT_BISTCTRL_RD()  );
    PDEBUG("HW_AUDIOOUT_BISTSTAT0= 0x%08x\n", HW_AUDIOOUT_BISTSTAT0_RD() );
    PDEBUG("HW_AUDIOOUT_BISTSTAT1= 0x%08x\n", HW_AUDIOOUT_BISTSTAT1_RD() );
    PDEBUG("HW_AUDIOOUT_ANACLKCTRL=0x%08x\n", HW_AUDIOOUT_ANACLKCTRL_RD());
    PDEBUG("\n");
    PDEBUG("\n");
    PDEBUG("HW_APBX_CTRL0 = 0x%08x\n", HW_APBX_CTRL0_RD() );
    PDEBUG("HW_APBX_CTRL1 = 0x%08x\n", HW_APBX_CTRL1_RD() );
    PDEBUG("HW_APBX_CH1_CURCMDAR = 0x%08x\n", HW_APBX_CHn_CURCMDAR_RD(1) );
    PDEBUG("HW_APBX_CH1_NXTCMDAR = 0x%08x\n", HW_APBX_CHn_NXTCMDAR_RD(1) );
    PDEBUG("HW_APBX_CH1_CMD      = 0x%08x\n", HW_APBX_CHn_CMD_RD(1) );
    PDEBUG("HW_APBX_CH1_BAR      = 0x%08x\n", HW_APBX_CHn_BAR_RD(1) );
    PDEBUG("HW_APBX_CH1_SEMA     = 0x%08x\n", HW_APBX_CHn_SEMA_RD(1) );
    PDEBUG("HW_APBX_CH1_DEBUG1   = 0x%08x\n", HW_APBX_CHn_DEBUG1_RD(1) );
    PDEBUG("HW_APBX_CH1_DEBUG2   = 0x%08x\n", HW_APBX_CHn_DEBUG2_RD(1) );
}

//------------------------------------------------------------------------------

static void dac_init( unsigned long sample_rate, unsigned long sample_width )
{
    PDEBUG(" dac_init()\n");

    // Keep audio input off
    HW_AUDIOOUT_CTRL.B.CLKGATE = false;
    HW_AUDIOIN_CTRL.B.CLKGATE = false;
    HW_AUDIOIN_CTRL.B.SFTRST = false;
    HW_AUDIOOUT_PWRDN.B.RIGHT_ADC = true;
    HW_AUDIOOUT_PWRDN.B.ADC = true;
    HW_AUDIOOUT_PWRDN.B.CAPLESS = true;
    HW_AUDIOOUT_ANACTRL.B.HP_HOLD_GND = true;        // Prevent Digital Noise
        
    // DAC reset and gating
    HW_AUDIOOUT_CTRL.B.SFTRST = false;
    
    // Digital filter gating
    HW_CLKCTRL_XTAL.B.FILT_CLK24M_GATE = false;
    msleep(1);
    
    // Disable the ADC
    HW_AUDIOOUT_PWRDN.B.ADC = true;
    HW_AUDIOIN_ANACLKCTRL.B.CLKGATE = true;
    
    // Enable the DAC
    HW_AUDIOOUT_PWRDN.B.DAC = false;    
    HW_AUDIOOUT_ANACLKCTRL.B.CLKGATE = false;
   
    HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 1;    // 16-bit PCM samples
    HW_AUDIOOUT_CTRL.B.DMAWAIT_COUNT = 16; // @todo fine-tune this 

    // Transfer XTAL bias to bandgap (power saving)
    //HW_AUDIOOUT_REFCTRL.B.XTAL_BGR_BIAS = 1;
    // Transfer DAC bias from self-bias to bandgap (power saving)
    HW_AUDIOOUT_PWRDN.B.SELFBIAS = 1;

    HW_AUDIOIN_CTRL.B.CLKGATE = true;
    HW_AUDIOOUT_CTRL.B.CLKGATE = false;
    
    msleep(2);
}

//------------------------------------------------------------------------------

// Smoothly ramp the last value in the DAC to zero
static void dac_to_zero()
{   
    unsigned dac_value = HW_AUDIOOUT_DATA_RD();
    short high = (short) (dac_value >> 16);
    short low = (short) (dac_value & 0xffff);

    PDEBUG("dac_to_zero\n");

    while (high != 0 || low != 0)
    {
	unsigned ulow  = (unsigned) low;
	unsigned uhigh = (unsigned) high;
	unsigned sample = (uhigh<<16) + ulow;

	while (true)
	{
	    if (HW_AUDIOOUT_DACDEBUG.B.FIFO_STATUS == 1)
	    {
		HW_AUDIOOUT_DATA_WR(sample);
		break;
	    }            
	}

	if (high < 0) ++high;
	else if (high > 0) --high;

	if (low < 0) ++low;
	else if (low > 0) --low;
    }    
}

//------------------------------------------------------------------------------

static void dac_ramp_to_ground()
{
    int16_t s16DacValue;

    PDEBUG(" dac_ramp_to_ground()\n");
    
    dac_to_zero();

    s16DacValue = 0;    
    
    dac_blat( s16DacValue, 512 );
    // Enabling DAC circuitry
    BF_CS2(AUDIOOUT_DACVOLUME, VOLUME_LEFT, DAC_MAX_VOLUME,               // Set max volume
                VOLUME_RIGHT, DAC_MAX_VOLUME);
    //SetGain(0);
    BF_CS2(AUDIOOUT_DACVOLUME,MUTE_LEFT,0,MUTE_RIGHT,0);        // Unmute DAC
    BF_CLR(AUDIOOUT_HPVOL,MUTE);                                // Unmute HP

    // Write DAC to VAG for several milliseconds
    dac_blat( s16DacValue, 512 );

    dac_ramp_partial(&s16DacValue, 0x7fff, 1);    

    dac_blat( 0x7fff, 512);
}

//------------------------------------------------------------------------------

static void dac_power_hp(bool on)
{
    PDEBUG(" dac_power_hp()\n");
          
     if (dev_cfg.hp_on != on)
     {
         dev_cfg.hp_on = on; 
 
         if (on)
         {
	    HW_AUDIOOUT_PWRDN.B.DAC = false;    
	    HW_AUDIOOUT_ANACLKCTRL.B.CLKGATE = false;

	    //ASSERT( HW_AUDIOOUT_STAT.B.DAC_PRESENT ); // Check the "DAC present" bit

	    HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 1;    // 16-bit PCM samples
	    HW_AUDIOOUT_CTRL.B.DMAWAIT_COUNT = 16; // @todo fine-tune this        
	    dac_vol_init();
	    dac_ramp_to_vag();

	    volume_set(VC_DAC, DAC_MAX_VOLUME);

            dac_power_lineout(true);
         }
         else
         {	 
            dac_power_lineout(false);

	    // Max out headphone amp first
    	    HW_AUDIOOUT_HPVOL.B.MUTE = 0;
	    HW_AUDIOOUT_HPVOL.B.VOL_LEFT = 0;
            HW_AUDIOOUT_HPVOL.B.VOL_RIGHT = 0;
           
	    dac_ramp_to_ground();
	    
	    BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, true);                // Set hold to GND      
	    
	    HW_AUDIOOUT_PWRDN.B.HEADPHONE = true; // Power down headphones
            HW_AUDIOOUT_PWRDN.B.LINEOUT = true;   // Power down lineout
	    
	    BF_SET(AUDIOOUT_CTRL, DAC_ZERO_ENABLE);                     // Start sending zeros                
	    BF_SET(AUDIOOUT_HPVOL,MUTE);                                // mute HP        
	    BF_CS2(AUDIOOUT_DACVOLUME,MUTE_LEFT,0,MUTE_RIGHT,0);        // mute DAC        
	    BF_CS2(AUDIOOUT_DACVOLUME, VOLUME_LEFT, 0,     		// Set min volume
		     VOLUME_RIGHT, 0);
	    BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, false);                // Disable ClassAB    

	    BF_SET(AUDIOOUT_PWRDN,DAC);                                 // Power down DAC
         }
     }
}

static void dac_power_lineout(bool up)
{
    int vol;

    if (up)
    {
        // Allow capasitor to discharge
        HW_AUDIOOUT_LINEOUTCTRL.B.CHARGE_CAP = 2;
        mdelay(100);

        // Set min gain
        HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT = 0x1F;
        HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_RIGHT = 0x1F;

        // Set VAG Centre voltage - VDDIO\2 - let's make it 1.65 v (3.3 / 2)
        HW_AUDIOOUT_LINEOUTCTRL.B.VAG_CTRL = 0x0C;

        // Power up line out
        HW_AUDIOOUT_PWRDN.B.LINEOUT = 0;

        // Charge the capsitor
        HW_AUDIOOUT_LINEOUTCTRL.B.CHARGE_CAP = 1;
        mdelay(100);

        // Unmute
        HW_AUDIOOUT_LINEOUTCTRL.B.MUTE = 0;

        // Ramp up volume
        for (vol = 0x1E; vol >= 0; vol--)
        {
            HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT = vol;
            HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_RIGHT = vol;
        }
    }
    else
    {
        // Ramp Down volume
        for (vol = HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT; vol < 0x1F; vol++)
        {
            HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT = vol;
            HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_RIGHT = vol;
        }

        // Mute
        HW_AUDIOOUT_LINEOUTCTRL.B.MUTE = 0;

        // Power down line out
        HW_AUDIOOUT_PWRDN.B.LINEOUT = 0;

        // Allow capasitor to discharge
        HW_AUDIOOUT_LINEOUTCTRL.B.CHARGE_CAP = 2;
        mdelay(100);
    }


}

//------------------------------------------------------------------------------

static void dac_start(void)
{
    PDEBUG(" dac_start()\n");    
    dac_power_hp(true);
    dev_cfg.dac_running = true;
}

//------------------------------------------------------------------------------

static void dac_stop(void)
{
    PDEBUG(" dac_stop()\n");
    dac_power_hp(false);
    dev_cfg.dac_running = false;
}

//------------------------------------------------------------------------------

static void volume_set(VOL_CTL_ID vol_ctrl_id, uint8_t new_vol)
{  
    switch (vol_ctrl_id)
    {
        case VC_DAC:
            // Apply limits.
            if( new_vol > DAC_MAX_VOLUME )
                new_vol = DAC_MAX_VOLUME;
            else if( new_vol < DAC_MIN_VOLUME )
                new_vol = DAC_MIN_VOLUME;

            // Apply at last
            HW_AUDIOOUT_DACVOLUME.B.MUTE_LEFT = (new_vol == 0) ? 1 : 0;
            HW_AUDIOOUT_DACVOLUME.B.MUTE_RIGHT = (new_vol == 0) ? 1 : 0;
            HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT = new_vol;
            HW_AUDIOOUT_DACVOLUME.B.VOLUME_RIGHT = new_vol;
            break;

        case VC_HEADPHONE:
            // Apply limits.
            if( new_vol > HP_MAX_VOLUME )
                new_vol = HP_MAX_VOLUME;
                
            // Invert range 
	    new_vol = -((int)new_vol - HP_MAX_VOLUME);

            // Apply at last
            HW_AUDIOOUT_HPVOL.B.MUTE = (new_vol == HP_MAX_VOLUME) ? 1 : 0;
            HW_AUDIOOUT_HPVOL.B.VOL_LEFT = new_vol;
            HW_AUDIOOUT_HPVOL.B.VOL_RIGHT = new_vol;
              
            break;

        case VC_LINEOUT:
            // Apply limits.
            if( new_vol > LO_MAX_VOLUME )
                new_vol = LO_MAX_VOLUME;
                
            // Invert range 
	    new_vol = -((int)new_vol - LO_MAX_VOLUME);

            // Apply at last
            HW_AUDIOOUT_LINEOUTCTRL.B.MUTE = (new_vol == LO_MAX_VOLUME) ? 1 : 0;
            HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT = new_vol;
            HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_RIGHT = new_vol;
            break;
    }

}

//------------------------------------------------------------------------------

static void dac_select_src(HP_SOURCE hp_source)
{
    BF_WR(AUDIOOUT_HPVOL, SELECT, hp_source);
}

//------------------------------------------------------------------------------

static void mute(VOL_CTL_ID vol_ctrl_id)
{
    PDEBUG(" mute(%d)\n", vol_ctrl_id);

    switch(vol_ctrl_id)
    {
        case VC_DAC:
            BF_SET(AUDIOOUT_DACVOLUME, MUTE_LEFT);
            BF_SET(AUDIOOUT_DACVOLUME, MUTE_RIGHT);
            break;

        case VC_HEADPHONE:
            HW_AUDIOOUT_HPVOL.B.MUTE = 1;
            break;

        case VC_LINEOUT:
            HW_AUDIOOUT_LINEOUTCTRL.B.MUTE = 1;
            break;

        default:
            break;
    }
}

//------------------------------------------------------------------------------

static void unmute(VOL_CTL_ID vol_ctrl_id)
{
    PDEBUG(" unmute(%d)\n", vol_ctrl_id);

    switch(vol_ctrl_id)
    {
        case VC_DAC:
            BF_CLR(AUDIOOUT_DACVOLUME, MUTE_LEFT);
            BF_CLR(AUDIOOUT_DACVOLUME, MUTE_RIGHT);
            break;

        case VC_HEADPHONE:
            HW_AUDIOOUT_HPVOL.B.MUTE = 0;
            break;

        case VC_LINEOUT:
             HW_AUDIOOUT_LINEOUTCTRL.B.MUTE = 0;
            break;

        default:
            break;
    }
}

//------------------------------------------------------------------------------
// Stolen unashamedly from codec_stmp36xx.cpp to remove the uber switch used in
// dac_sample_rate_set previously

static void calculate_samplerate_params(unsigned long rate, unsigned long *ret_basemult, long unsigned *ret_src_hold, unsigned long *ret_src_int, unsigned long *ret_src_frac)
{
    unsigned long basemult, src_hold, src_int, src_frac;
    unsigned long long num, den, divide, mod;
    
    num = 10;
    den = 5;
    divide = do_div(num, den);
 
    basemult = 1; 
    while( rate > 48000 )
    {
        rate /= 2;
        basemult *= 2;         
    }
    src_hold = 1;
    while( rate <= 24000 )
    {
        rate *= 2;
        src_hold *= 2;
    }
    src_hold--;

    // Work around 64 bit divides, which aren't supported in kernel space, using do_div(x, y) and a shift.
    // This will leave the result in x, and return the remainder, somewhat misleadingly...
    num = 6000000ULL << 13;
    den = rate * 8;   
    divide = num + (den >> 2);
    mod = do_div(divide, den);

    // 13 bit fixed point rate
    src_int = divide >> 13; // for the integer portion
    src_frac = divide & 0x1FFF; // the fractional portion    

    *ret_basemult = basemult;
    *ret_src_hold = src_hold;
    *ret_src_int = src_int;
    *ret_src_frac = src_frac;
}

//------------------------------------------------------------------------------

static int dac_sample_rate_set(unsigned long rate)
{
    unsigned long base_rate_mult, hold, sample_rate_int, sample_rate_frac;

    PDEBUG(" dac_sample_rate_set(%d)\n", (int)rate);
    
    calculate_samplerate_params(rate, &base_rate_mult, &hold, &sample_rate_int, &sample_rate_frac);
    
    BF_CS4(AUDIOOUT_DACSRR, SRC_HOLD, hold, SRC_INT, sample_rate_int, SRC_FRAC,
           sample_rate_frac, BASEMULT, base_rate_mult);

    dev_cfg.dac_sample_rate = rate;

    return 0;
}

//------------------------------------------------------------------------------

module_init(device_init);
module_exit(device_exit);

MODULE_AUTHOR(DRIVER_AUTH);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
