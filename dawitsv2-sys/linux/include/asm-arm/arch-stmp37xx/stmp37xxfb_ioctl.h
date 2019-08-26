#ifndef STMP37XXFB_IOCTL_H
#define STMP37XXFB_IOCTL_H

#include <linux/ioctl.h>

#if defined __KERNEL__
#define __USER	__user
#else
#define __USER
#endif


typedef struct
{
	void* __USER rgb_buffer;
    unsigned long rgb_buffer_size;
} refresh_data_t;

typedef struct
{
    unsigned vals[6];
} fb_perf_data_t;


typedef struct
{
	int x, y, w, h;
	int alphaValue;
	int alphaColor;
	int chromaColor;
	/* 2008.08.23: add for indexing { */
	union {
		unsigned int index;
		struct { /* Follow this order in case of little endian */
			unsigned char id;
			unsigned char en;
			unsigned char up;
			unsigned char rv;
		} __attribute__ ((packed)) b;
	};
	/* 2008.08.23: add for indexing } */
} osd_data_t;

/*
 * For Samsung App Interface
 */
struct fb_area {
	int x;
	int y;
	int w;
	int h;
};

typedef struct fb_mirror_t {
	unsigned char enable;
	struct fb_area box;
} fb_mirror;

#define STMP37XXFB_IOC_MAGIC  0xC1

#define STMP37XXFB_IOCT_REFRESH             _IO(STMP37XXFB_IOC_MAGIC,   0)
#define STMP37XXFB_IOCT_PERIODIC_ON         _IO(STMP37XXFB_IOC_MAGIC,   1)
#define STMP37XXFB_IOCT_PERIODIC_OFF        _IO(STMP37XXFB_IOC_MAGIC,   2)

#define STMP37XXFB_IOCT_REFRESH_COPYLESS    _IOW(STMP37XXFB_IOC_MAGIC,  3, refresh_data_t)
#define STMP37XXFB_IOCS_PERF		    _IOW(STMP37XXFB_IOC_MAGIC,  5, fb_perf_data_t)

#define STMP37XXFB_IOCS_BACKLIGHT	    _IOW(STMP37XXFB_IOC_MAGIC,  6, unsigned)
#define STMP37XXFB_IOCT_OSD_MODE            _IOW(STMP37XXFB_IOC_MAGIC,   7, int)
#define STMP37XXFB_IOCT_OSD_UI_DIRECTUPDATE  _IOW(STMP37XXFB_IOC_MAGIC,   8, int)

/* reset osd_data array if parameter is NULL */
#define STMP37XXFB_IOCT_OSD_DATA            _IOW(STMP37XXFB_IOC_MAGIC,   9, osd_data_t)
/*
 * For Samsung App Interface
 */
#define FBIO_STMP36XX_FB_FLUSH		_IOW(STMP37XXFB_IOC_MAGIC, 10, struct fb_area)
#define FBIO_STMP36XX_FB_FLUSH_ENABLE	_IOW(STMP37XXFB_IOC_MAGIC, 11, unsigned long)
#define FBIO_STMP36XX_FB_BACKLIGHT	_IOW(STMP37XXFB_IOC_MAGIC, 12, unsigned long)
#define FBIO_STMP36XX_FB_BACKLIGHT_MODE	_IOW(STMP37XXFB_IOC_MAGIC, 13, unsigned long)
#define FBIO_STMP36XX_FB_LCDIF		_IOW(STMP37XXFB_IOC_MAGIC, 14, unsigned long)


/* Modified ioctl code { */
#define FBIO_FB_BACKLIGHT	_IOW(STMP37XXFB_IOC_MAGIC, 15, unsigned long)
#define FBIO_LCD_ON_OFF      _IOW(STMP37XXFB_IOC_MAGIC, 16, unsigned char)
#define FBIO_FB_BACKLIGHT_OFF      _IOW(STMP37XXFB_IOC_MAGIC, 17, unsigned char)
/* Modified ioctl code { */

/* 2009.03.11: for flipping video frame manually { */
#define FBIO_FB_MIRROR_VIDEO _IOW(STMP37XXFB_IOC_MAGIC, 18, fb_mirror *)
/* 2009.03.11: for flipping video frame manually } */
#endif
