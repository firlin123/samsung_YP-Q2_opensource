/* $Id: stmp36xx_audioout.c,v 1.100 2008/01/03 07:38:23 zzinho Exp $ */


/**
 * \file stmp36xx_audioout.c
 * \brief audioout of stmp36xx
 * \author LIM JIN HO <jinho.lim@samsung.com>
 * \version $Revision: 1.100 $
 * \date $Date: 2008/01/03 07:38:23 $
 *
 * This file implements audioout(DAC) driver for SMTP36XX(sigmatel).
 * $Log: stmp36xx_audioout.c,v $
 * Revision 1.100  2008/01/03 07:38:23  zzinho
 * clear consumed data after dac interrupt
 *
 * by jinho.lim
 *
 * Revision 1.99  2008/01/03 06:32:39  zzinho
 * clear consumed data after dac interrupt
 *
 * by jinho.lim
 *
 * Revision 1.98  2007/10/08 08:07:21  zzinho
 * remove printk
 * by jinho.lim
 *
 * Revision 1.97  2007/09/19 08:55:40  zzinho
 * disable clkctrl intr wait when max volume
 * by jinho.lim
 *
 * Revision 1.96  2007/09/19 05:56:14  biglow
 * - board type detecting method changed
 *
 * -- Taehun Lee
 *
 * Revision 1.95  2007/08/01 09:13:04  zzinho
 * add fm volume table for s3 project
 * by jinho.lim
 *
 * Revision 1.94  2007/06/21 05:29:07  zzinho
 * update to latest ZB source
 * by jinho.lim
 *
 * Revision 1.93  2007/03/21 07:27:03  biglow
 * Update from China by zzinho
 *
 * - Taehun Lee
 *
 * Revision 1.92  2006/05/19 03:43:42  zzinho
 * update with static sound driver
 *
 * Revision 1.101  2006/05/19 02:03:40  zzinho
 * To fix under 150mV FR FM output,
 * FM ADC gain 0x05 -> 0x03 when FR
 *
 * Revision 1.100  2006/05/10 07:22:11  zzinho
 * audioin DMA reset after decrementing sema
 *
 * Revision 1.99  2006/04/26 00:19:15  yoonhark
 * changing ADC_Mux_Volume setting value in France Country from 0xb to 0x7
 *
 * Revision 1.98  2006/04/17 04:59:59  yoonhark
 * modification for removing pop noise in start part of recording file
 *
 * Revision 1.97  2006/04/08 01:59:27  zzinho
 * add fm open release event callback
 * add when audio sync, immediately stop audioin dma
 *
 * Revision 1.96  2006/04/05 06:30:21  zzinho
 * recording status check routine added
 *
 * Revision 1.95  2006/03/14 12:01:41  zzinho
 * add mic_bias ioctl to control mic bias
 *
 * Revision 1.94  2006/03/14 11:46:27  zzinho
 * add mic_bias ioctl to control mic bias
 *
 * Revision 1.93  2006/03/07 06:26:28  zzinho
 * add cradle mute condition as HW rev type
 *
 * Revision 1.92  2006/03/06 08:13:07  zzinho
 * audio driver ioctl change
 * SNDCTL_DSP_EU_OUTPUT -> SNDCTL_DSP_FRANCE_OUTPUT
 *
 * Revision 1.91  2006/03/06 00:33:09  zzinho
 * fix ADC volume to 100
 *
 * Revision 1.90  2006/03/02 04:31:24  zzinho
 * rev_type addition
 *
 * Revision 1.89  2006/03/02 01:29:43  zzinho
 * TA5 -> TB1 modify
 *
 * Revision 1.88  2006/02/28 07:30:38  zzinho
 * audio isr changed
 *
 * Revision 1.87  2006/02/27 13:14:07  zzinho
 * removed !USE_DOWN mode
 * and adc buffer change to 32Kbytes
 *
 * Revision 1.86  2006/02/20 07:21:49  zzinho
 * audioin buffer -> SDRAM
 *
 * Revision 1.85  2005/12/22 06:43:27  hcyun
 * Jinho's volume table setting ioctl code addition.
 *
 * Revision 1.84  2005/12/08 06:59:13  zzinho
 * volume table modification
 * EU volume table addition
 *
 * Revision 1.83  2005/12/05 11:53:03  zzinho
 * 1. volume table init version addition for EU
 * 2. Postprocessing mute modification
 * - 120msec -> 30msec first skip time
 *
 * Revision 1.82  2005/11/24 10:30:56  zzinho
 * volume set case modified
 *
 * Revision 1.81  2005/11/24 08:39:02  zzinho
 * if prev volume == UI set volume
 * return
 *
 * Revision 1.80  2005/11/20 06:09:38  zzinho
 * printk -> DPRINTK
 *
 * Revision 1.79  2005/11/18 05:27:13  zzinho
 * *** empty log message ***
 *
 * Revision 1.78  2005/11/15 07:38:48  zzinho
 * pop up improvement
 *
 * Revision 1.77  2005/11/13 10:25:39  zzinho
 * delay added when open and write
 *
 * Revision 1.76  2005/11/13 07:28:05  zzinho
 * volume fade in/out with 10msec timer
 *
 * Revision 1.75  2005/11/10 06:16:07  zzinho
 * *** empty log message ***
 *
 * Revision 1.74  2005/11/09 05:33:30  zzinho
 * audio exit addition
 *
 * Revision 1.73  2005/11/03 05:07:48  zzinho
 * audio in modified
 *
 * Revision 1.72  2005/11/01 09:12:39  zzinho
 * refer define hardware.h
 *
 * Revision 1.71  2005/11/01 07:30:41  zzinho
 * sem up position moved
 *
 * Revision 1.70  2005/10/31 09:40:54  zzinho
 * set fragment size added
 * remove checkDacPtr and add reset dma desc when audio is stopped.
 *
 * Revision 1.69  2005/10/28 11:43:47  zzinho
 * isr : when audio is stopped, setdac call
 *
 * Revision 1.68  2005/10/22 00:24:22  zzinho
 * select_sound_status addition
 *
 * Revision 1.67  2005/10/19 02:28:15  zzinho
 * when volume ioctl, setCoreLevel()
 *
 * Revision 1.66  2005/10/18 12:08:26  zzinho
 * volume set in isr
 *
 * Revision 1.65  2005/10/18 12:04:51  zzinho
 * sound stop codes changed
 * modified by jinho.lim
 *
 * Revision 1.64  2005/10/18 08:44:07  zzinho
 * remove printk
 *
 * Revision 1.63  2005/10/18 08:42:00  zzinho
 * 1. if audio is stopped, Hw mute on
 * 2. if audio is stopped && volume 100, we cannot change VDD for max sound output
 * 3. smoth audio stop is added
 * added by jinho.lim
 *
 * Revision 1.62  2005/10/14 08:10:20  zzinho
 * fix volume table and add checkdacptr function
 *
 * Revision 1.61  2005/10/12 11:48:20  zzinho
 * fix audio dac isr
 * if dac ptr < 30000
 * stop
 *
 * Revision 1.60  2005/10/12 07:45:13  zzinho
 * 1. sync block mode add
 * 2. buffer size define add
 *
 * Revision 1.59  2005/10/11 07:40:23  zzinho
 * audioin isr change
 *
 * Revision 1.58  2005/09/30 07:06:08  zzinho
 * *** empty log message ***
 *
 * Revision 1.57  2005/09/28 09:27:25  zzinho
 * *** empty log message ***
 *
 * Revision 1.56  2005/09/26 06:59:08  zzinho
 * remove printk
 *
 * Revision 1.55  2005/09/23 10:25:27  zzinho
 * change mmap for ogg sdram
 *
 * Revision 1.54  2005/09/23 07:57:25  zzinho
 * remove timeout timer
 *
 * Revision 1.53  2005/09/23 07:50:43  zzinho
 * remove timeout isr
 *
 * Revision 1.52  2005/09/23 07:43:17  zzinho
 * modify ISR
 * 1. remove sem init
 * 2. HW_APBX_CH1_INT_EN position change
 *
 * Revision 1.51  2005/09/22 02:10:27  zzinho
 * change buffer mechanism
 * 1. add kernel semaphore init at ISR
 *
 * Revision 1.50  2005/09/21 01:06:20  zzinho
 * remove printf
 *
 * Revision 1.49  2005/09/21 00:07:14  zzinho
 * no decoder buf mode add
 *
 * Revision 1.48  2005/09/12 04:26:32  zzinho
 * add new ioctl for mic gain & adc gain
 *
 * Revision 1.47  2005/09/12 00:10:03  zzinho
 * add new ioctl for mic gain
 *
 * Revision 1.46  2005/09/07 11:25:23  zzinho
 * add audioin
 *
 * Revision 1.45  2005/09/07 04:01:17  zzinho
 * add audioin and remove warning msg
 *
 * Revision 1.44  2005/09/06 10:44:56  zzinho
 * add audioin
 *
 * Revision 1.43  2005/09/06 10:42:18  zzinho
 * add audioin
 *
 * Revision 1.42  2005/09/02 02:24:28  zzinho
 * printf remove
 *
 * Revision 1.41  2005/09/01 08:20:31  zzinho
 * fix volume control from HP amp to DAC
 * HP amp default 0x03
 * DAC volume range -100dB ~ 0dB
 *
 * Revision 1.40  2005/08/25 09:31:19  zzinho
 * remove print
 *
 * Revision 1.39  2005/08/24 10:57:58  zzinho
 * all allocation is just one time at first
 *
 * Revision 1.38  2005/08/24 09:22:14  zzinho
 * *** empty log message ***
 *
 * Revision 1.37  2005/08/24 08:49:38  zzinho
 * change
 * audio_setup_buf open -> init
 *
 * Revision 1.36  2005/08/24 08:15:21  zzinho
 * modify dmastopped bug
 *
 * Revision 1.35  2005/08/23 09:16:07  zzinho
 * remove warning msg
 *
 * Revision 1.34  2005/08/22 01:44:57  zzinho
 * mmap, release change
 *
 * Revision 1.32  2005/08/11 11:22:36  zzinho
 * delay add
 *
 * Revision 1.31  2005/08/11 02:36:45  zzinho
 * remove start release sem
 * add delay 200msec in case release
 *
 * Revision 1.30  2005/08/11 00:44:52  zzinho
 * remove start release sem
 * add delay 200msec in case release
 *
 * Revision 1.29  2005/08/10 11:10:19  zzinho
 * start stop contiually bug modity
 *
 * Revision 1.28  2005/08/09 04:02:09  zzinho
 * jinho version up
 *
 * Revision 1.27  2005/08/03 05:24:00  zzinho
 * when stop the play, set the zero the current addr
 *
 * Revision 1.26  2005/08/03 00:40:48  zzinho
 * remove pop noise 80%
 *
 * Revision 1.25  2005/08/02 10:12:52  hcyun
 * add comments..
 *
 * - hcyun
 *
 * Revision 1.24  2005/08/02 09:47:46  hcyun
 * add comments..
 *
 * - hcyun
 *
 * Revision 1.23  2005/08/02 09:35:49  zzinho
 * *** empty log message ***
 *
 * Revision 1.22  2005/08/01 06:57:19  zzinho
 * back volume 0~100
 *
 * Revision 1.21  2005/07/29 09:39:46  zzinho
 * *** empty log message ***
 *
 * Revision 1.20  2005/07/29 08:24:08  zzinho
 * modify audioout dma structure
 *
 * Revision 1.19  2005/07/28 01:54:30  zzinho
 * up version
 *
 * Revision 1.18  2005/07/28 01:50:58  zzinho
 * up version
 *
 * Revision 1.17  2005/07/27 12:05:33  zzinho
 * add panic in case time out dac dma
 *
 * Revision 1.16  2005/07/27 04:33:45  zzinho
 * modify volume control
 * dac vol -> hp vol
 *
 * Revision 1.15  2005/07/23 05:35:03  zzinho
 * modify name
 * add apbx audioout dma irq timeout
 *
 * Revision 1.14  2005/07/15 12:42:58  zzinho
 * back 0708
 * because of dac dma kill
 * jinho.lim
 *
 * Revision 1.13  2005/07/15 11:58:51  zzinho
 * back 0708
 * because of dac dma kill
 * jinho.lim
 *
 * Revision 1.11  2005/07/04 05:22:12  zzinho
 * in case audio_write
 * if the driver buffer full, no wait and return.
 *
 * Revision 1.10  2005/06/30 02:37:37  zzinho
 * fill->copy->start : bug
 * fill->copy->fill->start : modify
 *
 * Revision 1.9  2005/06/24 12:49:11  zzinho
 * *** empty log message ***
 *
 * Revision 1.8  2005/06/22 09:46:23  zzinho
 * set seg ioctl support
 *
 * Revision 1.7  2005/06/11 17:37:31  zzinho
 * *** empty log message ***
 *
 * Revision 1.6  2005/06/02 19:10:48  zzinho
 * buffer management modify to use 4 dma chain
 *
 * Revision 1.5  2005/05/31 17:10:21  zzinho
 * modify doxygen format
 * and
 * add mmap ioctl
 *
 * Revision 0.1  2005/04/29 zzinho
 * - add first revision
 *
 */
 
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/poll.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/delay.h>
//#include <linux/devfs_fs_kernel.h>

#include <asm/div64.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/arch/hardware.h>
#include <asm/arch/ocram.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/dma.h>

#include <asm/arch/37xx/regsicoll.h>
#include <asm/arch/37xx/regsapbx.h>
#include <asm/arch/37xx/regsapbh.h>
#include <asm/arch/37xx/regsmemcpy.h>
#include <asm/arch/37xx/regspinctrl.h>
#include <asm/arch/37xx/regsaudioout.h>
#include <asm/arch/37xx/regsaudioin.h>
#include <asm/arch/digctl.h>

#include <asm/arch-stmp37xx/irqs.h>
#include <asm/arch-stmp37xx/pinctrl.h>
#include <asm/arch-stmp37xx/stmp37xx_pm.h>

#include <asm/arch/digctl.h>

#include "stmp37xx_audio.h"
#include "stmp37xx_audioout_dac.h"
#include "stmp37xx_audioin_adc.h"


#define SMTP_AUDIO_NAME		"audioout_in"
#define SMTP_AUDIO_MAJOR		251

#define WMA_INIT_CODE_VIRT 0x50100000
#define OGG_CODE_VIRT 0x50010000
#define OGG_SDRAM_CODE_SIZE 64*1024
#define WMA_SDRAM_CODE_SIZE 64*1024

#define BUS_ID	 "dac0"

typedef struct device device_t;

int isInitDac = 0;
int isInitAdc = 0;

int gIsAUDIOIN = 0;

static version_inf_t version_info;

/* ------------ Definitions ------------ */
/**
 * device file handle
 */
// for linux 2.4.X
//static	devfs_handle_t devfs_handle;

/* ------------ HW Independent Functions ------------ */
static void release(device_t * dev);
/**
 * \brief Interrupt service routine of DAC dma
 * \param irq INT_DAC_DMA number
 * \param dev_id audio_state_t structure pointer
 *
 * When DAC DMA transfer is completed, this isr is called
 * 1. buffer has valid(user) data : memcpy to dac dma memory block. it uses APBH memcpy function
 * 2. else if : stop dac and reset buffer
 */
static irqreturn_t stmp_dac_dma_isr(int irq, void *dev_id);
/**
 * \brief Interrupt service routine of DAC dma err
 * \param irq INT_DAC_ERROR number
 * \param dev_id audio_state_t structure pointer
 *
 * When DMA buffer underflow is happened, this isr is called
 */
static irqreturn_t stmp_dac_dmaerr_isr(int irq, void *dev_id);
/**
 * \brief /dev/mixer device ioctl to control volume
 * \param inode device inode structure
 * \param file file operation description
 * \param cmd mixer device ioctl command
 *
 * if application uses ioctl(mix_fd, (SOUND_MIXER_VOLUME|SIOC_OUT), &vol), called
 */
static int stmp_mixer_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
/**
 int freeCodecSDRAM(audio_stream_t *audio_s);
 static void audio_clear_buf_info(audio_stream_t * s);
*/
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 void audio_clear_buf(audio_stream_t * s, int buf);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static void audio_clear_buf_data(audio_stream_t * s);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static void audio_clear_buf_info(audio_stream_t * s);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 void audio_reset_buf(audio_stream_t * s);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 int audio_setup_buf(audio_stream_t * s, int buf);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static int audio_sync(struct file *file);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static int stmp_audio_write(struct file *file, const char *buffer, size_t count);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static int stmp_audio_read(struct file *file, char *buffer, size_t count);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static unsigned int stmp_audio_poll(struct file *file, struct poll_table_struct *wait);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static loff_t stmp_audio_llseek(struct file *file, loff_t offset, int origin);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static int audio_set_fragments(audio_stream_t *s, int val);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static int stmp_audio_mmap(struct file *file, struct vm_area_struct *vma);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static int stmp_audio_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg);
 /**
  * \brief 
  * \param 
  * \param 
  *
  * This function 
  */
 static int stmp_audio_release(struct inode *inode, struct file *file);
/**
 * \brief /dev/dsp device open to init dsp device
 * \param file file operation description
 *
 * it just call audio_attach function in stmp36xx_audio.c
 */
static int stmp_audio_open(struct inode *inode, struct file *file);

/* ------------ Initialization Functions ------------*/
/**
 *\brief initialize stmp sound driver module
 *\return 0 : success \n -22(-EINVAL) : invalid argument
 *
 * Initializing function of dac sound driver on linux
 * dsp and mixer device register
 * 4, 5(dac related) irq request
 */
static int __init stmp37xx_audio_init(void);
/**
 * \brief exit stmp sound driver module
 *
 * dsp and mixer device unregister
 */
static void __exit stmp37xx_audio_exit(void);

static device_t device = {
	.parent = NULL,
	.bus_id = BUS_ID,
	.release = release,
	.coherent_dma_mask = ISA_DMA_THRESHOLD,
};
device_t *dev = &device;

static audio_stream_t stmp_audio_in = {
  name:"STMP37XXAUDIOIN",
};

static audio_stream_t stmp_audio_out = {
  name:"STMP37XXAUDIOOUT",		/* name is now the channel name itself */
};

#define __MUTEX_INITIALIZER(name) \
	__SEMAPHORE_INIT(name,1)

static audio_state_t stmp_audio_state = {
	output_stream:&stmp_audio_out,
	input_stream:&stmp_audio_in,
	output_id:	"stmp37xx audio out",
	input_id:	"stmp37xx audio in",
	/* resets our codec */
	sem:__MUTEX_INITIALIZER(stmp_audio_state.sem),
};

static void release(device_t * dev)
{
	audio_state_t *state = &stmp_audio_state;
	DPRINTK(" - releasing device struct with bus_id: " BUS_ID "\n");

#ifdef CONFIG_STMP36XX_SRAM
	/* for dac - use sram */
	iounmap(state->output_stream->audio_buffers->pStart);
	/* for adc - use sdram */
	dma_free_coherent(dev,
			  ADC_BUF_SIZE,
			  state->input_stream->audio_buffers->pStart, state->input_stream->audio_buffers->phyStartaddr);
#else
	/* for dac - use sdram */
	dma_free_coherent(dev,
			  STMP37XX_SRAM_AUDIO_KERNEL_SIZE,
			  state->output_stream->audio_buffers->pStart, state->output_stream->audio_buffers->phyStartaddr);
	/* for adc - use sdram */
	dma_free_coherent(dev,
			  ADC_BUF_SIZE,
			  state->input_stream->audio_buffers->pStart, state->input_stream->audio_buffers->phyStartaddr);
#endif
}

void *
dev_dma_alloc( int size, dma_addr_t *ptr)
{
	return dma_alloc_coherent(dev, size, ptr, GFP_DMA);
}

void
dev_dma_free(int size, void *cpu_addr, dma_addr_t dma_handle)
{
	dma_free_coherent(dev, size, cpu_addr, dma_handle);
}

static irqreturn_t 
stmp_adc_dma_isr(int irq, void *dev_id)
{
	audio_state_t *state = dev_id;
	audio_stream_t *audio_s = state->input_stream;
	audio_buf_t *adc_b = audio_s->audio_buffers;
	adc_info_t *adcInfo = audio_s->adc_info;
	signed int validBufSize = 0;
	unsigned int bufferSize = 0;

	bufferSize = audio_s->fragsize * audio_s->nbfrags;

	/* reset interrupt request status bit */
	//BF_CLR(APBX_CTRL1, CH0_CMDCMPLT_IRQ);	
 	BW_APBX_CTRL1_CH0_CMDCMPLT_IRQ(1);

	/* Accounting */
	if(adc_b->initDevice == 1)
	{
		audio_s->bytecount += MIN_STREAM_SIZE;

		validBufSize = getADCValidBytes(adcInfo);
		//printk("IR[%d], B[%d]\n", validBufSize, bufferSize);
		if(validBufSize >= bufferSize)
		{
			if(audio_s->usr_rwcnt != 0)
			{
				updateADCData(audio_s);
				setADCValidBytes(adcInfo, MIN_STREAM_SIZE);
				DPRINTK("[DRIVER] audioin buf overflow -> discard old PCM data..\n");
			}
			else
			{
				stopAdcProcessing(audio_s);
				setRecordingStatus(NO_RECORDING);
				audio_reset_buf(audio_s);
				DPRINTK("[DRIVER] audioin is stopped(1)\n");
			}
		}
		else
		{
			updateADCData(audio_s);
		}
	}
	else
	{
		stopAdcProcessing(audio_s);

		setRecordingStatus(NO_RECORDING);
		audio_reset_buf(audio_s);
		DPRINTK("[DRIVER] audioin is stopped(2)\n");
	}
	up(&adc_b->sem);
	/* And any process polling on write. */
	wake_up(&audio_s->wq);
	return IRQ_HANDLED;
}

static irqreturn_t
stmp_adc_dmaerr_isr(int irq, void *dev_id)
{
  printk(" adc dma err ISR Execution!!! \n");

  INTR_DISABLE_HW_VECTOR(irq); 
  return IRQ_HANDLED;
}

static irqreturn_t 
stmp_dac_dma_isr(int irq, void *dev_id)
{
	audio_state_t *state = dev_id;
	audio_stream_t *audio_s = state->output_stream;
	decoder_buf_t *decoder_b = audio_s->decoder_buffers;
	audio_buf_t *dac_b = audio_s->audio_buffers;

	//printk("[I:%d] - ", decoder_b->decoderValidBytes);
	if(decoder_b->decoderValidBytes <= 0)//audio_s->usr_rwcnt <= audio_s->bytecount)
	{
		int sem=0;
		sem = BF_RDn(APBX_CHn_SEMA, 1, PHORE);
		if(sem !=0)		
		{
			DPRINTK("[DRIVER] remain sem.. return\n");
			BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);
			BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ_EN(1);
			return IRQ_HANDLED;
		}
		
		if(stopDacProcessing(audio_s) != 0)
			return IRQ_NONE;

		/* reset audio buffer */
		audio_reset_buf(audio_s);

		/* to block audio sync */
		up(&dac_b->sem);
		printk("[DRIVER] audio is stopped\n");
	}
	else
	{
		/* Allow this interrupt to occur again. */
		#if 1
		BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);		
		BF_WRn(APBX_CHn_SEMA, 1, INCREMENT_SEMA, 1); 
		#else
		BF_CLR(APBX_CTRL1, CH1_CMDCMPLT_IRQ);
		//BW_APBX_CTRL1_CH1_CMDCMPLT_IRQ(1);
		if (HW_AUDIOOUT_CTRL.B.FIFO_OVERFLOW_IRQ)
			HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_FIFO_OVERFLOW_IRQ);
		
		BF_WRn(APBX_CHn_SEMA, 1, INCREMENT_SEMA, 1);
		HW_AUDIOOUT_CTRL_CLR(BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ);
		#endif

		updateDacBytes(audio_s);
		audio_s->fragcount ++;
	}

	up(&decoder_b->sem);

	/* And any process polling on write. */
	wake_up(&audio_s->wq);
	return IRQ_HANDLED;
}

static irqreturn_t
stmp_dac_dmaerr_isr(int irq, void *dev_id)
{
  printk(" dac dma err ISR Execution!!! \n");

  INTR_DISABLE_HW_VECTOR(irq); 
  return IRQ_HANDLED;
}

static int
stmp_mixer_ioctl(struct inode *inode, struct file *file,
	    unsigned int cmd, unsigned long arg)
{
    int val = 0;
    int gain = 0;
    int mic_resistor = 0;
    int ret = 0;
    int nr = _IOC_NR(cmd);
	int inputSrc = 0;
#ifdef USE_MAX_CORE_LEVEL	
	version_inf_t *sw_ver ;
#endif
	int prev_volume = getDacVolume();

	DPRINTK("[DRIVER] stmp_mixer_ioctl [0x%08x]\n", cmd);

    if (_SIOC_DIR(cmd) & _SIOC_WRITE )
    {				
		switch (nr)
		{
			case SOUND_MIXER_VOLUME:
			{
		    	/* trap only the write command */
		    	ret = get_user(val, (int *)arg);
				if (ret)
					goto out;
		        // Ignore separate left/right channel for now, even the codec does support it.
				gain = val & 0xFF;
				if(gain < 0 || gain > 30)
				{
					ret = EINVAL;
					goto out;
				}
				DPRINTK("[DRIVER] DAC volume [%d]\n", gain);
				if(prev_volume != gain)
				{
					setUserVolume(gain);

					#ifdef USE_MAX_CORE_LEVEL
					int sw_ver = getSWVersion();
					if(sw_ver == VER_COMMON)
						setCoreLevel(gain);
					else if(sw_ver == VER_FR)
						setDefaultVGA();
					#endif					
				}
				
				//if(gIsAUDIOIN)
				//	setADCVolume(gain);				

				break;
			}
			case SOUND_MIXER_RECSRC:
				if(gIsAUDIOIN)
				{
					if (get_user(val, (int *)arg))
						return -EFAULT;

					if(val & SOUND_MASK_LINE)
						setRecodingSrc(INPUT_SRC_LINEIN);
					else if(val & SOUND_MASK_MIC)
						setRecodingSrc(INPUT_SRC_MICROPHONE);
					break;
				}
			case SOUND_MIXER_MIC_GAIN:
				if(gIsAUDIOIN)
				{
					if (get_user(val, (int *)arg))
						return -EFAULT;
					gain = val & 0xf;
					if(gain < 0x0 || gain > 0x3)
					{
						ret = EINVAL;
						goto out;
					}
					
					setMICGain(gain);
					break;
				}
			case SOUND_MIXER_MIC_BIAS:
				if(gIsAUDIOIN)
				{
					if (get_user(val, (int *)arg))
						return -EFAULT;
					mic_resistor = val & 0xf;
					if(mic_resistor < 0x0 || mic_resistor > 0x3)
					{
						ret = EINVAL;
						goto out;
					}
					
					setMICBias(mic_resistor);
					break;
				}
			case SOUND_MIXER_ADCVOL_GAIN:
				if(gIsAUDIOIN)
				{
					if (get_user(val, (int *)arg))
						return -EFAULT;
					gain = val & 0xf;
					if(gain < 0x0 || gain > 0xF)
					{
						ret = EINVAL;
						goto out;
					}

					/* TODO for 37xx */
					#if 0
					sw_ver = chk_sw_version();

					if (strcmp(sw_ver->nation, "FR") == 0) {

						if(gain == 0xB) {
							DPRINTK("[DRIVER] contry code %s recording ADC gain %d\n", sw_ver->nation, gain);		
							setADCGain(0x5);
						}
						else if(gain == 0x5) {
							DPRINTK("[DRIVER] contry code %s FM ADC gain %d\n", sw_ver->nation, gain);		
							setADCGain(0x3);
						}
						else {
							setADCGain(gain);							
						}
					} 
					else
					#endif
					{
						setADCGain(gain);
					}
					
					
					break;
				}
			case SOUND_MIXER_LINE:
			case SOUND_MIXER_MIC:
				val = 0;
				ret = -ENOSYS;
				break;

			default:
				ret = -EINVAL;
		}
	}
	else if (_SIOC_DIR(cmd) & _SIOC_READ)
    {
    	ret = 0;

		switch (nr) 
		{
			case SOUND_MIXER_VOLUME:     
				val = getDacVolume();	
				break;
			case SOUND_MIXER_RECSRC:   
				if(gIsAUDIOIN)
				{
					inputSrc = getRecodingSrc();
					if(inputSrc == INPUT_SRC_LINEIN)
						val = SOUND_MASK_LINE;
					else if(inputSrc == INPUT_SRC_MICROPHONE)
						val = SOUND_MASK_MIC;
					break;
				}
			case SOUND_MIXER_RECMASK:    
				if(gIsAUDIOIN)
				{
					val = STMP36XX_REC_DEVICES;
					break;
				}
			case SOUND_MIXER_DEVMASK:    
				if(gIsAUDIOIN)
				{
					val = STMP36XX_DEV_DEVICES;
					break;
				}
			case SOUND_MIXER_MIC_GAIN:
				if(gIsAUDIOIN)
				{
					val = getMICGain();
					break;
				}
			case SOUND_MIXER_ADCVOL_GAIN:
				if(gIsAUDIOIN)
				{
					val = getADCGain();
					break;
				}
			case SOUND_MIXER_LINE:      
			case SOUND_MIXER_MIC:        
			case SOUND_MIXER_CAPS:      
			case SOUND_MIXER_STEREODEVS: 
				val = 0;
				ret = -ENOSYS;
				break;
			default:	
				val = 0;     
				ret = -EINVAL;	
				break;
		}

		if (ret == 0)
			ret = put_user(val, (int *)arg);
	}
    
 out:
	return ret;

}

static struct file_operations stmp_mixer_fops = {
  ioctl:stmp_mixer_ioctl,
  llseek:no_llseek,
  owner:THIS_MODULE
};

void sleep( unsigned howlong )
{
	current->state	 = TASK_INTERRUPTIBLE;
	schedule_timeout(howlong);
}

#if 1 // TODO
static void 
audio_check_idle(ss_pm_request_t rqst)
{
	switch(rqst)
	{
		case SS_PM_IDLE:
			DPRINTK("[DRIVER] SS_PM_IDLE\n");
			stopHWAudioout();
			break;
		case SS_PM_SET_WAKEUP:
			DPRINTK("[DRIVER] SS_PM_SET_WAKEUP\n");
			startHWAudioout();
			break;
		default:
			DPRINTK("[DRIVER] Invalid audio_check_idle rqst\n");
			break;
	}
}
#endif

#if 0
static void 
audio_check_fm(ss_fm_request_t rqst)
{
	switch(rqst)
	{
		case SS_FM_OPEN:
			DPRINTK("[DRIVER] SS_FM_OPEN\n");
			setFMStatus(FM_OPEN);
			break;
		case SS_FM_RELEASE:
			DPRINTK("[DRIVER] SS_FM_RELEASE\n");
			setFMStatus(FM_RELEASE);
			break;
		default:
			DPRINTK("[DRIVER] Invalid audio_check_fm rqst\n");
			break;
	}
}
#endif

int freeCodecSDRAM(audio_stream_t *audio_s)
{
	if(audio_s->codec_info)
	{
		kfree(audio_s->codec_info->wma_init_code_addr); 	
		audio_s->codec_info->wma_init_code_addr = (unsigned long*)NULL; 	

		kfree(audio_s->codec_info->code_addr);		
		audio_s->codec_info->code_addr = (unsigned long*)NULL;		

		kfree(audio_s->codec_info);
		audio_s->codec_info = (codec_info_t*)NULL;
	}
	return 0;
}

static int allocCodecSDRAM(audio_stream_t *audio_s)
{
	unsigned long* virt_start1 = (unsigned long*)NULL;
	unsigned long* virt_start2 = (unsigned long*)NULL;

	/* to use sdram for codec when usb is connected */
	audio_s->codec_info = (codec_info_t *)
		kmalloc(sizeof(codec_info_t), GFP_KERNEL);
	if (!audio_s->codec_info)
		return -ENOMEM;
	/* allocate SDRAM for Ogg code section */
	virt_start1 = (unsigned long*)kmalloc(OGG_SDRAM_CODE_SIZE, GFP_KERNEL);
	if (!virt_start1)
	{
		printk("[DRIVER] kmalloc fail !!\n");
		return -ENOMEM;
	}
	audio_s->codec_info->code_addr = virt_start1;
	audio_s->codec_info->code_phys = virt_to_phys(virt_start1);

	/* allocate SDRAM for wma init code section */
	virt_start2 = (unsigned long*)kmalloc(WMA_SDRAM_CODE_SIZE, GFP_KERNEL);
	if (!virt_start2)
	{
		printk("[DRIVER] kmalloc fail !!\n");
		return -ENOMEM;
	}
	audio_s->codec_info->wma_init_code_addr = virt_start2;
	audio_s->codec_info->wma_init_code_phys = virt_to_phys(virt_start2);
	
	return 0;
}

void audio_clear_buf(audio_stream_t * s, int buf)
{
	DPRINTK("[DRIVER] audio_clear_buf\n");

	audio_clear_buf_info(s);

	if (s->audio_buffers) 
	{
		if (s->audio_buffers->master)
		{
#ifndef CONFIG_STMP36XX_SRAM
			dma_free_coherent(dev,
					  s->audio_buffers->master,
					  s->audio_buffers->pStart, s->audio_buffers->phyStartaddr);
#endif
			s->audio_buffers->master = 0;
			kfree(s->audio_buffers);
			s->audio_buffers = NULL;
		}
	}

	if (s->decoder_buffers) 
	{
		if (s->decoder_buffers->master)
		{
			s->decoder_buffers->master = 0;
			kfree(s->decoder_buffers);
			s->decoder_buffers = NULL;
			s->decoder_buffers->decoderBuffer = NULL;
			s->decoder_buffers->phydecoderaddr = (dma_addr_t)NULL;
		}
	}
}

static void audio_clear_buf_data(audio_stream_t * s)
{
	memset((void *)s->audio_buffers->pStart, 0, s->audio_buffers->master);
	//memset((void *)s->decoder_buffers->decoderBuffer, 0, s->decoder_buffers->master);
}

static void audio_clear_buf_info(audio_stream_t * s)
{
	if(s) 
	{
		s->channel = AUDIO_STEREO;

		if (s->audio_buffers) 
		{
			s->audio_buffers->lastDMAPosition = 0;
			s->audio_buffers->IsFillDac = 0;
		}

		if(s->decoder_buffers) 
		{
			s->decoder_buffers->AvaliableUserBytes = 0;
			s->decoder_buffers->decoderReadPointer = 0;
			s->decoder_buffers->decoderValidBytes = 0;
			s->decoder_buffers->decoderWritePointer = 0;
		}

		if(s->adc_info)
		{
			s->adc_info->ADCReadPointer=0;
			s->adc_info->ADCValidBytes=0;
		}
	}
}

void audio_reset_buf(audio_stream_t * s)
{
	DPRINTK("[DRIVER] audio_reset_buf\n");

	audio_clear_buf_data(s);
	audio_clear_buf_info(s);

	s->active = 0;
	s->usr_rwcnt = 0;
	s->bytecount = 0;
	s->fragcount = 0;
}

int audio_setup_buf(audio_stream_t * s, int buf)
{
	int audiobufsize = 0;
	void *audiobuf = NULL;
	dma_addr_t audiophys = 0;

	audio_buf_t *audio_b = NULL;
	decoder_buf_t *decoder_b = NULL;
	
	DPRINTK("[DRIVER] audio_setup_buf buf=%d\n", buf);
	
	/* stmp audioout dac buffer init. it means virtual mem buf for dac to tranfer to phy dac */
	s->audio_buffers = (audio_buf_t *)
		kmalloc(sizeof(audio_buf_t), GFP_KERNEL);
	if (!s->audio_buffers)
		goto err;

	/* stmp audioout decoder buffer init. it means virtual mem buf to tranfer pcm data to virtual dac buffer */
	s->decoder_buffers = (decoder_buf_t *)
		kmalloc(sizeof(decoder_buf_t), GFP_KERNEL);
	if (!s->decoder_buffers)
		goto err;

	audio_b = s->audio_buffers;
	decoder_b = s->decoder_buffers;

	/* we have two buffers */
	memset(s->audio_buffers, 0, sizeof(audio_buf_t));
	memset(s->decoder_buffers, 0, sizeof(decoder_buf_t));

	/* dac buffer allocation */
	if (!audiobufsize) {
		do {
#ifdef CONFIG_STMP36XX_SRAM
			if(buf == USE_SRAM)
			{
				audiobufsize = STMP37XX_SRAM_AUDIO_KERNEL_SIZE;
				#if 0 // for stmp3650
				audiophys = STMP37XX_SRAM_AUDIO_KERNEL_START; //real addr
				audiobuf = ioremap(STMP37XX_SRAM_AUDIO_KERNEL_START, audiobufsize); //virture addr
				#else // for 3750
				audiophys = OCRAM_DAC_START; //real addr
				audiobuf = ioremap(OCRAM_DAC_START, audiobufsize); //virture addr
				#endif
				//audiobuf = (void *)STMP37XX_SRAM_AUDIO_KERNEL; //linux 2.4				
			}
			else if(buf == USE_SDRAM)
			{
			#if 1
				/* allocate SDRAM for Ogg code section */
				audiobufsize = ADC_BUF_SIZE;
				audiobuf = (unsigned long*)kmalloc(ADC_BUF_SIZE, GFP_KERNEL);
				if (!audiobuf)
				{
					printk("[DRIVER] kmalloc fail !!\n");
					return -ENOMEM;
				}
				audiophys = virt_to_phys(audiobuf);
			#else
				audiobufsize = ADC_BUF_SIZE;
				audiobuf = dma_alloc_coherent(dev,
							audiobufsize,
							&audiophys, GFP_DMA);
			#endif
			}
			
#else
			if(buf == USE_SRAM)
			{
				audiobufsize = STMP37XX_SRAM_AUDIO_KERNEL_SIZE;
				audiobuf = dma_alloc_coherent(dev,
							audiobufsize,
							&audiophys, GFP_DMA);
			}
			else if(buf == USE_SDRAM)
			{
		#if 1
				/* allocate SDRAM for Ogg code section */
				audiobufsize = ADC_BUF_SIZE;
				audiobuf = (unsigned long*)kmalloc(ADC_BUF_SIZE, GFP_KERNEL);
				if (!audiobuf)
				{
					printk("[DRIVER] kmalloc fail !!\n");
					return -ENOMEM;
				}
				audiophys = virt_to_phys(audiobuf);
		#else
				audiobufsize = ADC_BUF_SIZE;
				audiobuf = dma_alloc_coherent(dev,
							audiobufsize,
							&audiophys, GFP_DMA);
		#endif
			}
#endif
			if (!audiobuf)
				audiobufsize -= STMP37XX_SRAM_AUDIO_KERNEL_SIZE;
		} while (!audiobuf && audiobufsize);
		if (!audiobuf)
			goto err;
		audio_b->master = audiobufsize;
		memzero(audiobuf, audiobufsize);
		DPRINTK("[DRIVER] audio_setup_buf -1-- audiobuf=%x, phys=%x, audiobufsize=%d\n", audiobuf, audiophys, audiobufsize);
	}

	audio_b->pStart = audiobuf;
	audio_b->phyStartaddr = audiophys;
	audio_b->stream = s;
	sema_init(&audio_b->sem, 1);

	decoder_b->decoderBuffer= audiobuf;
	decoder_b->phydecoderaddr = audiophys;
	decoder_b->stream = s;
	decoder_b->master = 0;
	sema_init(&decoder_b->sem, 1);
	return 0;

err:
	DPRINTK(AUDIO_NAME ": unable to allocate audio memory\n ");
	if(buf == USE_SRAM)
		audio_clear_buf(s, USE_SRAM);
	else if(buf == USE_SDRAM)
		audio_clear_buf(s, USE_SDRAM);		
	return -ENOMEM;
}

static int audio_sync(struct file *file)
{
	audio_state_t *state = (audio_state_t *)file->private_data;
	audio_stream_t *s = state->output_stream;
	audio_stream_t *is = state->input_stream;
	audio_buf_t *dac_b = s->audio_buffers;
	audio_buf_t *adc_b = is->audio_buffers;

	DPRINTK("[DRIVER] audio_sync\n");

	if (!(file->f_mode & FMODE_WRITE) || !(file->f_mode & FMODE_READ) || !s->audio_buffers)
		return 0;

	//printk("[DRIVER] decoderValidBytes[%d], initDevice[%d], IsFillDac[%d]\n", s->decoder_buffers->decoderValidBytes, dac_b->initDevice, dac_b->IsFillDac);
	if (file->f_mode & FMODE_READ) {
		
		if(gIsAUDIOIN)
		{
			if(adc_b->initDevice == 1)
			{
				DPRINTK("[DRIVER] audio_sync for read\n");
				adc_b->initDevice = 0;
			}
		}
	}
	
	if (file->f_mode & FMODE_WRITE) {

		if (s->decoder_buffers->decoderValidBytes > 0)
		{
			if(dac_b->initDevice == 0)
			{
				initDacSync(s);
			}
			else
			{
				DPRINTK("[DRIVER] audio_sync : fade out for audio stop\n");
				/* for volume fade out when audio stop */
				if(getDSPFade())				
					setVolumeFade(FADE_OUT);
			}
		}

		while(dac_b->initDevice == 1)		
		{
			udelay(1000);
			if (down_interruptible(&dac_b->sem))
					return -ERESTARTSYS;
			
			if(dac_b->initDevice == 0)
			{
				up(&dac_b->sem);
				break;
			}
		}
	}
	return 0;
}

static int stmp_audio_write(struct file *file, const char *buffer,
			   size_t count)
{
	audio_state_t *state = (audio_state_t *)file->private_data;
	audio_stream_t *s = state->output_stream;
	decoder_buf_t *decoder_b = s->decoder_buffers;
	audio_buf_t *dac_b = s->audio_buffers;
	int ret = 0;
	int filledBytes1 = 0;
	int filledBytes2 = 0;

	//DPRINTK("[DRIVER] audio_write: count=%d\n", count);

	decoder_b->AvaliableUserBytes = count;
	s->usr_rwcnt += count;
	//printk("[W:%d] - ",decoder_b->AvaliableUserBytes);
	
	/* init buffer fulling */
	if(dac_b->initDevice == 0)
	{
		s->active = 1;
		filledBytes1 = fillDecoderBuffer(s, buffer);
		if(filledBytes1 < 0)
			return -EFAULT;
		initDacWrite(s, buffer);
	}
	else
	{
		if(decoder_b->AvaliableUserBytes < MIN_STREAM_SIZE)
		{
			filledBytes1 = fillDecoderBuffer(s, buffer);
			if(filledBytes1 < 0)
				return -EFAULT;
		}		
	}

	/* if audio buffer full, there is no fill data to buf. that is, return will be zero */
	/*	while(decoder_b->AvaliableUserBytes > 0) : wait to buffer has space to fill */
	while(decoder_b->AvaliableUserBytes > 0)
	{
		if (file->f_flags & O_NONBLOCK) {
			if (down_trylock(&decoder_b->sem))
				return -EAGAIN;
		} 
		else {
			if (down_interruptible(&decoder_b->sem))
				return -ERESTARTSYS;
		}
		filledBytes2 = 0;
		filledBytes2 = fillDecoderBuffer(s, buffer+filledBytes1);

		if ( dac_b->initDevice == 0 ) { 
			up(&decoder_b->sem);
			audio_reset_buf(s);
			return 0; 
		}

		if(filledBytes2<0)
		{
			up(&decoder_b->sem);
			break;
		}
		filledBytes1 += filledBytes2;

		if(decoder_b->AvaliableUserBytes <= 0)
			break;
	}
	ret = count - decoder_b->AvaliableUserBytes;
	//DPRINTK("ret=%d\n", ret);
	/* it is important flag. decoder buffer's avaliabe or not is important thing to dacFill memcopy */
	return ret;
}

static int stmp_audio_read(struct file *file, char *buffer,
			  size_t count)
{
	audio_state_t *state = (audio_state_t *)file->private_data;
	audio_stream_t *s = state->input_stream;
	int ret=0;
	int filledBytes=0;
	int offset=0;
	int user_count=0;
	audio_buf_t *adc_b = s->audio_buffers;

	//printk("[DRIVER] audio_read: count=%d\n", count);
	
	if(!gIsAUDIOIN)
		return -ENOSYS;
	
	user_count = count;
	s->usr_rwcnt = count;
  
	if(adc_b->initDevice == 0)
	{
		initializeAudioin(s);
		adc_b->initDevice= 1;			
	}

	while (user_count > 0) 
	{
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			if (down_trylock(&adc_b->sem))
				break;
		} else {
			ret = -ERESTARTSYS;
			if (down_interruptible(&adc_b->sem))
				break;
		}

		setRecordingStatus(RECORDING);
			
		filledBytes = fillEncoderBuffer(s, (void*)buffer+offset, user_count);
		user_count -= filledBytes;
		offset += filledBytes;
	}
	
	ret = count - user_count;
	s->usr_rwcnt = 0;
	return ret;

}

static unsigned int stmp_audio_poll(struct file *file,
				   struct poll_table_struct *wait)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *is = state->input_stream;
	audio_stream_t *os = state->output_stream;
	unsigned int	mask = 0;

	DPRINTK("[DRIVER] stmp_audio_poll(): mode=%s %s\n",
		(file->f_mode & FMODE_READ) ? "r" : "",
		(file->f_mode & FMODE_WRITE) ? "w" : "");

	if (file->f_mode & FMODE_READ) 
	{
		if (!is->audio_buffers)
		{
			if ( audio_setup_buf(is, USE_SDRAM) ) 
				return -ENOMEM;
		}
		poll_wait(file, &is->wq, wait);

		if (atomic_read(&is->audio_buffers->sem.count) > 0)
			mask |= POLLOUT | POLLWRNORM;
	}

	if (file->f_mode & FMODE_WRITE) {
		if ((!os->audio_buffers) || (!os->decoder_buffers))
		{
			if ( audio_setup_buf(is, USE_SRAM) ) 
				return -ENOMEM;
		}
		poll_wait(file, &os->wq, wait);

		if (atomic_read(&os->decoder_buffers->sem.count) > 0)
			mask |= POLLOUT | POLLWRNORM;
	}

	DPRINTK("stmp_audio_poll() returned mask of %s %s\n",
		(mask & POLLIN) ? "r" : "", (mask & POLLOUT) ? "w" : "");

	return mask;
}

static loff_t stmp_audio_llseek(struct file *file, loff_t offset, int origin)
{
	return -ESPIPE;
}

static int audio_set_fragments(audio_stream_t *s, int val)
{
	DPRINTK(" audio_set_fragments()\n");

	if (s->active)
	{
		printk("[DRIVER] We can't set fragments!! now is playing..\n"); 	
		return -EBUSY;
	}

#ifdef CONFIG_STMP36XX_SRAM
	s->nbfrags = (val >> 16) & 0x7FFF;
	
	val &= 0xffff;
	/* val 0xf:32K 0xe:16K 0xd:8K 0xc:4K 0xb:2K */
	if (val < 11)
	{
		printk("[DRIVER] We can't support under 2KBytes fragsize\n");		
		return -EINVAL;
	}
	if (val > 15)
		val = 15;

	s->fragsize = 1 << val; // 4KB ~ 28KB

	DPRINTK("[DRIVER] audio_set_fragment [nbfrags:%d] [fragsize:%d] [buffer size:%d]\n", s->nbfrags, s->fragsize, s->nbfrags*s->fragsize);

	if (s->nbfrags < 1)
		s->nbfrags = 1;
	
	if (s->nbfrags * s->fragsize > STMP37XX_SRAM_AUDIO_KERNEL_SIZE)
	{
		printk("[DRIVER] We don't support the buffer size over [%dBytes]\n", STMP37XX_SRAM_AUDIO_KERNEL_SIZE);		
		return -EINVAL;
	}
	else if (s->nbfrags * s->fragsize < BUF_4K_SIZE)
	{
		printk("[DRIVER] We don't support the buffer size under 4KBytes\n");		
		return -EINVAL;
	}
#else

	if (s->audio_buffers || s->decoder_buffers)
		audio_clear_buf(s, USE_SDRAM);
	
	s->nbfrags = (val >> 16) & 0x7FFF;
	
	val &= 0xffff;
	if (val < 4)
		val = 4;
	if (val > 15)
		val = 15;
	s->fragsize = 1 << val;

	if (s->nbfrags < 2)
		s->nbfrags = 2;
	if (s->nbfrags * s->fragsize > 256 * 1024)
		s->nbfrags = 256 * 1024 / s->fragsize;
	s->nbfrags = 2;

   if (audio_setup_buf(s, USE_SDRAM))
		return -ENOMEM;
#endif
	return val | (s->nbfrags << 16);
}

static int audio_diagnosis_test(audio_stream_t* s, int channel)
{
	s->channel = AUDIO_STEREO;
/*
	if(channel == AUDIO_STEREO)
	{
		s->channel = AUDIO_STEREO;
	}
	else if(channel == AUDIO_RIGHT)
	{
		s->channel = AUDIO_RIGHT;
	}
	else if(channel == AUDIO_LEFT)
	{
		s->channel = AUDIO_LEFT;
	}
	*/
	return 0;
}

static int stmp_audio_mmap(struct file *file, struct vm_area_struct *vma)
{
	audio_state_t *state = file->private_data;
	audio_stream_t *s;
	unsigned long start = 0;
	unsigned long offset = 0;
	unsigned long size = 0;
	unsigned long pfn = 0;

	DPRINTK("[DRIVER] stmp_audio_mmap\n");

	if (((file->f_flags & O_WRONLY) != 0) ||
		((file->f_flags & O_RDWR) != 0)) {
		vma->vm_page_prot = (pgprot_t)PAGE_SHARED;
	} 
	else {
		vma->vm_page_prot = (pgprot_t)PAGE_READONLY;
	}
	if (vma->vm_flags & VM_WRITE) {
		if (!state->wr_ref)
			return -EINVAL;
		s = state->output_stream;

	} else if (vma->vm_flags & VM_READ) {
		if (!state->rd_ref)
			return -EINVAL;
		s = state->input_stream;
	} else return -EINVAL;

	/* physical sram audio start pointer start = 0x0000F000 */
	#if 0 // for stmp3650
	start = (STMP37XX_SRAM_AUDIO_KERNEL_START+STMP37XX_SRAM_AUDIO_KERNEL_SIZE) & PAGE_MASK;
	#else // for stmp3750
	start = OCRAM_CODEC_START & PAGE_MASK;
	#endif
	offset = vma->vm_pgoff << PAGE_SHIFT;

	vma->vm_pgoff = offset >> PAGE_SHIFT;
	/* Don't try to swap out physical pages.. */
	vma->vm_flags |= VM_RESERVED | VM_LOCKED;
	size = vma->vm_end - vma->vm_start;

	DPRINTK("[DRIVER] MMAP vma->vm_start = %x, size = %d, offset = %d\n", vma->vm_start, size, offset);

#ifdef CONFIG_STMP36XX_SRAM
	/* for wma init code section -- it use SDRAM */
	if (vma->vm_start == WMA_INIT_CODE_VIRT)
	{
		pfn = state->output_stream->codec_info->wma_init_code_phys;
		DPRINTK("[DRIVER] MMAP for WMA init code uses SDRAM\n");
	}
	else
	{
		/* ogg code section will be loaded SDRAM */
		if(size == OGG_SDRAM_CODE_SIZE)
		{
			pfn = state->output_stream->codec_info->code_phys;
			DPRINTK("[DRIVER] MMAP for Ogg V code uses SDRAM\n");			
		}
		else
		{
			/* for codec(mp3, wma, ogg) run code & data section */
			pfn = start+offset;
		}
	}
#else
	static unsigned long* pCodeVirt = (unsigned long*)NULL;
	static unsigned long pCodePhys = 0;

	pCodeVirt = (unsigned long*)kmalloc(size, GFP_KERNEL);
	pCodePhys = virt_to_phys(pCodeVirt);
	pfn = pCodePhys;
#endif

	DPRINTK("[DRIVER] mmap physical address = %x\n", pfn);			
	if (remap_pfn_range(vma, vma->vm_start, pfn>>PAGE_SHIFT,
		size, vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

static int stmp_audio_ioctl(struct inode *inode, struct file *file,
			   uint cmd, ulong arg)
{
	audio_state_t *state = (audio_state_t *)file->private_data;
	audio_stream_t *os = state->output_stream;
	audio_stream_t *is = state->input_stream;
	long val;
	int ret = 0;
	unsigned long sampleRate = 0;
	int buf_size = 0;

	//DPRINTK(" stmp_audio_ioctl() - cmd: 0x%X, arg: 0x%X\n", cmd, (int)arg);
	/* dispatch based on command */
	switch (cmd) {
	case OSS_GETVERSION:
		return put_user(SOUND_VERSION, (int *)arg);

	case SNDCTL_DSP_SETFMT:
		if (get_user(val, (int *) arg))
			return -EINVAL;
		DPRINTK("[DRIVER] SNDCTL_DSP_SETFMT val=%d\n", val);
		set_pcm_audio_bit(val);
		break;
		
	case SNDCTL_DSP_GETFMTS:
		DPRINTK("[DRIVER] SNDCTL_DSP_GETFMTS FMT=%d\n", AUDIO_FMT);
		/* Simple standard DACs are 16-bit only */
		return put_user(AUDIO_FMT, (long *) arg);

	case SNDCTL_DSP_GETBLKSIZE:
		DPRINTK("[DRIVER] SNDCTL_DSP_GETBLKSIZE fragsize=%d\n", os->fragsize);
		
		if (file->f_mode & FMODE_WRITE)
			return put_user(os->fragsize, (int *)arg);
		else
			return put_user(is->fragsize, (int *)arg);

	case SNDCTL_DSP_SETFRAGMENT:
		if (get_user(val, (long *) arg))
			return -EFAULT;

		DPRINTK("[DRIVER] SNDCTL_DSP_SETFRAGMENT\n");

		#if 0 //we fix 256K read buffer
		if(gIsAUDIOIN)
		{
			if (file->f_mode & FMODE_READ){
				int ret = audio_set_fragments(is, val);
				if (ret < 0)
					return ret;

				setAdcBufSize(is->nbfrags * is->fragsize);
				int buf_size = 0;
				buf_size = getAdcBufSize();
				printk("buf_size = %d\n", buf_size);

				setAdcDMADesc(is->audio_buffers->phyStartaddr, buf_size);			

				ret = put_user(ret, (int *)arg);
				if (ret)
					return ret;
			}
		}
		#endif
		
		if (file->f_mode & FMODE_WRITE) {
			ret = audio_set_fragments(os, val);
			if (ret < 0)
				return ret;

			setBufSize(os->nbfrags * os->fragsize);
			buf_size = getBufSize();

			setDMADesc(os->audio_buffers->phyStartaddr, buf_size);

			ret = put_user(ret, (int *)arg);
			if (ret)
				return ret;
		}
		return 0;

	case SNDCTL_DSP_SYNC:
		return audio_sync(file);

	case SNDCTL_DSP_GETODELAY:
	{
		//get_dma_bytes_inuse(&qstate.remaining_now_bytes, &qstate.remaining_next_bytes);
		//ret = put_user(qstate.remaining_next_bytes, (int*)arg);		
		int frags = 0, bytes = 0;
		int buf_size = 0;
		buf_size = getBufSize();
		bytes = buf_size - os->decoder_buffers->decoderValidBytes;

		ret = put_user(bytes, (int*)arg);
		break;
	}

	case SNDCTL_DSP_GETOSPACE:
	{
		audio_buf_info *inf = (audio_buf_info *) arg;
		
		// for linux 2.4.X TODO
#if 0
		int err = verify_area(VERIFY_WRITE, inf, sizeof(*inf));
		if (!(file->f_mode & FMODE_WRITE))
			return -EINVAL;

		if (err)
			return err;
#endif

		int frags = 0, bytes = 0;
		int buf_size = 0;
		buf_size = getBufSize();
		bytes = buf_size - os->decoder_buffers->decoderValidBytes;
		
		frags = os->nbfrags;

		put_user(bytes/os->fragsize, &inf->fragments);
		put_user(buf_size/os->fragsize, &inf->fragstotal);
		put_user(os->fragsize, &inf->fragsize);
		put_user(bytes, &inf->bytes);
		//DPRINTK("[DRIVER] SNDCTL_DSP_GETOSPACE fragments=%d, fragstotal=%d, fragsize=%d, bytes=%d\n", frags, os->nbfrags, os->fragsize, bytes);
		break;
	}

	case SNDCTL_DSP_RESET:
	{
		if (file->f_mode & FMODE_WRITE)
		{
			DPRINTK("[DRIVER] audio_dsp_reset\n");
			if(os->audio_buffers->initDevice) {
				setVolumeFade(FADE_OUT);
				
				while(os->audio_buffers->initDevice == 1)		
				{
					if (down_interruptible(&os->audio_buffers->sem))
							return -ERESTARTSYS;
					
					if(os->audio_buffers->initDevice == 0) {
						up(&os->audio_buffers->sem);
						break;
					}
				}
			}
			else {
				audio_reset_buf(os);
			}
		}
		if(gIsAUDIOIN)
		{
			if (file->f_mode & FMODE_READ)
			{
				if(is->audio_buffers->initDevice) {
					stopAdcProcessing(is);
				}
				else {
					audio_reset_buf(is);
				}
			}
		}
		return 0;
	}

	case SNDCTL_DSP_NONBLOCK:
		file->f_flags |= O_NONBLOCK;
		return 0;

	case SNDCTL_DSP_DIAGNOSIS_TEST:
		if (get_user(val, (long *) arg))
			return -EFAULT;
		return audio_diagnosis_test(os, val);

	case SNDCTL_DSP_COMMON_OUTPUT:
		setSWVersion(VER_COMMON);
		return 0;
		
	case SNDCTL_DSP_FRANCE_OUTPUT:
		setSWVersion(VER_FR);
		return 0;

	case SNDCTL_DSP_NORMAL_PROCESSING:
		setPostProcessingFlag(os->audio_buffers, NO_PROCESSING, (int)0);
		return 0;
		
	case SNDCTL_DSP_DNSE_PROCESSING:
		if (get_user(val, (long *) arg))
			return -EFAULT;
		setPostProcessingFlag(os->audio_buffers, DNSE_PROCESSING, (int)(val&0xF));
		return 0;

	case SNDCTL_DSP_SET_DEFAULT_VOL:
		if (get_user(val, (long *) arg))
			return -EFAULT;
		
		setDefaultVolume(val);
		return 0;

	case SNDCTL_DSP_SET_PLAY_SPEED:
		if (get_user(val, (long *) arg))
			return -EFAULT;
		
		setPlaySpeed(val);
		return 0;		

	case SNDCTL_DSP_SET_FADE_EFFECT:
		if (get_user(val, (long *) arg))
			return -EFAULT;
		setDSPFade(val);
		return 0;		

	case SNDCTL_DSP_ON_HW_MUTE: 
		onHWmute();
		return 0;
		
	case SNDCTL_DSP_GETOPTR:
	case SNDCTL_DSP_GETIPTR:
	case SNDCTL_DSP_GETISPACE:
	case SNDCTL_DSP_GETCAPS:
	case SNDCTL_DSP_POST:
	case SNDCTL_DSP_GETTRIGGER:
	case SNDCTL_DSP_SETTRIGGER:
	case SNDCTL_DSP_SUBDIVIDE:
	case SNDCTL_DSP_MAPINBUF:
	case SNDCTL_DSP_MAPOUTBUF:
	case SNDCTL_DSP_SETSYNCRO:
	case SNDCTL_DSP_SETDUPLEX:
		return -ENOSYS;

	case SNDCTL_DSP_STEREO:
		ret = get_user(val, (int *) arg);
		if (ret)
			return ret;
		/* stmp36xx DACs are stereo only */
		ret = (val == 0) ? -EINVAL : 1;
		return put_user(ret, (int *) arg);
	
	case SNDCTL_DSP_CHANNELS:
	case SOUND_PCM_READ_CHANNELS:
		/* stmp36xx DACs are stereo only */
		return put_user(AUDIO_CHANNELS, (long *) arg);
	
	case SNDCTL_DSP_SPEED:
		ret = get_user(val, (long *) arg);
		if (ret) 
			break;
		setDacSamplingRate(val);
		if(gIsAUDIOIN)
			setADCSamplingRate(val);
		/* fall through */
	
	case SOUND_PCM_READ_RATE:
		sampleRate = getDacSamplingRate();
		return put_user(sampleRate, (long *) arg);
	
	default:
	  /* Maybe this is meant for the mixer (As per OSS Docs) */
	  return stmp_mixer_ioctl(inode, file, cmd, arg);
	}

	return 0;
}

static int stmp_audio_release(struct inode *inode, struct file *file)
{
	audio_state_t  *state = file->private_data;
	
	down(&state->sem);

	if (file->f_mode & FMODE_READ)
	{
		state->rd_ref = 0;
	}

	if (file->f_mode & FMODE_WRITE)
	{
		DPRINTK("[DRIVER] audio released\n");
		audio_clear_buf_data(state->output_stream);
		state->wr_ref = 0;
	}

	up(&state->sem);

	return 0;
}


static int stmp_audio_open(struct inode *inode, struct file *file)
{
	int err;
	/* TODO for 37xx */
#if 0	
	version_inf_t *sw_ver ;
#endif

	audio_state_t *state = &stmp_audio_state;

	DPRINTK("[DRIVER] audio_open\n");
	
	down(&state->sem);
	/* access control */
	err = -ENODEV;
	if ((file->f_mode & FMODE_WRITE) && !state->output_stream)
		goto out;
	if ((file->f_mode & FMODE_READ) && !state->input_stream)
		goto out;
	err = -EBUSY;
	if ((file->f_mode & FMODE_WRITE) && state->wr_ref)
		goto out;
	if ((file->f_mode & FMODE_READ) && state->rd_ref)
		goto out;
	err = -EINVAL;

	/* stmp36xx don't need DMA irq in here */
	/* stmp36xx has APBX DAC DMA & ADC DMA. dma is initied by DMA setting at initializeAudioout() */
	/* And stmp36xx has DMA irq and DMA err irq for DAC. */
	/* So dma irq is already requested at module init */


	/* stmp audio out to dac */
	if ((file->f_mode & FMODE_WRITE)) {
		state->wr_ref = 1;
		state->output_stream->channel = AUDIO_STEREO;
		state->output_stream->fragsize = ONE_FRAGMENT_SIZE;
		state->output_stream->nbfrags = BUF_28K_SIZE/state->output_stream->fragsize;
		state->output_stream->bytecount = 0;
		state->output_stream->fragcount = 0;
		state->output_stream->usr_rwcnt = 0;
		init_waitqueue_head(&state->output_stream->wq);

		if(!isInitDac)
		{
			/* allocate sram buffer */
			if((err = audio_setup_buf(state->output_stream, USE_SRAM)) != 0)
				goto out;
			/* alloc codec(user) sdram memory to use when usb is connected */
			if((err = allocCodecSDRAM(state->output_stream)) != 0)
				goto out;
			/* allocate dma desc */
			if((err = allocDMADac_desc(STMP37XX_SRAM_AUDIO_KERNEL_SIZE)) != 0) //for APBX dac dma
				goto out;

			initializeAudioout(state->output_stream);

			isInitDac = 1;
			DPRINTK("[DRIVER] audioout/dac buffer size [%dbytes]\n", getBufSize());
			/* register call for idle event from power module */
			/* TODO */
			ss_pm_register(SS_PM_SND_DEV, audio_check_idle);
			
		}
		/* init write sem */
		sema_init(&state->output_stream->decoder_buffers->sem, 1);
		/* init set fragment size related value */
		setBufSize(state->output_stream->fragsize*state->output_stream->nbfrags);
		//setDMADesc(state->output_stream->audio_buffers->phyStartaddr, state->output_stream->fragsize*state->output_stream->nbfrags);
	}

	if(gIsAUDIOIN)
	{
		/* stmp audio in from linein to adc */
		if (file->f_mode & FMODE_READ) {
			state->rd_ref = 1;
			state->input_stream->channel = AUDIO_STEREO;
			state->input_stream->fragsize = ONE_FRAGMENT_SIZE;
			state->input_stream->nbfrags = ADC_BUF_SIZE/ONE_FRAGMENT_SIZE;
			state->input_stream->bytecount = 0;
			state->input_stream->fragcount = 0;
			state->input_stream->usr_rwcnt = 0;
			sema_init(&state->input_stream->sem, 1);
			init_waitqueue_head(&state->input_stream->wq);

			if(!isInitAdc)
			{
				/* allocate sram buffer */
				if((err = audio_setup_buf(state->input_stream, USE_SDRAM)) != 0)
					goto out;
				/* allocate dma desc */
				if((err = allocDMAAdc_desc(ADC_BUF_SIZE)) != 0) //for APBX dac dma
					goto out;

				/* adc buffer info */
				state->input_stream->adc_info = (adc_info_t *)kmalloc(sizeof(adc_info_t), GFP_KERNEL);
				if(!state->input_stream->adc_info)
					goto out;
				memset(state->input_stream->adc_info, 0, sizeof(adc_info_t));
				isInitAdc = 1;
				DPRINTK("[DRIVER] audioin/adc buffer size [%dbytes]\n", ADC_BUF_SIZE);

				/* register call for idle event from power module */
				//ss_fm_register(SS_FM_DSP_DEV, audio_check_fm);

			}
			/* 최초 cold boot 시 ADC power on 하지 않음 - Recording 하는 경우만 ADC init하고 Recording stop시는 ADC power off 함 */
			/* ADC power on/off시 1.5mW 차이 남 */
			//initializeADC();			
			setAdcBufSize(state->input_stream->fragsize*state->input_stream->nbfrags);
			setAdcDMADesc(state->input_stream->audio_buffers->phyStartaddr, state->input_stream->fragsize*state->input_stream->nbfrags);
			initializeAudioin(state->input_stream);
			udelay(1000);
			stopAdcProcessing(state->input_stream);
			setRecordingStatus(NO_RECORDING);
			audio_reset_buf(state->input_stream);
		}
	}

	/* Check system version */
	get_sw_version(&version_info);
	if(strcmp(version_info.nation, "FR") == 0) {
		DPRINTK("[DRIVER] contry code %s volume table\n", version_info.nation);
		setSWVersion(VER_FR);
	}
	else
	{
		DPRINTK("[DRIVER] contry code %s volume table\n", version_info.nation);
		setSWVersion(VER_COMMON);
	}

	/* check board revision */
	int type = get_hw_option_type();
	DPRINTK("[DRIVER] dsp oppen : check board type = %d\n", type);
	setBoardRevision((unsigned short)type);

	file->private_data	= state;
	/*
	file->f_op->release = stmp_audio_release;
	file->f_op->write	= stmp_audio_write;
	file->f_op->read	= stmp_audio_read;
	file->f_op->mmap	= stmp_audio_mmap;
	file->f_op->poll	= stmp_audio_poll;
	file->f_op->ioctl	= stmp_audio_ioctl;
	file->f_op->llseek	= stmp_audio_llseek;
*/
	err = 0;

out:
	up(&state->sem);
	return err;
}

/*
 * Missing fields of this structure will be patched with the call
 * to stmp_audio_attach().
 */
static struct file_operations stmp_audio_fops = {
	.read = stmp_audio_read,
	.write = stmp_audio_write,
	.ioctl = stmp_audio_ioctl,
	.open = stmp_audio_open,
	.poll = stmp_audio_poll,
	.release = stmp_audio_release,
	.mmap = stmp_audio_mmap,
	.llseek = stmp_audio_llseek
};


static int stmp37xx_dsp;
static int stmp37xx_mixer;

static int __init
stmp37xx_audio_init(void)
{
	/* request DMA irqs */
	int ret = 0;
	
    stmp37xx_dsp = register_sound_dsp(&stmp_audio_fops, -1);
    stmp37xx_mixer = register_sound_mixer(&stmp_mixer_fops, -1);
    
	/* register device handle */
    /*devfs_handle = devfs_register(NULL, SMTP_AUDIO_NAME, DEVFS_FL_DEFAULT,
				  SMTP_AUDIO_MAJOR, 0,
				  S_IFCHR | S_IRUSR | S_IWUSR,
				  &stmp_dac_fops, NULL);
	*/

	/* request irq */
	ret = request_irq(IRQ_DAC_DMA, stmp_dac_dma_isr, IRQF_DISABLED, 
			"dac_dma", &stmp_audio_state);
	if (ret) {
		printk(KERN_INFO " --> can't get assinged irq %s(%d): IRQ %d : T.T\n",
			__FUNCTION__, __LINE__, IRQ_DAC_DMA);
		goto out1;
	}

	/* register dac_underflow_irq */
	ret = request_irq(IRQ_DAC_ERROR, stmp_dac_dmaerr_isr, IRQF_DISABLED, 
			"dac_dma_err", &stmp_audio_state);
	if (ret) {
		printk(KERN_INFO " --> can't get assinged irq %s(%d): IRQ %d : T.T\n",
			__FUNCTION__, __LINE__, IRQ_DAC_ERROR);
		goto out2;
	}
	printk("[DRIVER] AUDIOOUT/DAC Init !!!\n"); 

#if defined(CONFIG_SOUND_STMP36XX_AUDIOIN)
	gIsAUDIOIN = 1;
	/* request irq */
	ret = request_irq(IRQ_ADC_DMA, stmp_adc_dma_isr, IRQF_DISABLED, 
			"adc_dma", &stmp_audio_state);
	if (ret) {
		printk(KERN_INFO " --> can't get assinged irq %s(%d): IRQ %d : T.T\n",
			__FUNCTION__, __LINE__, IRQ_ADC_DMA);
		goto out3;
	}

	/* register dac_underflow_irq */
	ret = request_irq(IRQ_ADC_ERROR, stmp_adc_dmaerr_isr, IRQF_DISABLED,
			"adc_dma_err", &stmp_audio_state);
	if (ret) {
		printk(KERN_INFO " --> can't get assinged irq %s(%d): IRQ %d : T.T\n",
			__FUNCTION__, __LINE__, IRQ_ADC_ERROR);
		goto out4;
	}
	printk("[DRIVER] AUDIOIN/ADC Init\n");
#else
	gIsAUDIOIN = 0;
#endif

	return ret;
	
	out4:
	free_irq(IRQ_ADC_DMA, NULL);
	out3:
	free_irq(IRQ_DAC_ERROR, NULL);
	out2:
	free_irq(IRQ_DAC_DMA, NULL);
	out1:
	return ret;
}

static void __exit
stmp37xx_audio_exit(void)
{
	unregister_sound_dsp(stmp37xx_dsp);
	unregister_sound_mixer(stmp37xx_mixer);

	// for linux 2.4.X
    //devfs_unregister(devfs_handle);

	/* free for audioout */
	exitAudioout();
	audio_clear_buf((audio_stream_t *)&stmp_audio_state.output_stream, USE_SRAM);
	freeCodecSDRAM((audio_stream_t *)&stmp_audio_state.output_stream);
    freeDMA_Desc(ONE_FRAGMENT_SIZE * FRAGMENT_NBR);

	if(gIsAUDIOIN)
	{	/* free for audioin */
		exitAudioin();
		audio_clear_buf((audio_stream_t *)&stmp_audio_state.input_stream, USE_SDRAM);
		freeAdcDMA_Desc(ONE_FRAGMENT_SIZE * FRAGMENT_NBR);
		kfree((const void *)&stmp_audio_state.input_stream->adc_info);
	}
}

module_init(stmp37xx_audio_init);
module_exit(stmp37xx_audio_exit);

