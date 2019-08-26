
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

#ifndef _STMP3XXX_CSC_H_
#define _STMP3XXX_CSC_H_

#include <linux/spinlock.h>
#include <asm/arch-stmp37xx/stmp3xxx_csc_ioctl.h>


#ifndef bool
#define bool int
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#ifndef CSC_DRIVER_MAJOR
#define CSC_DRIVER_MAJOR 249   /* dynamic major by default */
#endif

#define DRIVER_NAME "stmp3xxx_csc"
#define DRIVER_DESC "SigmaTel 3xxx CSC Driver"
#define DRIVER_AUTH "Copyright (C) 2007 SigmaTel, Inc."

/*
 * Macros to help debugging
 */

#define TRACE_CSC  0

#undef PDEBUG             /* undef it, just in case */
#if TRACE_CSC
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( DRIVER_NAME ": " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */


/* Structure for controlling CSC device */
typedef struct
{
	uint8_t             num_open;                 /* Number of times opened */
	spinlock_t          sp_lock;                  /* Mutex */
	wait_queue_head_t   wait_q;
	bool				s_doing_csc;
} CSC_Dev;


#endif /* _STMP3XXX_CSC_H_ */
