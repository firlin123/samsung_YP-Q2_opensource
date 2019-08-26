#ifndef __ASM_ARCH_UNCOMPRESS_H__
#define __ASM_ARCH_UNCOMPRESS_H__
/*
 *  linux/include/asm-arm/arch-stmp36xx/uncompress.h
 *
 *  Copyright (C) 2008 MIZI Research, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <asm/arch/hardware.h>

#ifndef UARTDBG_BASE
#define UARTDBG_BASE		0x80070000
#endif

#define UART ((volatile unsigned int*)UARTDBG_BASE)

static inline void putc(char c)
{
        while((UART[6] & (1<<7)) == 0) /* wait TX empty */
		barrier();
	UART[0] = c;
#ifdef UARTDBG_LFCR
	if (c == '\n') {
	        while((UART[6] & (1<<7)) == 0) /* wait TX empty */
			barrier();
		UART[0] = '\r';
	}
#endif
}

static inline void flush(void)
{
        while((UART[6] & (1<<7)) == 0)
		barrier();
	while (UART[6] & (1<<3)) /* wait free */
		barrier();
}


/*
 * nothing to do
 */
#define arch_decomp_setup()

#define arch_decomp_wdog()

#endif /* __ASM_ARCH_UNCOMPRESS_H__ */
