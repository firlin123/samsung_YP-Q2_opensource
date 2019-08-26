
/*
* Copyright (C) 2007 SigmaTel, Inc., Shaun Myhill <smyhill@sigmatel.com>
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation; either version 2 of the License, or (at your option) any later
* version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef _OCRAM_H_
#define _OCRAM_H_

#include <asm/arch/platform.h>

/*
	OCRAM Usages
	============== Codec Area ==============
	max OCRAM usage (153K) 0x00026400
	max SDRAM usage (64K)
	[ MP3 ]
		code(32k)			0x00000000-0x00008000 (virt 0x50000000-0x50008000)
		data(42k)			0x00008000-0x00012800 (virt 0x50009000-0x5001B900)
	[ OGG ]
		code(64k)			uses SDRAM		      (vrit 0x50000000-0x50010000)
		data(141k)			0x00000000-0x00023400 (vrit 0x50010000-0x50034500)
	[ WMA ]
		code(112K)			0x00000000-0x0001C000 (vrit 0x50000000-0x5001C000)
		code(6K)			uses SDRAM            (vrit 0x50100000-0x50101800)
		data(24K)			uses SDRAM            (vrit 0x50101800-0x50107800)
		data(41K)			0x0001C000-0x00026400 (vrit 0x5001F000-0x50029400)

	=========== DAC buffer area ===========
		size 0xCA00
		data				0x00026400-0x00032E00

	============== CODE area ==============
		code(incl. stack)	0x00033000-0x0003FFFF
*/

/* CODEC and USB Buffer : 153KB */
#define OCRAM_CODEC_OFFSET (0x00000000)
#define OCRAM_CODEC_START (STMP37XX_OCRAM_BASE + OCRAM_CODEC_OFFSET)
#define OCRAM_CODEC_START_VIRT (STMP37XX_OCRAM_BASE_VIRT + OCRAM_CODEC_OFFSET)
#define OCRAM_CODEC_SIZE (0x00026400)

/* DAC - see stmp37xx-audio.c :  51KB */
#define OCRAM_DAC_USE
#define OCRAM_DAC_OFFSET (0x00026400)
#define OCRAM_DAC_START (STMP37XX_OCRAM_BASE + OCRAM_DAC_OFFSET)
#define OCRAM_DAC_START_VIRT (STMP37XX_OCRAM_BASE_VIRT + OCRAM_DAC_OFFSET)
#define OCRAM_DAC_SIZE (0x0000CA00)

/* RFS : 40KB */
#define OCRAM_RFS_OFFSET (0x00033000)
#define OCRAM_RFS_OFFSET_VIRT (STMP37XX_OCRAM_BASE_VIRT + OCRAM_RFS_OFFSET)
#define OCRAM_RFS_START (STMP37XX_OCRAM_BASE + OCRAM_RFS_OFFSET)
#define OCRAM_RFS_START_VIRT (STMP37XX_OCRAM_BASE_VIRT + OCRAM_RFS_OFFSET)
#define OCRAM_RFS_SIZE (0x0000A000)

/* SDRAM Turn-off State : 12KB */
#define OCRAM_RUN_CODE_OFFSET (0x0003D000)
#define OCRAM_RUN_STACK_OFFSET (0x0003FFF0)
#define OCRAM_RUN_CODE_START (STMP37XX_OCRAM_BASE + OCRAM_RUN_CODE_OFFSET)
#define OCRAM_RUN_STACK_START (STMP37XX_OCRAM_BASE + OCRAM_RUN_STACK_OFFSET)
#define OCRAM_RUN_CODE_START_VIRT (STMP37XX_OCRAM_BASE_VIRT + OCRAM_RUN_CODE_OFFSET)
#define OCRAM_RUN_STACK_START_VIRT (STMP37XX_OCRAM_BASE_VIRT + OCRAM_RUN_STACK_OFFSET)
#define OCRAM_RUN_SIZE (0x00003000)


//hhb add
#define STMP37XX_SRAM_BASE    0xc8000000
#define STMP37XX_SRAM_START   0x00000000
#define STMP37XX_SRAM_SIZE    0x00040000 /* total 256KB sram */
 
#define STMP37XX_SRAM_AUDIO_KERNEL      0xc800b000
 
#define STMP37XX_SRAM_AUDIO_KERNEL_SIZE 0x0000a000 /* 40KB */ 
#define STMP37XX_SRAM_AUDIO_KERNEL_START 0x0000b000
 
#define STMP37XX_SRAM_AUDIO_KERNEL_DESC 0x00001000


#define SDRAM_MAX_ALLOC_SIZE	(64*1024)

#define WMA_INIT_CODE_VIRT 		0x50100000
#define OGG_CODE_SIZE			(64*1024)

#define SHARED_DATA_VIRT 		0x50300000
#define SHARED_DATA_SIZE 		OCRAM_CODEC_SIZE

 
#define is_shared(ubuf) ((unsigned)ubuf >= SHARED_DATA_VIRT && (unsigned)ubuf < (SHARED_DATA_VIRT + SHARED_DATA_SIZE))

void *get_shared_virt (char __user *buf);

#endif /* _OCRAM_H_ */
