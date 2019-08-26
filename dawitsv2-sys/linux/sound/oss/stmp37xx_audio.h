/* $Id: stmp36xx_audio.h,v 1.50 2007/07/31 08:59:34 zzinho Exp $ */

/**
 * \file stmp36xx_audio.h
 * \brief audioout of stmp36xx
 * \author LIM JIN HO <jinho.lim@samsung.com>
 * \version $Revision: 1.50 $
 * \date $Date: 2007/07/31 08:59:34 $
 *
 * This file implements audioout(DAC) driver for SMTP36XX(sigmatel).
 * $Log: stmp36xx_audio.h,v $
 * Revision 1.50  2007/07/31 08:59:34  zzinho
 * combine 3d/eq to dnse
 * by jinho.lim
 *
 * Revision 1.49  2007/06/21 05:29:07  zzinho
 * update to latest ZB source
 * by jinho.lim
 *
 * Revision 1.48  2007/05/08 01:58:41  zzinho
 * audio voluem step is changed by B&O's recommendation and Max output level is increased to 20mV over
 * by jinho.lim
 *
 * Revision 1.47  2007/03/21 07:27:03  biglow
 * Update from China by zzinho
 *
 * - Taehun Lee
 *
 * Revision 1.46  2006/02/27 13:14:07  zzinho
 * removed !USE_DOWN mode
 * and adc buffer change to 32Kbytes
 *
 * Revision 1.45  2006/02/20 07:21:49  zzinho
 * audioin buffer -> SDRAM
 *
 * Revision 1.44  2005/12/12 08:16:12  zzinho
 * remove debug and add country ioctl
 *
 * Revision 1.43  2005/11/24 10:30:56  zzinho
 * volume set case modified
 *
 * Revision 1.42  2005/11/01 09:12:39  zzinho
 * refer define hardware.h
 *
 * Revision 1.41  2005/11/01 05:37:42  zzinho
 * include hardware.h for audio sram address
 *
 * Revision 1.40  2005/10/31 09:38:48  zzinho
 * add set fragment size
 *
 * Revision 1.39  2005/10/10 07:34:12  zzinho
 * remove unnecessary code
 *
 * Revision 1.38  2005/10/08 04:50:25  zzinho
 * change 16K -> 32K audio driver buffer
 *
 * Revision 1.37  2005/10/08 03:53:46  zzinho
 * type change
 *
 * Revision 1.36  2005/10/08 02:01:56  zzinho
 * *** empty log message ***
 *
 * Revision 1.35  2005/10/08 01:33:00  zzinho
 *
 * volatile remove
 *
 * Revision 1.34  2005/10/04 05:49:44  zzinho
 * buffer point type change
 * short -> char
 *
 * Revision 1.33  2005/10/01 00:29:28  zzinho
 * change available user bytes type
 *
 * Revision 1.32  2005/09/30 06:06:57  zzinho
 * add 32K buffer define of audio
 *
 * Revision 1.31  2005/09/23 10:25:27  zzinho
 * change mmap for ogg sdram
 *
 * Revision 1.30  2005/09/22 02:08:30  zzinho
 * change buffer mechanism
 * 1. use one buffer and write/read buffer pointer
 * 2. remove memcpy dma
 *
 * Revision 1.29  2005/09/21 00:07:14  zzinho
 * no decoder buf mode add
 *
 * Revision 1.28  2005/09/20 05:19:05  zzinho
 * back buf size to 16K
 *
 * Revision 1.27  2005/09/20 05:10:18  zzinho
 * sound driver buffer size -> 32K
 *
 * Revision 1.26  2005/09/12 06:40:47  zzinho
 * buffer size 16K*2 -> 8K*2
 *
 * Revision 1.25  2005/09/08 08:18:50  zzinho
 * Sound driver buffer size is changed
 * 8*2 -> 16*2 Kbytes
 *
 * Revision 1.24  2005/09/07 04:01:17  zzinho
 * add audioin and remove warning msg
 *
 * Revision 1.23  2005/09/06 10:46:31  zzinho
 * add audioin
 *
 * Revision 1.22  2005/08/24 08:49:51  zzinho
 * change
 * audio_setup_buf open -> init
 *
 * Revision 1.21  2005/08/23 09:16:07  zzinho
 * remove warning msg
 *
 * Revision 1.20  2005/08/23 08:49:54  zzinho
 * mmap modify to use sdram for wma init code
 *
 * Revision 1.19  2005/08/22 01:44:57  zzinho
 * mmap, release change
 *
 * Revision 1.17  2005/08/11 02:36:45  zzinho
 * remove start release sem
 * add delay 200msec in case release
 *
 * Revision 1.16  2005/08/11 00:44:52  zzinho
 * remove start release sem
 * add delay 200msec in case release
 *
 * Revision 1.15  2005/08/09 04:02:09  zzinho
 * jinho version up
 *
 * Revision 1.14  2005/08/03 05:24:43  zzinho
 * remove #ifdef SECOND_STRATGY
 *
 * Revision 1.13  2005/07/29 08:24:08  zzinho
 * modify audioout dma structure
 *
 * Revision 1.12  2005/07/23 05:35:03  zzinho
 * modify name
 * add apbx audioout dma irq timeout
 *
 * Revision 1.11  2005/07/15 12:42:58  zzinho
 * back 0708
 * because of dac dma kill
 * jinho.lim
 *
 * Revision 1.10  2005/07/15 11:58:51  zzinho
 * back 0708
 * because of dac dma kill
 * jinho.lim
 *
 * Revision 1.8  2005/06/30 02:37:37  zzinho
 * fill->copy->start : bug
 * fill->copy->fill->start : modify
 *
 * Revision 1.7  2005/06/21 20:51:45  hcyun
 * sram support. seperation of codec code and buffer
 *
 * - hcyun
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

#ifndef STMP36XX_AUDIO_H
#define STMP36XX_AUDIO_H

#if 0
#define DPRINTK( x... )  printk( x )
#else
#define DPRINTK( x... )
#endif

#define CONFIG_BOARD_Q2
#define CONFIG_SOUND_STMP36XX_AUDIOIN 1
#define CONFIG_STMP36XX_SRAM 1
//#define HP_VOLUME_CONTROL 0

//#define SNDCTL_DSP_EVENT_COUNTRY_DEFAULT		_SIO ('P',74) /* jinho.lim@samsung.com */
//#define SNDCTL_DSP_EVENT_COUNTRY_EU		_SIO ('P',75) /* jinho.lim@samsung.com */
#define FALSE 0
#define TRUE  1
#define USE_APP_SRAM 1
#define AUDIO_NAME		"stmp37xx_audio"
#define USE_MAX_CORE_LEVEL
#define MINUS_9DB_HEADROOM 1

#define INTR_DISABLE_HW_VECTOR(vector) \
  (((vector) < 0 || (vector) >= 64) ? 1 : \
   (*((volatile unsigned long *) (HW_ICOLL_PRIORITYn_CLR_ADDR(0) + \
	  (vector)/4 * 0x10)) = 0x04 << ((vector)%4 * 8)), 0)

#define DAC_SAMPLING_RATE (44100)

/* CAUTION */
/* Generally, buffer size will be changed dynamically. */
/* But we have to use STMP internal SRMA for audio buffer */
/* So we has static buffer size 8192*2Byte */
#define BUF_32K_SIZE 32768
#define BUF_30K_SIZE 30720
#define BUF_28K_SIZE 28672
#define BUF_26K_SIZE 26624
#define BUF_24K_SIZE 24576
#define BUF_22K_SIZE 22528
#define BUF_20K_SIZE 20480
#define BUF_18K_SIZE 18432
#define BUF_16K_SIZE 16384
#define BUF_14K_SIZE 14336
#define BUF_12K_SIZE 12288
#define BUF_10K_SIZE 10240
#define BUF_8K_SIZE 8192
#define BUF_4K_SIZE 4096
#define BUF_2K_SIZE 2048
#define ONE_FRAGMENT_SIZE 4096
#define FRAGMENT_NBR STMP37XX_SRAM_AUDIO_KERNEL_SIZE/ONE_FRAGMENT_SIZE
#define MIN_STREAM_SIZE 2048
#define DAC_DESC_SIZE 12
#define ADC_BUF_SIZE (256*1024)

#define AUDIO_CHANNELS		2
#define AUDIO_FMT		AFMT_S16_LE

#define MEMCPY_NOWAIT  0
#define MEMCPY_WAIT    1
#define MEMCPY_WAIT_YIELD 2

#define STMP36XX_REC_DEVICES (SOUND_MASK_LINE | SOUND_MASK_MIC)
#define STMP36XX_DEV_DEVICES (STMP36XX_REC_DEVICES | SOUND_MASK_VOLUME)

enum
{
	USE_SRAM = 0,
	USE_SDRAM
};

enum audio_channel {
	AUDIO_STEREO = 0,
	AUDIO_RIGHT,
	AUDIO_LEFT
};

/**
 * Structure with adc buffer informations of audioin
 */
typedef struct {
	unsigned int ADCReadPointer;
	signed int ADCValidBytes;
} adc_info_t;

/**
 * Structure for buffer address/pointer/bytes informations  when writing PCM data of audioout
 */
typedef struct {
	void *decoderBuffer;
	dma_addr_t phydecoderaddr;	/* physical buffer address */
	unsigned int decoderWritePointer;
	unsigned int decoderReadPointer;
	unsigned int AvaliableUserBytes;
	signed int decoderValidBytes;
	int master;		/* owner for buffer allocation, contain size when true */
	struct semaphore sem;	/* down before touching the buffer */
	struct audio_stream_s *stream;	/* owning stream */
} decoder_buf_t;


typedef struct {
	unsigned long *wma_init_code_addr;
	unsigned long *code_addr;
	unsigned long wma_init_code_phys;
	unsigned long code_phys;
	pgprot_t page_prot;		/* Access permissions of this VMA. */
} codec_info_t;

/**
 * Structure with buffer informations of audioout/in
 */
typedef struct {
	void *pStart;		/* points to actual buffer */
	dma_addr_t phyStartaddr;	/* physical buffer address */
	unsigned int lastDMAPosition;
	int initDevice; 
	int IsFillDac;
	int master; 	/* owner for buffer allocation, contain size when true */
	struct semaphore sem;	/* down before touching the buffer */
	struct audio_stream_s *stream;	/* owning stream */
} audio_buf_t;

	
/**
 * Structure with stream informations of audioout/audioin
 */
typedef struct audio_stream_s {
	char *name;        /* stream identifier */
	audio_buf_t *audio_buffers;	/* pointer to audio buffer structures */
	decoder_buf_t *decoder_buffers;	
	codec_info_t *codec_info;
	adc_info_t *adc_info;
	u_int fragsize;		/* fragment i.e. buffer size */
	u_int nbfrags;		/* nbr of fragments i.e. buffers */
	int channel;		/* stream channel i.e. right or left or stereo */
	int usr_rwcnt;		/* user Read/write count */
	int bytecount;		/* nbr of processed bytes */
	int fragcount;		/* nbr of fragment transitions */
	wait_queue_head_t wq;	/* for poll */
	int active:1;		/* actually in progress */
	struct semaphore sem;	/* to protect against races in release() */
} audio_stream_t;

/**
 * Structure with stat information of audio
 */
typedef struct {
	audio_stream_t *output_stream;
	audio_stream_t *input_stream;
	char *output_id;
	char *input_id;
	int rd_ref:1;		/* open reference for recording */
	int wr_ref:1;		/* open reference for playback */
	void *data;
	struct semaphore sem;	/* to protect against races in attach() */
} audio_state_t;

extern void * dev_dma_alloc(int size, dma_addr_t *ptr);
extern void dev_dma_free(int size, void *cpu_addr, dma_addr_t dma_handle);
#endif
