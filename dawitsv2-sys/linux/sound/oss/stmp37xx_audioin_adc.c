/* $Id: stmp36xx_audioin_adc.c,v 1.36 2008/03/18 08:25:12 zzinho Exp $ */

/**
 * \file stmp36xx_audioin_adc.c
 * \brief audioout of stmp36xx
 * \author LIM JIN HO <jinho.lim@samsung.com>
 * \version $Revision: 1.36 $
 * \date $Date: 2008/03/18 08:25:12 $
 *
 * This file implements audioin(ADC) driver for SMTP36XX(sigmatel).
 * $Log: stmp36xx_audioin_adc.c,v $
 * Revision 1.36  2008/03/18 08:25:12  zzinho
 * stop adc -> power down adc module
 *
 * by jinho
 *
 * Revision 1.35  2008/03/07 03:17:08  zzinho
 * MIC power is changed LRADC0 -> LRADC1
 *
 * by jinho
 *
 * Revision 1.34  2008/01/15 01:44:10  zzinho
 * change MIC bias select
 * LRADC0 => LRADC1
 * by jinho
 *
 * Revision 1.33  2006/09/21 23:52:43  biglow
 * ADC off
 *
 * Revision 1.32  2006/08/08 07:32:01  release
 * - Reduce Rec Noise for changing MIC settings
 *
 * Revision 1.31  2006/05/19 03:43:42  zzinho
 * update with static sound driver
 *
 * Revision 1.45  2006/05/10 09:30:37  zzinho
 * remove printk debug
 *
 * Revision 1.44  2006/05/10 09:20:51  zzinho
 * remove printk debug
 *
 * Revision 1.43  2006/05/10 07:21:03  zzinho
 * audioin DMA reset after decrementing sema
 *
 * Revision 1.42  2006/04/29 08:21:22  yoonhark
 * (1) fixing ADC bug for mute
 * (2) changing from 160*1024 to 50*1024 for INIT_READ_MUTE_COUNT
 *
 * Revision 1.40  2006/04/18 14:45:22  yoonhark
 * changing mute time from 80*1024 to 160*1024 for safe removing pop noise in recording file
 *
 * Revision 1.39  2006/04/17 04:59:33  yoonhark
 * modification for removing pop noise in start part of recording file
 *
 * Revision 1.38  2006/03/28 07:09:07  zzinho
 * default adc sampling rate 44100 -> 22050
 *
 * Revision 1.37  2006/03/15 08:32:41  zzinho
 * ADC init when dsp open
 *
 * Revision 1.36  2006/03/15 06:30:38  zzinho
 * MIC Resistor IOCTL addition
 *
 * Revision 1.35  2006/03/14 12:04:12  zzinho
 * add mic_bias ioctl to control mic bias
 *
 * Revision 1.34  2006/03/14 11:46:27  zzinho
 * add mic_bias ioctl to control mic bias
 *
 * Revision 1.33  2006/03/06 00:45:33  zzinho
 * fix ADC volume to 100 and change init Mux volume to 0
 *
 * Revision 1.32  2006/03/06 00:33:09  zzinho
 * fix ADC volume to 100
 *
 * Revision 1.31  2006/03/03 00:17:38  zzinho
 * remove debug
 *
 * Revision 1.30  2006/03/02 05:49:13  zzinho
 * back audioin init
 *
 * Revision 1.29  2006/03/02 05:37:58  zzinho
 * remove adc vag
 *
 * Revision 1.28  2006/03/02 01:26:50  zzinho
 * TA5 -> TB1 modify
 *
 * Revision 1.27  2006/02/28 07:29:56  zzinho
 * modify audioin
 *
 * Revision 1.26  2006/02/27 13:22:49  zzinho
 * remove dbg
 *
 * Revision 1.25  2006/02/27 13:14:07  zzinho
 * removed !USE_DOWN mode
 * and adc buffer change to 32Kbytes
 *
 * Revision 1.24  2006/02/22 00:42:11  zzinho
 * removed warning message
 *
 * Revision 1.23  2006/02/08 06:28:28  zzinho
 * audioin stop process modify
 *
 * Revision 1.22  2006/02/08 01:12:18  zzinho
 * audioin for phase 2
 *
 * Revision 1.21  2005/12/12 05:57:56  zzinho
 * define addition
 *
 * Revision 1.20  2005/11/12 08:27:40  zzinho
 * *** empty log message ***
 *
 * Revision 1.19  2005/11/09 05:33:30  zzinho
 * audio exit addition
 *
 * Revision 1.18  2005/11/03 05:06:03  zzinho
 * audio in modified
 *
 * Revision 1.17  2005/11/01 09:12:39  zzinho
 * refer define hardware.h
 *
 * Revision 1.16  2005/10/31 09:39:18  zzinho
 * buffer size naming changed
 *
 * Revision 1.15  2005/10/12 07:43:37  zzinho
 * buffer size define add
 *
 * Revision 1.14  2005/09/22 02:09:09  zzinho
 * change stream size
 *
 * Revision 1.13  2005/09/12 04:26:32  zzinho
 * add new ioctl for mic gain & adc gain
 *
 * Revision 1.12  2005/09/12 00:10:03  zzinho
 * add new ioctl for mic gain
 *
 * Revision 1.11  2005/09/08 06:03:15  zzinho
 * When input mic, gain is fixed
 * MIC gain : 0x0
 * ADC gain : Max 0xf
 *
 * Revision 1.10  2005/09/07 11:25:23  zzinho
 * add audioin
 *
 * Revision 1.9  2005/09/07 04:01:17  zzinho
 * add audioin and remove warning msg
 *
 * Revision 1.8  2005/09/06 10:37:07  zzinho
 * add audioin
 *
 * Revision 1.6  2005/09/02 02:24:28  zzinho
 * printf remove
 *
 * Revision 1.5  2005/08/23 09:16:07  zzinho
 * remove warning msg
 *
 * jinho version up
 * Revision 1.4  2005/08/09 04:02:09  zzinho
=======
 * Revision 1.6  2005/09/02 02:24:28  zzinho
 * printf remove
 *
 * Revision 1.5  2005/08/23 09:16:07  zzinho
 * remove warning msg
 *
 * Revision 1.4  2005/08/09 04:02:09  zzinho
 * jinho version up
>>>>>>> 1.6
 *
 * Revision 1.3  2005/07/15 13:45:20  zzinho
 * *** empty log message ***
 *
 * Revision 1.2  2005/07/13 07:20:09  zzinho
 * modified
 * write and name
 *
 * Revision 1.1  2005/07/11 08:34:50  zzinho
 * add audioin
 *
 * Revision 0.1  2005/07/11 zzinho
 * - add first revision
 *
 */
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>

#include <asm/memory.h>
#include <asm/io.h>

#include <asm-arm/arch-stmp37xx/37xx/regs.h>
#include <asm-arm/arch-stmp37xx/37xx/regsaudioin.h>
#include <asm-arm/arch-stmp37xx/37xx/regsaudioout.h>
#include <asm-arm/arch-stmp37xx/37xx/regsapbx.h>
#include <asm-arm/arch-stmp37xx/37xx/regspinctrl.h>

#include "stmp37xx_audio.h"
#include "stmp37xx_audioin_adc.h"

#define ADC_READ_SIZE 400
#define INIT_READ_MUTE_COUNT (50*1024)

/*---------- Structure Definitions ----------*/
struct stmp_adc_value_t
{
	unsigned char volume;
	unsigned short volume_reg;
	unsigned short mic_gain;
	unsigned short adc_gain;
	unsigned short mic_bias;
	int recodingSrc;
	int buf_size;
	int FMStatus;
	unsigned long samplingRate;
};

struct stmp_adc_value_t stmp_adc_value = {
	volume: 100,
	volume_reg: 0xFF,
	mic_gain: 0x1,
	adc_gain: 0x0,
	mic_bias: 0x2,
	recodingSrc: INPUT_SRC_LINEIN,
	buf_size: ADC_BUF_SIZE,
	FMStatus: FM_RELEASE,
	samplingRate: 22050
};

struct stmp_dma_cmd_t
{
	volatile reg32_t *desc_cmd_ptr;
	volatile reg32_t phys_desc_cmd_ptr;
};

static volatile struct stmp_dma_cmd_t adc_dma_cmd = {
	desc_cmd_ptr: (unsigned int*)NULL,
	phys_desc_cmd_ptr: 0,
};

static unsigned long long gReadInitMuteCnt = 0;

/* mutex to access only one */
static spinlock_t adc_lock;

/* ------------ Definitions ------------ */

/* ------------ HW Dependent Functions ------------ */
static void addADCValidBytes(adc_info_t *adcInfo, signed int validBytes);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int allocDMAAdc_desc(unsigned int bufferSize);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void exitAudioin(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void freeADCDMA_Desc(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getRecodingSrc(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int getADCVolume(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
unsigned short getADCGain(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
unsigned int getAdcPointer(unsigned int phyadc_addr);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
unsigned long getADCSamplingRate(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
unsigned short getMICGain(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void initializeADC(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int initializeAudioin(audio_stream_t *s);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
static void subADCValidBytes(adc_info_t *adcInfo, signed int validBytes);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setAdcDMADesc(unsigned int p_adcPointer, unsigned long bufferSize);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setupADCDMA(unsigned int p_adcPointer, unsigned long adcInBytesSize);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setRecodingSrc(unsigned char input_src);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setADCGain(unsigned short gain);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setMICGain(unsigned short gain);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setMICBias(int mic_resistor);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void startADC(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void stopADC(void);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int stopAdcProcessing(audio_stream_t *audio_s);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setADCVolume(int val);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
void setADCSamplingRate(unsigned long samplingRate);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */
int updateADCData(audio_stream_t *audio_s);
/**
 * \brief 
 * \param 
 * \param 
 *
 * This function 
 */

/* ------------ Function Implementation ------------ */

static void addADCValidBytes(adc_info_t *adcInfo, signed int validBytes)
{
	spin_lock_irq(&adc_lock);
	adcInfo->ADCValidBytes+=validBytes;
    //printk("[DRIVER] add [%d], bytes[%d] \n", adcInfo->ADCValidBytes, validBytes);
	spin_unlock_irq(&adc_lock);

}

int allocDMAAdc_desc(unsigned int bufferSize)
{
	int dmadescsize = 0, n=0;
	unsigned int *desc_cmd_ptr = NULL;
	unsigned int phys_desc_cmd_ptr = 0;

	n = bufferSize / MIN_STREAM_SIZE;
	dmadescsize = DAC_DESC_SIZE*n;

	/* desc 0 consistent alloc */
	do {
		desc_cmd_ptr = dev_dma_alloc(dmadescsize, &phys_desc_cmd_ptr);

		if (!desc_cmd_ptr)
			dmadescsize -= DAC_DESC_SIZE*n;
	} while (!desc_cmd_ptr && dmadescsize);

	if(desc_cmd_ptr == (unsigned int *)NULL)
	{
		printk("[DRIVER] ADC_DMA: ERROR stmp36xx adc dma descriptor00 is not allocated\n");
		return -ENOMEM;
	}
	
	memzero(desc_cmd_ptr, dmadescsize);

	adc_dma_cmd.desc_cmd_ptr = desc_cmd_ptr;
	adc_dma_cmd.phys_desc_cmd_ptr = phys_desc_cmd_ptr;

	return 0;
}

void exitAudioin(void)
{
    BF_CS2(AUDIOIN_ADCVOLUME, 
        VOLUME_LEFT,  0x37, 
        VOLUME_RIGHT, 0x37);
	BW_APBX_CTRL1_CH0_CMDCMPLT_IRQ_EN(0);
    stopADC();
}

int fillEncoderBuffer(audio_stream_t *audio_s, const void *userBuffer, int userBufferSize)
{
	adc_info_t *adcInfo = audio_s->adc_info;
	audio_buf_t *adc_b = audio_s->audio_buffers;
	signed int validBytes = 0;
	unsigned int len=0, lastReadPosition=0;
	unsigned int buffer_size = 0;
	int iFMStatus = FM_RELEASE;

	buffer_size = audio_s->fragsize * audio_s->nbfrags;

	validBytes = getADCValidBytes(adcInfo);
	lastReadPosition = adcInfo->ADCReadPointer;
	//printk("[FillEncoder] valid[%d], lastRead[%d], userBufferSize[%d]\n", validBytes, lastReadPosition, userBufferSize);

	if(validBytes > 0)
	{
		if(validBytes > userBufferSize)
			len = userBufferSize;
		else
			len = validBytes;
		
		if( (lastReadPosition + len) <= buffer_size )
		{
			iFMStatus = getFMStatus();
			if( (gReadInitMuteCnt < INIT_READ_MUTE_COUNT) && (iFMStatus == FM_RELEASE) )
			{
				//printk("ReadInitMute %d\n", gReadInitMuteCnt);
				memset((void *)adc_b->pStart+lastReadPosition, 0, len);
			}
			
			if(copy_to_user((void*)userBuffer, (const void*)adc_b->pStart+lastReadPosition, len))
			{
				printk("[DRIVER] FillEncoderBuffer1 : pcm data copy_to_user faill!\n");
				up(&adc_b->sem);
				return -EFAULT;
			}

			gReadInitMuteCnt += len;

			subADCValidBytes(adcInfo, len);
			/* Adjust our read pointer */
			adcInfo->ADCReadPointer += len;
			if (adcInfo->ADCReadPointer >= buffer_size)
			{
				adcInfo->ADCReadPointer = adcInfo->ADCReadPointer - buffer_size; //Wrap it around
			} 
		}
		else
		{
			unsigned int bytesRemainingADC = buffer_size - lastReadPosition;
			unsigned int tmp = len - bytesRemainingADC;
			
			iFMStatus = getFMStatus();
			if( (gReadInitMuteCnt < INIT_READ_MUTE_COUNT) && (iFMStatus == FM_RELEASE) )
			{
				//printk("ReadInitMute %d\n", gReadInitMuteCnt);
				memset((void *)adc_b->pStart+lastReadPosition, 0, bytesRemainingADC);
				memset((void *)adc_b->pStart, 0, tmp);
			}
			
			if(copy_to_user((void*)userBuffer, (const void*)adc_b->pStart+lastReadPosition, bytesRemainingADC))
			{
				printk("[DRIVER] FillEncoderBuffer2 : pcm data copy_to_user faill!\n");
				up(&adc_b->sem);
				return -EFAULT;
			}
			
			if(copy_to_user((void*)userBuffer+bytesRemainingADC, (const void*)adc_b->pStart, tmp))
			{
				printk("[DRIVER] FillEncoderBuffer3 : pcm data copy_to_user faill!\n");
				up(&adc_b->sem);
				return -EFAULT;
			}

			gReadInitMuteCnt += len;

			subADCValidBytes(adcInfo, len);
			//Adjust our read pointer
			adcInfo->ADCReadPointer=tmp;
		}
		//printk("[Read] Rptr[%d], len[%d]\n", adcInfo->ADCReadPointer, len);
	}

	return len;
	
}

void freeAdcDMA_Desc(unsigned int bufferSize)
{
	int dmadescsize = 0, n=0;

	n = bufferSize / MIN_STREAM_SIZE;
	dmadescsize = DAC_DESC_SIZE*n;

	/* free DMAAdc_desc */
	if (adc_dma_cmd.desc_cmd_ptr) 
	{
		dev_dma_free(dmadescsize,
				  adc_dma_cmd.desc_cmd_ptr, adc_dma_cmd.phys_desc_cmd_ptr);
		
		adc_dma_cmd.desc_cmd_ptr = (reg32_t *)NULL;
		adc_dma_cmd.phys_desc_cmd_ptr = (reg32_t)NULL;
	}
}

int getRecodingSrc(void)
{
	return stmp_adc_value.recodingSrc;
}

signed int getADCValidBytes(adc_info_t *adcInfo)
{
    signed int ret;
	spin_lock_irq(&adc_lock);
	ret=adcInfo->ADCValidBytes;
	spin_unlock_irq(&adc_lock);
    return ret;
}

unsigned short getADCGain(void)
{
	return stmp_adc_value.adc_gain;
}

unsigned short getMICGain(void)
{
	return stmp_adc_value.mic_gain;
}

unsigned short getFMStatus(void)
{
	return stmp_adc_value.FMStatus;
}

int initializeAudioin(audio_stream_t *s)
{
	audio_stream_t *audio_s = (audio_stream_t *)s;
	audio_buf_t *audio_b = audio_s->audio_buffers;

	DPRINTK("initializeAudioin()\n");
	
	// DMA setup & enable IRQ
	setupAdcDMA(audio_b->phyStartaddr, audio_s->fragsize*audio_s->nbfrags);

	initializeADC();

	startADC();
	return 0;
}

void initializeADC(void)
{
	DPRINTK("initializeADC()\n");

    /************************************************/
    /*** Initialize Digital Filter ADC (AUDIOIN) ***/
    /************************************************/
    /* Clear SFTRST, Turn off Clock Gating, Set Loopback. */
    BF_CS3(AUDIOIN_CTRL, SFTRST, 0, CLKGATE, 0, LOOPBACK, 0);  // cleared LOOPBACK to see 1 bit outputs.
    BF_CS1(AUDIOIN_ANACLKCTRL, CLKGATE, 0);

    BF_CS1(AUDIOIN_CTRL, WORD_LENGTH, 1);
	
    BF_CS1(AUDIOOUT_PWRDN, ADC, 0);

    /* Set the ADC VAG */
//    BF_CS4(AUDIOOUT_REFCTRL, ADJ_ADC, 1,
//                            ADC_REFVAL, 0xF, ADJ_VAG, 1, VAG_VAL, 0xB);

	/* Set analog ADC input mux */
	setRecodingSrc(stmp_adc_value.recodingSrc);
		
    /* Set Over Sample Rate, Base Rate Multiplier, & Sample Rate Conversion Factor. */
	setADCSamplingRate(stmp_adc_value.samplingRate);

    /* Clear Left & Right Mute */
    setADCVolume(stmp_adc_value.volume);
    
    return;
}   // initialize()

unsigned long getADCSamplingRate(void)
{
	return stmp_adc_value.samplingRate;
}

int getADCVolume(void)
{
	return stmp_adc_value.volume;
}

unsigned int getAdcPointer(unsigned int phyadc_addr)
{
	hw_apbx_chn_debug2_t ptr;
	unsigned int dmaBytes;
	
	ptr=HW_APBX_CHn_DEBUG2(0);

	dmaBytes=(HW_APBX_CHn_BAR_RD(0) - (unsigned long)phyadc_addr);

	dmaBytes &= ~1; //If were really 1 byte extra into it, forget it... we don't play mono samples.
	
	return dmaBytes;
}

int getAdcBufSize(void)
{
	return stmp_adc_value.buf_size;
}

void setADCValidBytes(adc_info_t *adcInfo, signed int validBytes)
{
	spin_lock_irq(&adc_lock);
	adcInfo->ADCValidBytes = validBytes;
	spin_unlock_irq(&adc_lock);
}

void setAdcBufSize(int buf_size)
{
	stmp_adc_value.buf_size = buf_size;
}

void setADCVolume(int val)
{
	/* adc volume */
	/* Volume ranges from full scale 0dB (0xFF) to -100dB (0x37). Each increment of this bit-field causes */
	/* a half dB increase in volume. Note that values 0x00-0x37 all produce the same attenuation level of -100dB. */
	/* So convert 0 -> 100 volume to 0x37 -> 0xFF */
	stmp_adc_value.volume = val;
	stmp_adc_value.volume_reg = 0x37 + (val << 1); 

    BF_CS2(AUDIOIN_ADCVOLUME, 
        VOLUME_LEFT,  stmp_adc_value.volume_reg, 
        VOLUME_RIGHT, stmp_adc_value.volume_reg);
}

void setADCGain(unsigned short gain)
{
	//printk("[DRIVER] ADC gain = %x\n", gain);
	stmp_adc_value.adc_gain = gain;
	
	BW_AUDIOIN_ADCVOL_MUTE(MUTE_OFF);
	BW_AUDIOIN_ADCVOL_GAIN_LEFT(gain); 
	BW_AUDIOIN_ADCVOL_GAIN_RIGHT(gain); 
}

void setADCSamplingRate(unsigned long samplingRate)
{
	// Set Sample Rate Convert Hold, Sample Rate Conversion Factor Integer and Fractional values.
	// Use Base Rate Multiplier default value.
	unsigned long base_rate_mult = 0x4;
	unsigned long hold = 0x0;
	unsigned long sample_rate_int = 0xF;
	unsigned long sample_rate_frac = 0x13FF;

	DPRINTK("[DRIVER] setADCSamplingRate rate = %u\n", samplingRate);

     /*
     *  Available sampling rates:
     *  192kHz, 176.4kHz, 128kHz, 96kHz, 88.2KHz, 64kHz, 48kHz, 44.1kHz, 32kHz, 24kHz, 22.05kHz, 16kHz, 12kHz, 11.025kHz, 8kHz,
     *  and half of those 24kHz, 16kHz, (4kHz)
     *  Won't bother supporting those in ().
     */
    if (samplingRate >= 192000)
            samplingRate = 192000;
    else if (samplingRate >= 176400)
            samplingRate = 176400;
    else if (samplingRate >= 128000)
            samplingRate = 128000;
    else if (samplingRate >= 96000)
            samplingRate = 96000;
    else if (samplingRate >= 88200)
            samplingRate = 88200;
    else if (samplingRate >= 64000)
            samplingRate = 64000;
    else if (samplingRate >= 48000)
            samplingRate = 48000;
    else if (samplingRate >= 44100)
            samplingRate = 44100;
    else if (samplingRate >= 32000)
            samplingRate = 32000;
    else if (samplingRate >= 24000)
            samplingRate = 24000;
    else if (samplingRate >= 22050)
            samplingRate = 22050;
    else if (samplingRate >= 16000)
            samplingRate = 16000;
    else if (samplingRate >= 12000)
            samplingRate = 12000;
    else if (samplingRate >= 11025)
            samplingRate = 11025;
    else 
            samplingRate = 8000;

	switch (samplingRate)
	{
		case 8000:
			base_rate_mult=1;
			hold=3;
			sample_rate_int=0x17;
			sample_rate_frac=0x0e00;
			break;
		case 11025:
			base_rate_mult=1;
			hold=3;
			sample_rate_int=0x11;
			sample_rate_frac=0x37;
			break;
		case 12000:
			base_rate_mult=1;
			hold=3;
			sample_rate_int=0xf;
			sample_rate_frac=0x13ff;
			break;
		case 16000:
			base_rate_mult=1;
			hold=1;
			sample_rate_int=0x17;
			sample_rate_frac=0x0e00;
			break;
		case 22050:
			base_rate_mult=1;
			hold=1;
			sample_rate_int=0x11;
			sample_rate_frac=0x37;
			break;
		case 24000:
			base_rate_mult=1;
			hold=1;
			sample_rate_int=0xf;
			sample_rate_frac=0x13ff;
			break;
		case 32000:
			base_rate_mult=1;
			hold=0;
			sample_rate_int=0x17;
			sample_rate_frac=0x0e00;
			break;
		case 44100:
			base_rate_mult=1;
			hold=0;
			sample_rate_int=0x11;
			sample_rate_frac=0x37;
			break;
		case 48000:
			base_rate_mult=1;
			hold=0;
			sample_rate_int=0xf;
			sample_rate_frac=0x13ff;
			break;
		case 64000:
			base_rate_mult=2;
			hold=0;
			sample_rate_int=0x17;
			sample_rate_frac=0x0e00;
			break;
		case 88200:
			base_rate_mult=2;
			hold=0;
			sample_rate_int=0x11;
			sample_rate_frac=0x37;
			break;
		case 96000:
			base_rate_mult=2;
			hold=0;
			sample_rate_int=0xf;
			sample_rate_frac=0x13ff;
			break;
		case 128000:
			base_rate_mult=4;
			hold=0;
			sample_rate_int=0x11;
			sample_rate_frac=0x37;
			break;
		case 176400:
			base_rate_mult=4;
			hold=0;
			sample_rate_int=0x11;
			sample_rate_frac=0x37;
			break;
		case 192000:
			base_rate_mult=4;
			hold=0;
			sample_rate_int=0xf;
			sample_rate_frac=0x13ff;
			break;

		default:
			break; 
	}

	stmp_adc_value.samplingRate = samplingRate;

	BF_CS4(AUDIOIN_ADCSRR, 
		SRC_HOLD, hold, 
		SRC_INT,  sample_rate_int, 
		SRC_FRAC, sample_rate_frac,
		BASEMULT, base_rate_mult);
	
}

void setMICGain(unsigned short gain)
{
	stmp_adc_value.mic_gain = gain;

//	BW_AUDIOIN_MICLINE_MIC_MUTE(gain); 
	// for 36xx
	//BW_AUDIOIN_MICLINE_FORCE_MICAMP_PWRUP(0x0); 
	BW_AUDIOIN_MICLINE_MIC_GAIN(gain);
}

void setFMStatus(unsigned short status)
{
	stmp_adc_value.FMStatus = status;
}


void setMICBias(int mic_resistor)
{
	stmp_adc_value.mic_bias = mic_resistor;

	if(mic_resistor == 0x0)
	{
		BW_AUDIOIN_MICLINE_MIC_RESISTOR(0x0); 
		/* No use MIC, Set LRADC1 to block power */
		BW_AUDIOIN_MICLINE_MIC_SELECT(0x0); 
		BW_AUDIOIN_MICLINE_MIC_BIAS(0x0); 
	}
	else
	{
#if 1
		// Reduce Mic Noise by yoonhark at 20060808 */
		BW_AUDIOIN_MICLINE_MIC_RESISTOR(0x1); 
#else
		BW_AUDIOIN_MICLINE_MIC_RESISTOR(mic_register);
#endif
		/* when use MIC, Set LRADC0 to block power => check B'd which port use LRADC0 or LRADC1*/
		BW_AUDIOIN_MICLINE_MIC_SELECT(0x1); 
		BW_AUDIOIN_MICLINE_MIC_BIAS(0x7); 
	}
}

void setupAdcDMA(unsigned int p_adcPointer, unsigned long bufferSize)
{
	unsigned int  resetMask=0;
	int wait=0, n=0, i=0, desc_size=0, phy_ptr_n=0;
	unsigned int phys_adcpointer = p_adcPointer;
	unsigned long sz = 0;

	DPRINTK("setupAdcDMA() - bufferSize=%d\n", bufferSize);

	sz = MIN_STREAM_SIZE;
	n = bufferSize / sz;
	desc_size = DAC_DESC_SIZE;

	if(sz == 0)
		panic("[Serious Error] dma desc size zero!!\n");

	for (i=0; i<n; i++)
	{
		phy_ptr_n++;
		if(phy_ptr_n == n)
			phy_ptr_n = 0;
		
		/* adc dma desc init */
		adc_dma_cmd.desc_cmd_ptr[i*3] = adc_dma_cmd.phys_desc_cmd_ptr + desc_size*phy_ptr_n;
		adc_dma_cmd.desc_cmd_ptr[i*3+1] = (BF_APBX_CHn_CMD_XFER_COUNT(sz) | 
	    	                     BF_APBX_CHn_CMD_CHAIN(1) | 
	        	                 BF_APBX_CHn_CMD_IRQONCMPLT(1) |
	            	             BV_FLD(APBX_CHn_CMD, COMMAND, DMA_WRITE) ); // Assemble DMA command word.
		adc_dma_cmd.desc_cmd_ptr[i*3+2] = (reg32_t) phys_adcpointer+(sz*i);              // Point to input buffer.
	}

	// Reset DMA channels 1 AUDIOOUT.
	resetMask = BF_APBX_CTRL0_RESET_CHANNEL(BV_APBX_CTRL0_RESET_CHANNEL__AUDIOIN);
	HW_APBX_CTRL0_WR(resetMask);

	// Poll for both channels to complete reset.
	for (wait = 0; wait < 10; wait++)
	{
		if ( (resetMask & HW_APBX_CTRL0_RD()) == 0 )
		  break;
	}

	if (wait == 10)
	{
		printk("\nERROR: Timeout waiting for DMA Channel(s) to Reset.\n");
	}

	// Set up DMA channel configuration.
	BF_WRn(APBX_CHn_NXTCMDAR, 0, CMD_ADDR, adc_dma_cmd.phys_desc_cmd_ptr);
	// Enable IRQ
	BW_APBX_CTRL1_CH0_CMDCMPLT_IRQ(1);
	BW_APBX_CTRL1_CH0_CMDCMPLT_IRQ_EN(1);
	/* removed by jinho.lim - for 37xx */	
	BF_WRn(APBX_CHn_SEMA, 0, INCREMENT_SEMA, 1);
	
} 

void setAdcDMADesc(unsigned int p_adcPointer, unsigned long bufferSize)
{
	int n=0, i=0, desc_size=0, phy_ptr_n=0;
	unsigned int phys_adcpointer = p_adcPointer;
	unsigned long sz = 0;

	sz = MIN_STREAM_SIZE;
	n = bufferSize / sz;
	desc_size = DAC_DESC_SIZE;

	if(sz == 0)
		panic("[Serious Error] dma desc size zero!!\n");

	for (i=0; i<n; i++)
	{
		phy_ptr_n++;
		if(phy_ptr_n == n)
			phy_ptr_n = 0;
		
		/* adc dma desc init */
		adc_dma_cmd.desc_cmd_ptr[i*3] = adc_dma_cmd.phys_desc_cmd_ptr + desc_size*phy_ptr_n;
		adc_dma_cmd.desc_cmd_ptr[i*3+1] = (BF_APBX_CHn_CMD_XFER_COUNT(sz) | 
	    	                     BF_APBX_CHn_CMD_CHAIN(1) | 
	        	                 BF_APBX_CHn_CMD_IRQONCMPLT(1) |
	            	             BV_FLD(APBX_CHn_CMD, COMMAND, DMA_WRITE) ); // Assemble DMA command word.
		adc_dma_cmd.desc_cmd_ptr[i*3+2] = (reg32_t) phys_adcpointer+(sz*i);              // Point to input buffer.
	}

	// Set up DMA channel configuration.
	BF_WRn(APBX_CHn_NXTCMDAR, 0, CMD_ADDR, adc_dma_cmd.phys_desc_cmd_ptr);

}

int stopAdcProcessing(audio_stream_t *audio_s)
{
	BW_APBX_CTRL1_CH0_CMDCMPLT_IRQ_EN(0);

//	setAdcDMADesc(adc_b->phyStartaddr, audio_s->fragsize*audio_s->nbfrags);
    stopADC();

	BF_CS1(AUDIOOUT_ANACLKCTRL, CLKGATE, 0);
	BW_AUDIOOUT_PWRDN_ADC(1);
	/* audioin reset */
	BF_CS1(AUDIOIN_CTRL, RUN, 0);
	BF_CS2(AUDIOIN_CTRL, SFTRST, 1, CLKGATE, 1);  // cleared LOOPBACK to see 1 bit outputs.
	BF_CS1(AUDIOIN_ANACLKCTRL, CLKGATE, 1);    

	gReadInitMuteCnt = 0;
    audio_s->audio_buffers->initDevice = 0;
	return 0;				
}

void startADC(void)
{
    /* Set APBX DMA Engine to Run Mode. */
    BF_CS2(APBX_CTRL0, SFTRST, 0, CLKGATE, 0);

    BF_CS1(AUDIOIN_CTRL, RUN, 1);
}

void stopADC(void)
{
	unsigned phore;
	signed char decrement = -1; 

	// read the current semaphore
	phore = BF_RDn(APBX_CHn_SEMA, 0, PHORE);

	if ( phore == 1 ) { // for debugging
		DPRINTK("%s: phore(%d) decrement(%x) ", 
		       __FUNCTION__, phore, decrement); 
	}

	// clear the semaphore to stop the end-of-chain loop
	BF_WRn(APBX_CHn_SEMA, 0, INCREMENT_SEMA, (unsigned)decrement);

	while(1)
	{
		if((BM_APBX_CTRL1_CH0_CMDCMPLT_IRQ & HW_APBX_CTRL1_RD()) == 1)
		{
			DPRINTK("AUDIOIN Soft Reset\n");
		   	BF_CS1(AUDIOIN_CTRL, SFTRST, 1);
			break;
		}
	}
}

static void subADCValidBytes(adc_info_t *adcInfo, signed int validBytes)
{       
	spin_lock_irq(&adc_lock);
	adcInfo->ADCValidBytes-=validBytes;
	if (adcInfo->ADCValidBytes < 0)
	{
	  //printk("Underrun!\n");
	  adcInfo->ADCValidBytes=0;
	}
	//printk("[read] V[%d], bytes[%d] \n", adcInfo->ADCValidBytes, validBytes);
	spin_unlock_irq(&adc_lock);
}

void setRecodingSrc(unsigned char input_src)
{
	stmp_adc_value.recodingSrc = input_src;

	if(input_src == INPUT_SRC_MICROPHONE)
	{
		BW_AUDIOIN_ADCVOL_SELECT_LEFT(0x0); 
		BW_AUDIOIN_ADCVOL_SELECT_RIGHT(0x0); 
		setMICGain(stmp_adc_value.mic_gain);
		setMICBias(stmp_adc_value.mic_bias);
	}
	else if(input_src == INPUT_SRC_LINEIN)
	{
		BW_AUDIOIN_ADCVOL_SELECT_LEFT(0x1); // line1 input source
		BW_AUDIOIN_ADCVOL_SELECT_RIGHT(0x1); // line1 input source
	}
	else if(input_src == INPUT_SRC_HPAMP)
	{
		BW_AUDIOIN_ADCVOL_SELECT_LEFT(0x2);
		BW_AUDIOIN_ADCVOL_SELECT_RIGHT(0x2);
	}
	setADCGain(stmp_adc_value.adc_gain);
}

int updateADCData(audio_stream_t *audio_s)
{
	adc_info_t *adcInfo = audio_s->adc_info;
	audio_buf_t *adc_b = audio_s->audio_buffers;
	unsigned int currentPosition=0, lastPosition=0;
	unsigned int copySize1=0, copySize2=0;
	u_int bufferSize = 0;

	bufferSize = audio_s->fragsize * audio_s->nbfrags;
	
	lastPosition = adc_b->lastDMAPosition;

	currentPosition = getAdcPointer(adc_b->phyStartaddr); //Since in theory, the pointer is always moving, lets just grab a snapshot of it.
	//printk("[DRIVER] AdcPtr[%d]\n", currentPosition);

	if ( (currentPosition == lastPosition) && (currentPosition != 0))
	{
		printk("[DRIVER] the DMA has stopped !! \n");
		return 0;
	}

	if (currentPosition >= lastPosition)
	{
		copySize1 = currentPosition - lastPosition;
		copySize2 = 0;
	}
	else
	{
		copySize1 = bufferSize - lastPosition;
		copySize2 = currentPosition;
	}

	//printk("[ISR] P[%d] V[%d]\n", currentPosition, copySize1+copySize2);

	addADCValidBytes(adcInfo, copySize1+copySize2);

	adc_b->lastDMAPosition = currentPosition;

	return 0;
}


