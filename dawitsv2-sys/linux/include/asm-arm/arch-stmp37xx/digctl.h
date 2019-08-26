/*
 * linux/include/asm/arch-stmp36xx/digctl.h
 *
 * This file contains stmp36xx digctl block related code
 *
 * Copyright (C) 2005 Samsung Electronics, Inc
 * Author : Heechul Yun <heechul.yun@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ARCH_PINCTRL_H_

#ifndef __KERNEL__
#error This is a kernel API
#endif

#define SERIAL_NUMBER_MAX_RAW_BYTES 16
#define SERIAL_NUMBER_MAX_ASCII_CHARS (SERIAL_NUMBER_MAX_RAW_BYTES * 2)

typedef struct
{
        u8 raw[SERIAL_NUMBER_MAX_RAW_BYTES + 1];
        u32 rawsizeinbytes;
        u8 ascii[SERIAL_NUMBER_MAX_ASCII_CHARS + 1];
        u32 asciisizeinchars;
        u8 deviceid;
}serialnumber_t;

typedef struct
{
	u8 version[16];
	u8 nation[4];
	u8 model[32];
}version_inf_t;

extern unsigned long get_usec(void);
extern unsigned long get_usec_elapsed(unsigned long start, unsigned long stop); 
extern unsigned long get_usec_elapsed_from_prev_call(void); 

extern unsigned long get_digctl_status(void); 

extern int get_chip_serial_number(serialnumber_t *psn);
extern int get_hw_option_type(void); //add dhsong
extern int get_sw_version(version_inf_t *ver);

#endif /* _ARCH_PINCTRL_H_ */ 
