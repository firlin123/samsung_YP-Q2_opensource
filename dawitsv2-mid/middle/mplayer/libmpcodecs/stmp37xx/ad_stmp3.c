#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <sys/mman.h>

#include "config.h"
#include "mp_msg.h"
#include "help_mp.h"

#include "ad_internal.h"
#include "mpbswap.h"
#include "mp3_struct.h"

static ad_info_t info = 
{
	"STMP37xx mp3 codecs",
	"stmp3",
	NULL,
	NULL,
	NULL
};

LIBAD_EXTERN(stmp3)

/* Common */
#define CODE_VIRT 0x50000000
#define POST_VIRT 0x50200000
#define SRAM_POST_SIZE (0*1024)
#define SRAM_POST_OFFSET 0
#define SRAM_CODE_OFFSET (SRAM_POST_OFFSET + SRAM_POST_SIZE)
#define SRAM_PROTECTION_REGION_SIZE (0*1024)

/* For MP3 */
#define DATA_VIRT_MP3 0x50009000

#define SRAM_CODE_SIZE_MP3 (32*1024) //rom file size
#define SRAM_DATA_SIZE_MP3 (42*1024)	// 32K = 16K (inputbuffer) + 6K (outputbuffer) + 16K (TSpiritMP3Decoder structure)

#define SRAM_DATA_OFFSET_MP3 (SRAM_CODE_OFFSET + SRAM_CODE_SIZE_MP3)

#define DECODER_DATA_SIZE_MP3	   (20*1024)
#define SRAM_MP3IN_BUFFER_SIZE (16*1024)
#define SRAM_MP3WAVOUT_BUFFER_SIZE (6*1024)
#define SRAM_MP3IN_BUFFER_OFFSET (DECODER_DATA_SIZE_MP3 + SRAM_PROTECTION_REGION_SIZE)
#define SRAM_MP3WAVOUT_BUFFER_OFFSET (SRAM_MP3IN_BUFFER_OFFSET + SRAM_MP3IN_BUFFER_SIZE)

#define FRAME_SIZE_SAMPLES	576
#define SAMPLE_SIZE			2
#define CHANNEL_NUMBER		2

static Dec_fptr dec_fptr; // Function pointer available to application

static pdecoder pfnDecoder; // Decoder function pointer table.
static unsigned char *decoder_mem;
static unsigned char *code;

static TSpiritMP3Decoder  *mp3_Decoder;  // Decoder object
static TSpiritMP3Info     *mp3_Info;     // MP3 audio information

static int ocram_fd;

static unsigned int RetrieveMP3Data (
    void * pMP3CompressedData,			// [OUT] Bitbuffer
    unsigned int nMP3DataSizeInChars,	// sizeof(Bitbuffer)
    unsigned int* pUserData				// Application-supplied parameter
)
{
	sh_audio_t *sh = (sh_audio_t *)pUserData;
	unsigned char *buf = pMP3CompressedData;
	int len_read = 0;
	int len_to_read = nMP3DataSizeInChars;

//	mp_msg(MSGT_DECAUDIO,MSGL_INFO,"-nMP3DataSizeInChars %d\n", nMP3DataSizeInChars);
	while (len_read < nMP3DataSizeInChars)
	{
		int nread;
		nread = demux_read_data(sh->ds, buf, len_to_read);
		if (nread <= 0)
		{
			break;
		}
		buf += nread;
		len_to_read -= nread;
		len_read += nread;
	}
//	mp_msg(MSGT_DECAUDIO,MSGL_INFO,"-nread %d \n", len_read);

	return len_read;
}

static int control(sh_audio_t *sh,int cmd,void* arg, ...)
{
	/*
  int skip;
    switch(cmd)
    {
      case ADCTRL_SKIP_FRAME:
	skip=sh->i_bps/16;
	skip=skip&(~3);
	demux_read_data(sh->ds,NULL,skip);
	return CONTROL_TRUE;
    }
	*/
  return CONTROL_UNKNOWN;
}

static int preinit(sh_audio_t *sh)
{
	// minimum output buffer size (should be the uncompressed max. frame size)
	// Default: 8192
	sh->audio_out_minsize = FRAME_SIZE_SAMPLES*SAMPLE_SIZE*CHANNEL_NUMBER;
	//sh->audio_out_minsize = 8192;

	// minimum input buffer size (set only if you need input buffering)
	// (should be the max compressed frame size)
	//sh->audio_in_minsize=2048; // Default: 0 (no input buffer)
	return 1;
}

static int init (sh_audio_t *sh)
{
	int code_fd;
	int  i, nread;
	char buf[4096];
	char path[128] = {0}, *ssroot; 

	/* Sigmatel decoder code */ 
	if ((ssroot = getenv("SSROOT")))
		strcpy(path, ssroot);
	strcat(path, "/codec/mp3lib.rom");

	code_fd = open(path, O_RDONLY); 
	
	if (code_fd < 0)
	{
		mp_msg(MSGT_DECAUDIO,MSGL_INFO,"mp3lib.rom open error\n");
		goto err1;
	}
	ocram_fd = open("/dev/ocram", O_RDWR | O_NONBLOCK);
	if (ocram_fd < 0)
	{
		mp_msg(MSGT_DECAUDIO,MSGL_INFO,"/dev/ocram open error\n");
		goto err2;
	}

	code = (unsigned char *)mmap((void *)CODE_VIRT, SRAM_CODE_SIZE_MP3, 
					 PROT_EXEC|PROT_READ|PROT_WRITE,
					 MAP_SHARED | MAP_FIXED, 
					 ocram_fd, SRAM_CODE_OFFSET);
	memset((void*)code, 0, SRAM_CODE_SIZE_MP3);
	decoder_mem = (unsigned char *)mmap((void *)DATA_VIRT_MP3, SRAM_DATA_SIZE_MP3, 
					 PROT_EXEC|PROT_READ|PROT_WRITE,
					 MAP_SHARED | MAP_FIXED, 
					 ocram_fd, SRAM_DATA_OFFSET_MP3);
	memset((void*)decoder_mem, 0, SRAM_DATA_SIZE_MP3);
	//mp_msg(MSGT_DECAUDIO,MSGL_INFO,"code %p decoder_mem %p\n", code, decoder_mem);

	/* load decoder binary to the SRAM */ 
	nread = 0;  
	while ((i = read(code_fd, buf, sizeof(buf)))) {
		memcpy(code + nread, buf, i); 
		nread += i; 
	}
	close(code_fd);

	/* Assign function pointer to the sram */ 
	pfnDecoder = (pdecoder)code; 
	/* decode mp3 -> wav to the sram wav buffer */ 
	pfnDecoder(&dec_fptr);

	mp3_Decoder = (TSpiritMP3Decoder *)decoder_mem;
	mp3_Info = (TSpiritMP3Info *)((uint32_t)decoder_mem + sizeof(TSpiritMP3Decoder));

	dec_fptr.SpiritMP3DecoderInit((TSpiritMP3Decoder *)decoder_mem, RetrieveMP3Data, (int*)sh);

	i = decode_audio(sh, sh->a_buffer, 1, sh->a_buffer_size);
	if (i > 0)
		sh->a_buffer_len = i;

	sh->samplesize = SAMPLE_SIZE;					// bytes (not bits!) per sample per channel
	sh->sample_format = AF_FORMAT_S16_LE;			// sample format, see libao2/afmt.h
	sh->channels = mp3_Info->nChannels;			// number of channels
	sh->samplerate = mp3_Info->nSampleRateHz;		// samplerate

	sh->i_bps = mp3_Info->nBitrateKbps*1000/8;		// input data rate (compressed bytes per second)
	// Note: if you have VBR or unknown input rate, set it to some common or
	// average value, instead of zero. it's used to predict time delay of
	// buffered compressed bytes, so it must be more-or-less real!}
	// 
/*
	mp_msg(MSGT_DECAUDIO,MSGL_INFO,"sample size %d\n", sh->samplesize);
	mp_msg(MSGT_DECAUDIO,MSGL_INFO,"channels %d\n", sh->channels);
	mp_msg(MSGT_DECAUDIO,MSGL_INFO,"samplerate %d\n", sh->samplerate);
	mp_msg(MSGT_DECAUDIO,MSGL_INFO,"i_bps %d\n", sh->i_bps);
*/
	return 1;
	
err2:
	close(code_fd);
err1:
	return 0;
}

static void uninit(sh_audio_t *sh)
{
	munmap(code, SRAM_CODE_SIZE_MP3);
	munmap(decoder_mem, SRAM_DATA_SIZE_MP3);
	close(ocram_fd);
	ocram_fd = 0;
}

static int decode_audio (sh_audio_t *sh, unsigned char *buf, int minlen, int maxlen)
{
	unsigned int uiSamplesProduced;
	int nDecodedBytes;
	int nDecodedBytesTotal = 0;

	//mp_msg(MSGT_DECAUDIO,MSGL_INFO,"-buf %p min %d max %d\n", buf, minlen, maxlen);

	while (nDecodedBytesTotal < minlen)
	{
		uiSamplesProduced = dec_fptr.SpiritMP3Decode(
							mp3_Decoder,           	// Decoder structure
							(short*)buf,            // [OUT] Output PCM buffer
							FRAME_SIZE_SAMPLES,     // Output PCM buffer size in samples
							mp3_Info               	// [OUT] Audio Information structure
							);
		if (uiSamplesProduced == 0)
		{
			break;
		}
		nDecodedBytes = uiSamplesProduced * mp3_Info->nChannels * sizeof(short);

		nDecodedBytesTotal += nDecodedBytes;
		buf += nDecodedBytes;
	}
	//mp_msg(MSGT_DECAUDIO,MSGL_INFO,"-len_decoded %d\n", nDecodedBytesTotal);


	// return value: number of _bytes_ written to output buffer
	// or -1 for EOF (or uncorrectable error)
	return nDecodedBytesTotal;	
}


