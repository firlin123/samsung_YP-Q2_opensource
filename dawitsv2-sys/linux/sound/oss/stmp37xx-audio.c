/*
 *  sound/oss/stmp37xx-audio.c
 *
 *  Copyright (C) 2006 Sigmatel Inc
 *  Copyright (C) 2008 MIZI Research, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Sun Feb 17, Won-young Chung <pain@mizi.com>
 *  - initial
 *
 */

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
#include <linux/sound.h>
#include <linux/soundcard.h>

#include <asm/div64.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/arch-stmp37xx/irqs.h>
#include <asm/arch-stmp37xx/ocram.h>
#include <asm/arch-stmp37xx/pinctrl.h>


#define OCRAM_DAC_USE

/* STMP3xxx ioctl header */
/* NEED TO CLEANUP!!!!-------------------------------------------------*/
#include <linux/ioctl.h>

#define STMP3XXX_DAC_IOC_MAGIC  0xC2
#define STMP3XXX_ADC_IOC_MAGIC  0xC3

#define GET_MIN_VOL(range)  (range & 0xFFFF)
#define GET_MAX_VOL(range)  (range >> 16)
#define MAKE_VOLUME(vol_ctl, vol) ((vol_ctl << 24) | (vol & 0x00FFFFFF))
#define AUDIO_FMT       AFMT_S16_LE

typedef enum VOL_CTL_ID
{
    VC_DAC = 0,
    VC_HEADPHONE,
    VC_LINEOUT
}VOL_CTL_ID;

typedef enum HP_SOURCE
{
    HP_DAC = 0,
    HP_LINEIN = 1
} HP_SOURCE;

//------------------------------------------------------------------------------
// IOCT = Tell = parameter contains info
//------------------------------------------------------------------------------
#define STMP3XXX_DAC_IOCT_MUTE            _IOW(STMP3XXX_DAC_IOC_MAGIC,  0, int)
#define STMP3XXX_DAC_IOCT_UNMUTE          _IOW(STMP3XXX_DAC_IOC_MAGIC,  1, int)
#define STMP3XXX_DAC_IOCT_VOL_SET         _IOW(STMP3XXX_DAC_IOC_MAGIC,  2, int)
#define STMP3XXX_DAC_IOCT_SOURCE          _IOW(STMP3XXX_DAC_IOC_MAGIC,  3, int)
#define STMP3XXX_DAC_IOCT_SAMPLE_RATE     _IOW(STMP3XXX_DAC_IOC_MAGIC,  4, int)
#define STMP3XXX_DAC_IOCT_EXPECT_UNDERRUN _IOW(STMP3XXX_DAC_IOC_MAGIC,  5, bool)
#define STMP3XXX_DAC_IOCT_INIT            _IO(STMP3XXX_DAC_IOC_MAGIC,   6)
#define STMP3XXX_DAC_IOCT_HP_SOURCE       _IOW(STMP3XXX_DAC_IOC_MAGIC,  7, int)

#define STMP3XXX_ADC_IOCT_VOL_SET        _IOW(STMP3XXX_ADC_IOC_MAGIC,  0, int)
#define STMP3XXX_ADC_IOCT_START          _IO(STMP3XXX_ADC_IOC_MAGIC,   1)
#define STMP3XXX_ADC_IOCT_STOP           _IO(STMP3XXX_ADC_IOC_MAGIC,   2)
#define STMP3XXX_ADC_IOCT_SAMPLE_RATE    _IOW(STMP3XXX_ADC_IOC_MAGIC,  3, int)

//------------------------------------------------------------------------------
// IOCQ = Query = return value contains info
//------------------------------------------------------------------------------
#define STMP3XXX_DAC_IOCQ_IS_MUTED         _IOR(STMP3XXX_DAC_IOC_MAGIC, 8, int)
#define STMP3XXX_DAC_IOCQ_VOL_RANGE_GET    _IOR(STMP3XXX_DAC_IOC_MAGIC, 9, int)
#define STMP3XXX_DAC_IOCQ_VOL_GET          _IOR(STMP3XXX_DAC_IOC_MAGIC, 10, int)
#define STMP3XXX_DAC_IOCQ_SAMPLE_RATE      _IOR(STMP3XXX_DAC_IOC_MAGIC, 11, int)
#define STMP3XXX_DAC_IOCQ_HP_SOURCE_GET    _IOR(STMP3XXX_DAC_IOC_MAGIC, 12, int)
#define STMP3XXX_DAC_IOCQ_ACTIVE           _IOR(STMP3XXX_DAC_IOC_MAGIC, 13, int)

#define STMP3XXX_ADC_IOCQ_VOL_GET        _IOR(STMP3XXX_ADC_IOC_MAGIC, 4, int)
#define STMP3XXX_ADC_IOCQ_VOL_RANGE_GET  _IOR(STMP3XXX_ADC_IOC_MAGIC, 5, int)
#define STMP3XXX_ADC_IOCQ_SAMPLE_RATE    _IOR(STMP3XXX_ADC_IOC_MAGIC, 6, int)
//------------------------------------------------------------------------------
// IOCS = Set = argument contains user space pointer to info
//------------------------------------------------------------------------------
typedef struct
{
    // 0 <= remaining_next_bytes <= remaining_now_bytes <= total_bytes
    //
    // total_bytes will not change between calls.
    //
    // remaining_now_bytes is the number of bytes queued in the hardware
    //     DMA channel, instantaneously.
    //
    // remaining_next_bytes is the number of bytes that will still be
    //     queued when we next wake up. This can be used to decide whether
    //     it's safe to sleep until the next period, or whether data needs
    //     to be provided before then.
    unsigned long total_bytes;
    unsigned long remaining_now_bytes;
    unsigned long remaining_next_bytes;
} stmp3xxx_dac_queue_state_t;
#define STMP3XXX_DAC_IOCS_QUEUE_STATE	 _IOR(STMP3XXX_DAC_IOC_MAGIC, 14, stmp3xxx_dac_queue_state_t)
/* -----------------------------------------------------------------*/






#ifndef bool
#define bool int
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif


#define STMP37XX_SOUND_DEBUG 0
#if (STMP37XX_SOUND_DEBUG > 0)
#define DPRINTK(x...)   \
	do { printk("[%s:%d] ", __func__, __LINE__); printk(x); } while(0)
#else
#define DPRINTK(x...)
#endif

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

#define DRIVER_NAME          "stmp37xx-audio"
#define BUS_ID               "dac0"
#define DMA_NUM_SAMPLES      800
#define CHAIN_LENGTH         32

#define ADC_DMA_CHANNEL		 0
#define DAC_DMA_CHANNEL      1
#define DAC_MIN_VOLUME       0x37
#define DAC_MAX_VOLUME       0xFE
#define ADC_MIN_VOLUME       0x37
#define ADC_MAX_VOLUME       0xFE
#define HP_MAX_VOLUME        0x7F
#define HP_MIN_VOLUME        0
#define LO_MAX_VOLUME        0x1F
#define LO_MIN_VOLUME        0
#define DEFAULT_SAMPLE_RATE  44100
#define DEFAULT_SAMPLE_WIDTH 16

#define RESET_WAIT_COUNT_MAX 10

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------
typedef struct device device_t;
typedef uint16_t dma_buf_t[DMA_NUM_SAMPLES];

typedef struct {
	reg32_t next;
	hw_apbx_chn_cmd_t cmd;
	reg32_t buf_ptr;
} dma_descriptor_t;

typedef struct {
	bool inuse;
	dma_descriptor_t dd;	// dma descriptor
	dma_buf_t data;
} dma_block_t;

typedef struct {
	dma_block_t *rd_ptr;
	dma_block_t *wr_ptr;
	dma_block_t *first_ptr;
	dma_block_t *last_ptr;
	uint16_t inuse_count;
	dma_block_t block[CHAIN_LENGTH];
} dma_chain_t;

typedef struct {
	uint32_t irq_dma_count;
	uint32_t irq_err_count;
	wait_queue_head_t wait_q;
	dev_t dev;
	device_t *device;
	unsigned int irq_dma;
	unsigned int irq_err;
	spinlock_t lock;
	spinlock_t irq_lock;
	dma_addr_t real_addr;
	void *virt_addr;
	dma_chain_t *dma_chain;
	bool running;
	unsigned long sample_rate;
	unsigned long sample_width;
	uint8_t volume;
	bool hp_on;
	bool init_device;
} dev_config_t;

typedef struct {
	unsigned status;
	unsigned samples_sent;
} dac_status_t;

enum DAC_STATUS_BITS {
	DAC_STATUS_UNDERRUN = 0x00000001,
	DAC_STATUS_DRAINED = 0x00000002,
	DAC_STATUS_SAMPLES_SENT = 0x00000004
};

#define MAKE_VOL_RANGE(max_vol, min_vol) ((max_vol << 16) | (min_vol & 0xFFFF))
#define GET_VOL_CTL_ID(volume_and_control) (volume_and_control >> 24)
#define GET_VOLUME(volume_and_control) (volume_and_control & 0x00FFFFFF)

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
static void release(device_t * dev);
static void dma_chain_init(dev_config_t *dev);
static void dma_enable(dev_config_t *dev);
void adc_init( unsigned long sample_rate, unsigned long sample_width );
static void dac_init(unsigned long sample_rate, unsigned long bit_width_is_32);
static void dac_uninit(void);
void get_dma_bytes_inuse(unsigned long *ret_total_left,
			 unsigned long *ret_after_next_interrupt);
void adc_start(void);
void dac_start(void);
void dac_stop(void);
void adc_stop(void);
void adc_volume_set(uint8_t new_vol);
void dump_apbx_reg(void);
void dump_dac_reg(void);
void dac_select_src(HP_SOURCE hp_source);
void calculate_samplerate_params(unsigned long rate,
				 unsigned long *ret_basemult,
				 unsigned long *ret_src_hold,
				 unsigned long *ret_src_int,
				 unsigned long *ret_src_frac);
int dac_sample_rate_set(unsigned long rate);
void dac_ramp_to_vag(void);
void dac_ramp_to_ground(void);
void dac_vol_init(void);
void dac_to_zero(void);
void dac_trace_regs(void);
void mute(VOL_CTL_ID vol_ctl_id);
void unmute(VOL_CTL_ID vol_ctl_id);
void volume_set(VOL_CTL_ID vol_ctl_id, uint8_t new_vol);
void mute_cntl(unsigned int mute_val); //dhsong
void dac_power_lineout(bool up);
int calc_to_dac_volume(int vol);
int calc_to_usr_volume(int vol);
void set_pcm_audio_bit(int bit);
static int stmp37xx_mix_ioctl(struct inode *inode, struct file *file, 
			  unsigned int cmd, unsigned long arg);

static device_t device = {
	.parent = NULL,
	.bus_id = BUS_ID,
	.release = release,
	.coherent_dma_mask = ISA_DMA_THRESHOLD,
};

static dev_config_t dev_dac_cfg = {
	.irq_dma = IRQ_DAC_DMA,
	.irq_err = IRQ_DAC_ERROR,
	.irq_dma_count = 0,
	.irq_err_count = 0,
	.device = &device,
	.running = false,
	.sample_rate = DEFAULT_SAMPLE_RATE,
	.sample_width = DEFAULT_SAMPLE_WIDTH,
	.hp_on = false,
	.init_device = false
};

static dev_config_t dev_adc_cfg = {
	.irq_dma = IRQ_ADC_DMA,
	.irq_err = IRQ_ADC_ERROR,
	.irq_dma_count = 0,
	.irq_err_count = 0,
	.device = &device,
	.running = false,
	.sample_rate = DEFAULT_SAMPLE_RATE,
	.sample_width = DEFAULT_SAMPLE_WIDTH,
	.hp_on = false,
	.init_device = false
};

static bool expect_underrun = false;
static dac_status_t dac_status;

static unsigned session_activatedbytes = 0;
static unsigned interrupt_sentbytes = 0;
static unsigned session_sentbytes = 0;

static int audio_dev_dsp;
static int audio_dev_mix;

static int audio_rd_refcount;
static int audio_wr_refcount;
#define audio_active	(audio_rd_refcount || audio_wr_refcount)

static int audio_channels;

static void *map_phys_to_virt(unsigned real, dev_config_t dev)
{
	return (void *)(real - dev.real_addr + (unsigned)dev.virt_addr);
}


void get_dma_bytes_inuse(unsigned long *ret_total_left,
			 unsigned long *ret_after_next_interrupt)
{
	unsigned total_left;
	unsigned after_next_interrupt;
	unsigned loop_check;
	unsigned bytes;
	unsigned sem;

	bool had_interrupt_marker;

	dma_descriptor_t *first, *desc, *check_first;
	hw_apbh_chn_debug2_t debug;

	for (;;) {
		total_left = 0;
		after_next_interrupt = 0;

		do {
			first =
			    (dma_descriptor_t *)
			    map_phys_to_virt(HW_APBX_CHn_CURCMDAR_RD
					     (DAC_DMA_CHANNEL), dev_dac_cfg);
			sem = BF_RDn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, PHORE);

		}
		while (first !=
		       (dma_descriptor_t *)
		       map_phys_to_virt(HW_APBX_CHn_CURCMDAR_RD
					(DAC_DMA_CHANNEL), dev_dac_cfg));

		if (!first || sem == 0) {
			// Empty chain 
			break;
		}

		desc = first;
		had_interrupt_marker = false;

		loop_check = 10000;
		while (desc->cmd.B.CHAIN) {
			if (--loop_check == 0) {
				break;
			}

			if (desc->cmd.B.SEMAPHORE && --sem == 0) {
				break;
			}

			if (desc->cmd.B.IRQONCMPLT)
				had_interrupt_marker = true;

			desc =
			    (dma_descriptor_t *) map_phys_to_virt(desc->next, dev_dac_cfg);

			bytes = desc->cmd.B.XFER_COUNT;

			if (had_interrupt_marker)
				after_next_interrupt += bytes;

			total_left += bytes;
		}

		// Some amount of the current command will still be in transit
		debug = HW_APBH_CHn_DEBUG2(DAC_DMA_CHANNEL);
		total_left += debug.B.AHB_BYTES;

		check_first =
		    (dma_descriptor_t *)
		    map_phys_to_virt(HW_APBX_CHn_CURCMDAR_RD(DAC_DMA_CHANNEL), dev_dac_cfg);
		if (first == check_first)
			break;
	}

	if (ret_total_left)
		*ret_total_left = total_left;
	if (ret_after_next_interrupt)
		*ret_after_next_interrupt = after_next_interrupt;
}


static irqreturn_t device_adc_dma_handler(int irq, void *dev_idp)
{
    dev_config_t *dev = dev_idp;

    spin_lock_irq(&dev->irq_lock);
    dev->irq_dma_count++;

    // manage dma chain
    if (dev->dma_chain->inuse_count < CHAIN_LENGTH)
    {
        dev->dma_chain->inuse_count++;
        dev->dma_chain->wr_ptr->inuse = true;

        // get setup for the next dma block, wrap read pointer if necessary
        if (dev->dma_chain->wr_ptr == dev->dma_chain->last_ptr)
        {
            dev->dma_chain->wr_ptr = dev->dma_chain->first_ptr;
        }
        else
        {
            // point at next dma block
            dev->dma_chain->wr_ptr++;
        }

        // wake up reading process, as dma block has become available
        wake_up_interruptible(&dev->wait_q);
    }
    else
    {
         printk( KERN_ALERT "device_adc_dma_handler() - dma chain full\n");
    }

    spin_unlock_irq(&dev->irq_lock);

    // clear interrupt status
    BF_CLR(APBX_CTRL1, CH0_CMDCMPLT_IRQ);
    //BW_APBX_CTRL1_CH0_CMDCMPLT_IRQ(1);

    return IRQ_HANDLED;
}

static irqreturn_t device_dac_dma_handler(int irq, void *dev_idp)
{
	dev_config_t *dev = dev_idp;
	const unsigned long sample_size = sizeof(short);
	const unsigned long channels_count = 2;	/* stereo */

	unsigned long remaining_bytes, now_sentbytes, sent_bytes;
	unsigned long samples_count;

	unsigned sema_dec;

	bool underrun = false;

	spin_lock_irq(&dev->irq_lock);

	get_dma_bytes_inuse(&remaining_bytes, NULL);

	now_sentbytes = session_activatedbytes - remaining_bytes;
	interrupt_sentbytes = now_sentbytes - session_sentbytes;
	session_sentbytes = now_sentbytes;

	sent_bytes = interrupt_sentbytes;
	samples_count = sent_bytes / (sample_size * channels_count);

	dev->irq_dma_count++;

	// Check for underruns, handling the case where the flag state
	// changes during this call
	if (BF_RDn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, PHORE) == 0) {
		underrun = true;
	} else {
		if (HW_AUDIOOUT_CTRL.B.FIFO_UNDERFLOW_IRQ) {
			underrun = true;
			// Clear underrun flag
			HW_AUDIOOUT_CTRL_CLR
			    (BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ);
		} else if (BF_RDn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, PHORE) == 0)
			underrun = true;
	}

	// manage dma chain
	if (dev->dma_chain->inuse_count > 0) {
		sema_dec =
		    dev->dma_chain->inuse_count - BF_RDn(APBX_CHn_SEMA,
							    DAC_DMA_CHANNEL,
							    PHORE);
		dev->dma_chain->inuse_count -= sema_dec;

		while (sema_dec--) {
			dev->dma_chain->rd_ptr->inuse = false;

			// get setup for the next dma block, wrap read pointer if necessary
			if (dev->dma_chain->rd_ptr ==
			    dev->dma_chain->last_ptr) {
				dev->dma_chain->rd_ptr =
				    dev->dma_chain->first_ptr;
			} else {
				// point at next dma block
				dev->dma_chain->rd_ptr++;
			}
		}

		//ASSERT( ( sent_bytes % ( sample_size * channels_count ) ) == 0 ); // no remainder

		if (samples_count) {
			dac_status.status |= DAC_STATUS_SAMPLES_SENT;
			dac_status.samples_sent += samples_count;
		}

		if (!expect_underrun && underrun) {
			DPRINTK(" ********* UNDERRUN *********\n");
			dac_status.status |= DAC_STATUS_UNDERRUN;
		} else if (dev->dma_chain->inuse_count == 0) {
			if (expect_underrun) {
				DPRINTK(" drained %d 0x%x\n", expect_underrun,
				       HW_AUDIOOUT_CTRL.B.FIFO_UNDERFLOW_IRQ);
				dac_status.status |= DAC_STATUS_DRAINED;

				// Drained, so reset the byte counts
				session_activatedbytes = 0;
				session_sentbytes = 0;
				interrupt_sentbytes = 0;

				dac_to_zero();

				expect_underrun = false;
			} else
				printk
				    (" empty dma chain, but FIFO_UNDERFLOW_IRQ unset and not expecting underrun!\n");
		}
		// Push out zeros while the chain is empty
		//HW_AUDIOOUT_CTRL_SET(BM_AUDIOOUT_CTRL_DAC_ZERO_ENABLE);

		// wake up writing process, as dma block has become available
		wake_up_interruptible(&dev->wait_q);

	} else {
		DPRINTK(" device_dac_dma_handler() - "
		       "unexpected dma interrupt w/empty dma chain\n");
	}

	// clear interrupt status
	BF_CLR(APBX_CTRL1, CH1_CMDCMPLT_IRQ);
	//BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);

	if (HW_AUDIOOUT_CTRL.B.FIFO_OVERFLOW_IRQ)
		HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_FIFO_OVERFLOW_IRQ);

	spin_unlock_irq(&dev->irq_lock);

	return IRQ_HANDLED;
}


#if 0
static irqreturn_t device_err_handler(int irq, void *dev_idp)
{
	DPRINTK(" - not implemented\n");

	dev_cfg.irq_err_count++;

	// clear interrupt status
	// TODO:

	return IRQ_HANDLED;
}
#endif


static void release(device_t * dev)
{
	DPRINTK(" - releasing device struct with bus_id: " BUS_ID "\n");

#ifdef OCRAM_DAC_USE
	iounmap(dev_dac_cfg.virt_addr);
#else
	dma_free_coherent(dev_dac_cfg.device,
			  sizeof(dma_block_t) * CHAIN_LENGTH,
			  dev_dac_cfg.virt_addr, dev_dac_cfg.real_addr);
#endif
}


static void dma_chain_init(dev_config_t *dev)
{
	int i;

	// overlay the dma_chain structure on the dma memory (accessible via kernel
	// virtual address or real physical address)
	dma_chain_t *dma_chain = (dma_chain_t *) dev->real_addr;
	dev->dma_chain = (dma_chain_t *) dev->virt_addr;

	DPRINTK(" dma_chain_init()\n");

	// chain descriptors together
	for (i = 0; i < CHAIN_LENGTH; i++) {
		dev->dma_chain->block[i].inuse = false;

		// link current descriptor to next descriptor
		dev->dma_chain->block[i].dd.next =
		    (reg32_t) & dma_chain->block[i + 1].dd;
		dev->dma_chain->block[i].dd.cmd =
		    (hw_apbx_chn_cmd_t) (BF_APBX_CHn_CMD_XFER_COUNT
					 (sizeof(dma_buf_t)) |
					 BF_APBX_CHn_CMD_CHAIN(1) |
					 BF_APBX_CHn_CMD_SEMAPHORE(1) |
					 BF_APBX_CHn_CMD_IRQONCMPLT(1) |
					 BV_FLD(APBX_CHn_CMD, COMMAND, DMA_READ)
		    );
		dev->dma_chain->block[i].dd.buf_ptr =
		    (reg32_t) dma_chain->block[i].data;
	}
	// wrap last descriptor around to first
	dev->dma_chain->block[i - 1].dd.next =
	    (reg32_t) & dma_chain->block[0].dd;

	dev->dma_chain->rd_ptr = &dev->dma_chain->block[0];
	dev->dma_chain->wr_ptr = &dev->dma_chain->block[0];
	dev->dma_chain->first_ptr = &dev->dma_chain->block[0];
	dev->dma_chain->last_ptr =
	    &dev->dma_chain->block[CHAIN_LENGTH - 1];
	dev->dma_chain->inuse_count = 0;
}

static void dma_enable (dev_config_t *pdev)
{
	dma_chain_t *dma_chain = (dma_chain_t *) pdev->real_addr;

	DPRINTK("\n");

	BF_CLR(APBX_CTRL0, CLKGATE);
	BF_CLR(APBX_CTRL0, SFTRST);
	BF_WR(APBX_CTRL0, RESET_CHANNEL, (1 << DAC_DMA_CHANNEL));
	while (HW_APBX_CTRL0.B.RESET_CHANNEL & (1 << DAC_DMA_CHANNEL)) ;

	// give APBX block the first DAC DMA descriptor
	BF_WRn(APBX_CHn_NXTCMDAR, DAC_DMA_CHANNEL, CMD_ADDR,
	       (reg32_t) & dma_chain->block[0].dd);

	BF_CLR(APBX_CTRL1, CH1_CMDCMPLT_IRQ);
	BF_SET(APBX_CTRL1, CH1_CMDCMPLT_IRQ_EN);
}

static void dma_disable (void)
{
	DPRINTK("\n");

	BF_SET(APBX_CTRL0, CLKGATE);
	BF_SET(APBX_CTRL0, SFTRST);
}


#define VDDD_BASE_MIN       1400	// minimum voltage requested (in mV)
#define VAG_BASE_VALUE_ADJ  ((((((VDDD_BASE_MIN*160)/(2*125)))-625)/25))	// Adjust for LW_REF (1.6/1.25)

void dac_vol_init()
{
	hw_audioout_refctrl_t ref_ctrl;

	DPRINTK(" dac_vol_init\n");

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
	ref_ctrl.B.ADJ_VAG = 1; //if 0 VAG=VDDA/2
#if 1 //add dhsong
	if (ref_ctrl.B.ADJ_VAG == 1)
	{
		//ref_ctrl.B.VAG_VAL = VAG_BASE_VALUE_ADJ;
		//ref_ctrl.B.VAG_VAL = 0xD; //for vdda 1.950V,
		//ref_ctrl.B.VAG_VAL = 0xE; //for vdda 2.0V, 
		ref_ctrl.B.VAG_VAL = 0xF; //for vdda 2.1V, 
	}
	else ;
#endif
	ref_ctrl.B.VBG_ADJ = 0x3;
	ref_ctrl.B.LW_REF = 1;
	ref_ctrl.B.RAISE_REF = 1; //raise_ref bit setting for voltage, add dhsong
	//HW_AUDIOOUT_REFCTRL_SET(0x1 << 25);//raise_ref bit setting for voltage, add dhsong

	HW_AUDIOOUT_REFCTRL_WR(ref_ctrl.U);

	printk("\n\nHW_AUDIOOUT_REFCTRL= 0x%8x\n\n", HW_AUDIOOUT_REFCTRL); //add dhsong
}


// Bit-bang samples to the dac
void dac_blat(unsigned short value, int num_samples)
{
	unsigned sample;

	//  ASSERT(HW_APBX_CHn_DEBUG1(1).B.STATEMACHINE == 0);

	sample = (unsigned)value;	// copy to other channel
	sample += (sample << 16);

	while (num_samples > 0) {
		if (HW_AUDIOOUT_DACDEBUG.B.FIFO_STATUS == 1) {
			HW_AUDIOOUT_DATA_WR(sample);
			num_samples--;
		}
	}
}


void dac_ramp_partial(short *u16DacValue, short target_value, short step)
{
	if (step < 0) {
		while (*u16DacValue > target_value) {
			dac_blat(*u16DacValue, 1);
			*u16DacValue += step;
		}
	} else {
		while (*u16DacValue < target_value) {
			dac_blat(*u16DacValue, 1);
			*u16DacValue += step;
		}
	}
}


void dac_ramp_up(uint8_t newVagValue, uint8_t oldVagValue)
{
	int16_t s16DacValue = 0x7FFF;	// set to lowest voltage    

	DPRINTK("\n");

	dac_ramp_partial(&s16DacValue, 0x7000, -2);
	dac_ramp_partial(&s16DacValue, 0x6000, -3);
	dac_ramp_partial(&s16DacValue, 0x5000, -3);
	dac_ramp_partial(&s16DacValue, 0x4000, -4);

	// Slowly raise VAG back to set level
	while (newVagValue != oldVagValue) {
		if (newVagValue > oldVagValue) {
			BF_CLR(AUDIOOUT_REFCTRL, VAG_VAL);
			BF_SETV(AUDIOOUT_REFCTRL, VAG_VAL, --newVagValue);
			dac_blat(s16DacValue, 512);
		} else {
			BF_CLR(AUDIOOUT_REFCTRL, VAG_VAL);
			BF_SETV(AUDIOOUT_REFCTRL, VAG_VAL, ++newVagValue);
			dac_blat(s16DacValue, 512);
		}
	}

	dac_ramp_partial(&s16DacValue, 0x3000, -4);
	dac_ramp_partial(&s16DacValue, 0x2000, -3);
	dac_ramp_partial(&s16DacValue, 0x1000, -2);
	dac_ramp_partial(&s16DacValue, 0x0000, -1);
}


void dac_ramp_to_vag(void)
{
	int16_t u16DacValue;
	uint8_t oldVagValue;
	uint8_t newVagValue;

	DPRINTK("\n");

	// Gather values from registers we are going to override
	oldVagValue = (uint8_t) BF_RD(AUDIOOUT_REFCTRL, VAG_VAL);

	// Setup Output for a clean signal
	BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, true);	// Prevent Digital Noise

	msleep(1);

	BF_CLR(AUDIOOUT_PWRDN, DAC);	// Power UP DAC   
	BF_CLR(AUDIOOUT_PWRDN, CAPLESS);	// Power UP Capless
	//BF_SET(AUDIOOUT_PWRDN, CAPLESS);	// Power down Capless, dhsong
	BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, false);	// Disable ClassAB    

	// Reduce VAG so that we can get closer to ground when starting 
	newVagValue = 0;
	BF_CLR(AUDIOOUT_REFCTRL, VAG_VAL);
	BF_SETV(AUDIOOUT_REFCTRL, VAG_VAL, newVagValue);

	// Enabling DAC circuitry
	//BF_CS2(AUDIOOUT_DACVOLUME, VOLUME_LEFT, DAC_MAX_VOLUME,	// Set max volume
	//       VOLUME_RIGHT, DAC_MAX_VOLUME);
	BF_CS2(AUDIOOUT_DACVOLUME, MUTE_LEFT, 0, MUTE_RIGHT, 0);	// Unmute DAC
	BF_CLR(AUDIOOUT_HPVOL, MUTE);	// Unmute HP

	BF_CLR(AUDIOOUT_CTRL, DAC_ZERO_ENABLE);	// Stop sending zeros

	BF_SET(AUDIOOUT_CTRL, RUN);	// Start DAC

	// Full deflection negative
	u16DacValue = 0x7fff;

	// Write DAC to ground for several milliseconds
	dac_blat(u16DacValue, 512);

	// PowerUp Headphone, Unmute
	BF_CLR(AUDIOOUT_PWRDN, HEADPHONE);	// Pwrup HP amp

	dac_blat(u16DacValue, 256);

	BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, false);	// Release hold to GND
	dac_blat(u16DacValue, 256);
	dac_ramp_up(newVagValue, oldVagValue);	// Ramp to VAG

	// Need to wait here - block for the time being...       
	//msleep(1700);
	BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, true);	// Enable ClassAB    
}


void dac_trace_regs()
{
	DPRINTK("HW_AUDIOOUT_CTRL     = 0x%08x\n", HW_AUDIOOUT_CTRL_RD());
	DPRINTK("HW_AUDIOOUT_STAT     = 0x%08x\n", HW_AUDIOOUT_STAT_RD());
	DPRINTK("HW_AUDIOOUT_DACSRR   = 0x%08x\n", HW_AUDIOOUT_DACSRR_RD());
	DPRINTK("HW_AUDIOOUT_DACVOLUME= 0x%08x\n", HW_AUDIOOUT_DACVOLUME_RD());
	DPRINTK("HW_AUDIOOUT_DACDEBUG = 0x%08x\n", HW_AUDIOOUT_DACDEBUG_RD());
	DPRINTK("HW_AUDIOOUT_HPVOL    = 0x%08x\n", HW_AUDIOOUT_HPVOL_RD());
	DPRINTK("HW_AUDIOOUT_PWRDN    = 0x%08x\n", HW_AUDIOOUT_PWRDN_RD());
	DPRINTK("HW_AUDIOOUT_REFCTRL  = 0x%08x\n", HW_AUDIOOUT_REFCTRL_RD());
	DPRINTK("HW_AUDIOOUT_ANACTRL  = 0x%08x\n", HW_AUDIOOUT_ANACTRL_RD());
	DPRINTK("HW_AUDIOOUT_TEST     = 0x%08x\n", HW_AUDIOOUT_TEST_RD());
	DPRINTK("HW_AUDIOOUT_BISTCTRL = 0x%08x\n", HW_AUDIOOUT_BISTCTRL_RD());
	DPRINTK("HW_AUDIOOUT_BISTSTAT0= 0x%08x\n", HW_AUDIOOUT_BISTSTAT0_RD());
	DPRINTK("HW_AUDIOOUT_BISTSTAT1= 0x%08x\n", HW_AUDIOOUT_BISTSTAT1_RD());
	DPRINTK("HW_AUDIOOUT_ANACLKCTRL=0x%08x\n", HW_AUDIOOUT_ANACLKCTRL_RD());
	DPRINTK("\n");
	DPRINTK("\n");
	DPRINTK("HW_APBX_CTRL0 = 0x%08x\n", HW_APBX_CTRL0_RD());
	DPRINTK("HW_APBX_CTRL1 = 0x%08x\n", HW_APBX_CTRL1_RD());
	DPRINTK("HW_APBX_CH1_CURCMDAR = 0x%08x\n", HW_APBX_CHn_CURCMDAR_RD(1));
	DPRINTK("HW_APBX_CH1_NXTCMDAR = 0x%08x\n", HW_APBX_CHn_NXTCMDAR_RD(1));
	DPRINTK("HW_APBX_CH1_CMD      = 0x%08x\n", HW_APBX_CHn_CMD_RD(1));
	DPRINTK("HW_APBX_CH1_BAR      = 0x%08x\n", HW_APBX_CHn_BAR_RD(1));
	DPRINTK("HW_APBX_CH1_SEMA     = 0x%08x\n", HW_APBX_CHn_SEMA_RD(1));
	DPRINTK("HW_APBX_CH1_DEBUG1   = 0x%08x\n", HW_APBX_CHn_DEBUG1_RD(1));
	DPRINTK("HW_APBX_CH1_DEBUG2   = 0x%08x\n", HW_APBX_CHn_DEBUG2_RD(1));
}

void hw_audioin_PowerdownADC(bool bPwrdn)
{
    bool bAudioOutSRState = BF_RD(AUDIOOUT_CTRL, SFTRST);
    bool bAudioOutCGState = BF_RD(AUDIOOUT_CTRL, CLKGATE);

    /* Power up AudioOut if necessary */
    if(bAudioOutSRState || bAudioOutCGState)
    {
        BF_CLR(AUDIOOUT_CTRL, SFTRST);
        BF_CLR(AUDIOOUT_CTRL, CLKGATE);
    }

    BF_CLR(AUDIOOUT_PWRDN,ADC);
    BF_SETV(AUDIOOUT_PWRDN,ADC, bPwrdn);

    BF_CLR(AUDIOOUT_PWRDN,RIGHT_ADC);
    BF_SETV(AUDIOOUT_PWRDN,RIGHT_ADC, bPwrdn);

    /* Return AudioOut to previous state */
    BF_SETV(AUDIOOUT_CTRL, SFTRST, bAudioOutSRState);
    BF_SETV(AUDIOOUT_CTRL, CLKGATE, bAudioOutCGState);
}

void set_pcm_audio_bit(int bit)
{
	if (bit == 8)
		DPRINTK("8 bit is not support\n");
	else if (bit == 16)
		HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 1;
	else if (bit == 32)
		HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 0;
}

int adc_sample_width_set(unsigned long width)
{
    int ret = 0;

    switch (width)
    {
        case 16:
            BF_SET(AUDIOIN_CTRL, WORD_LENGTH);
            break;
        case 32:
            BF_CLR(AUDIOIN_CTRL, WORD_LENGTH);
            break;
        default:
            ret = -1;
    }

    if (ret != -1)
    {
        dev_adc_cfg.sample_width = width;
    }

    return ret;
}

int adc_sample_rate_set(unsigned long rate)
{
    int ret = 0;
    unsigned long BASE_RATE_MULT = 0x4;
    unsigned long HOLD = 0x0;
    unsigned long SAMPLE_RATE_INT = 0xF;
    unsigned long SAMPLE_RATE_FRAC = 0x13FF;

    switch (rate)
    {
        case 8000:
            BASE_RATE_MULT = 1;
            HOLD = 3;
            SAMPLE_RATE_INT = 0x17;
            SAMPLE_RATE_FRAC = 0x0e00;
            break;

        case 11025:
            BASE_RATE_MULT = 1;
            HOLD = 3;
            SAMPLE_RATE_INT = 0x11;
            SAMPLE_RATE_FRAC = 0x37;
            break;

        case 12000:
            BASE_RATE_MULT = 1;
            HOLD = 3;
            SAMPLE_RATE_INT = 0xf;
            SAMPLE_RATE_FRAC = 0x13ff;
            break;

        case 16000:
            BASE_RATE_MULT = 1;
            HOLD = 1;
            SAMPLE_RATE_INT = 0x17;
            SAMPLE_RATE_FRAC = 0x0e00;
            break;

        case 22050:
            BASE_RATE_MULT = 1;
            HOLD = 1;
            SAMPLE_RATE_INT = 0x11;
            SAMPLE_RATE_FRAC = 0x37;
            break;

        case 24000:
            BASE_RATE_MULT = 1;
            HOLD = 1;
            SAMPLE_RATE_INT = 0xf;
            SAMPLE_RATE_FRAC = 0x13ff;
            break;

        case 32000:
            BASE_RATE_MULT = 1;
            HOLD = 0;
            SAMPLE_RATE_INT = 0x17;
            SAMPLE_RATE_FRAC = 0x0e00;
            break;

        case 44100:
            BASE_RATE_MULT = 1;
            HOLD = 0;
            SAMPLE_RATE_INT = 0x11;
            SAMPLE_RATE_FRAC = 0x37;
            break;

        case 48000:
            BASE_RATE_MULT = 1;
            HOLD = 0;
            SAMPLE_RATE_INT = 0xf;
            SAMPLE_RATE_FRAC = 0x13ff;
            break;

        case 64000:
            BASE_RATE_MULT = 2;
            HOLD = 0;
            SAMPLE_RATE_INT = 0x17;
            SAMPLE_RATE_FRAC = 0x0e00;
            break;

        case 88200:
            BASE_RATE_MULT = 2;
            HOLD = 0;
            SAMPLE_RATE_INT = 0x11;
            SAMPLE_RATE_FRAC = 0x37;
            break;

        case 96000:
            BASE_RATE_MULT = 2;
            HOLD = 0;
            SAMPLE_RATE_INT = 0xf;
            SAMPLE_RATE_FRAC = 0x13ff;
            break;

        case 128000:
            BASE_RATE_MULT = 4;
            HOLD = 0;
            SAMPLE_RATE_INT = 0x11;
            SAMPLE_RATE_FRAC = 0x37;
            break;

        case 176400:
            BASE_RATE_MULT = 4;
            HOLD = 0;
            SAMPLE_RATE_INT = 0x11;
            SAMPLE_RATE_FRAC = 0x37;
            break;

        case 192000:
            BASE_RATE_MULT = 4;
            HOLD = 0;
            SAMPLE_RATE_INT = 0xf;
            SAMPLE_RATE_FRAC = 0x13ff;
            break;

        default:
            ret = -1;
            break;
    }

    BF_CS4(AUDIOIN_ADCSRR, SRC_HOLD, HOLD, SRC_INT, SAMPLE_RATE_INT, SRC_FRAC,
           SAMPLE_RATE_FRAC, BASEMULT, BASE_RATE_MULT);

    if (ret != -1)
    {
        dev_adc_cfg.sample_rate = rate;
    }

    return ret;
}

void hw_audioin_MuteAdcInputChannels(bool bValue)
{
    BF_CLR(AUDIOIN_ADCVOL, MUTE);
    BF_SETV(AUDIOIN_ADCVOL, MUTE, bValue);
}

void adc_init( unsigned long sample_rate, unsigned long sample_width )
{
    unsigned char u8_WaitCount;
    bool bAudioOutCGState;
    
    BF_CLR(CLKCTRL_XTAL, FILT_CLK24M_GATE); 

    /* Clear SFTRST, Turn off Clock Gating, Clear ADC powerdown (and so
        also power up the AudioOut circuitry */
    HW_AUDIOIN_CTRL_CLR(BM_AUDIOIN_CTRL_SFTRST | BM_AUDIOIN_CTRL_CLKGATE);

    /* Enable the ADC */
    hw_audioin_PowerdownADC(false);

    // Get the current Clock gate status from AUDIO-OUT
    bAudioOutCGState = BF_RD(AUDIOOUT_CTRL, CLKGATE);
    
    /* AudioOut contains several ADC control bits, so make sure it is enabled */
    HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_SFTRST | BM_AUDIOOUT_CTRL_CLKGATE);

    /* Set the ADC VAG */
    //BF_CS4(AUDIOOUT_REFCTRL, ADJ_ADC, 1, ADC_REFVAL, 0xF, ADJ_VAG, 1, VAG_VAL, 0xB);

    /* Return AudioOut Clock Gate to previous state */
    BF_SETV(AUDIOOUT_CTRL, CLKGATE, bAudioOutCGState);

    /* Enable the APBX DMA Engine */
    HW_APBX_CTRL0_CLR(BM_APBX_CTRL0_SFTRST | BM_APBX_CTRL0_CLKGATE);

    /* Reset the AudioIn channel before initialization */
    BF_SETV(APBX_CTRL0, RESET_CHANNEL, BV_APBX_CTRL0_RESET_CHANNEL__AUDIOIN);

    /* Poll for the AudioIn channel to complete reset. */
    for(u8_WaitCount = 0;
            ((u8_WaitCount < RESET_WAIT_COUNT_MAX) &&
        (0!=((BF_RD(APBX_CTRL0, RESET_CHANNEL))&BV_APBX_CTRL0_RESET_CHANNEL__AUDIOIN)));
                                                        u8_WaitCount++);

    /* Enable the APBX Ch0 IRQ bit */
    BF_SET(APBX_CTRL1, CH0_CMDCMPLT_IRQ_EN);

    /* Set up the ADC Word Length */
    adc_sample_width_set(sample_width);

    /* Set Sample Rate  */
    adc_sample_rate_set(sample_rate);

    /* Set defaults */
    BF_CS4(AUDIOIN_CTRL, DMAWAIT_COUNT, 0, LR_SWAP, 0, EDGE_SYNC, 0,
            INVERT_1BIT, 0);
    
    BF_CS3(AUDIOIN_CTRL, OFFSET_ENABLE, 1, HPF_ENABLE, 1, LOOPBACK, 0);

    mdelay(250);

    //HW_AUDIOIN_CTRL.B.HPF_ENABLE = 1;
    //HW_AUDIOIN_CTRL.B.OFFSET_ENABLE = 0;

    /* Enable the ADC Convertor & Dig Filter */
    BF_CLR(AUDIOIN_ANACLKCTRL, CLKGATE);

    HW_AUDIOOUT_REFCTRL.B.ADJ_ADC = 1;
    HW_AUDIOOUT_REFCTRL.B.ADC_REFVAL = 0xF;

    /* Unmute the ADC Input */
    hw_audioin_MuteAdcInputChannels(false);

    // The MUTE_LEFT and MUTE_RIGHT fields need to be cleared for TA5.
    // Unfortunately the header files represent TB1, thus the hardcode.
    HW_AUDIOIN_ADCVOLUME_CLR(0x01000100);

    /* Set the Input channel gain */
    BF_CLR(AUDIOIN_ADCVOL, GAIN_LEFT);
    BF_SETV(AUDIOIN_ADCVOL, GAIN_LEFT, 2);
    BF_CLR(AUDIOIN_ADCVOL, GAIN_RIGHT);
    BF_SETV(AUDIOIN_ADCVOL, GAIN_RIGHT, 2);

    /* Set the ADC Input source */
    BF_CLR(AUDIOIN_ADCVOL, SELECT_LEFT);
    BF_SETV(AUDIOIN_ADCVOL, SELECT_LEFT, 1);
    BF_CLR(AUDIOIN_ADCVOL, SELECT_RIGHT);
    BF_SETV(AUDIOIN_ADCVOL, SELECT_RIGHT, 1);

    /* Update volume on zero crossings */
    BF_SET(AUDIOIN_ADCVOLUME, EN_ZCD);

    /* Set the output volume */
    BF_CLR(AUDIOIN_ADCVOLUME, VOLUME_LEFT);
    BF_SETV(AUDIOIN_ADCVOLUME, VOLUME_LEFT, 0xFE);
    BF_CLR(AUDIOIN_ADCVOLUME, VOLUME_RIGHT);
    BF_SETV(AUDIOIN_ADCVOLUME, VOLUME_RIGHT, 0xFE);
}

static void dac_init(unsigned long sample_rate, unsigned long sample_width)
{
	DPRINTK("\n");
	// Keep audio input off
	HW_AUDIOOUT_CTRL.B.CLKGATE = false;
	HW_AUDIOIN_CTRL.B.CLKGATE = false;
	HW_AUDIOIN_CTRL.B.SFTRST = false;
	HW_AUDIOOUT_PWRDN.B.RIGHT_ADC = true;
	HW_AUDIOOUT_PWRDN.B.ADC = true;
	HW_AUDIOOUT_PWRDN.B.CAPLESS = true;
	HW_AUDIOOUT_ANACTRL.B.HP_HOLD_GND = true;	// Prevent Digital Noise

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

	HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 1;	// 16-bit PCM samples
	HW_AUDIOOUT_CTRL.B.DMAWAIT_COUNT = 16;	// @todo fine-tune this 

	// Transfer XTAL bias to bandgap (power saving)
	//HW_AUDIOOUT_REFCTRL.B.XTAL_BGR_BIAS = 1;
	// Transfer DAC bias from self-bias to bandgap (power saving)
	HW_AUDIOOUT_PWRDN.B.SELFBIAS = 1;

	HW_AUDIOIN_CTRL.B.CLKGATE = true;
	HW_AUDIOOUT_CTRL.B.CLKGATE = false;
	
	msleep(2);
}

static void dac_uninit (void)
{
	BF_SET(AUDIOOUT_ANACLKCTRL, CLKGATE);
    HW_AUDIOOUT_CTRL_SET(BM_AUDIOOUT_CTRL_CLKGATE);
}

// Smoothly ramp the last value in the DAC to zero
void dac_to_zero()
{
	unsigned dac_value = HW_AUDIOOUT_DATA_RD();
	short high = (short)(dac_value >> 16);
	short low = (short)(dac_value & 0xffff);

	DPRINTK("dac_to_zero\n");

	while (high != 0 || low != 0) {
		unsigned ulow = (unsigned)low;
		unsigned uhigh = (unsigned)high;
		unsigned sample = (uhigh << 16) + ulow;

		while (true) {
			if (HW_AUDIOOUT_DACDEBUG.B.FIFO_STATUS == 1) {
				HW_AUDIOOUT_DATA_WR(sample);
				break;
			}
		}

		if (high < 0)
			++high;
		else if (high > 0)
			--high;

		if (low < 0)
			++low;
		else if (low > 0)
			--low;
	}
}


void dac_ramp_to_ground()
{
	int16_t s16DacValue;

	DPRINTK(" dac_ramp_to_ground()\n");

	dac_to_zero();

	s16DacValue = 0;

	dac_blat(s16DacValue, 512);
	// Enabling DAC circuitry
	//BF_CS2(AUDIOOUT_DACVOLUME, VOLUME_LEFT, DAC_MAX_VOLUME,	// Set max volume
	//       VOLUME_RIGHT, DAC_MAX_VOLUME);
	//SetGain(0);
	BF_CS2(AUDIOOUT_DACVOLUME, MUTE_LEFT, 0, MUTE_RIGHT, 0);	// Unmute DAC
	BF_CLR(AUDIOOUT_HPVOL, MUTE);	// Unmute HP

	// Write DAC to VAG for several milliseconds
	dac_blat(s16DacValue, 512);

	dac_ramp_partial(&s16DacValue, 0x7fff, 1);

	dac_blat(0x7fff, 512);
}


void dac_power_hp(bool on)
{
	DPRINTK("\n");

	if (dev_dac_cfg.hp_on != on) {
		dev_dac_cfg.hp_on = on;

		if (on) {
			HW_AUDIOOUT_PWRDN.B.DAC = false;
			HW_AUDIOOUT_ANACLKCTRL.B.CLKGATE = false;

			//ASSERT( HW_AUDIOOUT_STAT.B.DAC_PRESENT ); // Check the "DAC present" bit

			HW_AUDIOOUT_CTRL.B.WORD_LENGTH = 1;	// 16-bit PCM samples
			HW_AUDIOOUT_CTRL.B.DMAWAIT_COUNT = 16;	// @todo fine-tune this        
			dac_vol_init();
			dac_ramp_to_vag();

			volume_set(VC_DAC, dev_dac_cfg.volume);


			//dac_power_lineout(true);
		} else {
			//dac_power_lineout(false);

			// Max out headphone amp first
			HW_AUDIOOUT_HPVOL.B.MUTE = 0;
			HW_AUDIOOUT_HPVOL.B.VOL_LEFT = 0;
			HW_AUDIOOUT_HPVOL.B.VOL_RIGHT = 0;

			dac_ramp_to_ground();

			BF_CS1(AUDIOOUT_ANACTRL, HP_HOLD_GND, true);	// Set hold to GND      

			HW_AUDIOOUT_PWRDN.B.HEADPHONE = true;	// Power down headphones
			HW_AUDIOOUT_PWRDN.B.LINEOUT = true;	// Power down lineout

			BF_SET(AUDIOOUT_CTRL, DAC_ZERO_ENABLE);	// Start sending zeros                
			BF_SET(AUDIOOUT_HPVOL, MUTE);	// mute HP        
			BF_CS2(AUDIOOUT_DACVOLUME, MUTE_LEFT, 0, MUTE_RIGHT, 0);	// mute DAC        
			BF_CS2(AUDIOOUT_DACVOLUME, VOLUME_LEFT, 0,	// Set min volume
			       VOLUME_RIGHT, 0);
			BF_CS1(AUDIOOUT_ANACTRL, HP_CLASSAB, false);	// Disable ClassAB    

			BF_SET(AUDIOOUT_PWRDN, DAC);	// Power down DAC
			BF_SET(AUDIOOUT_PWRDN, CAPLESS);	// Power down Capless
	
		}
	}
}

void dac_power_lineout(bool up)
{
	int vol;

	if (up) {
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
		for (vol = 0x1E; vol >= 0; vol--) {
			HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT = vol;
			HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_RIGHT = vol;
		}
	} else {
		// Ramp Down volume
		for (vol = HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT; vol < 0x1F;
		     vol++) {
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

void adc_start(void)
{
    DPRINTK(" adc_start()\n");

    if (!dev_adc_cfg.running)
    {
        // Init the chain and enabled the DMA
        dma_chain_init(&dev_adc_cfg);
        dma_enable(&dev_adc_cfg);
        BF_WRn(APBX_CHn_SEMA, ADC_DMA_CHANNEL, INCREMENT_SEMA, CHAIN_LENGTH);

        HW_AUDIOIN_CTRL_SET(BM_AUDIOIN_CTRL_RUN);
        dev_adc_cfg.running = true;
    }
}

void dac_start(void)
{
	DPRINTK("\n");
	dac_power_hp(true);
	dev_dac_cfg.running = true;
}

void adc_stop(void)
{
    DPRINTK(" adc_stop()\n");
    HW_AUDIOIN_CTRL_CLR(BM_AUDIOIN_CTRL_RUN);
    dev_adc_cfg.running = false;
}

int calc_to_dac_volume(int vol)
{
	int ret = 0;

	ret = (vol * (DAC_MAX_VOLUME - DAC_MIN_VOLUME) / 100) + DAC_MIN_VOLUME;
	
	return ret;
}

int calc_to_usr_volume(int vol)
{
	int ret = 0;
	
	ret = vol / ((DAC_MAX_VOLUME - DAC_MIN_VOLUME) / 100) - DAC_MIN_VOLUME;

	return ret;
}

void adc_volume_set(uint8_t new_vol)
{
     if( new_vol > ADC_MAX_VOLUME )
         new_vol = ADC_MAX_VOLUME;
     else if( new_vol < ADC_MIN_VOLUME )
         new_vol = ADC_MIN_VOLUME;

    BF_WR(AUDIOIN_ADCVOLUME, VOLUME_LEFT, new_vol);
    BF_WR(AUDIOIN_ADCVOLUME, VOLUME_RIGHT, new_vol);
}

void dac_stop(void)
{
	DPRINTK("\n");
	dac_power_hp(false);
	dev_dac_cfg.running = false;
}

const unsigned int DAC_CM_VOL_TBL[31] =
{
	1,
	34, 39, 43, 47, 51, 54, 57, 60, 62, 64, 
	66, 68, 70, 72, 74, 75, 76, 77, 78, 79, 
	80, 81, 82, 83, 85, 87, 89, 91, 93, 95
};



void volume_set(VOL_CTL_ID vol_ctrl_id, uint8_t new_vol)
{
	new_vol = DAC_CM_VOL_TBL[new_vol]<<1;

	dev_dac_cfg.volume = new_vol;

	switch (vol_ctrl_id) {
	case VC_DAC:
		new_vol = calc_to_dac_volume(new_vol);
		printk("\ndac calc(setting) volume %d\n", new_vol);
		// Apply limits.
		if (new_vol > DAC_MAX_VOLUME)
			new_vol = DAC_MAX_VOLUME;
		else if (new_vol < DAC_MIN_VOLUME)
			new_vol = DAC_MIN_VOLUME;

		// Apply at last
		HW_AUDIOOUT_DACVOLUME.B.MUTE_LEFT = (new_vol == 0) ? 1 : 0;
		HW_AUDIOOUT_DACVOLUME.B.MUTE_RIGHT = (new_vol == 0) ? 1 : 0;
		HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT = new_vol;
		HW_AUDIOOUT_DACVOLUME.B.VOLUME_RIGHT = new_vol;
	
		break;

	case VC_HEADPHONE:
		// Apply limits.
		if (new_vol > HP_MAX_VOLUME)
			new_vol = HP_MAX_VOLUME;
		// Invert range 
		new_vol = -((int)new_vol - HP_MAX_VOLUME);
		printk("hp calc(setting) volume %d\n", new_vol);
		// Apply at last
		HW_AUDIOOUT_HPVOL.B.MUTE = (new_vol == HP_MAX_VOLUME) ? 1 : 0;
		HW_AUDIOOUT_HPVOL.B.VOL_LEFT = new_vol;
		HW_AUDIOOUT_HPVOL.B.VOL_RIGHT = new_vol;

		break;

	case VC_LINEOUT:
		// Apply limits.
		if (new_vol > LO_MAX_VOLUME)
			new_vol = LO_MAX_VOLUME;

		// Invert range 
		new_vol = -((int)new_vol - LO_MAX_VOLUME);

		// Apply at last
		HW_AUDIOOUT_LINEOUTCTRL.B.MUTE =
		    (new_vol == LO_MAX_VOLUME) ? 1 : 0;
		HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT = new_vol;
		HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_RIGHT = new_vol;
		break;
	}
}

void dac_select_src(HP_SOURCE hp_source) {
	BF_WR(AUDIOOUT_HPVOL, SELECT, hp_source);
}

void mute_cntl(unsigned int mute_val) //dhsong
{

//#define MUTEON 
#ifndef MUTEON
        static unsigned int bank = 0;
        static unsigned int pin = 21;
        static unsigned int i = 1;
 #if 0
        HW_PINCTRL_MUXSEL0_CLR(0x3 << pin*2);
        udelay(5);
        HW_PINCTRL_MUXSEL0_SET(0x3 << pin*2);
        udelay(5);

        HW_PINCTRL_DOE0_CLR(1 << 21);
        HW_PINCTRL_DOE0_SET(1 << 21);
        for (i = 0; i < 11; i++)
        {
                // Send pulse
                HW_PINCTRL_DOUT0_CLR(1 << pin);
                udelay(5);
                HW_PINCTRL_DOUT0_SET(1 << pin);
                udelay(5);
        }
 #else
        stmp37xx_gpio_set_af( pin_GPIO(bank,  pin), GPIO_MODE); //muxsel set gpio func
        stmp37xx_gpio_set_dir( pin_GPIO(bank,  pin), GPIO_DIR_OUT); //output
	
        stmp37xx_gpio_set_level( pin_GPIO(bank,  pin), mute_val); //if val=1=mute off
 #endif
#endif //MUTEON

}

void mute(VOL_CTL_ID vol_ctrl_id)
{
	DPRINTK(" mute(%d)\n", vol_ctrl_id);

	switch (vol_ctrl_id) {
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


void unmute(VOL_CTL_ID vol_ctrl_id)
{
	DPRINTK(" unmute(%d)\n", vol_ctrl_id);

	switch (vol_ctrl_id) {
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

void calculate_samplerate_params(unsigned long rate,
				 unsigned long *ret_basemult,
				 long unsigned *ret_src_hold,
				 unsigned long *ret_src_int,
				 unsigned long *ret_src_frac)
{
	unsigned long basemult, src_hold, src_int, src_frac;
	unsigned long long num, den, divide, mod;

	num = 10;
	den = 5;
	divide = do_div(num, den);

	basemult = 1;
	while (rate > 48000) {
		rate /= 2;
		basemult *= 2;
	}
	src_hold = 1;
	while (rate <= 24000) {
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
	src_int = divide >> 13;	// for the integer portion
	src_frac = divide & 0x1FFF;	// the fractional portion    

	*ret_basemult = basemult;
	*ret_src_hold = src_hold;
	*ret_src_int = src_int;
	*ret_src_frac = src_frac;
}


int dac_sample_rate_set(unsigned long rate)
{
	unsigned long base_rate_mult, hold, sample_rate_int, sample_rate_frac;

	DPRINTK(" dac_sample_rate_set(%d)\n", (int)rate);

	calculate_samplerate_params(rate, &base_rate_mult, &hold,
				    &sample_rate_int, &sample_rate_frac);

	BF_CS4(AUDIOOUT_DACSRR, SRC_HOLD, hold, SRC_INT, sample_rate_int,
	       SRC_FRAC, sample_rate_frac, BASEMULT, base_rate_mult);

	dev_dac_cfg.sample_rate = rate;

	return 0;
}

static int stmp37xx_open(struct inode * inode, struct file * file)
{
	int cold = !audio_active;
	DPRINTK("\n");

	if ((file->f_flags & O_ACCMODE) == O_RDONLY) {
		if (audio_rd_refcount) {
			printk("stmp37xx device already open for RDONLY\n");
			return -EBUSY;
		}
		++audio_rd_refcount;
	} else if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
		if (audio_wr_refcount) {
			printk("stmp37xx device already open for WRONLY\n");
			return -EBUSY;
		}
		++audio_wr_refcount;
	} else if ((file->f_flags & O_ACCMODE) == O_RDWR) {
		if (audio_rd_refcount || audio_wr_refcount) {
		printk("stmp37xx device already open for RDWR\n");
			return -EBUSY; 
		}
		++audio_rd_refcount;
		++audio_wr_refcount;
	} else
		return -EINVAL;

	if (audio_wr_refcount) {
		dma_chain_init(&dev_dac_cfg);
		// load dma chain into dac dma machine, enable dac dma interrupts
		dma_enable(&dev_dac_cfg);
	}

	if (dev_dac_cfg.init_device == 0) {
		dac_init(DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_WIDTH);
		dev_dac_cfg.init_device = 1;
	}

	if (cold) {
		DPRINTK(" cold\n");
	}

	//adc_start();

	return 0;
}

static unsigned int stmp37xx_poll(struct file * file, struct poll_table_struct * wait)
{
	int ret = 0;
	DPRINTK("\n");

	poll_wait(file, &dev_dac_cfg.wait_q, wait);

	spin_lock_irq(&dev_dac_cfg.irq_lock);
	// Check if there's space in the DMA chain
	if (dev_dac_cfg.dma_chain->inuse_count < CHAIN_LENGTH)
		ret |= (POLLOUT | POLLWRNORM);

	// Has our status changed?   
	if (dac_status.status)
		ret |= (POLLIN | POLLRDNORM);

	spin_unlock_irq(&dev_dac_cfg.irq_lock);

	return ret;
}

static ssize_t stmp37xx_read(struct file * file, char * buffer,
					size_t size, loff_t * offp)				
{
	int copy_ok;

	DPRINTK("\n");
	//if (dev_adc_cfg.init_device == 0) {
	//	adc_init(DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_WIDTH);
	//	dev_adc_cfg.init_device = 1;
	//}

    // if the dma chain is empty, sleep until isr wakes us up, unless we're
    // non-blocking mode
    if (dev_adc_cfg.dma_chain->inuse_count == 0)
    {
        if (file->f_flags & O_NONBLOCK)
        {
            DPRINTK(" non blocking and no data to read()\n");
            return -EAGAIN;
        }

        interruptible_sleep_on(&dev_adc_cfg.wait_q);
        /* If the sleep was terminated by a signal give up */
        if (signal_pending(get_current()))
            return -ERESTARTSYS;
    }

    spin_lock(dev_adc_cfg.lock);

    // copy the data from dma buffer to user space
    copy_ok = copy_to_user(buffer, dev_adc_cfg.dma_chain->rd_ptr->data, size);

    // mark the dma block as available, isr will make unavailable after ADC dma
    // completes
    dev_adc_cfg.dma_chain->rd_ptr->inuse = false;

    dev_adc_cfg.dma_chain->inuse_count--;


    // get setup for the next dma block, wrap write pointer if necessary
    if (dev_adc_cfg.dma_chain->rd_ptr == dev_adc_cfg.dma_chain->last_ptr)
    {
        dev_adc_cfg.dma_chain->rd_ptr = dev_adc_cfg.dma_chain->first_ptr;
    }
    else
    {
        dev_adc_cfg.dma_chain->rd_ptr++;
    }

    //kick dma (increment dma descriptor semaphore)
    BF_WRn(APBX_CHn_SEMA, ADC_DMA_CHANNEL, INCREMENT_SEMA, 1);

    spin_unlock(dev_adc_cfg.lock);

    return size;
}

void dac_send(uint16_t * data, size_t size)
{
	hw_apbx_chn_cmd_t dma_cmd;

	//DPRINTK("\n");

	spin_lock(dev_dac_cfg.lock);

	if (!dev_dac_cfg.running) {
		dac_start();
	}

	DPRINTK(" - send from dma dd: %p : %d : %d\n",
	       dev_dac_cfg.dma_chain->wr_ptr, dev_dac_cfg.dma_chain->inuse_count, size);

	// mark the dma block as in use, isr will make available after DAC dma
	// completes
	dev_dac_cfg.dma_chain->wr_ptr->inuse = true;
	dev_dac_cfg.dma_chain->inuse_count++;

	// set descriptor's data size
	dma_cmd.U = dev_dac_cfg.dma_chain->wr_ptr->dd.cmd.U;
	dma_cmd.B.XFER_COUNT = size;
	dev_dac_cfg.dma_chain->wr_ptr->dd.cmd = dma_cmd;

	// get setup for the next dma block, wrap write pointer if necessary
	if (dev_dac_cfg.dma_chain->wr_ptr == dev_dac_cfg.dma_chain->last_ptr) {
		dev_dac_cfg.dma_chain->wr_ptr = dev_dac_cfg.dma_chain->first_ptr;
	} else {
		dev_dac_cfg.dma_chain->wr_ptr++;
	}

	// Renable the output
	//HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_DAC_ZERO_ENABLE);

	//kick dma (increment dma descriptor semaphore)
	BF_WRn(APBX_CHn_SEMA, DAC_DMA_CHANNEL, INCREMENT_SEMA, 1);

	HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ);

	session_activatedbytes += size;

	spin_unlock(dev_dac_cfg.lock);
}

static inline ssize_t stmp37xx_write_oneblock (struct file * file, const char * buffer, size_t size)
{
	int ret = 0;
	DPRINTK("\n");

	if (dev_dac_cfg.dma_chain->inuse_count == CHAIN_LENGTH) {
		if (file->f_flags & O_NONBLOCK) {
			printk("O_NONBLOCK!\n");
			if (ret > 0) return ret;
			return -EAGAIN;
		}

		/* wait for a buffer to become free */
		// otherwise atomically wait until the dma chain is non-full
		if (wait_event_interruptible(dev_dac_cfg.wait_q, dev_dac_cfg.dma_chain->inuse_count < CHAIN_LENGTH))
			return -ERESTARTSYS;
	}
	
	// only allow writes that will fit in allocated buffer space
	if (size > sizeof(dma_buf_t)) {
		size = sizeof(dma_buf_t);
	}
	
	// copy the data to dma buffer
	ret = copy_from_user(dev_dac_cfg.dma_chain->wr_ptr->data, buffer, size);
	
	dac_send(dev_dac_cfg.dma_chain->wr_ptr->data, size);

	return size;
}

static ssize_t stmp37xx_write(struct file * file, const char * buffer, size_t size, loff_t * offp)
{
	size_t sz_written = 0;
	DPRINTK("\n");

	do
	{
		 int cnt;
		 if ((cnt = stmp37xx_write_oneblock(file, buffer, size - sz_written)) <= 0)
			 break;
		sz_written += cnt;
		buffer+=cnt;
	} 
	while (sz_written < size);

	return sz_written;
}

static int audio_sync(struct file *file)
{
	wait_event_interruptible(dev_dac_cfg.wait_q, 
		dev_dac_cfg.dma_chain->inuse_count == 0);

        return 0;
}

static int stmp37xx_dsp_ioctl(struct inode *inode, struct file *file, 
			  unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long val;
	stmp3xxx_dac_queue_state_t state;
	audio_buf_info inf;

	DPRINTK(" stmp37xx_dsp_ioctl() - cmd: 0x%X, arg: 0x%X\n", cmd, (int)arg);

	switch (cmd) {
	case OSS_GETVERSION:
		return put_user(SOUND_VERSION, (int *)arg);

	case SNDCTL_DSP_SETFMT:
		if (get_user(val, (int *) arg))
			return -EINVAL;
		set_pcm_audio_bit(val);
		break;
	case SNDCTL_DSP_GETFMTS:
		/* Simple standard DACs are 16-bit only */
		return put_user(AUDIO_FMT, (long *) arg);

	case SNDCTL_DSP_GETBLKSIZE:
		return put_user(DMA_NUM_SAMPLES, (long *)arg);

	case SNDCTL_DSP_SETFRAGMENT:
		// not implemented
		return 0;

	case SNDCTL_DSP_GETOSPACE:
		spin_lock_irq(&dev_dac_cfg.irq_lock);
		get_dma_bytes_inuse(&state.remaining_now_bytes, &state.remaining_next_bytes);
		spin_unlock_irq(&dev_dac_cfg.irq_lock);

		inf.bytes = (sizeof(dma_buf_t)*CHAIN_LENGTH) - state.remaining_next_bytes;
		inf.fragments = inf.bytes/DMA_NUM_SAMPLES;
		inf.fragsize = DMA_NUM_SAMPLES;
		inf.fragstotal = CHAIN_LENGTH;

		ret = copy_to_user((void *)arg, &inf, sizeof(inf));
		break;

	case SNDCTL_DSP_GETODELAY:
		spin_lock_irq(&dev_dac_cfg.irq_lock);
		get_dma_bytes_inuse(&state.remaining_now_bytes, &state.remaining_next_bytes);
		spin_unlock_irq(&dev_dac_cfg.irq_lock);
		ret = put_user(state.remaining_next_bytes, (int*)arg);
		break;

	case SNDCTL_DSP_NONBLOCK:
		file->f_flags |= O_NONBLOCK;
		return 0;

	case SNDCTL_DSP_CHANNELS:
	case SNDCTL_DSP_STEREO:
#if 0 //add dhsong for debug
		printk("HW_AUDIOOUT_PWRDN 01= 0x%8x\n", HW_AUDIOOUT_PWRDN);
	BF_SET(AUDIOOUT_PWRDN, DAC);	// Power UP Capless
	msleep(1);
	adc_init(DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_WIDTH);
	BF_CLR(AUDIOOUT_PWRDN, ADC);	// Power UP Capless
	BF_CLR(AUDIOOUT_PWRDN, RIGHT_ADC);	
	msleep(1);
		printk("HW_AUDIOOUT_PWRDN 02= 0x%8x\n", HW_AUDIOOUT_PWRDN);

		printk("HW_SAIF_DATA 01= 0x%8x\n", HW_SAIF_DATA);
		HW_SAIF_DATA_SET(0x1 << 16);
		printk("HW_SAIF_DATA 02= 0x%8x\n", HW_SAIF_DATA);
#endif
		if (get_user(val, (long *) arg))
			return -EINVAL;
			if (cmd == SNDCTL_DSP_STEREO)
				val = val ? 2 : 1;
			if (val != 1 && val != 2)
				return -EINVAL;
			audio_channels = val;
                        break;

	case SOUND_PCM_READ_CHANNELS:
		if (put_user(audio_channels, (long *) arg))
			return -EINVAL;
		break;

	case SNDCTL_DSP_SPEED:
		if (get_user(val, (long *) arg))
			return -EINVAL;
		val = dac_sample_rate_set(val);
		if (val < 0)
			ret = -EINVAL;
		break;

	case SOUND_PCM_READ_RATE:
		// bug??
		return dev_dac_cfg.sample_rate;

	case SNDCTL_DSP_RESET:
		/* temporary implementation */
#if 0
		dma_disable();
		dma_chain_init(&dev_dac_cfg);
		dma_enable(&dev_dac_cfg);
#endif
		return 0;

	case SNDCTL_DSP_SYNC:
		return audio_sync(file);

	case STMP3XXX_DAC_IOCT_MUTE: //not oss standard, dhsong
		mute_cntl(arg);
		break;
	// ---------------- DAC IOCTLS (not oss) ---------------- //
	/*
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
		dac_select_src((HP_SOURCE) arg);
		break;

	case STMP3XXX_DAC_IOCQ_VOL_GET:
		switch (arg) {
		case VC_DAC:
			ret = HW_AUDIOOUT_DACVOLUME.B.VOLUME_LEFT;
			break;

		case VC_HEADPHONE:
			ret =
			    -((int)HW_AUDIOOUT_HPVOL.B.VOL_LEFT -
			      HP_MAX_VOLUME);
			break;

		case VC_LINEOUT:
			ret =
			    -((int)HW_AUDIOOUT_LINEOUTCTRL.B.VOLUME_LEFT -
			      LO_MAX_VOLUME);
			break;

		default:
			break;
		}
		break;

	case STMP3XXX_DAC_IOCQ_VOL_RANGE_GET:
		switch (arg) {
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
		switch (arg) {
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
		ret = dev_dac_cfg.sample_rate;
		break;

	case STMP3XXX_DAC_IOCT_EXPECT_UNDERRUN:
		spin_lock_irq(&dev_dac_cfg.irq_lock);
		expect_underrun = (bool) arg;
		break;

	case STMP3XXX_DAC_IOCQ_ACTIVE:
		spin_lock_irq(&dev_dac_cfg.irq_lock);
		ret = dev_dac_cfg.dma_chain->inuse_count;
		spin_unlock_irq(&dev_dac_cfg.irq_lock);
		break;

	case STMP3XXX_DAC_IOCT_INIT:
		if (!dev_dac_cfg.running)
			dac_start();
		break;

	case STMP3XXX_DAC_IOCS_QUEUE_STATE:
		{
			stmp3xxx_dac_queue_state_t state;
			state.total_bytes = sizeof(dma_buf_t) * CHAIN_LENGTH;
			spin_lock_irq(&dev_dac_cfg.irq_lock);
			get_dma_bytes_inuse(&state.remaining_now_bytes,
					    &state.remaining_next_bytes);
			spin_unlock_irq(&dev_dac_cfg.irq_lock);
			ret = copy_to_user((void *)arg, &state, sizeof(state));
			break;
		}
	*/

	case SNDCTL_DSP_DIAGNOSIS_TEST:
		// not implemented
		break;
	/* to control volume when postprocessing is running */
	case SNDCTL_DSP_NORMAL_PROCESSING:
		break;
	case SNDCTL_DSP_DNSE_PROCESSING:
		break;
	case SNDCTL_DSP_SET_DEFAULT_VOL:
		if (get_user(val, (long *) arg))
			return -EFAULT;
		volume_set(VC_DAC, val);
		break;
	case SNDCTL_DSP_SET_PLAY_SPEED:	//eqal with SNDCTL_DSP_SPEED
		if (get_user(val, (long *) arg))
			return -EINVAL;
		val = dac_sample_rate_set(val);
		if (val < 0)
			ret = -EINVAL;
		break;
	
	// ---------------- DAC IOCTLS (not oss) ---------------- //
	/*
	case STMP3XXX_ADC_IOCT_START:
	    adc_start();
	    break;

	case STMP3XXX_ADC_IOCT_STOP:
	    adc_stop();
	    break;

	case STMP3XXX_ADC_IOCT_VOL_SET:
	    adc_volume_set((uint8_t)arg);
	    break;

   case STMP3XXX_ADC_IOCT_SAMPLE_RATE:
	    adc_sample_rate_set(arg);
	    break;

	case STMP3XXX_ADC_IOCQ_SAMPLE_RATE:
	    ret = dev_adc_cfg.sample_rate;
	    break;

	case STMP3XXX_ADC_IOCQ_VOL_GET:
	    ret = BF_RD(AUDIOIN_ADCVOLUME, VOLUME_LEFT);
	    break;

	case STMP3XXX_ADC_IOCQ_VOL_RANGE_GET:
	    ret = (ADC_MAX_VOLUME << 16) | (ADC_MIN_VOLUME);
	    break;
	*/
	case SNDCTL_DSP_GETOPTR:
	case SNDCTL_DSP_GETIPTR:
	case SNDCTL_DSP_GETISPACE:
	case SNDCTL_DSP_GETCAPS:
	case SNDCTL_DSP_POST:
	case SNDCTL_DSP_GETTRIGGER:
	case SNDCTL_DSP_SETTRIGGER:
	case SNDCTL_DSP_SUBDIVIDE:
	//case SNDCTL_DSP_MAPINBUF:
	case SNDCTL_DSP_MAPOUTBUF:
	case SNDCTL_DSP_SETSYNCRO:
	case SNDCTL_DSP_SETDUPLEX:
	default:
		//ret = -ENOTTY;
		return stmp37xx_mix_ioctl(inode, file, cmd, arg);
	}

	return ret;
}

static int stmp37xx_mmap (struct file *file, struct vm_area_struct *vma)
{
	extern int ocram_mmap(struct file *file, struct vm_area_struct *vma);
	return ocram_mmap(file,vma);
}

static int stmp37xx_release(struct inode * inode, struct file * file)
{
	DPRINTK("\n");

	switch (file->f_flags & O_ACCMODE) {
		case O_RDONLY:
		case O_RDWR:
			if(audio_rd_refcount)
				--audio_rd_refcount;
	}

	switch (file->f_flags & O_ACCMODE) {
		case O_WRONLY:
		case O_RDWR:
			if (audio_wr_refcount) {
				audio_sync(file);
				dac_stop();
				dac_uninit();
				--audio_wr_refcount;
			}
	}
	dev_adc_cfg.init_device = 0;
	dev_dac_cfg.init_device = 0;

	return 0;
}


struct file_operations stmp37xx_dsp_fops = {
	.read = stmp37xx_read,
	.write = stmp37xx_write,
	.ioctl = stmp37xx_dsp_ioctl,
	.open = stmp37xx_open,
	.poll = stmp37xx_poll,
	.release = stmp37xx_release,
	.mmap = stmp37xx_mmap
};

static int stmp37xx_mix_ioctl(struct inode *inode, struct file *file, 
			  unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int nr = _IOC_NR(cmd);
	int val = 0;

	DPRINTK(" stmp37xx_mix_ioctl() - cmd: 0x%X, arg: 0x%X\n", cmd, (int)arg);

	if (_SIOC_DIR(cmd) & _SIOC_WRITE ) {
		switch (nr) {
			case SOUND_MIXER_VOLUME:
			case SOUND_MIXER_PCM:
				ret = get_user(val, (int *)arg);
				if (ret)
					break;
				/* use only left volume for now*/
			//	val &= 0xff;
				printk("DAC volume [%d]\n", val);
#if 1 //dhsong
				volume_set(VC_DAC, val);
				printk("HW_AUDIOOUT_PWRDN = 0x%8x\n",HW_AUDIOOUT_PWRDN); 
#else
			        HW_AUDIOOUT_DACVOLUME_SET(0xfe <<  16); //set to default volume max 0xFE,
			        HW_AUDIOOUT_DACVOLUME_SET(0xfe <<  0); //set to default volume max 0xFE, 
			        printk("\n\nHW_AUDIOOUT_DACVOLUME_SET = 0x%8x\n\n", HW_AUDIOOUT_DACVOLUME);
			
			        HW_AUDIOOUT_HPVOL_SET(0x00 <<  8); //set to default volume max 0x00,
			        HW_AUDIOOUT_HPVOL_SET(0x00 <<  0); //set to default volume max 0x00,
			        printk("\n\nHW_AUDIOOUT_HPVOL_SET = 0x%8x\n\n", HW_AUDIOOUT_HPVOL);
#endif
				break;
	
			case SOUND_MIXER_RECSRC:
				break;
			case SOUND_MIXER_MIC_GAIN:
				break;
			case SOUND_MIXER_MIC_BIAS:
				break;
			case SOUND_MIXER_ADCVOL_GAIN:
				break;
			case SOUND_MIXER_LINE:
				break;
			case SOUND_MIXER_MIC:
				break;
			default:
				ret = -EINVAL;
		}
	}
	else if (_SIOC_DIR(cmd) & _SIOC_READ) {
		switch (nr) {
			case SOUND_MIXER_VOLUME:
			case SOUND_MIXER_PCM:
				val = dev_dac_cfg.volume;
				val |= (val<<8);	/* make left/right value */
				return put_user(val, (int *)arg);

			case SOUND_MIXER_RECSRC:
				break;
			case SOUND_MIXER_RECMASK:
				break;
			case SOUND_MIXER_DEVMASK:
				return put_user(SOUND_MIXER_VOLUME|SOUND_MASK_PCM, (int *)arg);
			case SOUND_MIXER_MIC_GAIN:
				break;
			case SOUND_MIXER_ADCVOL_GAIN:
				break;
			case SOUND_MIXER_LINE:
			case SOUND_MIXER_MIC:
			case SOUND_MIXER_CAPS:
			case SOUND_MIXER_STEREODEVS:
				val = 0;
				ret = -ENOSYS;
			default:
				ret = -EINVAL;
		}
		if (ret == 0)
			ret = put_user(val, (int *)arg);
	}
	return ret;
}

static struct file_operations stmp37xx_mix_fops = {
  ioctl:stmp37xx_mix_ioctl,
  llseek:no_llseek,
  owner:THIS_MODULE
};

static int __init stmp37xx_init(void)
{
	int ret = 0;

	DPRINTK(DRIVER_NAME " - installing module\n");

	audio_dev_dsp = register_sound_dsp(&stmp37xx_dsp_fops, -1);
	audio_dev_mix = register_sound_mixer(&stmp37xx_mix_fops, -1);
//#define MUTEON //dhsong
#ifndef MUTEON
        static int bank = 0;
        static int pin = 21;
        static int i = 1;
 #if 0
        HW_PINCTRL_MUXSEL0_CLR(0x3 << pin*2);
        udelay(5);
        HW_PINCTRL_MUXSEL0_SET(0x3 << pin*2);
        udelay(5);

        HW_PINCTRL_DOE0_CLR(1 << 21);
        HW_PINCTRL_DOE0_SET(1 << 21);
        for (i = 0; i < 11; i++)
        {
                // Send pulse
                HW_PINCTRL_DOUT0_CLR(1 << pin);
                udelay(5);
                HW_PINCTRL_DOUT0_SET(1 << pin);
                udelay(5);
        }
 #else
        stmp37xx_gpio_set_af( pin_GPIO(bank,  pin), GPIO_MODE); //3); //muxsel set gpio func
        stmp37xx_gpio_set_dir( pin_GPIO(bank,  pin), GPIO_DIR_OUT); //output
        stmp37xx_gpio_set_level( pin_GPIO(bank,  pin), 0); //low
        stmp37xx_gpio_set_level( pin_GPIO(bank,  pin), 1); //high
 #endif
#endif //MUTEON


#ifdef OCRAM_DAC_USE
	dev_dac_cfg.real_addr = OCRAM_DAC_START;
	dev_dac_cfg.virt_addr = ioremap(OCRAM_DAC_START, sizeof(dma_block_t) * CHAIN_LENGTH);

#else
	dev_dac_cfg.virt_addr = dma_alloc_coherent(dev_dac_cfg.device,
				sizeof(dma_block_t) * CHAIN_LENGTH,
				&(dev_dac_cfg.real_addr), GFP_DMA);
#endif
	if (dev_dac_cfg.virt_addr == NULL) {
		DPRINTK(" - unable to allocate dma memory\n");
		return -1;
	}

	DPRINTK(" - [AUDIO_OUT] dma_alloc_coherent() - "
           "virt addr: %08X, real addr = %08X\n", (uint32_t)dev_dac_cfg.virt_addr,
           (uint32_t)dev_dac_cfg.real_addr);

	dev_adc_cfg.virt_addr = dma_alloc_coherent(dev_adc_cfg.device,
				sizeof(dma_block_t) * CHAIN_LENGTH,
				&(dev_adc_cfg.real_addr), GFP_DMA);
	
	if (dev_adc_cfg.virt_addr == NULL) {
		DPRINTK(" - unable to allocate dma memory\n");
		return -1;
	}

	DPRINTK(" - [AUDIO_IN] dma_alloc_coherent() - "
           "virt addr: %08X, real addr = %08X\n", (uint32_t)dev_adc_cfg.virt_addr,
           (uint32_t)dev_adc_cfg.real_addr);

	//dma_chain_init(&dev_adc_cfg);

	init_waitqueue_head(&dev_dac_cfg.wait_q);
	init_waitqueue_head(&dev_adc_cfg.wait_q);


	ret = request_irq(dev_dac_cfg.irq_dma, device_dac_dma_handler, IRQF_DISABLED,
			  DRIVER_NAME, &dev_dac_cfg);
	if (ret != 0) {
		DPRINTK (" - error: request_irq() returned error: %d\n", ret);
		return ret;
	}

#if 0
	ret = request_irq(dev_adc_cfg.irq_dma, device_adc_dma_handler, IRQF_DISABLED,
			  DRIVER_NAME, &dev_adc_cfg);
	if (ret != 0) {
		DPRINTK (" - error: request_irq() returned error: %d\n", ret);
		return ret;
	}


	ret = request_irq(dev_cfg.irq_err, device_err_handler, IRQF_DISABLED,
			  DRIVER_NAME, NULL);
	if (ret != 0) {
		DPRINTK (" - error: request_irq() returned error: %d\n", ret);
		return ret;
	}

	// initialize dac
	//dac_init(DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_WIDTH);
	if (dev_adc_cfg.init_device == 0) {
		adc_init(DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_WIDTH);
		dev_adc_cfg.init_device = 1;
	}
#endif


	volume_set(VC_DAC, 70); //set to default volume, max = 100
	volume_set(VC_HEADPHONE, 127); // hp vol max set, dhsong

	return ret;
}

static void __exit stmp37xx_exit(void)
{
	DPRINTK(" - removing module\n");

	free_irq(dev_dac_cfg.irq_dma, &dev_dac_cfg);
	free_irq(dev_adc_cfg.irq_dma, &dev_adc_cfg);

	unregister_sound_dsp(audio_dev_dsp);
	unregister_sound_mixer(audio_dev_mix);
}


module_init(stmp37xx_init);
module_exit(stmp37xx_exit);
