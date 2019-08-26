/*
 *  linux/drivers/video/stmp37xxfb.c -- Virtual frame buffer device
 *
 *      Copyright (C) 2007 Sigmatel
 *
 *      Copyright (C) 2002 James Simmons
 *
 *	Copyright (C) 1997 Geert Uytterhoeven
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/arch/irqs.h>
#include <linux/dma-mapping.h>
#include <asm/semaphore.h>
#include <asm/cacheflush.h>

#include <asm/uaccess.h>
#include <linux/fb.h>
#include <linux/init.h>

#include <asm/arch/stmp37xxfb_ioctl.h>
#include <asm/arch/dma.h>
#include <asm/arch/clk.h>

#include <linux/delay.h>
#include <linux/proc_fs.h>

// Test defines
//#define FB_DEBUG 1

#include "stmp37xxfb_panel.h"

//#define USE_VSYNC_MODE
//#define USE_WSYNC_MODE

#define USE_OLD_CLK
//#define USE_OLD_DMA

/*
 * For Samsung App Interface
 */

static volatile int lcd_onoff;
static int need_bl_init;

static void init_lcd(void);
static void lcd_on(void);
static void lcd_off(void);

wait_queue_head_t wait_q;
DECLARE_MUTEX(refresh_mutex);

#ifdef USE_WSYNC_MODE
static struct semaphore wsync_mutex;
static irqreturn_t device_lcd_err_handler (int irq_num, void* dev_idp);
#endif

#define FBIO_DATA_TRANSFER      _IO(STMP37XXFB_IOC_MAGIC, 18)

typedef struct
{
        reg32_t next;
        reg32_t cmd;
        reg32_t buf_ptr;
        reg32_t pio;
} dma_des_t;

typedef struct
{
	dma_addr_t *dma_addr_p;
	unsigned offset;
} dma_chain_info_t;

static stmp37xx_dma_user_t dma_user =
{
    .name = "stmp37xx frame buffer"
};

static stmp37xx_dma_descriptor_t setup_dma_descriptor;
static dma_addr_t setup_phys;
static unsigned char *setup_memory;

static unsigned long refresh_period = HZ/15;
static struct timer_list refresh_timer;
static int periodic_refresh = 0;
static volatile int s_doing_refresh = 0;

#define VIDEOMEMSIZE	(LCD_WIDTH * LCD_HEIGHT * 2)

static stmp37xx_dma_descriptor_t video_dma_descriptor[MAX_CHAIN_LEN];
static dma_chain_info_t dma_chain_info[MAX_CHAIN_LEN];
static unsigned dma_chain_info_pos;
static void *video_memory[4];
static u_long videomemorysize = VIDEOMEMSIZE;
module_param(videomemorysize, ulong, 0);

static struct fb_var_screeninfo stmp37xxfb_default __initdata = {
	.xres =		LCD_WIDTH,
	.yres =		LCD_HEIGHT,
	.xres_virtual =	LCD_WIDTH,
	.yres_virtual =	LCD_HEIGHT,
	.bits_per_pixel = 16,
	.red =		{ 11, 5, 0 },
	.green =	{ 5, 6, 0 },
	.blue =		{ 0, 5, 0 },
	.activate =	FB_ACTIVATE_TEST,
	.height =	-1,
	.width =	-1,
	.pixclock =	20000,
	.left_margin =	64,
	.right_margin =	64,
	.upper_margin =	32,
	.lower_margin =	32,
	.hsync_len =	64,
	.vsync_len =	2,
	.vmode =	FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo stmp37xxfb_fix __initdata = {
	.id =		"Stmp36xxfb",
	.smem_len =     VIDEOMEMSIZE,
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =       FB_VISUAL_TRUECOLOR,
	.xpanstep =	0,
	.ypanstep =	0,
	.ywrapstep =	0,
	.accel =	FB_ACCEL_NONE,
};

#define FRAME_UI		0
#define FRAME_VIDEO		1
#define FRAME_UI_BACKUP		2
#define FRAME_VIDEO_BACKUP	3

static dma_addr_t video_phys[4];

/* For rotate 270 */
static unsigned short blend_width = LCD_WIDTH;
static unsigned short blend_height = LCD_HEIGHT;

static void stmp37xxfb_platform_release(struct device *device);

static struct platform_device stmp37xxfb_device = {
        .name   = "stmp37xxfb",
        .id     = 0,
        .dev    = {
                .release = stmp37xxfb_platform_release,
		.coherent_dma_mask = ISA_DMA_THRESHOLD,
        }
};

static int stmp37xxfb_enable __initdata = 0;	/* disabled by default */
module_param(stmp37xxfb_enable, int, 0);

static int stmp37xxfb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info);
static int stmp37xxfb_set_par(struct fb_info *info);
static int stmp37xxfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			 u_int transp, struct fb_info *info);
static int stmp37xxfb_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *info);
static int stmp37xxfb_mmap(struct fb_info *info,
		    struct vm_area_struct *vma);
static int stmp37xxfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
static int stmp37xxfb_release (struct fb_info *info, int user);
static int stmp37xxfb_open (struct fb_info *info, int user);

static struct fb_ops stmp37xxfb_ops = {
	.fb_check_var	= stmp37xxfb_check_var,
	.fb_set_par	= stmp37xxfb_set_par,
	.fb_setcolreg	= stmp37xxfb_setcolreg,
	.fb_pan_display	= stmp37xxfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_ioctl       = stmp37xxfb_ioctl,
	.fb_mmap	= stmp37xxfb_mmap,
	.fb_open	= stmp37xxfb_open,
	.fb_release	= stmp37xxfb_release,
};
/*
 *  Internal routines
 */
static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;
	return (length);
}

    /*
     *  Setting the video mode has been split into two parts.
     *  First part, xxxfb_check_var, must not write anything
     *  to hardware, it should only verify and adjust var.
     *  This means it doesn't alter par but it does use hardware
     *  data from it to check this var.
     */

static int stmp37xxfb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info)
{
	u_long line_length;

	/*
	 *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
	 *  as FB_VMODE_SMOOTH_XPAN is only used internally
	 */

	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = info->var.xoffset;
		var->yoffset = info->var.yoffset;
	}

	/*
	 *  Some very basic checks
	 */
	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	/*
	 *  Memory limit
	 */
	line_length =
	    get_line_length(var->xres_virtual, var->bits_per_pixel);
	if (line_length * var->yres_virtual > videomemorysize)
		return -ENOMEM;


	/* RGBA 5551 */
	if (var->transp.length) {
		var->red.offset = 0;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 5;
		var->blue.offset = 10;
		var->blue.length = 5;
		var->transp.offset = 15;
		var->transp.length = 1;
	} else {	/* RGB 565 */
		var->red.offset = 0;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 11;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
	}
                
	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	return 0;
}

/* This routine actually sets the video mode. It's in here where we
 * the hardware state info->par and fix which can be affected by the
 * change in par. For this driver it doesn't do much.
 */
static int stmp37xxfb_set_par(struct fb_info *info)
{
	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);

#if 0
	blend_height = info->var.yres_virtual;
	blend_width = info->var.xres_virtual;
	
	if (info->var.xres_virtual != LCD_WIDTH || info->var.yres_virtual != LCD_HEIGHT)
		lcd_panel_rotation(270);
	else
		lcd_panel_rotation(0);
#endif
	return 0;
}

    /*
     *  Set a single color register. The values supplied are already
     *  rounded down to the hardware's capabilities (according to the
     *  entries in the var structure). Return != 0 for invalid regno.
     */

static int stmp37xxfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			 u_int transp, struct fb_info *info)
{
	if (regno >= 256)	/* no. of hw registers */
		return 1;
	/*
	 * Program hardware... do anything you want with transp
	 */

	/* grayscale works only partially under directcolor */
	if (info->var.grayscale) {
		/* grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue =
		    (red * 77 + green * 151 + blue * 28) >> 8;
	}

	/* Directcolor:
	 *   var->{color}.offset contains start of bitfield
	 *   var->{color}.length contains length of bitfield
	 *   {hardwarespecific} contains width of RAMDAC
	 *   cmap[X] is programmed to (X << red.offset) | (X << green.offset) | (X << blue.offset)
	 *   RAMDAC[X] is programmed to (red, green, blue)
	 *
	 * Pseudocolor:
	 *    uses offset = 0 && length = RAMDAC register width.
	 *    var->{color}.offset is 0
	 *    var->{color}.length contains widht of DAC
	 *    cmap is not used
	 *    RAMDAC[X] is programmed to (red, green, blue)
	 * Truecolor:
	 *    does not use DAC. Usually 3 are present.
	 *    var->{color}.offset contains start of bitfield
	 *    var->{color}.length contains length of bitfield
	 *    cmap is programmed to (red << red.offset) | (green << green.offset) |
	 *                      (blue << blue.offset) | (transp << transp.offset)
	 *    RAMDAC does not exist
	 */
#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
	case FB_VISUAL_PSEUDOCOLOR:
		red = CNVT_TOHW(red, info->var.red.length);
		green = CNVT_TOHW(green, info->var.green.length);
		blue = CNVT_TOHW(blue, info->var.blue.length);
		transp = CNVT_TOHW(transp, info->var.transp.length);
		break;
	case FB_VISUAL_DIRECTCOLOR:
		red = CNVT_TOHW(red, 8);	/* expect 8 bit DAC */
		green = CNVT_TOHW(green, 8);
		blue = CNVT_TOHW(blue, 8);
		/* hey, there is bug in transp handling... */
		transp = CNVT_TOHW(transp, 8);
		break;
	}
#undef CNVT_TOHW
	/* Truecolor has hardware independent palette */
	if (info->fix.visual == FB_VISUAL_TRUECOLOR) {

		if (regno >= 16)
			return 1;

		((u32 *) (info->pseudo_palette))[regno] = (red << info->var.red.offset) |
			                                  (green << info->var.green.offset) |
							  (blue << info->var.blue.offset) |
							  (transp << info->var.transp.offset);

		return 0;
	}
	return 0;
}

    /*
     *  Pan or Wrap the Display
     *
     *  This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
     */

static int stmp37xxfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if (var->vmode & FB_VMODE_YWRAP) {
		if (var->yoffset < 0
		    || var->yoffset >= info->var.yres_virtual
		    || var->xoffset)
			return -EINVAL;
	} else {
		if (var->xoffset + var->xres > info->var.xres_virtual ||
		    var->yoffset + var->yres > info->var.yres_virtual)
			return -EINVAL;
	}
	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;
	if (var->vmode & FB_VMODE_YWRAP)
		info->var.vmode |= FB_VMODE_YWRAP;
	else
		info->var.vmode &= ~FB_VMODE_YWRAP;
	return 0;
}

static void dma_enable(unsigned refresh_phys, int node);

#define MAX_OSD_DATA 10
#define ALPHA_FIXED_128

static int osd_mode = 0;
static int osd_video_backuped = 0;
static int osd_ui_directupdate = 0;
static osd_data_t osd_data[MAX_OSD_DATA];
/* 2008.09.04: to flip the FRAME_UI_BACKUP */
static unsigned short osd_flip_x[MAX_OSD_DATA];
/* 2008.08.28: for erasing OSD area on pause*/
static osd_data_t osd_data_backup[MAX_OSD_DATA];
static uint16_t FgRComponent;
static uint16_t FgGComponent;
static uint16_t FgBComponent;

static char osd_ui_updated = false;

#define HALF_MASK_16 0xF7DE
#define QUATER_MASK_16 0xE79C
#define HALF_MASK_32 0xF7DEF7DE

static unsigned char osd_data_flag = 0; /* 2008.11.18: for avoiding the problem of previous OSD displaying */

/* #define CAL_OSD_PERF */
#ifdef CAL_OSD_PERF
static unsigned long get_elasped(unsigned long begin)
{
	unsigned long ret = 0;
	unsigned long end = HW_DIGCTL_MICROSECONDS_RD();

	if (begin > end) {
		ret = 0xFFFFFFFF - begin + end;
	} else {
		ret = end - begin;
	}
	return ret;
}
#endif
/**
 * @brief            Adjust overlay
 */
static void osd_overlay(short x, short y, short w, short h)
{
	register unsigned short *pSourceOrigin;
	register unsigned short *pDestOrigin;
	register unsigned short *pSourceData;
	register unsigned short *pDestData;
	int iSourceStride = blend_width;
	int iDestStride = blend_width;
	int i;

	pSourceOrigin = &((unsigned short *) video_memory[FRAME_UI_BACKUP])[iSourceStride * y + x];
	pDestOrigin = &((unsigned short *) video_memory[FRAME_VIDEO_BACKUP])[iDestStride * y + x]; 

	do {
		i = w;

		pSourceData = pSourceOrigin;
		pDestData = pDestOrigin;

		while(i--) {
			if (*pSourceData != 0) {
				*pDestData = ((*pDestData & HALF_MASK_16) >> 1)	+ ((*pSourceData & HALF_MASK_16) >> 1);
			}

			pDestData++;
			pSourceData++;
		}

		pSourceOrigin += iSourceStride;
		pDestOrigin += iDestStride;

	} while(--h);
}
/**
 * @brief         Adjust blending with flip, since STMP37xx only provides single layer
 *                this function specialized with Q2 OSD scheme.
 * @date          2008.11.04
 */
static void osd_flip_blend(int osd_id, int vid_id, int dst_id)
{
	unsigned short *posd_base;
	unsigned short *pdst_base;
	unsigned short *pvid_base;

	unsigned short *posd;
	unsigned short *pdst;
	unsigned short *pvid;

	register unsigned long *plosd;
	register unsigned long *pldst;
	register unsigned long *plvid;
	
	unsigned long flip_pixel;
	unsigned long osd_start = 0;
	unsigned long osd_end = 0;

	int i, h, w;

	int osd_x, osd_y, osd_w, osd_h;

	/* Calculate flip area */
	/* Adjust flip and blending */
	for(i = 0; i < MAX_OSD_DATA; i++) {
		osd_end += osd_data[i].w * osd_data[i].h;

		if(osd_data[i].b.en == 1 && osd_ui_updated) {
			osd_x = osd_data[i].x;
			osd_y = osd_data[i].y;
			osd_w = osd_data[i].w;
			osd_h = osd_data[i].h;

			h = osd_h;
		
			posd_base = &((unsigned short *) video_memory[osd_id])[osd_start];
			pdst_base = &((unsigned short *) video_memory[dst_id])[blend_width * osd_y + blend_width - osd_x];
			pvid_base = pdst_base;

			if(i == 5) { /* For flip and overlay : 16bit operation */
				/* 2008.12.10: Clear HOLD icon from osd_backup { */
				w = (osd_data[0].w * osd_data[0].h) >> 1;
				posd = &((unsigned short *) video_memory[osd_id])[0];
				while(w--) {
					if(*(posd) == 0xF5E5) {
						*(posd) = 0x0000;
					}
					posd++;
				}
				/* 2008.12.10: Clear Key box icon from osd_backup } */
				while(h > 0) {
					posd = posd_base;
					pdst = pdst_base;
					pvid = pvid_base;

					w = osd_w;
					while(w > 0) {
						pdst--;
						pvid--;
						if(*posd != 0) {	
							*pdst = ((*pvid & HALF_MASK_16) >> 1) + ((*posd & HALF_MASK_16) >> 1);
						}
						posd++;
						w--;
					}
					posd_base += osd_w;
					pdst_base += LCD_WIDTH;
					pvid_base += LCD_WIDTH;
					h--;
				}
			} else {  /* For flip and blending : 32bit operation */
				if (i == 4) {
					if(osd_data_flag == 1) { /* 2008.11.18 */
						continue;
					}
					pvid_base = &((unsigned short *) video_memory[vid_id])[blend_width * osd_y + blend_width - osd_x];
				}
				while(h > 0) {
					posd = posd_base;
					pdst = pdst_base;
					pvid = pvid_base;
					
					w = osd_w;
					
					if((osd_x & 0x0001)) { /* For making 32bit align */
						pdst--;
						pvid--;
						*pdst = ((*pvid & HALF_MASK_16) >> 1)+ ((*posd & HALF_MASK_16) >> 1);
						posd++;
						w--;
					}
					
					plosd = (unsigned long *) posd;
					pldst = (unsigned long *) pdst;
					plvid = (unsigned long *) pvid;

					while(w > 1) { /* Adjust OSD flip for 32bit operation */
						flip_pixel = ((*plosd & 0x0000FFFF) << 16) | ((*plosd & 0xFFFF0000) >> 16);
						
						pldst--;
						plvid--;
						if(osd_data[4].b.en && (i == 3)) {
							if(osd_data_flag == 1) { /* 2008.11.18 */
								*pldst = ((*plvid & HALF_MASK_32) >> 1) + ((flip_pixel & HALF_MASK_32) >> 1);
							} else {
								if(((osd_x + osd_w - w) > osd_data[4].x) &&
								   ((osd_y + osd_h - h) > osd_data[4].y)) {
									break; /* Skip current line for avoiding overlaping */
								}
								*pldst = ((*plvid & HALF_MASK_32) >> 1) + ((flip_pixel & HALF_MASK_32) >> 1);
							}
						} else {
							*pldst = ((*plvid & HALF_MASK_32) >> 1) + ((flip_pixel & HALF_MASK_32) >> 1);
						}
						plosd++;
						w -= 2;
					}

					if(w == 1) { /* Last 16bit */
						posd = (unsigned short *) (plosd);
						pdst = (unsigned short *) (pldst);
						pvid = (unsigned short *) (plvid);

						*pdst = ((*(--pvid) & HALF_MASK_16) >> 1)+ ((*(--posd) & HALF_MASK_16) >> 1);
					}

					posd_base += osd_w;

					/* For 32bit align */
					if(!(osd_x & 0x0001) && (osd_w & 0x0001)) {
						posd_base++;
						osd_end++;
					} else if((osd_x & 0x0001) && (osd_w & 0x0001)) {
						posd_base++;
						osd_end++;
					}

					pdst_base += LCD_WIDTH;
					pvid_base += LCD_WIDTH;
					h--;
				}
			}
		}
		osd_start = osd_end;

		/* For 32bit align */
		if((i < (MAX_OSD_DATA - 1)) && (osd_data[i + 1].x & 0x0001)) {
			osd_start += (~osd_end & 0x00000001);
		}
	}
}
/**
 * @brief         Adjust half blending force.
 *                osd_flip_blend will replace this as blending function
 */
static void osd_half_blend(short x, short y, short w, short h)
{
        register unsigned short *pSourceOrigin;
	register unsigned short *pDestOrigin;
	register unsigned short *pSourceData;
	register unsigned short *pDestData;
	register unsigned long *plsrc, *pldst; /* For 32bit force blending */
	int iSourceStride = blend_width;
	int iDestStride = blend_width;
	int i;

	pSourceOrigin = &((unsigned short *) video_memory[FRAME_UI_BACKUP])[iSourceStride * y + x];
	pDestOrigin = &((unsigned short *) video_memory[FRAME_VIDEO_BACKUP])[iDestStride * y + x]; 

	do {
		i = w;
		pSourceData = pSourceOrigin;
		pDestData = pDestOrigin;

		if((x & 0x0001)) { /* For making 32bit align */
			*pDestData = ((*pDestData & HALF_MASK_16) >> 1)	+ ((*pSourceData & HALF_MASK_16) >> 1);
			pSourceData++;
			pDestData++;
			i--;
		}

		plsrc = (unsigned long *) pSourceData;
		pldst = (unsigned long *) pDestData;
		
		while(i > 1) {
			*pldst = ((*pldst & HALF_MASK_32) >> 1) + ((*plsrc & HALF_MASK_32) >> 1);
			plsrc++;
			pldst++;
			i -= 2;
		}

		if(i == 1) { /* Last 16bit */
			pSourceData = (unsigned short *) plsrc;
			pDestData = (unsigned short *) pldst;
			*pDestData = ((*pDestData & HALF_MASK_16) >> 1)	+ ((*pSourceData & HALF_MASK_16) >> 1);
		}

		pSourceOrigin += iSourceStride;
		pDestOrigin += iDestStride;

	} while(--h);
}
/* osd_flip_blend will replace this as blending function */
static void osd_blend(int x, int y, int w, int h, int alphaValue, int alphaColor, int chromaColor)
{
        register unsigned short *pSourceOrigin;
	register unsigned short *pDestOrigin;
	register unsigned short *pSourceData;
	register unsigned short *pDestData;
	int iSourceStride = LCD_WIDTH;
	int iDestStride = LCD_WIDTH;
	int i;
	
	pSourceOrigin = &((unsigned short *) video_memory[FRAME_UI_BACKUP])[iSourceStride * y + x];
	pDestOrigin = &((unsigned short *) video_memory[FRAME_VIDEO])[iDestStride * y + x];
	do {
		pSourceData = pSourceOrigin;
		pDestData = pDestOrigin;
		i = w;
		do {
			//do the blending
			if (*pSourceData != chromaColor) {
				//if (*pSourceData != alphaColor)	{
				if (*pSourceData != 0)	{
					*pDestData = *pSourceData;
				} else {
					uint16_t BgRComponent;
					uint16_t BgGComponent;
					uint16_t BgBComponent;

					#ifdef ALPHA_FIXED_128
					BgRComponent = ((*pDestData & 0xF800) >> 8) << 7;
					BgGComponent = ((*pDestData & 0x07E0) >> 3) << 7;
					BgBComponent = ((*pDestData & 0x001F) << 3) << 7;
					FgRComponent = ((*pSourceData & 0xF800) >> 8) << 7;
					FgGComponent = ((*pSourceData & 0x07E0) >> 3) << 7;
					FgBComponent = ((*pSourceData & 0x001F) << 3) << 7;
					#else
					BgRComponent = ((*pDestData & 0xF800) >> 8) * (256-alphaValue);
					BgGComponent = ((*pDestData & 0x07E0) >> 3) * (256-alphaValue);
					BgBComponent = ((*pDestData & 0x001F) << 3) * (256-alphaValue);
					FgRComponent = ((*pSourceData & 0xF800) >> 8) * (alphaValue);
					FgGComponent = ((*pSourceData & 0x07E0) >> 3) * (alphaValue);
					FgBComponent = ((*pSourceData & 0x001F) << 3) * (alphaValue);
					#endif
					FgRComponent = (FgRComponent + BgRComponent) >> 8;
					FgGComponent = (FgGComponent + BgGComponent) >> 8;
					FgBComponent = (FgBComponent + BgBComponent) >> 8;
					*pDestData   = ((FgBComponent & 0x00F8) >>3) |
						       ((FgGComponent & 0x00FC) << 3) |
					               ((FgRComponent & 0x00F8) << 8);
				}
			}
			pDestData++;
			pSourceData++;
		} while (--i);

		pDestOrigin += iDestStride;
		pSourceOrigin += iSourceStride;
	} while(--h);            
}
/* Only available for OSD scheme of STMP37xx. */
static void osd_memcpy (int dst_idx, int src_idx)
{
	unsigned short *src;
	unsigned short *dst;
	int i, h;
	unsigned long osd_start = 0;
	unsigned long osd_end = 0;

	for (i = 0; i < MAX_OSD_DATA; i++) { /* 2008.08.23: draw all enabled OSD area */
		osd_end += osd_data[i].w * osd_data[i].h;

		if(osd_data[i].b.up == 1) { /* 2008.08.23: add for indexing */
			osd_data[i].b.up = 0;
			if(i == 4) { /* 2008.11.18 */
				osd_data_flag = 0;
			}
			if(i == 3 && (osd_data[4].b.up == 1)) {
				/* Do not backup OSD, there is overlaped area between id3 and id4, but just increase osd position */
				h = osd_data[i].h;

				if(!(osd_data[i].x & 0x0001) && (osd_data[i].w & 0x0001)) {
					dst += h;
					osd_end += h;
				} else if((osd_data[i].x & 0x0001) && (osd_data[i].w & 0x0001)) {
					dst += h;
					osd_end += h;
				}
			} else {
				h = osd_data[i].h;
				src = (unsigned short *)video_memory[src_idx] + osd_data[i].x + osd_data[i].y * blend_width;
				dst = (unsigned short *)video_memory[dst_idx] + osd_start;
				
				while (h--) {
					memcpy(dst, src, osd_data[i].w * 2);
					src += blend_width;
					dst += osd_data[i].w;
					
					/* For 32bit align: 2008.11.04 */
					if(!(osd_data[i].x & 0x0001) && (osd_data[i].w & 0x0001)) {
						dst++;
						osd_end++;
					} else if((osd_data[i].x & 0x0001) && (osd_data[i].w & 0x0001)) {
						dst++;
						osd_end++;
					}
				}
			}
			osd_ui_updated = true;
		}
		osd_start = osd_end;

		/* For 32bit align: 2008.11.04 */
		if((i < (MAX_OSD_DATA - 1)) && osd_data[i + 1].x & 0x0001) {
			osd_start += (~osd_end & 0x00000001);
		}
	}
	/* osd_ui_updated = true; */
}
/** 
 * @brief       flip and backup OSD image 
 *              osd_flip_blend will replace this as flip function
 * @date        2008.08.28
 */
static void sw_flip(int index)
{
	register int i, j;
	register unsigned short *pSrc = (unsigned short *) video_memory[FRAME_UI];
	register unsigned short *pDst = (unsigned short *) video_memory[FRAME_UI_BACKUP];
	unsigned short sline[blend_width]; 
	
	int hmin = LCD_HEIGHT, hmax = 0;
	
	for(i = 0; i < MAX_OSD_DATA; i++) { /* Find box for updating */
		if(osd_data[i].b.up == 1) {
			osd_data[i].b.en = 0;
			if((osd_data[i].h + osd_data[i].y) > hmax) {
				hmax = osd_data[i].h + osd_data[i].y;
			} 
			if (osd_data[i].y < hmin) {
				hmin = osd_data[i].y;
			}
		}
	}

	for(j = hmin; j < hmax; j++){
		memcpy(&sline[0], pSrc + j * LCD_WIDTH, LCD_WIDTH * 2);

		for(i = 0; i < LCD_WIDTH; i++) {
			*(pDst + (j * LCD_WIDTH) + i) = sline[LCD_WIDTH - i - 1];
		}
		osd_ui_updated = true;
	}
}
/* 2008.08.28: for erasing OSD area on pause */
static void restore_frame_video(short dst_idx, short src_idx)
{
	unsigned short *src;
	unsigned short *dst;
	int i, h;
	
	for (i = 0; i < MAX_OSD_DATA; i++) {
		if(osd_data_backup[i].b.en == 1) {
			h = osd_data_backup[i].h;
			src = (unsigned short *)video_memory[src_idx] + osd_data_backup[i].x + osd_data_backup[i].y * blend_width;
			dst = (unsigned short *)video_memory[dst_idx] + osd_data_backup[i].x + osd_data_backup[i].y * blend_width;
			while (h--) {
				memcpy(dst, src, osd_data[i].w * 2);
				src += blend_width;
				dst += blend_width;
			}
		}
	}
}

static void video_copy(int target, int source)
{
	memcpy(video_memory[target], video_memory[source], LCD_WIDTH * LCD_HEIGHT * 2);
}
/* 2009.03.11: for flipping video frame manually { */
fb_mirror mirror_info = {.enable = 0};
unsigned int is_video_mirrored;
static void video_copy_flip(int target, int source)
{
	register int h;
	register unsigned short *pSrc = &((unsigned short *) video_memory[source])[0];
	register unsigned short *pDst = &((unsigned short *) video_memory[target])[LCD_WIDTH * LCD_HEIGHT - LCD_WIDTH];

	h = LCD_HEIGHT;
	
	while(h) {
		memcpy(pDst, pSrc, LCD_WIDTH * 2);
		pSrc += LCD_WIDTH;
		pDst -= LCD_WIDTH;
		h--;
	}
}
/* 2009.03.11: for flipping video frame manually } */

static int draw_osd (int node)
{
#ifdef CAL_OSD_PERF
	unsigned long useconds;

	useconds = HW_DIGCTL_MICROSECONDS_RD(); 
#endif
	/* VIDEO update */
	if (node == FRAME_VIDEO) {
		/* make Video backup */
		/* 2009.03.11: for flipping video frame manually { */
		if(mirror_info.enable) {
			video_copy_flip(FRAME_VIDEO_BACKUP, FRAME_VIDEO);
			is_video_mirrored = 1;
		} else {
			video_copy(FRAME_VIDEO_BACKUP, FRAME_VIDEO);
			is_video_mirrored = 0;
		}
		/* 2009.03.11: for flipping video frame manually } */
		osd_video_backuped = 1;
	} else { /* UI update */
		if (osd_ui_directupdate) {
			if (!osd_video_backuped) {
				/* backup video frame */
				/* 2009.03.11: for flipping video frame manually { */
				if(is_video_mirrored == 1) {
					video_copy_flip(FRAME_VIDEO_BACKUP, FRAME_VIDEO);
				} else {
					video_copy(FRAME_VIDEO_BACKUP, FRAME_VIDEO);
				}	
				/* 2009.03.11: for flipping video frame manually } */
				osd_video_backuped = 1;
			}
			/* refresh video frame */
			/* 2008.08.28: for erasing OSD area on pause */
			/* 2009.03.11: for flipping video frame manually { */
			if(is_video_mirrored == 1) {
				video_copy_flip(FRAME_VIDEO_BACKUP, FRAME_VIDEO);
			} else {
				video_copy(FRAME_VIDEO_BACKUP, FRAME_VIDEO);
			}	
			/* 2009.03.11: for flipping video frame manually } */
		}

		/* make UI backup */
		/* Didn't need osd_memcpy() */
		osd_memcpy(FRAME_UI_BACKUP, FRAME_UI);
		/* 2008.11.04: no sw_flip() needs anymore */
		/* sw_flip(FRAME_UI_BACKUP); */

		if (!osd_ui_directupdate) {
			return -1;
		}
	}

	/* backup OSD area */
	/* 2008.08.28: for erasing OSD area on pause */
	/* memcpy(osd_data_backup, osd_data,  MAX_OSD_DATA * sizeof(osd_data_t)); /\* Pair with restore_frame_video() *\/ */

	/* osd blending */
	osd_flip_blend(FRAME_UI_BACKUP, FRAME_VIDEO, FRAME_VIDEO_BACKUP);
#ifdef CAL_OSD_PERF
	useconds = get_elasped(useconds);
	printk("%s elapsed time for blending = %d \n", __func__, useconds);
#endif
	return FRAME_VIDEO_BACKUP;
}
 
static void lcd_on_off(unsigned char on) 
{
	if (on) {
		lcd_on();
	} else {
		lcd_off();
	}
}
static int last_node; /* 2008.12.01: Now FB will send previous screen data to LCD */
static int stmp37xxfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = -ENOTTY;
	
	switch(cmd) {
	case FBIO_STMP36XX_FB_FLUSH_ENABLE:
		periodic_refresh = !arg;
		ret = 0;
		break;
		
        case STMP37XXFB_IOCT_PERIODIC_ON:
		periodic_refresh = 1;
		ret = 0;
		break;
		
        case STMP37XXFB_IOCT_PERIODIC_OFF:
		periodic_refresh = 0;
		ret = 0;
		break;
		
	case STMP37XXFB_IOCT_OSD_MODE:
		osd_mode = arg;
		/* by jinho.lim */
		
		osd_ui_updated = false;

		if(osd_mode == false) {
			memset(&osd_data[0], 0, MAX_OSD_DATA * sizeof(osd_data_t));

			if (s_doing_refresh) {
				wait_event_interruptible(wait_q, s_doing_refresh == 0);
			}
			lcd_panel_flip(false);
			mirror_info.enable = 0;
			is_video_mirrored = 0;
			//lcd_panel_rotation(0);
		}
		ret = 0;
		break;
		
	case STMP37XXFB_IOCT_OSD_UI_DIRECTUPDATE:
		osd_ui_directupdate = arg;

		if (!osd_mode) {
			/* need video backup data for UI directupdate */
			osd_video_backuped = 0;
		}
		ret = 0;
		break;
	case STMP37XXFB_IOCT_OSD_DATA:
		{
			osd_data_t *temp_osd_data = (osd_data_t *) arg;
		
			/* reset osd_data if NULL */
			if (arg == 0) {
				memset(&osd_data[0], 0, MAX_OSD_DATA * sizeof(osd_data_t));
			} else {
				if((temp_osd_data->b.id == 4) && (temp_osd_data->b.en == 1)) { /* 2008.11.18 */
					if (osd_data[4].b.en == 0){
						osd_data_flag = 1;
					} else {
						osd_data_flag = 0;
					}
				}

				if(temp_osd_data->b.id == 1 && temp_osd_data->b.en == 0) {
					if(osd_data[1].b.en == 1) {
						osd_ui_updated = false;
					}
				}
				
				if (copy_from_user(&osd_data[temp_osd_data->b.id], (void*) arg, sizeof(osd_data_t))) {
					printk("stmp37xxfb: osd_data could not copy_from_user\n");
					return -EINVAL;
				}
				osd_flip_x[temp_osd_data->b.id] = /* LCD_WIDTH */ blend_width - temp_osd_data->x - temp_osd_data->w;
#if 0
				{
					unsigned short alphaColor = osd_data[osd_data_cnt].alphaColor;
					// alpht value is fixed to 128
					FgRComponent = ((alphaColor & 0xF800) >> 8) << 7;
					FgGComponent = ((alphaColor & 0x07E0) >> 3) << 7;
					FgBComponent = ((alphaColor & 0x001F) << 3) << 7;
				}
#endif
			}
		}

		ret = 0;
		break;
	case FBIO_STMP36XX_FB_FLUSH:
		/*
		 * fb_area is not needed
		 */
        case STMP37XXFB_IOCT_REFRESH:
		if(!lcd_onoff) {
			return -EPERM;
		}
		
		/* Refresh is not allowed if we are updating periodicly */
 		if (periodic_refresh) {
			return -EBUSY;
		}
		if (s_doing_refresh) {
			wait_event_interruptible(wait_q, s_doing_refresh == 0);
		}
		
		/* Protect against multiple calls */
		if (down_interruptible(&refresh_mutex)) {
			return -ERESTART;
		}
		last_node = info->node;
		
		/* disable ui directupdate automatically once video update is initiated */
		if (last_node == FRAME_VIDEO || osd_mode) { /* 2008.8.28 osd_mode: add for fliping while wakeup from sleep with pause */
			if (last_node == FRAME_VIDEO) {
				osd_ui_directupdate = 0;
			}
			/* 2008.08.25: add for adjust flip function when it awake from sleep mode */ 
			if (s_doing_refresh) {
				wait_event_interruptible(wait_q, s_doing_refresh == 0);
			}
			lcd_panel_flip(true);
			/* lcd_panel_rotation(270); */
		}
		
		if (osd_mode) {
			last_node = draw_osd(last_node);
		}
		/* draw_osd may return minus value. 
		   In this case, do not update display if node is minus. Video update will reflect the ui changes.
		*/
		if (last_node >= 0) {
			dma_enable(0, last_node);
		}
		up(&refresh_mutex);
		ret = 0;
		break;
	case FBIO_DATA_TRANSFER:
		/* Initialize LCD */
		lcd_on_off(1);
		/* But do not turn on backlight */
		need_bl_init = 0;
		break;
	case STMP37XXFB_IOCS_PERF:
		{
			unsigned vals[6];
			vals[0] = HW_DIGCTL_L1_AHB_ACTIVE_CYCLES_RD();
			vals[1] = HW_DIGCTL_L1_AHB_DATA_STALLED_RD();
			vals[2] = HW_DIGCTL_L1_AHB_DATA_CYCLES_RD();
			vals[3] = HW_DIGCTL_L2_AHB_ACTIVE_CYCLES_RD();
			vals[4] = HW_DIGCTL_L2_AHB_DATA_STALLED_RD();
			vals[5] = HW_DIGCTL_L2_AHB_DATA_CYCLES_RD();
			return copy_to_user((void *) arg, vals, sizeof(vals));
		}
		break;
	case FBIO_STMP36XX_FB_BACKLIGHT:
	case FBIO_FB_BACKLIGHT:
		if(lcd_onoff) {
			lcd_bl_control(arg);
		} else {
			lcd_bl_setlevel(arg);
		}
		break;
	case FBIO_FB_BACKLIGHT_OFF:
		lcd_bl_on_off(arg);
		break;
	case FBIO_LCD_ON_OFF:
		lcd_on_off(arg);
		break;
		/*
		 * the value is differnt from mendel kernel
		 * don't care yet, do something Samson
		 */
	case STMP37XXFB_IOCS_BACKLIGHT:
		if(lcd_onoff) {
			lcd_bl_control(arg);
		} else {
			lcd_bl_setlevel(arg);
		}
		break;
        case FBIO_STMP36XX_FB_BACKLIGHT_MODE:
		/*
		 * Not Supported
		 */
		ret = 0;	
		break;
        case FBIO_STMP36XX_FB_LCDIF:
		if (arg == 1) {
			lcd_on();
			ret = 0;
		} else if (arg == 0) {
			lcd_off();
			ret = 0;
		} else {
			ret = -1;
		}
		break;
		/* 2009.03.11: for flipping video frame manually { */
	case FBIO_FB_MIRROR_VIDEO:
		if (copy_from_user(&mirror_info, (void *) arg, sizeof(fb_mirror))) {
			printk("stmp37xxfb: mirror information could not copy_from_user\n");
			return -EINVAL;
		}
		break;
		/* 2009.03.11: for flipping video frame manually } */
        default:
		printk("stmp37xxfb: Unrecognised ioctl 0x%08x\n", cmd);
		break;
	}
	
	return ret;
}

static int stmp37xxfb_open (struct fb_info *info, int user)
{
	/* clear fb1 screen (video player) when opening */

	if (info->node == 1) {
		/* 2008.08.23: add for indexing { */
		memset(&osd_data[0], 0, MAX_OSD_DATA * sizeof(osd_data_t));
		/* 2008.08.23: add for indexing } */

		if (down_interruptible(&refresh_mutex))
			return -ERESTART;

		if (s_doing_refresh) {
			wait_event_interruptible(wait_q, s_doing_refresh == 0);
		}
		lcd_panel_flip(true);
		/* temporary add: remove before releaseing */
		/* lcd_panel_rotation(270); */

		up(&refresh_mutex);
		memset(video_memory[info->node], 0, videomemorysize);
	}

	return 0;
}

static int stmp37xxfb_release (struct fb_info *info, int user)
{

	if (info->node == 1) {
		if (down_interruptible(&refresh_mutex))
			return -ERESTART;
		
		if (s_doing_refresh) {
			wait_event_interruptible(wait_q, s_doing_refresh == 0);
		}
		lcd_panel_flip(false);
		/* lcd_panel_rotation(0); */
		up(&refresh_mutex);

		/* 2008.08.23: add for indexing { */
		memset(&osd_data[0], 0, MAX_OSD_DATA * sizeof(osd_data_t));
		/* 2008.08.23: add for indexing } */
	}

	return 0;
}

static int stmp37xxfb_mmap(struct fb_info *info,
		    struct vm_area_struct *vma)
{
	unsigned long off;
	unsigned long start;
	u32 len;
	
/* 	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT)) */
/* 		return -EINVAL; */
	off = vma->vm_pgoff << PAGE_SHIFT;

	/* lock_kernel(); */

	/* frame buffer memory */
	start = video_phys[info->node];
	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
	
	/* unlock_kernel(); */
	start &= PAGE_MASK;
	
	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_RESERVED;
	
	if (remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
	    vma->vm_end - vma->vm_start, vma->vm_page_prot))
	{
		return -EAGAIN;
	}
	return 0;
}

#ifndef MODULE
static int __init stmp37xxfb_setup(char *options)
{
	char *this_opt;

	stmp37xxfb_enable = 1;

	if (!options || !*options)
		return 1;

	while ((this_opt = strsep(&options, ",")) != NULL) {
		if (!*this_opt)
			continue;
		if (!strncmp(this_opt, "disable", 7))
			stmp37xxfb_enable = 0;
	}
	return 1;
}
#endif  /*  MODULE  */

/*
 * For Samsung App Interface
 */
static int lfb_read_proc(char *buf, char **start, off_t offset,
	int count, int *eof, void *data)
{
	int len = 0; 
	
	*start = buf; 
	
	if(offset == 0)
	{ 
		len += sprintf(buf + len, "\n<< [LCD Control] Current Status >>\n");
		if(lcd_onoff)
			len += sprintf(buf + len, " - backlight : %d\n", lcd_bl_getlevel());
		else
			len += sprintf(buf + len, " - backlight : 0\n");
		if(!periodic_refresh)
			len += sprintf(buf + len, " - refresh : manual\n");
		else
			len += sprintf(buf + len, " - refresh : auto\n");
		if(lcd_onoff)
			len += sprintf(buf + len, " - lcdif : on\n");
		else
			len += sprintf(buf + len, " - lcdif : off\n");

		*start = buf + offset; 
		offset = 0; 
	}

	*eof = 1; 

	return len; 
}

#define isdigit(c) (c >= '0' && c <= '9')
__inline static int atoi( char *s)
{
	int i = 0;
	while (isdigit(*s))
		i = i * 10 + *(s++) - '0';
	return i;
}
void write_command_16(unsigned short cmd);
void write_register(unsigned reg, unsigned value);
static ssize_t lfb_write_proc(struct file * file, const char * buf, 
	unsigned long count, void *data)
{
	char cmd0[64], cmd1[64]; 
	int value;

	sscanf(buf, "%s %s", cmd0, cmd1);
	
	if(!strcmp(cmd0, "backlight")) {
		value = atoi(cmd1);
		lcd_bl_control(value);
		printk("%s %d\n", cmd0, lcd_bl_getlevel());
	} else if (!strcmp(cmd0, "refresh")) {
		if(!strcmp(cmd1, "1")) {
			periodic_refresh = 0;
			printk("%s manual\n", cmd0);
		} else if(!strcmp(cmd1, "0")) {
			periodic_refresh = 1;
			printk("%s auto\n", cmd0);
		}
	}
	else if (!strcmp(cmd0, "lcdif")) {
		if(!strcmp(cmd1, "off")) {
			if (s_doing_refresh) {
				wait_event_interruptible(wait_q, s_doing_refresh == 0);
			}
			/* Protect against multiple calls */
			if (down_interruptible(&refresh_mutex)) {
				return -ERESTART;
			}
			lcd_off();
			up(&refresh_mutex);
		} else if(!strcmp(cmd1, "on")) {
			lcd_on();
		}

		printk("%s %s\n", cmd0, cmd1);
	} else if (!strcmp(cmd0, "rotate")) {
		if(!strcmp(cmd1, "0")){
			lcd_panel_rotation(0);
		}
		if(!strcmp(cmd1, "90")){
			lcd_panel_rotation(90);
		}
		if(!strcmp(cmd1, "180")){
			lcd_panel_rotation(180);
		}
		if(!strcmp(cmd1, "270")){
			lcd_panel_rotation(270);
		}
	/* By leeth, for new LCD at 20090502 */
	} else if (!strcmp(cmd0, "flip")) {
		if (!strcmp(cmd1, "on")) {
			lcd_panel_flip(1);
		}
		if (!strcmp(cmd1, "off")) {
			lcd_panel_flip(0);
		}
	} else if (!strcmp(cmd0, "320")) {
		blend_width = 320;
		blend_height = 240;
	} else if (!strcmp(cmd0, "240")) {
		blend_width = 240;
		blend_height = 320;
	} else if (!strcmp(cmd0, "def_window")) {
		write_register(0x0020, 0x0000);
		write_register(0x0021, 0x0000);
		write_register(0x0050, 0x0000);
		write_register(0x0051, 0x00EF);
		write_register(0x0052, 0x0000);
		write_register(0x0053, 0x013F);

		write_command_16(0x0022);
	}
	return count; 
}

    /*
     *  Initialisation
     */

static void stmp37xxfb_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero. */
}

static int __init stmp37xxfb_probe_node(struct platform_device *dev, int node)
{
	struct fb_info *info;
	int retval = -ENOMEM;

        /* printk("stmp37xxfb_probe()\n"); */

#if 0
	video_memory = ioremap_nocache(0x41c00000, 4*1024*1024);
	video_phys = 0x41c00000;
#else
	video_memory[node] = dma_alloc_writecombine(&dev->dev, VIDEOMEMSIZE,
			&video_phys[node], GFP_KERNEL);
#endif
	if (!video_memory[node])
		goto err;
        
	/* printk("video_memory = 0x%08x; video_phys = 0x%08x\n", (unsigned int)video_memory[node], video_phys[node]); */
 
	if (node == 1) {
		video_memory[2] = dma_alloc_writecombine(&dev->dev, VIDEOMEMSIZE + 240 * 80, /* 2008.11.11: Increase size of UI backup */
			&video_phys[2], GFP_KERNEL);
		video_memory[3] = dma_alloc_writecombine(&dev->dev, VIDEOMEMSIZE,
			&video_phys[3], GFP_KERNEL);
	}

	stmp37xxfb_fix.smem_start = video_phys[node];

	/*
	 * STMP37XXFB must clear memory to prevent kernel info
	 * leakage into userspace
	 * VGA-based drivers MUST NOT clear memory if
	 * they want to be able to take over vgacon
	 */
	if (node == 0)
		memset(video_memory[node], 0, videomemorysize);
        
	info = framebuffer_alloc(sizeof(u32) * 256, &dev->dev);
	if (!info)
		goto err;

	info->screen_base = (char __iomem *)video_memory[node];
	info->fbops = &stmp37xxfb_ops;

	retval = fb_find_mode(&info->var, info, NULL, NULL, 0, NULL, 8);

	if (!retval || (retval == 4))
		info->var = stmp37xxfb_default;
	info->fix = stmp37xxfb_fix;
	info->pseudo_palette = info->par;
	info->par = NULL;
	info->flags = FBINFO_FLAG_DEFAULT;

	retval = fb_alloc_cmap(&info->cmap, 256, 0);
	if (retval < 0)
		goto err1;

	retval = register_framebuffer(info);
	if (retval < 0)
		goto err2;
	platform_set_drvdata(dev, info);

/* 	printk(KERN_INFO */
/* 	       "fb%d: Virtual frame buffer device, using %ldK of video memory\n", */
/* 	       info->node, videomemorysize >> 10); */

	return 0;
err2:
	fb_dealloc_cmap(&info->cmap);
err1:
	framebuffer_release(info);
	dma_free_writecombine(&dev->dev,
			VIDEOMEMSIZE, video_memory[node], video_phys[node]);
err:
	return retval;
}

static int __init stmp37xxfb_probe(struct platform_device *dev)
{
	struct proc_dir_entry *proc_entry;

	proc_entry = create_proc_entry("lfbctrl", S_IWUSR | S_IRUGO, NULL);
	proc_entry->read_proc = lfb_read_proc;
	proc_entry->write_proc = lfb_write_proc;
	proc_entry->data = NULL; 	
	
	stmp37xxfb_probe_node(dev, 0);
	return stmp37xxfb_probe_node(dev, 1);
}

static int stmp37xxfb_remove(struct platform_device *dev)
{
	struct fb_info *info = platform_get_drvdata(dev);

	remove_proc_entry("lfbctrl", NULL);

	if (info) {
		unregister_framebuffer(info);
		framebuffer_release(info);

	}
	return 0;
}

static struct platform_driver stmp37xxfb_driver = {
	.probe	= stmp37xxfb_probe,
	.remove = stmp37xxfb_remove,
	.driver = {
		.name	= "stmp37xxfb",
	},
};

#ifdef FB_DEBUG
static void dump_registers(const char *id)
{
	/* Dump registers */
	printk("----- %s:\n", id);
	printk("HW_APBH_CTRL0 = 0x%08x3\n", HW_APBH_CTRL0_RD());
	printk("HW_LCDIF_CTRL = 0x%08x\n", HW_LCDIF_CTRL_RD());
	printk("HW_LCDIF_CTRL1 = 0x%08x\n", HW_LCDIF_CTRL1_RD());
	printk("HW_APBH_CHn_CMD = 0x%08x\n", HW_APBH_CHn_CMD_RD(0));
	printk("HW_APBH_CHn_NXTCMDAR = 0x%08x\n", HW_APBH_CHn_NXTCMDAR_RD(0));
	printk("HW_APBH_CHn_SEMA = 0x%08x\n", HW_APBH_CHn_SEMA_RD(0));
	printk("HW_APBH_CHn_DEBUG1 = 0x%08x\n", HW_APBH_CHn_DEBUG1_RD(0));
}
static void dump_descriptor(const char *id, dma_des_t *dd)
{
/* 	printk("Descriptor %s:\n", id); */
/* 	printk("next   : 0x%08x\n", dd->next); */
/* 	printk("cmd    : 0x%08x\n", dd->cmd); */
/* 	printk("buf_ptr: 0x%08x\n", dd->buf_ptr); */
/* 	printk("pio    : 0x%08x\n", dd->pio); */
}
#else
static inline void dump_registers(const char *id)
{
}
static inline void dump_desscriptor(const char *id, dma_des_t dd)
{
}
#endif

void write_command_16(unsigned short cmd)
{
	const hw_lcdif_ctrl_t lcd_ctrl = {
		.B = {
			.COUNT = 1,
			.WORD_LENGTH = 0,
			.RUN = 1
		}
	};
	
	/* PIO */
	setup_dma_descriptor.command->pio_words[0] = lcd_ctrl.U;
	/* Transfer count */
	setup_dma_descriptor.command->cmd &= ~BM_APBH_CHn_CMD_XFER_COUNT;
	setup_dma_descriptor.command->cmd |= BF_APBH_CHn_CMD_XFER_COUNT(2);
	/* printk("write_command(0x%02x)\n", cmd); */
	
	/* Copy commandc */
	memcpy(setup_memory, &cmd, 2);
	
	/* Go */
	stmp37xx_dma_go(DMA_LCDIF, &setup_dma_descriptor, 1);
	
	/* dump_registers("write_command() waiting"); */
	
	/* Wait for it to finish */
	while (stmp37xx_dma_running(DMA_LCDIF))
		;
}

void write_data_16(unsigned short data)
{
	const hw_lcdif_ctrl_t lcd_ctrl = {
		.B = {
			.COUNT = 1,
			.RUN = 1,
			.WORD_LENGTH = 0,
			.DATA_SELECT = 1
		}
	};
	/* PIO */
	setup_dma_descriptor.command->pio_words[0] = lcd_ctrl.U;
	/* Transfer count */
	setup_dma_descriptor.command->cmd &= ~BM_APBH_CHn_CMD_XFER_COUNT;
	setup_dma_descriptor.command->cmd |= BF_APBH_CHn_CMD_XFER_COUNT(2);
	/* printk("write_data(0x%02x)\n", data); */
	/* Copy command */
	memcpy(setup_memory, &data, 2);
	/* Go */
	stmp37xx_dma_go(DMA_LCDIF, &setup_dma_descriptor, 1);
	
	/* Wait for it to finish */
	while (stmp37xx_dma_running(DMA_LCDIF))
		;/* dump_registers("write_data() waiting"); */
}

void write_register(unsigned reg, unsigned value)
{
	write_command_16(reg);
	write_data_16(value);
}

static void dma_enable(unsigned refresh_phys, int node)
{
	unsigned i, phys;
	/* printk("dma_enable()\n"); */

	BUG_ON(s_doing_refresh);

	/* If we have been given a buffer to use, use it. Otherwise use the buffer
	we allocated on init */
	if (refresh_phys) {
		phys = refresh_phys; 
	}
	else
		phys = video_phys[node];

	/* Modify the chain addresses */
	for (i = 0; i < dma_chain_info_pos; ++i) {
		*dma_chain_info[i].dma_addr_p = phys + dma_chain_info[i].offset;
#ifdef USE_OLD_DMA
		arm926_dma_clean_range((unsigned) dma_chain_info[i].dma_addr_p,
				       ((unsigned) dma_chain_info[i].dma_addr_p) + 4);
#endif
	}

#ifdef USE_WSYNC_MODE
	init_MUTEX_LOCKED(&wsync_mutex);
	HW_LCDIF_CTRL1_SET(BM_LCDIF_CTRL1_VSYNC_EDGE_IRQ_EN);
	if (down_interruptible(&wsync_mutex))
		return;
#else
	stmp37xx_dma_go(DMA_LCDIF, &video_dma_descriptor[0], 1);
#endif

	s_doing_refresh = 1;
}

static void dma_chain_init(void)
{
	unsigned i, phys, bytes_left;
        
	hw_lcdif_ctrl_t lcd_ctrl = {
		.B = {
			.DATA_SELECT = 1,
			.RUN = 1,
#ifdef USE_VSYNC_MODE
			.VSYNC_MODE = 1,
			.WAIT_FOR_VSYNC_EDGE = 1
#endif
		}
	};

	/* Chain for video */
	bytes_left = VIDEOMEMSIZE;
	phys = video_phys[0];

	for(i = 0; bytes_left > 0; ++i) {
		unsigned this_chain = bytes_left < 65536U ? bytes_left : 65536U;
		/* Count of 0 in the DMA word means 65536 */
		unsigned xfer_count = this_chain & 65535;
		stmp37xx_dma_allocate_command(&dma_user, &video_dma_descriptor[i]);
		if(i != 0) {
			/* Chain previous command to this one */
			video_dma_descriptor[i - 1].command->next = video_dma_descriptor[i].handle;
			/* Enable DMA chaining on previous */
			/* Disable IRQ and semaphore on previous */
			video_dma_descriptor[i - 1].command->cmd |= BF_APBH_CHn_CMD_CHAIN(1);
			video_dma_descriptor[i - 1].command->cmd &= ~(BM_APBH_CHn_CMD_IRQONCMPLT | BM_APBH_CHn_CMD_SEMAPHORE);
#ifdef USE_VSYNC_MODE
			lcd_ctrl.B.WAIT_FOR_VSYNC_EDGE = 0;
#endif
		}
		video_dma_descriptor[i].command->cmd =
							BF_APBH_CHn_CMD_XFER_COUNT(xfer_count) |
							BF_APBH_CHn_CMD_CMDWORDS(1) |
							BF_APBH_CHn_CMD_SEMAPHORE(1) |
							BF_APBH_CHn_CMD_IRQONCMPLT(1) |
							BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
							BV_FLD(APBH_CHn_CMD,COMMAND, DMA_READ);

		lcd_ctrl.B.COUNT = this_chain / 2;
		video_dma_descriptor[i].command->pio_words[0] = lcd_ctrl.U;
		video_dma_descriptor[i].command->buf_ptr = phys;
		dma_chain_info[dma_chain_info_pos].dma_addr_p = &video_dma_descriptor[i].command->buf_ptr;
		dma_chain_info[dma_chain_info_pos].offset = phys - video_phys[0];
		++dma_chain_info_pos;
		phys += this_chain;
		bytes_left -= this_chain;
	}
	/* printk("Used %u DMA chains to cover %u bytes\n", i, VIDEOMEMSIZE); */
}

static void dma_init(void)
{
	stmp37xx_dma_user_init(&stmp37xxfb_device.dev, &dma_user);
	
	dma_chain_init();
	
	/* Allocate memory for command/data */
	setup_memory = dma_alloc_coherent(&stmp37xxfb_device.dev,
					  sizeof(char)*2,
					  &setup_phys,
					  GFP_DMA);
	
	/* Chain for setup */
	stmp37xx_dma_allocate_command(&dma_user, &setup_dma_descriptor);
	
	setup_dma_descriptor.command->next = (reg32_t)setup_dma_descriptor.handle;
	setup_dma_descriptor.command->cmd = BF_APBH_CHn_CMD_XFER_COUNT(1) |
					    BF_APBH_CHn_CMD_CMDWORDS(1) |
					    BF_APBH_CHn_CMD_SEMAPHORE(1) |
					    BF_APBH_CHn_CMD_WAIT4ENDCMD(1) |
					    BV_FLD(APBH_CHn_CMD,COMMAND, DMA_READ);
	setup_dma_descriptor.command->buf_ptr = (reg32_t)setup_phys;

#ifdef USE_OLD_DMA
        /* Reset channel */
        stmp37xx_dma_reset_channel(DMA_LCDIF);
        /* Enable interrupt */
        stmp37xx_dma_clear_interrupt(DMA_LCDIF);
        stmp37xx_dma_enable_interrupt(DMA_LCDIF);
#endif
}

static void periodic_function(unsigned long data)
{
	/* printk("periodic_function()\n"); */
    	del_timer(&refresh_timer);

	if (periodic_refresh) {
		dma_enable(0,0);
	}
}

static void __init init_timings(unsigned ns_cycle, unsigned xbus_div)
{
#ifdef USE_OLD_CLK
	/* use ref_xtal for 24MHz PIXCLK */
	/* or use ref_pix for other PIXCLK ragne */
	if (ns_cycle == 82) {
		
		HW_CLKCTRL_PIX_WR((HW_CLKCTRL_PIX_RD() & ~BM_CLKCTRL_PIX_DIV) | 1);
	}
	else {
/* 		 On 3700, we can get most timings exact by modifying ref_pix */
/* 		 and the divider, but keeping the phase timings at 1 (2 */
/* 		 phases per cycle). */
		
/* 		 ref_pix can be between 480e6*18/35=246.9MHz and 480e6*18/18=480.0MHz, */
/* 		 which is between 18/(18*480e6)=2.084ns and 35/(18*480e6)=4.050ns. */
		
/* 		 ns_cycle >= 2*18e3/(18*480) = 25/6 */
/* 		 ns_cycle <= 2*35e3/(18*480) = 875/108 */
		
/* 		 Multiply the ns_cycle by 'div' to lengthen it until it fits the */
/* 		 bounds. This is the divider we'll use after ref_pix. */
		
/* 		 6 * ns_cycle >= 25 * div */
/* 		 108 * ns_cycle <= 875 * div */
		unsigned div;
		unsigned lowest_result = (unsigned) -1;
		unsigned lowest_div = 0, lowest_fracdiv = 0;
		for(div = 1; div < 256; ++div) {
			unsigned fracdiv;
			unsigned ps_result;
			bool lower_bound = 6 * ns_cycle >= 25 * div;
			bool upper_bound = 108 * ns_cycle <= 875 * div;
			if(!lower_bound)
				break;
			if(!upper_bound)
				continue;
			/* Found a matching div. */
			/* Calculate fractional divider needed, rounded up. */
			fracdiv = ((480*18/2) * ns_cycle + 1000*div - 1) / (1000*div);
			BUG_ON(fracdiv < 18 || fracdiv > 35);
			/* Calculate the actual cycle time this results in */
			ps_result = 6250 * div * fracdiv / 27;
			/* Use the fastest result that doesn't break ns_cycle */
			if(ps_result <= lowest_result) {
				lowest_result = ps_result;
				lowest_div = div;
				lowest_fracdiv = fracdiv;
			}
		}
		/* Safety net. */
		BUG_ON(div >= 256);
		BUG_ON(lowest_result == (unsigned) -1);
/* 		printk("Programming PFD=%u,DIV=%u ref_pix=%uMHz " */
/* 			   "PIXCLK=%uMHz cycle=%u.%03uns\n", */
/* 			   lowest_fracdiv, lowest_div, */
/* 			   480*18/lowest_fracdiv, 480*18/lowest_fracdiv/lowest_div, */
/* 			   lowest_result / 1000, lowest_result % 1000); */
		/* Program ref_pix phase fractional divider */
		HW_CLKCTRL_FRAC_WR((HW_CLKCTRL_FRAC_RD() & ~BM_CLKCTRL_FRAC_PIXFRAC) |
				   BF_CLKCTRL_FRAC_PIXFRAC(lowest_fracdiv));
		/* Ungate PFD */
		HW_CLKCTRL_FRAC_CLR(BM_CLKCTRL_FRAC_CLKGATEPIX);
		/* Program pix divider */
		HW_CLKCTRL_PIX_WR((HW_CLKCTRL_PIX_RD() & ~BM_CLKCTRL_PIX_DIV) |
				  BF_CLKCTRL_PIX_DIV(lowest_div));
		/* Wait for divider update */
		while(HW_CLKCTRL_PIX.B.BUSY)
			;
		/* Switch to ref_pix source */
		HW_CLKCTRL_CLKSEQ_CLR(BM_CLKCTRL_CLKSEQ_BYPASS_PIX);
	}
#else
	struct clk *ref_xtal, *clk_pix;
	unsigned long long ll = 1000000000; /* 1 sec = 1000000000 ns */
	int ret;
	long round;

	do_div(ll, ns_cycle); /* ns -> Hz */

	ref_xtal = clk_get(NULL, "ref_xtal");
	clk_pix = clk_get(NULL, "clk_pix");

	ret = clk_change_parent(clk_pix, ref_xtal);
	if (ret < 0) {
		printk("%s(): clk_change_parent() fail(%d)\n", __func__, ret);
	}

	/* clk_pix: must be enabled before change rate */
	ret = clk_enable(clk_pix);
	if (ret < 0) {
		printk("%s(): clk_enable() fail(%d)\n", __func__, ret);
	}

	round = clk_round_rate(clk_pix, ll);
	if (round < 0) {
		printk("%s(): clk_round_rate() fail(%ld)\n", __func__, round);
	}

	printk("%s(): REF_PIX:%lu, CLK_PIX:%lu\n", __func__,
		clk_get_rate(ref_xtal), clk_get_rate(clk_pix));
#endif /* !USE_OLD_CLK */

	BW_LCDIF_TIMING_DATA_SETUP(LCD_DATA_SETUP); 
	BW_LCDIF_TIMING_DATA_HOLD(LCD_DATA_HOLD);
	BW_LCDIF_TIMING_CMD_SETUP(LCD_CMD_SETUP);
	BW_LCDIF_TIMING_CMD_HOLD(LCD_CMD_HOLD);
}

static void init_lcd_3700(void)
{
#ifdef USE_OLD_CLK
	HW_CLKCTRL_PIX_WR(0x00000001);
#endif

	/* Setup LCD PinMux */
	/* Assign LCD0-7 or LCD0-15 to LCDIF depending on word size */
	HW_PINCTRL_MUXSEL2_WR(0);
	/* Control lines to LCDIF */
	HW_PINCTRL_MUXSEL3_CLR(0xfff);
	/* connect LCD_BUSY to VSYNC */
	HW_PINCTRL_MUXSEL3_SET(0x400);

	/* Take controller out of reset */
	HW_LCDIF_CTRL_CLR(BM_LCDIF_CTRL_SFTRST | BM_LCDIF_CTRL_CLKGATE);

	/* Reset LCD display */
	HW_LCDIF_CTRL1_CLR(BM_LCDIF_CTRL1_RESET);
	
	/* Setup the bus protocol */
	HW_LCDIF_CTRL_CLR(BM_LCDIF_CTRL_WORD_LENGTH);

	HW_LCDIF_CTRL1_CLR(BM_LCDIF_CTRL1_MODE86);
	HW_LCDIF_CTRL1_CLR(BM_LCDIF_CTRL1_BUSY_ENABLE);
	BW_LCDIF_CTRL1_BYTE_PACKING_FORMAT(0x0f);

#ifdef USE_WSYNC_MODE
	{
		int ret;
		/* HW_LCDIF_CTRL1_SET(BM_LCDIF_CTRL1_VSYNC_EDGE_IRQ_EN); */
		ret = request_irq(IRQ_LCDIF_ERROR, device_lcd_err_handler, 0, "stmp37xxfb_err", NULL);
		init_MUTEX_LOCKED(&wsync_mutex);
	}
#endif

#ifdef USE_VSYNC_MODE
	/* Setup Vsync */
	HW_LCDIF_VDCTRL0.B.VSYNC_OEB = false;
	HW_LCDIF_VDCTRL0.B.VSYNC_POL = 0;
	HW_LCDIF_VDCTRL0.B.VSYNC_PULSE_WIDTH_UNIT = 0;
	HW_LCDIF_VDCTRL0.B.VSYNC_PERIOD_UNIT = 0;
	
	HW_LCDIF_VDCTRL1.B.VSYNC_PULSE_WIDTH = 4095;
	HW_LCDIF_VDCTRL1.B.VSYNC_PERIOD = 400000;
	
	HW_LCDIF_VDCTRL3.B.VERTICAL_WAIT_CNT = 0;
	HW_LCDIF_VDCTRL3.B.SYNC_SIGNALS_ON = 1;
	
	HW_LCDIF_VDCTRL0.B.ENABLE_PRESENT = 0;
	HW_LCDIF_VDCTRL0.B.HSYNC_POL = 0;
	HW_LCDIF_VDCTRL0.B.ENABLE_POL = 0;
	HW_LCDIF_VDCTRL0.B.DOTCLK_POL = 0;
	HW_LCDIF_VDCTRL0.B.DOTCLK_V_VALID_DATA_CNT = 0;
	
	HW_LCDIF_VDCTRL2.B.HSYNC_PULSE_WIDTH = 0;
	HW_LCDIF_VDCTRL2.B.HSYNC_PERIOD = 0;
	HW_LCDIF_VDCTRL2.B.DOTCLK_H_VALID_DATA_CNT = 0;
	
	HW_LCDIF_VDCTRL3.B.HORIZONTAL_WAIT_CNT = 0;
	/* BF_CS1 (LCDIF_CTRL, RUN, 1); */
#else
	/* VSYNC is an input */
	HW_LCDIF_VDCTRL0.B.VSYNC_OEB = true;
#endif

	/* Setup timings */
	init_timings(LCD_CYCLE_TIME_NS, 0);

	ndelay(10000);
	HW_LCDIF_CTRL1_SET(BM_LCDIF_CTRL1_RESET);
	msleep(20);
	HW_LCDIF_CTRL1_CLR(BM_LCDIF_CTRL1_RESET);
	msleep(10);
	HW_LCDIF_CTRL1_SET(BM_LCDIF_CTRL1_RESET);
	msleep(20);
#if 0
	/* Turn on performance counters */
	HW_DIGCTL_AHB_STATS_SELECT.B.L1_MASTER_SELECT = 1; /* ARM I */
	HW_DIGCTL_AHB_STATS_SELECT.B.L2_MASTER_SELECT = 1; /* ARM D */
#endif
}

static void init_lcd(void)
{	
	init_lcd_3700();

	/* Post-reset delay */
	msleep(10);

	lcd_panel_init();
	lcd_onoff = 1;

	need_bl_init = 1;
}

static void term_lcd(void)
{
	lcd_panel_term();
}

static void lcd_on(void)
{
	/* printk("[%s] >> %d\n", __func__, lcd_onoff); */
	if (lcd_onoff)
		return;

	/* FIXME: Turn on DMA */
	/* dma_init(); */
	init_lcd();
	/* HW_LCDIF_CTRL_CLR(BM_LCDIF_CTRL_CLKGATE); */
	/* HW_CLKCTRL_PIX_CLR(BM_CLKCTRL_PIX_CLKGATE); */
	/* lcd_panel_power(1); */
	msleep(50); /* 2008.11.29: waiting for step-up circuit in LCD panel */

	/* Transfer UI to LCD if pervious updating was occured by UI */
	if(last_node == FRAME_UI) {
		if (s_doing_refresh) {
			wait_event_interruptible(wait_q, s_doing_refresh == 0);
		}
		
		/* Protect against multiple calls */
		if (down_interruptible(&refresh_mutex)) {
			return;
		}
		dma_enable(0, last_node);
		up(&refresh_mutex);
	}
}

static void lcd_off(void)
{
	/* printk("[%s]\n", __func__); */
	if (!lcd_onoff)
		return;

	lcd_bl_power(0);

	while(HW_APBH_CHn_SEMA(0).B.PHORE);
	term_lcd();
	while(HW_APBH_CHn_SEMA(0).B.PHORE);

	lcd_onoff = 0;

	HW_LCDIF_CTRL1_CLR(BM_LCDIF_CTRL1_RESET);
	udelay(1000);
	
	/* FIXME: Turn off DMA */
	/* HW_LCDIF_CTRL_SET(BM_LCDIF_CTRL_CLKGATE); */
	/* HW_CLKCTRL_PIX_SET(BM_CLKCTRL_PIX_CLKGATE); */
}

#ifdef USE_OLD_DMA
static irqreturn_t device_lcd_dma_handler(int irq_num, void* dev_idp)
{
	/* printk("device_lcd_dma_handler()\n"); */
	
	BUG_ON(!s_doing_refresh);
	
	/* Paranoia */
	while (HW_LCDIF_CTRL.B.RUN || HW_APBH_CHn_SEMA(0).B.PHORE)
		;/* dump_registers("dma_handler() waiting"); */
	
	/* clear interrupt status */
	BF_CLR(APBH_CTRL1, CH0_CMDCMPLT_IRQ);
	
	/* Reschedule timer */
	if (periodic_refresh)
	{
		BUG_ON(del_timer(&refresh_timer));
		
		refresh_timer.expires += refresh_period;
		add_timer(&refresh_timer);
	}
	
	s_doing_refresh = 0;
	wake_up_interruptible(&wait_q);
	
	return IRQ_HANDLED;
}
#else
static void device_lcd_dma_handler(int id, unsigned int status, void* dev_id)
{
	/* printk("%s(): %lu: id:%d, status:%d\n", __func__, jiffies, id, status); */

	if ((status != STMP37XX_DMA_ERROR) && (status != STMP37XX_DMA_COMPLETE)) {
		/* do nothing, have no idea. */
		return;
	}
    
	BUG_ON(status == STMP37XX_DMA_ERROR);

	BUG_ON(s_doing_refresh == 0);

	/* Paranoia */
	/* printk("paranoia check\n"); */
        while (HW_LCDIF_CTRL.B.RUN || HW_APBH_CHn_SEMA(0).B.PHORE)
		;/* dump_registers("dma_handler() waiting"); */

        s_doing_refresh = 0;

        if (periodic_refresh) {
		BUG_ON(del_timer(&refresh_timer));
            
		refresh_timer.expires += refresh_period;
		add_timer(&refresh_timer);
        }

	if (need_bl_init){
		lcd_bl_init();
		need_bl_init = 0;
	}

        wake_up_interruptible(&wait_q);
}

#ifdef USE_WSYNC_MODE
static irqreturn_t device_lcd_err_handler (int irq_num, void* dev_idp)
{
	if (HW_LCDIF_CTRL1.B.VSYNC_EDGE_IRQ)
	{
		stmp37xx_dma_go(DMA_LCDIF, &video_dma_descriptor[0], 1);
		HW_LCDIF_CTRL1_CLR(BM_LCDIF_CTRL1_VSYNC_EDGE_IRQ);
		HW_LCDIF_CTRL1_CLR(BM_LCDIF_CTRL1_VSYNC_EDGE_IRQ_EN);
		up(&wsync_mutex);
	}
	return IRQ_HANDLED;
}
#endif

#endif /* !USE_OLD_DMA */

static int __init stmp37xxfb_init(void)
{
	int ret = 0;

#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("stmp37xxfb", &option))
		return -ENODEV;
	stmp37xxfb_setup(option);
#endif
	/* printk("stmp37xxfb_init()\n"); */

	if (!stmp37xxfb_enable)
		return -ENXIO;

	ret = platform_driver_register(&stmp37xxfb_driver);

	if (!ret) {
		ret = platform_device_register(&stmp37xxfb_device);
		if (ret)
			platform_driver_unregister(&stmp37xxfb_driver);
	}
	
	if (!ret) {
		dma_init();
		{ /* Set window address as default */
			lcd_onoff = 1;
			/* init_lcd(); */
			need_bl_init = 0;

			write_register(0x0020, 0x0000);
 			write_register(0x0021, 0x0000); 
 			write_register(0x0050, 0x0000); 
 			write_register(0x0051, 0x00EF); 
 			write_register(0x0052, 0x0000); 
 			write_register(0x0053, 0x013F); 

			write_command_16(0x0022);
		}
#ifdef USE_OLD_DMA
		/* register irq handler with kernel */
		ret = request_irq(IRQ_LCDIF_DMA, device_lcd_dma_handler, 0, "stmp37xxfb", NULL);
#else
		ret = stmp37xx_request_dma(DMA_LCDIF, "stmp37xxfb", device_lcd_dma_handler, NULL);
#endif
		if (ret != 0) {
			/* printk("stmp37xxfb_init() - error: request_irq() returned error: %d\n", ret); */
		}
		else {
			init_waitqueue_head(&wait_q);
			init_timer(&refresh_timer);

			refresh_timer.expires = jiffies + refresh_period;
			refresh_timer.function = periodic_function;

			/* use timer to send first data since LCD needs some delay after initialization. */
			add_timer(&refresh_timer);
			/* dma_enable(0, 0); */
		}
	}

	return ret;
}

module_init(stmp37xxfb_init);

#ifdef MODULE
static void __exit stmp37xxfb_exit(void)
{
	unsigned i;
	/* Clear up dma resources */
	stmp37xx_dma_free_command(&setup_dma_descriptor);
	for (i = 0; i < MAX_CHAIN_LEN; ++i) {
		if (video_dma_descriptor[i] != 0)
			stmp37xx_dma_free_command(video_dma_descriptor[i]);
	}
	stmp37xx_dma_user_destroy(&dma_user);

	dma_free_coherent(&stmp37xxfb_device.dev, sizeof(char)*2, setup_memory, &setup_phys);

	platform_device_unregister(&stmp37xxfb_device);
	platform_driver_unregister(&stmp37xxfb_driver);
}

module_exit(stmp37xxfb_exit);

MODULE_LICENSE("GPL");
#endif				/* MODULE */
