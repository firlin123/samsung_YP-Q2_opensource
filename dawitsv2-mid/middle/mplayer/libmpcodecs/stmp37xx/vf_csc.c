/*
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include "config.h"
#include "mp_msg.h"
#include "help_mp.h"
#include "cpudetect.h"

#include "img_format.h"
#include "mp_image.h"
#include "vf.h"
#include "vf_csc.h"

#include "mcw_yuv2rgb16_api.h"
#include "stmp3xxx_csc_ioctl.h"
#include "vd_stcommon.h"

#if (MAX_FRAME_WIDTH < MAX_FRAME_HEIGHT)
	#define ROTATE 1
#else
	#define ROTATE 0
#endif

static struct vf_priv_s priv =
{
	// below working only in sw conversion mode
	.brightness	= 0,		// default 0, (-255 ~ 255)
	.contrast	= 4,		// default 4, (0 ~ 8)
};

static convertor_data_t convertor_data;		// HW conversion data
static YUV2RGB_SIZE_INFO Yuv2rgbSizeInfo;	// SW conversion data

static int config(struct vf_instance_s* vf,
				  int width, int height, int d_width, int d_height,
				  unsigned int flags, unsigned int outfmt)
{
	/* check resolution if CSC does support it */
	if ((d_width > MAX_FRAME_WIDTH) || (d_height > MAX_FRAME_HEIGHT))
	{
		mp_msg(MSGT_VO, MSGL_ERR, "[CSC] Over 320x240 : Cannot support %dx%d\n", d_width, d_height);
		return 0;
	}

	/* use HW method for full size image only */
	/* added by jinho.lim to support HW csc even not 320x240 */
	#ifdef STMP_BUILD
	vf->priv->bUseHWCSC = 1;
	#else
	if (d_width == MAX_FRAME_WIDTH && d_height == MAX_FRAME_HEIGHT) {
		vf->priv->bUseHWCSC = 1;
	}
	else {
		vf->priv->bUseHWCSC = 0;
	}
	#endif

	if (vf->priv->bUseHWCSC) {
		convertor_data.y_buffer_size = d_width * d_height;
		convertor_data.u_buffer_size = d_width * d_height/4;
		convertor_data.v_buffer_size = d_width * d_height/4;
		convertor_data.memory_stride  = d_width;
		convertor_data.height = d_height;
		convertor_data.rgb_buffer_size = MAX_FRAME_WIDTH*MAX_FRAME_HEIGHT*2;
	}
	else {
		Yuv2rgbSizeInfo.image_buffer_width  = d_width;
		Yuv2rgbSizeInfo.image_width     = d_width;
		Yuv2rgbSizeInfo.image_height    = d_height;
	#ifdef STMP_BUILD
		Yuv2rgbSizeInfo.lcd_width		= MAX_FRAME_HEIGHT;
		Yuv2rgbSizeInfo.lcd_height		= MAX_FRAME_WIDTH;
		Yuv2rgbSizeInfo.lcd_xoffset 	= (MAX_FRAME_HEIGHT - d_height)/2;
		Yuv2rgbSizeInfo.lcd_yoffset 	= (MAX_FRAME_WIDTH	- d_width)/2;
		Yuv2rgbSizeInfo.output_rotate	= 3;		// 0: normal, 1: rotate 90, 2: 180, 3:270
	#else
		Yuv2rgbSizeInfo.lcd_width       = MAX_FRAME_WIDTH;
		Yuv2rgbSizeInfo.lcd_height      = MAX_FRAME_HEIGHT;
		Yuv2rgbSizeInfo.lcd_xoffset 	= (MAX_FRAME_WIDTH	- d_width)/2;
		Yuv2rgbSizeInfo.lcd_yoffset 	= (MAX_FRAME_HEIGHT - d_height)/2;
		Yuv2rgbSizeInfo.output_rotate	= ROTATE;		// 0: normal 1:rotate 90
	#endif
		// todo: scale??
		Yuv2rgbSizeInfo.Scale        	= 2;	// default 2, 0:center 1:half 2:same 3:double size

		if (Yuv2rgbSizeInfo.lcd_xoffset < 0)
			Yuv2rgbSizeInfo.lcd_xoffset = 0;
		if (Yuv2rgbSizeInfo.lcd_yoffset < 0)
			Yuv2rgbSizeInfo.lcd_xoffset = 0;
	}

	mp_msg(MSGT_VO, MSGL_DBG2, "[CSC] Using %s\n", vf->priv->bUseHWCSC ? "HW" : "SW");
	return vf_next_config(vf, width, height, d_width, d_height, flags, IMGFMT_BGR16);
}

/* Filter handler */
static int put_image (struct vf_instance_s* vf, mp_image_t *mpi, double pts)
{
    mp_image_t        *dmpi;
    struct vf_priv_s  *p = vf->priv;

	// get image buffer to write
	dmpi = vf_get_image(vf->next, IMGFMT_BGR16, MP_IMGTYPE_STATIC, 0, mpi->width, mpi->height);

	if (dmpi)
	{
		if (p->bUseHWCSC)
		{
			// ==================================================================
			//	HW CSC conversion
			// ==================================================================
			convertor_data.y_buffer = mpi->planes[0];
			convertor_data.u_buffer = mpi->planes[1];
			convertor_data.v_buffer = mpi->planes[2];
			convertor_data.rgb_buffer = dmpi->planes[0];

			/*
			fprintf(stderr, "%p %p %p %p\n", convertor_data.y_buffer, convertor_data.u_buffer, convertor_data.v_buffer, 
					convertor_data.rgb_buffer);
			fprintf(stderr, "%d %d \n", convertor_data.memory_stride, convertor_data.height);
			*/

			if (ioctl(p->fd, STMP3XXX_CSC_IOCS_CONVERT, &convertor_data) < 0)
				return 0;
			dmpi->stride[0] = mpi->width*2;
		}
		else
		{
			// ==================================================================
			//	SW conversion
			// ==================================================================
			POINTER_YUV YUV;
			
			/////////////////////////////////////////////
			//eI - 116 - added code to support YUV to RGB
			YUV.pbY = mpi->planes[0];
			YUV.pbU = mpi->planes[1];
			YUV.pbV = mpi->planes[2];
			Yuv2rgbSizeInfo.Bright   = p->brightness;	// default 0, (-255 ~ 255)
			Yuv2rgbSizeInfo.Contrast = p->contrast;		// default 4, (0 ~ 8)
		
			//fprintf(stderr, "%p %p %p %p\n", YUV.pbY, YUV.pbU, YUV.pbV, mpi->planes[0]);
			MCW_SIGMATEL_YUV420_To_RGB16((unsigned short *)dmpi->planes[0], &YUV, &Yuv2rgbSizeInfo);
			dmpi->stride[0] = mpi->width*2;
		}
	} // if (dpmi)

	return vf_next_put_image(vf, dmpi, pts);
}

static void uninit (struct vf_instance_s* vf)
{
#if 0
	/* !!!hack !!! Do not close CSC driver due to error when opening CSC driver again */
	int i;
	for (i = 0; i > MAX_YUV_BUFFER; i++) {
		munmap(vf->priv->y[i], vf->priv->buf_size);
	}
	close(vf->priv->fd);
#endif
}

/* only YV12 is supported */
static int query_format (struct vf_instance_s* vf, unsigned int fmt)
{
	switch (fmt)
	{
        case IMGFMT_YV12:
			return vf_next_query_format(vf, IMGFMT_BGR16);
		default:
			break;
	}
	return 0;
}

static int control (struct vf_instance_s* vf, int request, void* data)
{
	vf_equalizer_t *eq;

	// NOTE: EQ is working only in SW conversion mode
	switch (request) {
	case VFCTRL_SET_EQUALIZER:
		eq = data;
		if (!strcmp(eq->item,"brightness")) {
			vf->priv->brightness = eq->value;
			return CONTROL_TRUE;
		}
		else if (!strcmp(eq->item,"contrast")) {
			vf->priv->contrast = eq->value;
			return CONTROL_TRUE;
		}
		break;
	case VFCTRL_GET_EQUALIZER:
		eq = data;
		if (!strcmp(eq->item,"brightness")) {
			eq->value = vf->priv->brightness;
			return CONTROL_TRUE;
		}
		else if (!strcmp(eq->item,"contrast")) {
			eq->value = vf->priv->contrast;
			return CONTROL_TRUE;
		}
		break;
	}
	return vf_next_control(vf, request, data);
}


/* Main entry funct for the filter */
static int csc_open (vf_instance_t *vf, char* args)
{
    struct vf_priv_s *p;
	char			*yuv;
	int i;

    vf->put_image    = put_image;
    vf->query_format = query_format;
    vf->config       = config;
    vf->uninit       = uninit;
    vf->default_reqs = VFCAP_ACCEPT_STRIDE;
	vf->control		= control;

	/* Private data */
    vf->priv = p = &priv;

	if (p->fd <= 0) {
		p->fd = open("/dev/csc", O_RDWR);

		if (p->fd < 0)
		{
			mp_msg(MSGT_VFILTER,MSGL_ERR,"Sigmatel CSC device open error\n");
			goto err2;
		}

		/* YUV frame buffer size */
		p->buf_size = MAX_FRAME_WIDTH*MAX_FRAME_HEIGHT*2;
		/* align to page boundary */
		p->buf_size = (p->buf_size + 4095) & ~4095;

		/* mmap YUV frame */
		for (i = 0; i < MAX_YUV_BUFFER; i++)
		{
			if ((yuv = (char *)mmap(0, p->buf_size,
									PROT_READ | PROT_WRITE, MAP_SHARED, p->fd,
									p->buf_size*i)) == (char *)-1)
			{
				mp_msg(MSGT_VFILTER,MSGL_ERR,"Sigmatel CSC device mmap error (%d) %s\n", i, strerror(errno));
				goto err1;
			}
			p->y[i] = yuv;
			p->u[i] = p->y[i] + (MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT);
			p->v[i] = p->u[i] + (MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT/4);
			//mp_msg(MSGT_VFILTER,MSGL_INFO,"%p %p %p\n", p->y[i], p->u[i], p->v[i]);
		}
	}
	return 1;

err1:
	close(p->fd);
err2:
	return 0;
}

vf_info_t vf_info_csc =
{
    "STMP37xx CSC (YV12->BGR16)",
    "csc",
    "Sigmatel",
    "",
    csc_open,
    NULL
};

