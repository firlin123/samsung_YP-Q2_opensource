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

//------------------------------------------------------------------------------
// This file contains ioctl commands that control the STMP3xxx CSC driver
//------------------------------------------------------------------------------

#ifndef _STMP3XXX_CSC_IOCTL_H
#define _STMP3XXX_CSC_IOCTL_H

#include <linux/ioctl.h>

#if defined __KERNEL__
#define __USER	__user
#else
#define __USER
#endif

typedef struct
{
    void* y_buffer; 
    unsigned long y_buffer_size;
    
    void* u_buffer;
    unsigned long u_buffer_size;
    
    void* v_buffer;
    unsigned long v_buffer_size;
    
    unsigned height;
    unsigned memory_stride;

    void* rgb_buffer;
    unsigned long rgb_buffer_size;
} convertor_data_t;

/* IOCT Magic */
#define STMP3XXX_CSC_IOC_MAGIC  0xD2

//------------------------------------------------------------------------------
// IOCT = Tell - Use MAKE_ENABLE_PARAM to create param
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// IOCT = Tell - GPIO ID is the param
//------------------------------------------------------------------------------
#define STMP3XXX_CSC_IOCS_CONVERT	_IOW(STMP3XXX_CSC_IOC_MAGIC,  0, convertor_data_t)

//------------------------------------------------------------------------------
// IOCG = Get = return value contains value, input value contains the GPIO ID
//------------------------------------------------------------------------------
#define STMP3XXX_GPIO_IOCQ_GET     _IOWR(STMP3XXX_GPIO_IOC_MAGIC,  5, int)

//#define STPMP3XXX_COMMNAND_COUNT    6

#endif //_STMP3XXX_CSC_IOCTL_H
