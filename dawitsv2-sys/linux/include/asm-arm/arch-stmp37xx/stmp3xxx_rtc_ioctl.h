/* include/asm-arm/arch-stmp3xxx/stmp3xxx_rtc_ioctl.h
 *
 * Copyright (c) 2007 Sigmatel, Inc.
 *         Peter Hartley, <peter.hartley@sigmatel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef STMP3XXX_RTC_IOCTL_H
#define STMP3XXX_RTC_IOCTL_H

/* ioctl commands to control the STMP3xxx RTC driver
 *
 * These ioctls should be issued to /dev/rtc0, just like the standard RTC
 * ioctls in <linux/rtc.h>.
 */

enum {

    /* Read the RTC_LOCK bit into the unsigned int parameter.
     *
     * See 3700 datasheet v1.0 section 20.8.7.
     */
    STMP3XXX_RTC_GET_LOCK = _IOR('p', 0x50, unsigned int),

    /* Set the RTC_LOCK bit. In the locked state, the RTC cannot be
     * set. The RTC_LOCK bit cannot be cleared by software: the chip.
     * including the "RTC analog domain", must be power-cycled
     * (e.g. by removing battery).
     *
     * See 3700 datasheet v1.0 section 20.8.7.
     */
    STMP3XXX_RTC_LOCK     = _IO('p',  0x51),
};

#endif
