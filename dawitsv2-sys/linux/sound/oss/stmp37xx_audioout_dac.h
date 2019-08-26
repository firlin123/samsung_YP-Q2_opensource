/* $Id: stmp36xx_audioout_dac.h,v 1.55 2007/12/17 01:34:42 zzinho Exp $ */

/**
 * \file stmp36xx_audioout_dac.h
 * \brief audioout of stmp36xx
 * \author LIM JIN HO <jinho.lim@samsung.com>
 * \version $Revision: 1.55 $
 * \date $Date: 2007/12/17 01:34:42 $
 *
 * This file implements audioout(DAC) driver for SMTP36XX(sigmatel).
 * $Log: stmp36xx_audioout_dac.h,v $
 * Revision 1.55  2007/12/17 01:34:42  zzinho
 * enum changed
 *
 * by jinho.lim
 *
 * Revision 1.54  2007/12/14 11:14:32  zzinho
 * add playspeed function
 *
 * by jinho.lim
 *
 * Revision 1.53  2007/11/28 01:52:04  zzinho
 * add set default volume ioctl for beep
 * by jinho.lim
 *
 * Revision 1.52  2007/11/28 00:28:46  zzinho
 * add set dac volume ioctl
 * by jinho.lim
 *
 * Revision 1.51  2007/09/19 08:55:40  zzinho
 * disable clkctrl intr wait when max volume
 * by jinho.lim
 *
 * Revision 1.50  2007/08/23 09:04:32  zzinho
 * add 4 case volume tabe
 * by jinho.lim
 *
 * Revision 1.49  2007/07/31 09:00:59  zzinho
 * change volume table for S3 project
 * by jinho.lim
 *
 * Revision 1.48  2006/05/19 03:43:42  zzinho
 * update with static sound driver
 *
 * Revision 1.51  2006/04/17 05:00:11  yoonhark
 * modification for removing pop noise in start part of recording file
 *
 * Revision 1.50  2006/04/10 02:15:04  zzinho
 * remove fm event check function
 *
 * Revision 1.49  2006/04/08 01:59:27  zzinho
 * add fm open release event callback
 * add when audio sync, immediately stop audioin dma
 *
 * Revision 1.48  2006/04/05 06:30:21  zzinho
 * recording status check routine added
 *
 * Revision 1.47  2006/03/06 08:13:07  zzinho
 * audio driver ioctl change
 * SNDCTL_DSP_EU_OUTPUT -> SNDCTL_DSP_FRANCE_OUTPUT
 *
 * Revision 1.46  2006/03/02 04:34:00  zzinho
 * add cradle mute control
 *
 * Revision 1.45  2006/02/22 00:42:11  zzinho
 * removed warning message
 *
 * Revision 1.44  2005/12/22 06:43:27  hcyun
 * Jinho's volume table setting ioctl code addition.
 *
 * Revision 1.43  2005/12/12 05:59:34  zzinho
 * remove unnecessary function and define
 *
 * Revision 1.42  2005/12/08 06:59:13  zzinho
 * volume table modification
 * EU volume table addition
 *
 * Revision 1.41  2005/12/05 11:53:03  zzinho
 * 1. volume table init version addition for EU
 * 2. Postprocessing mute modification
 * - 120msec -> 30msec first skip time
 *
 * Revision 1.40  2005/11/20 06:06:08  zzinho
 * MEMCPY is modified
 * decrement_sema 1 -> 2 => it cause gabage data addition when low clock
 *
 * Revision 1.39  2005/11/17 08:34:36  zzinho
 * VAG reference is added to remove hissing noise when VDD is changed
 *
 * Revision 1.38  2005/11/13 07:29:09  zzinho
 * volume fade in/out with 10msec timer
 *
 * Revision 1.37  2005/11/12 08:31:35  zzinho
 * volume fade in addition
 *
 * Revision 1.36  2005/11/12 04:31:55  zzinho
 * *** empty log message ***
 *
 * Revision 1.35  2005/11/11 06:33:55  zzinho
 * pm idle/wakeup reset/restart addition
 *
 * Revision 1.34  2005/11/10 06:19:52  zzinho
 * *** empty log message ***
 *
 * Revision 1.33  2005/11/09 05:33:30  zzinho
 * audio exit addition
 *
 * Revision 1.32  2005/11/05 04:47:37  zzinho
 * fade in/out addition when postprocessing is changed
 *
 * Revision 1.31  2005/10/31 09:41:42  zzinho
 * function added
 *
 * Revision 1.30  2005/10/22 00:36:07  zzinho
 * select_sound_status addition
 *
 * Revision 1.29  2005/10/19 02:28:54  zzinho
 * setPlayingStatus(), getPlayingStatus(), setCoreLevle() addition
 *
 * Revision 1.28  2005/10/18 12:05:45  zzinho
 * controlStopVol() is added by jinho.lim
 *
 * Revision 1.27  2005/10/18 08:43:16  zzinho
 * if audio is stopped && volume 100, we cannot change VDD for max sound output
 * added by jinho.lim
 *
 * Revision 1.26  2005/10/14 08:10:20  zzinho
 * fix volume table and add checkdacptr function
 *
 * Revision 1.25  2005/10/01 01:34:47  zzinho
 * add if initDev == 0
 * skip checkPostProcessing
 *
 * Revision 1.24  2005/09/28 09:27:25  zzinho
 * *** empty log message ***
 *
 * Revision 1.23  2005/09/22 02:11:00  zzinho
 * change buffer mechanism
 * 1. use one buffer and write/read buffer pointer
 * 2. remove memcpy dma
 *
 * Revision 1.22  2005/09/21 00:07:14  zzinho
 * no decoder buf mode add
 *
 * Revision 1.21  2005/09/15 00:35:58  zzinho
 * add volume table & ioctl for eq & 3D post processing status of user
 *
 * Revision 1.20  2005/09/14 02:48:44  zzinho
 * hw mute on function added
 *
 * Revision 1.19  2005/09/07 11:25:23  zzinho
 * add audioin
 *
 * Revision 1.18  2005/08/23 09:16:07  zzinho
 * remove warning msg
 *
 * Revision 1.17  2005/08/22 01:44:57  zzinho
 * mmap, release change
 *
 * Revision 1.15  2005/08/11 00:44:52  zzinho
 * remove start release sem
 * add delay 200msec in case release
 *
 * Revision 1.14  2005/08/09 04:02:09  zzinho
 * jinho version up
 *
 * Revision 1.13  2005/08/02 09:37:02  zzinho
 * *** empty log message ***
 *
 * Revision 1.12  2005/07/29 08:24:08  zzinho
 * modify audioout dma structure
 *
 * Revision 1.11  2005/07/28 01:50:58  zzinho
 * up version
 *
 * Revision 1.10  2005/07/27 12:07:12  zzinho
 * add hpvolume function
 *
 * Revision 1.9  2005/07/23 05:35:03  zzinho
 * modify name
 * add apbx audioout dma irq timeout
 *
 * Revision 1.8  2005/07/15 12:42:58  zzinho
 * back 0708
 * because of dac dma kill
 * jinho.lim
 *
 * Revision 1.7  2005/07/15 11:58:51  zzinho
 * back 0708
 * because of dac dma kill
 * jinho.lim
 *
 * Revision 1.5  2005/07/04 00:39:24  zzinho
 * add volatile to the dma struct value
 *
 * Revision 1.4  2005/06/30 02:37:37  zzinho
 * fill->copy->start : bug
 * fill->copy->fill->start : modify
 *
 * Revision 1.3  2005/06/22 09:46:23  zzinho
 * set seg ioctl support
 *
 * Revision 1.2  2005/06/02 19:10:48  zzinho
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

/*
 * Functions exported by this module
 */

#ifndef STMP36XX_AUDIOOUT_DAC_H
#define STMP36XX_AUDIOOUT_DAC_H

enum {
	NO_PROCESSING = 0,
	DNSE_PROCESSING
};

enum {
	NO_MAX_VDD = 0,
	MAX_VDD
};

enum {
	NO_PLAYING = 0,
	PLAYING
};

enum {
	NO_RECORDING = 0,
	RECORDING
};

enum {
	NO_SELECT = 0,	
	SOUND_SELECT
};

enum {
	FADE_IN = 0,
	FADE_OUT = 1,
	NO_FADE = 0xF
};

enum {
	LAST_CHECK = 0,
	NO_LAST_CHECK = 1
};

enum {
	VER_COMMON = 0,
	VER_FR
};

enum {
	PLAY_SPEED_X0_7=0,
	PLAY_SPEED_X0_8,
	PLAY_SPEED_X0_9,
	PLAY_SPEED_X1_0,
	PLAY_SPEED_X1_1,
	PLAY_SPEED_X1_2,
	PLAY_SPEED_X1_3
};

#define DAC_VOLUME_MAX 0xFF // 0dB
#define DAC_VOLUME_MIN 0x40 
#define HP_VOLUME_MIN 0x7E
#define HP_VOLUME_MAX 0x03 // +6dB
#define SPKR_VOLUME_MAX 0x03 // +6dB

extern int allocDMADac_desc(unsigned int bufferSize);
extern int allocDMAMemcpy_desc(void);
extern int checkDacPtr(audio_stream_t *audio_s);
extern void checkPostprocessing(unsigned long arg);
extern void checkVolumeFade(unsigned long arg);
extern void controlStopVol(int effect);
extern void exitAudioout(void);
extern int fillDecoderBuffer(audio_stream_t *audio_s, const void *userBuffer);
extern void freeDMAMemcpy_desc(void);
extern void freeDMA_Desc(unsigned int bufferSize);
extern int getBufSize(void);
extern unsigned int getDacPointer(unsigned int phydac_addr);
extern int getDacVolume(void);
extern unsigned long getDacSamplingRate(void);
extern int getHPVolume(void);
extern int getMaxSoundflag(void);
extern int getPlayingStatus(void);
extern int getRecordingStatus(void);
extern int getSelectSoundStatus(void);
extern int getSWVersion(void);
extern int initializeAudioout(audio_stream_t *s);
extern void initDacSync(audio_stream_t *s);
extern void initDacWrite(audio_stream_t *s, const char *buffer);
extern int offHWmute(void);
extern int onHWmute(void);
extern void setBufSize(int bufferSize);
extern void setupDMA(unsigned int p_dacPointer, unsigned long bufferSize);
extern void setDMADesc(unsigned int p_dacPointer, unsigned long bufferSize);
extern void setDacVolume(int val);
extern void setUserVolume(int user_volume);
extern void setDefaultVolume(int val);
extern void setDacSamplingRate(unsigned long samplingRate);
extern void startDac(void);
extern void setHPVolume(int val);
extern void setPostProcessingFlag(audio_buf_t *dac_b, unsigned short setValue, int gain_attenuation);
extern void setVDDflag(int MAX_VDD);
extern void setDefaultVGA(void);
extern void setCoreLevel(int volume);
extern void setClkIntrWait(int onoff);
extern void setPlayingStatus(int isPlaying);
extern void setRecordingStatus(int isRecording);
extern void setSWVersion(unsigned short version);
extern int stopDacProcessing(audio_stream_t *audio_s);
extern void stopHWAudioout(void);
extern void startHWAudioout(void);
extern void setVolumeFade(int fade);
extern int updateDacBytes(void *audio_s);
extern void set_pcm_audio_bit(int bit);
extern void setPlaySpeed(int val);
extern void setDSPFade(int val);
extern void setBoardRevision(unsigned short revision);
extern int getDSPFade(void);


#endif

