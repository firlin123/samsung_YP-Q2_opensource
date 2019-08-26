/******************************************************************************
                      MCUBEWORKS YUV2RGB CONVERSION
           Copyright (c) 2006 McubeWorks, Inc. All rights reserved.

 This software was developed by McubeWorks(MCW) and is strictly limited for use
 on Sigmatel's ARM9E-based products only. This is company confidential
 information and the property of MCW, and can not be reproduced or disclosed in
 any form without written authorization of MCW. To decompile, disassemble,
 or otherwise reverse engineer or attempt to reconstruct or discover any source
 code or underlying ideas or algorithms is strictly prohibited.

 Those intending to use this software illegally or for other purposes
 are advised that it infringes MCW's copyright and shall have full responsibility
 of any relevant claim. MCW has no liability for any claim of infringement in use
 of a superceded or altered release of this software and in use of this software
 combined with other products than Sigmatel's ARM9E-based products.
 This copyright notice must be included in all copies or derivative works.
******************************************************************************/

/*============================== FILE HEADER ==================================
    File Name          : mcw_sigmatel_yuv2rgb.h
    Author(s)          : Jae-ill, Pi (issun@mcubeworks.com)
    Created            : 2006/03/02

    Description        : define interface for YUV to RGB conversion
    Internal Functions : MCW_SIGMATEL_YUV420_To_RGB12( )
                         MCW_SIGMATEL_YUV420_To_RGB16( )
                         MCW_SIGMATEL_YUV420_To_RGB24( )
    Notes              :
=============================================================================*/

/*===================== Modification History (Reverse Order) ==================
    2006/03/02         : Jae-ill, generated
=============================================================================*/

#ifndef __MCW_YUV2RGB_API_H__
#define __MCW_YUV2RGB_API_H__


//====== Structure Definition ================================================

typedef struct YUV2RGB_MCW_VERSION_tag
{
    int           iMajor;       // Major version
    int           iMinor;       // Minor version
    int           iPatch;       // Patch version
} YUV2RGB_MCW_VERSION;

typedef struct   POINTER_YUV_tag
{
	unsigned char *pbY;     	// Y Left-Upper pointer
	unsigned char *pbU;     	// U Left-Upper pointer
	unsigned char *pbV;     	// V Left-Upper pointer
} POINTER_YUV;

typedef struct YUV2RGB_SIZE_INFO_tag
{
	int image_buffer_width; 	// source yuv image buffer width
	int image_width;       		// source yuv image width
	int image_height;      		// source yuv image height
	int lcd_width;         		// display lcd width
	int lcd_height;        		// disphay lcd height
	int lcd_xoffset;       		// display lcd x offset
	int	lcd_yoffset;			// display lcd y offset
	int output_rotate;			// 0: normal, 1: ratate 90
	int	Bright;					// default 0, (-255~255)
	int Contrast;				// default 4, (0 ~ 8)
	int Scale;					// default 2, (0: quarter, 1: half, 2: same, 3: double) image width/height
} YUV2RGB_SIZE_INFO;

// Function Declaration
#ifdef __cplusplus
extern "C"
{
#endif

//=========================== Get Codec Version ==============================
void MCW_SIGMATEL_YUV420_To_RGB_GetVersion (YUV2RGB_MCW_VERSION *pCodecVer);

//======================YUV2RGB Conversion Functions =========================
void MCW_SIGMATEL_YUV420_To_RGB12(unsigned short *pwRGB12, POINTER_YUV *pYUV, YUV2RGB_SIZE_INFO *pYuv2rgbSizeInfo);
void MCW_SIGMATEL_YUV420_To_RGB16(unsigned short *pwRGB16, POINTER_YUV *pYUV, YUV2RGB_SIZE_INFO *pYuv2rgbSizeInfo);
void MCW_SIGMATEL_YUV420_To_RGB24(unsigned char  *pbRGB24, POINTER_YUV *pYUV, YUV2RGB_SIZE_INFO *pYuv2rgbSizeInfo);
//============================================================================

#ifdef __cplusplus
}
#endif

#endif    // __MCW_YUV2RGB_API_H__
