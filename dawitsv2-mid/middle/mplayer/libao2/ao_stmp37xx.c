#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "libavutil/common.h"
#include "mpbswap.h"
#include "subopt-helper.h"
#include "libaf/af_format.h"
#include "audio_out.h"
#include "audio_out_internal.h"
#include "mp_msg.h"
#include "help_mp.h"
#include "stmp3xxx_dac_ioctl.h"

static ao_info_t info = 
{
	"Stmp37xx DAC",
	"stmp37xx",
	"",
	""
};

LIBAO_EXTERN(stmp37xx)

static int audio_fd = 0;

// to set/get/query special features/parameters
static int control(int cmd,void *arg){
    return -1;
}
#include <errno.h>
// open & setup audio device
// return: 1=success 0=fail
static int init(int rate,int channels,int format,int flags)
{
	  audio_fd = open("/dev/dac", O_RDWR | O_APPEND | O_NONBLOCK);
	if (audio_fd < 0)
	{
		mp_msg(MSGT_AO,MSGL_INFO,"/dev/dac open error\n");
		return 0;
	}
	else
		printf("\n\n/dev/dac open\n"); //dhsong
	ioctl(audio_fd, STMP3XXX_DAC_IOCT_HP_SOURCE, HP_DAC);

	// Init DAC device
	ioctl(audio_fd, STMP3XXX_DAC_IOCT_INIT);

	{
		// Set dac and headphone volumes
		unsigned hp_vol_range = ioctl(audio_fd, STMP3XXX_DAC_IOCQ_VOL_RANGE_GET, VC_HEADPHONE);
		unsigned dac_vol_range = ioctl(audio_fd, STMP3XXX_DAC_IOCQ_VOL_RANGE_GET, VC_DAC);

		ioctl(audio_fd, STMP3XXX_DAC_IOCT_VOL_SET, MAKE_VOLUME(VC_DAC, GET_MAX_VOL(dac_vol_range)));
		ioctl(audio_fd, STMP3XXX_DAC_IOCT_VOL_SET, MAKE_VOLUME(VC_HEADPHONE, 100));
		//ioctl(audio_fd, STMP3XXX_DAC_IOCT_VOL_SET, MAKE_VOLUME(VC_DAC, 256)); //dhsong
		//ioctl(audio_fd, STMP3XXX_DAC_IOCT_VOL_SET, MAKE_VOLUME(VC_HEADPHONE, 128)); //dhsong
	}

	ioctl(audio_fd, STMP3XXX_DAC_IOCT_SAMPLE_RATE, rate);  

	ao_data.format = format;
	ao_data.channels = channels;
	ao_data.samplerate = rate;
	ao_data.outburst = 800*10;;
	ao_data.bps=ao_data.channels;

	if(ao_data.format != AF_FORMAT_U8 && ao_data.format != AF_FORMAT_S8)
	  ao_data.bps*=2;

	ao_data.outburst-=ao_data.outburst % ao_data.bps; // round down
	ao_data.bps*=ao_data.samplerate;

	return 1;
}

// close audio device
static void uninit (int immed)
{
	if (audio_fd > 0)
		close(audio_fd);
}

// stop playing and empty buffers (for seeking/pause)
static void reset(void){

}

// stop playing, keep buffers (for pause)
static void audio_pause(void)
{
}

// resume playing, after audio_pause()
static void audio_resume(void)
{
}

// return: how many bytes can be played without blocking
static int get_space(void)
{
	stmp3xxx_dac_queue_state_t qs;
	ioctl(audio_fd, STMP3XXX_DAC_IOCS_QUEUE_STATE, &qs);
	//fprintf(stderr, "%d %d %d\n",qs.total_bytes, qs.remaining_now_bytes, qs.remaining_next_bytes);
	return qs.total_bytes - qs.remaining_next_bytes;
}

// plays 'len' bytes of 'data'
// it should round it down to outburst*n
// return: number of bytes played
static int play(void* data,int len,int flags)
{
    int num_written = 0;
	char *iii = (char*)data;
	do
	{
		 int rd;
		 if ((rd = write(audio_fd, iii + num_written, len - num_written)) <= 0)
			 break;
		num_written += rd;
	} 
	while (num_written < len);
	return num_written;
}

// return: delay in seconds between first and last sample in buffer
static float get_delay(void)
{
	stmp3xxx_dac_queue_state_t qs;
	ioctl(audio_fd, STMP3XXX_DAC_IOCS_QUEUE_STATE, &qs);
	return (float)qs.remaining_next_bytes/ao_data.samplerate/4;
}






