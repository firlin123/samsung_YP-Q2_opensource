/* $Id: stmp36xx_audioin_adc.h,v 1.14 2006/05/19 03:43:42 zzinho Exp $ */

/**
 * \file stmp36xx_audioin_adc.h
 * \brief audioout of stmp36xx
 * \author LIM JIN HO <jinho.lim@samsung.com>
 * \version $Revision: 1.14 $
 * \date $Date: 2006/05/19 03:43:42 $
 *
 * This file implements audioin(ADC) driver for SMTP36XX(sigmatel).
 * $Log: stmp36xx_audioin_adc.h,v $
 * Revision 1.14  2006/05/19 03:43:42  zzinho
 * update with static sound driver
 *
 * Revision 1.13  2006/04/17 04:59:56  yoonhark
 * modification for removing pop noise in start part of recording file
 *
 * Revision 1.12  2006/03/14 11:46:27  zzinho
 * add mic_bias ioctl to control mic bias
 *
 * Revision 1.11  2006/02/22 00:42:11  zzinho
 * removed warning message
 *
 * Revision 1.10  2005/11/09 05:33:30  zzinho
 * audio exit addition
 *
 * Revision 1.9  2005/11/03 05:06:03  zzinho
 * audio in modified
 *
 * Revision 1.8  2005/09/22 02:09:09  zzinho
 * change stream size
 *
 * Revision 1.7  2005/09/12 04:26:32  zzinho
 * add new ioctl for mic gain & adc gain
 *
 * Revision 1.6  2005/09/07 04:01:17  zzinho
 * add audioin and remove warning msg
 *
 * Revision 1.5  2005/09/06 10:38:54  zzinho
 * add audioin
 *
 * Revision 1.4  2005/08/23 09:16:07  zzinho
 * remove warning msg
 *
 * Revision 1.3  2005/08/09 04:02:09  zzinho
 * jinho version up
=======
 * Revision 1.4  2005/08/23 09:16:07  zzinho
 * remove warning msg
 *
 * Revision 1.3  2005/08/09 04:02:09  zzinho
 * jinho version up
>>>>>>> 1.4
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

/*---------- Structure Definitions ----------*/

enum {
	INPUT_SRC_MICROPHONE = 0,
	INPUT_SRC_LINEIN,
	INPUT_SRC_HPAMP
};

enum {
	MUTE_OFF = 0,
	MUTE_ON
};

enum {
	FM_OPEN = 0,
	FM_RELEASE
};

extern int allocDMAAdc_desc(unsigned int bufferSize);
extern void exitAudioin(void);
extern int fillEncoderBuffer(audio_stream_t *audio_s, const void *userBuffer, int userBufferSize);
extern void freeAdcDMA_Desc(unsigned int bufferSize);
extern unsigned int getAdcPointer(unsigned int phyadc_addr);
extern int getRecodingSrc(void);
extern void setADCValidBytes(adc_info_t *adcInfo, signed int validBytes);
extern int getAdcBufSize(void);
extern int getADCVolume(void);
extern unsigned short getADCGain(void);
extern unsigned short getMICGain(void);
extern unsigned short getFMStatus(void);
extern signed int getADCValidBytes(adc_info_t *adcInfo);
extern unsigned long getADCSamplingRate(void);
extern int initializeAudioin(audio_stream_t *s);
extern void initializeADC(void);
extern void setAdcBufSize(int buf_size);
extern void setRecodingSrc(unsigned char input_src);
extern void setADCSamplingRate(unsigned long samplingRate);
extern void setADCVolume(int val);
extern void setADCGain(unsigned short gain);
extern void setMICGain(unsigned short gain);
extern void setMICBias(int mic_resistor);
extern void setFMStatus(unsigned short status);
extern void setAdcDMADesc(unsigned int p_adcPointer, unsigned long bufferSize);
extern void setupAdcDMA(unsigned int p_adcPointer, unsigned long bufferSize);
extern void startADC(void);
extern void stopADC(void);
extern int stopAdcProcessing(audio_stream_t *audio_s);
extern int updateADCData(audio_stream_t *audio_s);


