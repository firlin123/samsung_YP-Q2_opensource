/*
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

#ifndef _ASM_ARCH_PLATFORM_H_
#define _ASM_ARCH_PLATFORM_H_

#define STMP37XX_BOOT_ROM_HI		0xFFFF0000		/* Normal position */
#define STMP37XX_BOOT_ROM_LO		0xC0000000		/* ROM aliases through 1GB */
#define STMP37XX_BOOT_ROM_BASE		STMP37XX_BOOT_ROM_HI
#define STMP37XX_BOOT_ROM_SIZE		SZ_64K

#define STMP37XX_REGS_BASE		0x80000000		/* Peripheral space */
#define STMP37XX_REGS_SIZE		SZ_1M

#define STMP37XX_FLASH_BASE		0x60000000		/* NOR flash */

#define STMP37XX_SDRAM_BASE		0x40000000		/* SDRAM */

#define STMP37XX_OCRAM_HI		0x00040000		/* 4095 aliases of On-Chip RAM */
#define STMP37XX_OCRAM_LO		0x00000000
#define STMP37XX_OCRAM_BASE		STMP37XX_OCRAM_LO
#define STMP37XX_OCRAM_SIZE		SZ_256K
#define STMP37XX_OCRAM_BASE_VIRT      0xF1000000

#endif /* _ASM_ARCH_PLATFORM_H_ */
