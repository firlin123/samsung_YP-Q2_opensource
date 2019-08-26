/*
* Copyright (C) 2006 SigmaTel, Inc., David Weber <dweber@sigmatel.com>
*                                    Shaun Myhill <smyhill@sigmatel.com>
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
// This file contains ioctl commands that control the STMP36xx DAC (audioout)
//------------------------------------------------------------------------------

#ifndef _STMP3XXX_DAC_IOCTL_H
#define _STMP3XXX_DAC_IOCTL_H

#include <linux/ioctl.h>

#define STMP3XXX_DAC_IOC_MAGIC  0xC2

#define GET_MIN_VOL(range)  (range & 0xFFFF)
#define GET_MAX_VOL(range)  (range >> 16)
#define MAKE_VOLUME(vol_ctl, vol) ((vol_ctl << 24) | (vol & 0x00FFFFFF))

typedef enum VOL_CTL_ID
{
    VC_DAC = 0,
    VC_HEADPHONE,
    VC_LINEOUT
}VOL_CTL_ID;

typedef enum HP_SOURCE
{
    HP_DAC = 0,
    HP_LINEIN = 1
} HP_SOURCE;

//------------------------------------------------------------------------------
// IOCT = Tell = parameter contains info
//------------------------------------------------------------------------------
#define STMP3XXX_DAC_IOCT_MUTE            _IOW(STMP3XXX_DAC_IOC_MAGIC,  0, int)
#define STMP3XXX_DAC_IOCT_UNMUTE          _IOW(STMP3XXX_DAC_IOC_MAGIC,  1, int)
#define STMP3XXX_DAC_IOCT_VOL_SET         _IOW(STMP3XXX_DAC_IOC_MAGIC,  2, int)
#define STMP3XXX_DAC_IOCT_SOURCE          _IOW(STMP3XXX_DAC_IOC_MAGIC,  3, int)
#define STMP3XXX_DAC_IOCT_SAMPLE_RATE     _IOW(STMP3XXX_DAC_IOC_MAGIC,  4, int)
#define STMP3XXX_DAC_IOCT_EXPECT_UNDERRUN _IOW(STMP3XXX_DAC_IOC_MAGIC,  5, bool)
#define STMP3XXX_DAC_IOCT_INIT            _IO(STMP3XXX_DAC_IOC_MAGIC,   6)
#define STMP3XXX_DAC_IOCT_HP_SOURCE       _IOW(STMP3XXX_DAC_IOC_MAGIC,  7, int)

//------------------------------------------------------------------------------
// IOCQ = Query = return value contains info
//------------------------------------------------------------------------------
#define STMP3XXX_DAC_IOCQ_IS_MUTED         _IOR(STMP3XXX_DAC_IOC_MAGIC, 8, int)
#define STMP3XXX_DAC_IOCQ_VOL_RANGE_GET    _IOR(STMP3XXX_DAC_IOC_MAGIC, 9, int)
#define STMP3XXX_DAC_IOCQ_VOL_GET          _IOR(STMP3XXX_DAC_IOC_MAGIC, 10, int)
#define STMP3XXX_DAC_IOCQ_SAMPLE_RATE      _IOR(STMP3XXX_DAC_IOC_MAGIC, 11, int)
#define STMP3XXX_DAC_IOCQ_HP_SOURCE_GET    _IOR(STMP3XXX_DAC_IOC_MAGIC, 12, int)
#define STMP3XXX_DAC_IOCQ_ACTIVE           _IOR(STMP3XXX_DAC_IOC_MAGIC, 13, int)

//------------------------------------------------------------------------------
// IOCS = Set = argument contains user space pointer to info
//------------------------------------------------------------------------------
typedef struct
{
    // 0 <= remaining_next_bytes <= remaining_now_bytes <= total_bytes
    //
    // total_bytes will not change between calls.
    //
    // remaining_now_bytes is the number of bytes queued in the hardware
    //     DMA channel, instantaneously.
    //
    // remaining_next_bytes is the number of bytes that will still be
    //     queued when we next wake up. This can be used to decide whether
    //     it's safe to sleep until the next period, or whether data needs
    //     to be provided before then.
    unsigned long total_bytes;
    unsigned long remaining_now_bytes;
    unsigned long remaining_next_bytes;
} stmp3xxx_dac_queue_state_t;
#define STMP3XXX_DAC_IOCS_QUEUE_STATE	 _IOR(STMP3XXX_DAC_IOC_MAGIC, 14, stmp3xxx_dac_queue_state_t)

//------------------------------------------------------------------------------
// IOCG = Get = return value contains info
//------------------------------------------------------------------------------

#endif //_STMP3XXX_DAC_IOCTL_H
