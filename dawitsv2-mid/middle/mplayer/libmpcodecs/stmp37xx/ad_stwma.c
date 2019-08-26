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
#include "wmadecS_api.h"

static ad_info_t info = 
{
	"STMP37xx wma codecs",
	"stwma",
	NULL,
	NULL,
	NULL
};

LIBAD_EXTERN(stwma)

#define MAX_SAMPLES     	64

#define SAMPLE_SIZE			2
#define CHANNEL_NUMBER		2

/* WMA rom code */
#define CODE_VIRT			0x50000000
#define DATA_VIRT			0x5001F000
#define SDRAM_CODE_VIRT		0x50100000

#define SRAM_CODE_SIZE  (112*1024)	//rom file size (wmar_ocram.rom)
#define SDRAM_CODE_SIZE (6*1024) 	//rom file size (wmar_sdram.rom)
#define SRAM_DATA_SIZE  (44*1024)	//(120 * 1024 )

#define SRAM_CODE_OFFSET 0
#define SRAM_DATA_OFFSET (SRAM_CODE_OFFSET + SRAM_CODE_SIZE)

static WMARawDecHandle pWMA;
static sh_audio_t *sh_wma;

static tBuffers *Baseadd;
static int block_left_bytes;
static int all_frame_done;

static int ocram_fd;

static unsigned char *decoder_mem;
static unsigned char *ocram_code, *sdram_code;

static Dec_fptr_t Dec_fptr; // Function pointer available to application
static pdecoder pfnDecoder; // Decoder function pointer table.

static WMARESULT WMARawDecCBGetData (U8_WMARawDec **ppBuffer, U32_WMARawDec *pcbBuffer);


static int control(sh_audio_t *sh,int cmd,void* arg, ...)
{
	// various optional functions you MAY implement:
	switch(cmd){
	case ADCTRL_RESYNC_STREAM:
		// it is called once after seeking, to resync.
		Dec_fptr.WMARawDecReset(pWMA);
		all_frame_done = 1;
		block_left_bytes = 0;
		return CONTROL_TRUE;
	}
	return CONTROL_UNKNOWN;
}

static int preinit (sh_audio_t *sh)
{
	// minimum output buffer size (should be the uncompressed max. frame size)
	// Default: 8192
	//sh->audio_out_minsize = 8192;
	sh->audio_out_minsize = MAX_SAMPLES*SAMPLE_SIZE*CHANNEL_NUMBER;

	// minimum input buffer size (set only if you need input buffering)
	// (should be the max compressed frame size)
	//sh->audio_in_minsize = 2048; // Default: 0 (no input buffer)
	return 1;
}

static int load_rom (void)
{
	int ret = 1;
	int ocram_rom_fd, sdram_rom_fd;
	int  i, nread;
	char buf[4096]; 
	char path[128] = {0}, *ssroot; 

	/* get SSROOT env */ 
	ssroot = getenv("SSROOT");

	/* open wmar_ocram.rom  */ 
	if (ssroot)
		strcpy(path, ssroot);
	strcat(path, "/codec/wmar_ocram.rom");
	ocram_rom_fd = open(path, O_RDONLY);
	if (ocram_rom_fd < 0)
	{
		mp_msg(MSGT_DECAUDIO,MSGL_INFO,"%s open error\n", path);
		ret = 0;
		goto err1;
	}

	/* open wmar_sdram.rom  */ 
	if (ssroot)
		strcpy(path, ssroot);
	strcat(path, "/codec/wmar_sdram.rom");
	sdram_rom_fd = open(path, O_RDONLY);
	if (sdram_rom_fd < 0)
	{
		mp_msg(MSGT_DECAUDIO,MSGL_INFO,"%s open error\n", path);
		ret = 0;
		goto err2;
	}

	/* open ocram device */ 
	ocram_fd = open("/dev/ocram", O_RDWR | O_NONBLOCK);
	if (ocram_fd < 0)
	{
		mp_msg(MSGT_DECAUDIO,MSGL_INFO,"/dev/ocram open error\n");
		ret = 0;
		goto err3;
	}

	/* mmap ocram & sdram */ 
	ocram_code = (unsigned char *)mmap((void *)CODE_VIRT, SRAM_CODE_SIZE,
								 PROT_EXEC|PROT_READ|PROT_WRITE,
								 MAP_SHARED | MAP_FIXED,
								 ocram_fd, 0x0);
	sdram_code = (unsigned char *)mmap((void *)SDRAM_CODE_VIRT, SDRAM_CODE_SIZE,
								 PROT_EXEC|PROT_READ|PROT_WRITE,
								 MAP_SHARED | MAP_FIXED,
								 ocram_fd, 0x0);
	decoder_mem = (unsigned char *)mmap((void *)DATA_VIRT, SRAM_DATA_SIZE, 
									PROT_EXEC|PROT_READ|PROT_WRITE,
									MAP_SHARED | MAP_FIXED, 
									ocram_fd, SRAM_DATA_OFFSET);
	//mp_msg(MSGT_DECAUDIO,MSGL_INFO,"ocram_code %p sdram_code %p decoder_mem %p\n", ocram_code, sdram_code, decoder_mem);

	/* load decoder binary to the SRAM */ 
	nread = 0;  
	while ((i = read(ocram_rom_fd, buf, sizeof(buf))) ) {
		memcpy(ocram_code + nread, buf, i); 
		nread += i; 
	}

	nread = 0;  
	while ((i = read(sdram_rom_fd, buf, sizeof(buf)))) {
		memcpy(sdram_code + nread, buf, i); 
		nread += i; 
	}

	/* get sram buffer ptr */
    Dec_fptr.WMARawDecCBGetData = WMARawDecCBGetData;
    
    pfnDecoder = (pdecoder)ocram_code;
    pfnDecoder(&Dec_fptr);

	Baseadd = (void *)decoder_mem;
	pWMA = (WMARawDecHandle)&Baseadd;

err3:
    close(sdram_rom_fd);
err2:
	close(ocram_rom_fd);
err1:
	return ret;
}

static int init (sh_audio_t *sh)
{
	int version, SamplesPerBlock, nEncodeOpt;
	WMARESULT wmar;

	if (!sh->wf)
		return 0;

	if (sh->wf->wFormatTag == 0x160)
		version = 1;
	else if (sh->wf->wFormatTag == 0x161)
		version = 2;
	else {
		mp_msg(MSGT_DECAUDIO,MSGL_ERR,"Tag(%x) is not supported\n", sh->wf->wFormatTag);
		return 0;
	}

	sh->samplesize = SAMPLE_SIZE;					// bytes (not bits!) per sample per channel
	sh->sample_format = AF_FORMAT_S16_LE;			// sample format, see libao2/afmt.h

	sh->channels = sh->wf->nChannels;
	sh->samplerate = sh->wf->nSamplesPerSec;
	sh->i_bps = sh->wf->nAvgBytesPerSec;			// input data rate (compressed bytes per second)

	// get codec specific data
	if (sh->wf->cbSize == 0) {
		mp_msg(MSGT_DECAUDIO,MSGL_ERR,"codec specific size is zero\n");
		return 0;
	}
	sh->codecdata = (char*)(sh->wf+1);
	sh->codecdata_len = sh->wf->cbSize;

	// get samples per block & encode optionfrom codec specific data
	if (version == 2) {
		// MSAudio v2
		SamplesPerBlock = (int)(sh->codecdata[0]) +
						  (int)(sh->codecdata[1]<<8) +
						  (int)(sh->codecdata[2]<<16) +
						  (int)(sh->codecdata[3]<<26);
		nEncodeOpt = (short)(sh->codecdata[4]) +
					 (short)(sh->codecdata[5]<<8);
	}
	else {
		// MSAudio v1
		SamplesPerBlock = (int)(sh->codecdata[0]) +
						  (int)(sh->codecdata[1]<<8);
		nEncodeOpt = (short)(sh->codecdata[2]) +
					 (short)(sh->codecdata[3]<<8);
	}

	#if 1
	{
		int i;
		mp_msg(MSGT_DECAUDIO,MSGL_INFO, "WMA specific info size - %d [ ", sh->codecdata_len);
		for (i = 0; i < sh->codecdata_len; i++) {
			mp_msg(MSGT_DECAUDIO,MSGL_INFO, "%02x ", sh->codecdata[i]);
		}
		mp_msg(MSGT_DECAUDIO,MSGL_INFO, "]\n");

		mp_msg(MSGT_DECAUDIO,MSGL_INFO, "version %d, samplesperblock %d, samplespersec %d\n"
				"nchannes %d, avgbytespersec %d, blockalign %d, encodeopt %d\n", 
			   version, SamplesPerBlock, 
			   sh->wf->nSamplesPerSec, sh->wf->nChannels, sh->wf->nAvgBytesPerSec,
			   sh->wf->nBlockAlign, nEncodeOpt);
	}
	#endif

	/* setup internal variables */
	sh_wma = sh;
	block_left_bytes = 0;
	all_frame_done = 1;

	/* INIT WMA decoder */
	if (!load_rom()) {
		mp_msg(MSGT_DECAUDIO,MSGL_ERR,"Error loading ROM image\n");
	}

	wmar = Dec_fptr.WMARawDecInit (*pWMA,
						(U16_WMARawDec) version, 
						(U16_WMARawDec) SamplesPerBlock, 
						(U16_WMARawDec) sh->wf->nSamplesPerSec, 
						(U16_WMARawDec) sh->wf->nChannels, 
						(U16_WMARawDec) sh->wf->nAvgBytesPerSec, 
						(U16_WMARawDec) sh->wf->nBlockAlign, 
						(U16_WMARawDec) nEncodeOpt ); 

	if (wmar == WMA_E_NOTSUPPORTED) { 
		mp_msg(MSGT_DECAUDIO,MSGL_ERR,"Bad Sampling Rate\n");
		goto err1; 
	} 
	if (pWMA == NULL || wmar != WMA_OK) { 

		mp_msg(MSGT_DECAUDIO,MSGL_ERR,"Cannot initialize the WMA decoder\n");
		goto err1; 
	}

	mp_msg(MSGT_DECAUDIO,MSGL_DBG2,"WMA decoder init OK\n");
	return 1;

err1:
	return 0;
}

static void uninit(sh_audio_t *sh)
{
	munmap(ocram_code, SRAM_CODE_SIZE);
	munmap(sdram_code, SDRAM_CODE_SIZE);
	munmap(decoder_mem, SRAM_DATA_SIZE);

	close(ocram_fd);
	ocram_fd = 0;
}

WMARESULT WMARawDecCBGetData (U8_WMARawDec **ppBuffer, U32_WMARawDec *pcbBuffer)
{
    WMARESULT retcode = WMA_OK;
	unsigned char *buf;
	int len_to_read = 128;
	int nread;

	buf = (char*)*ppBuffer;

	if (block_left_bytes == 0) {
		block_left_bytes = sh_wma->wf->nBlockAlign;
		retcode = WMA_S_NEWPACKET;
	}
	if (len_to_read > block_left_bytes)
		len_to_read = block_left_bytes;

	nread = demux_read_data(sh_wma->ds, buf, len_to_read);

	#if 0 // code from test harness, but not necessary
	if((block_left_bytes == sh_wma->wf->nBlockAlign) || (block_left_bytes == 0))
	{
		*pcbBuffer = nread;
		block_left_bytes -= *pcbBuffer;
		return WMA_S_NEWPACKET;
	}
	#endif

	*pcbBuffer = nread;
	block_left_bytes -= nread;

	return retcode;
}


static int decode_audio (sh_audio_t *sh, unsigned char *buf, int minlen, int maxlen)
{
	int nDecodedBytesTotal = 0;
	WMARESULT wmar;

	while (nDecodedBytesTotal < minlen) {
		U32_WMARawDec decoded_samples;

		// exit if eof
		if (sh->ds->eof)
			return -1;

		if (all_frame_done) {
			all_frame_done = 0;

			wmar = Dec_fptr.WMARawDecStatus(pWMA);
            if (wmar == WMA_E_ONHOLD) {
				//fprintf(stderr, "## WMA_E_ONHOLD 1\n");
				return -1;
			}
		}

		wmar = Dec_fptr.WMARawDecDecodeData(pWMA, (U8_WMARawDec *)0, (U32_WMARawDec *)&decoded_samples);

		if (wmar == WMA_S_NO_MORE_FRAME) {
			all_frame_done = 1;
		}
		else if (wmar == WMA_E_LOSTPACKET) {
			all_frame_done = 1;
		}
		else if (wmar == WMA_E_BROKEN_FRAME) {
			// reset is recommended then go to the next payload 
			Dec_fptr.WMARawDecReset(pWMA);
			all_frame_done = 1;
		}
		else if (wmar == WMA_E_ONHOLD) {
			//fprintf(stderr, "## WMA_E_ONHOLD 2\n");
			all_frame_done = 1;
			return -1;
		}
		else if (wmar == WMA_S_NO_MORE_SRCDATA) {
			return -1;
		}
		else if (WMARAW_FAILED(wmar)) { 
			// Wei-ge recommends resetting after any error
			//fprintf("%x\n", wmar);
			Dec_fptr.WMARawDecReset(pWMA);
		}

		while (1) {
			unsigned int samples_produced;
			int nDecodedBytes;

			samples_produced = MAX_SAMPLES;
			samples_produced >>= 1;

			wmar = Dec_fptr.WMARawDecGetPCM(pWMA,
							(U16_WMARawDec*) &samples_produced,
							(U8_WMARawDec*) buf, 
							(U32_WMARawDec) 512);

			if (samples_produced == 0) {
				break;
			}
    		if (wmar == WMA_OK) {
				nDecodedBytes = samples_produced * sh->channels * sizeof(short);
				nDecodedBytesTotal += nDecodedBytes;
				buf += nDecodedBytes;
			}
		}
	}

	// return value: number of _bytes_ written to output buffer
	// or -1 for EOF (or uncorrectable error)
	return nDecodedBytesTotal;	
}

