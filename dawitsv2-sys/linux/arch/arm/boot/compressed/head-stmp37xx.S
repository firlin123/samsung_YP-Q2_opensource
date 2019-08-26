/*
 * linux/arch/arm/boot/compressed/head-stmp36xx.S
 *
 * stmp36xx specific stuff - turning on the PLL, etc
 *
 */

#include <linux/autoconf.h>
#include <linux/linkage.h>
#include <asm/mach-types.h>

@ Define REGS_BASE before including hardware.h so that we have the MMU-disabled
@ register addresses used here (otherwise it defaults to the MMU-mapped addresses)

#define REGS_BASE	0x80000000
#include <asm/arch/stmp37xx.h>

		.section        ".start", "ax"

__stmp37xx_start:

		@ Preserve r8/r7 i.e. kernel entry values

		mrs	r0, cpsr
		bic	r0, r0, #0x1f
		orr	r0, r0, #0xd3		@ Supervisor mode, IRQ/FIQ disabled
		msr	cpsr, r0

		@ Clean, invalidate Dcache, drain write buffer
1:		// Test, clean and invalidate Dcache line.
		mrc	p15, 0, r15, c7, c14, 3
		bne	1b
		// Drain write buffer
		mov	r0, #0
		mcr	p15, 0, r0, c7, c10, 4

		@ CP15r1 - core setup
		mrc	p15, 0, r0, c1, c0, 0
		@ 15		L4=0		Loads to PC set T bit
		@ 14		RR=0		Random replacement cache
		@ 13		V=1		Exception vectors 0xffff0000
		@ 12		I=0		Icache disabled
		@ 9		R=0		(modifies MMU AP meanings)
		@ 8		S=0		(modifies MMU AP meanings)
		@ 7		B=0		Little endian
		@ 2		C=0		Dcache disabled
		@ 1		A=1		Alignment fault checking
		@ 0		M=0		MMU disabled
		@ Note that the R/S bits are 0 by default on ARM-926, and
		@ we don't want to upset Linux by doing something else.
		bic	r0, r0, #0x0087
		bic	r0, r0, #0xf300
		orr	r0, r0, #0x0002
		orr	r0, r0, #0x2000
		mcr	p15, 0, r0, c1, c0, 0
		b	1f
1:
		// Invalidate caches
		mov	r0, #0
		mcr	p15, 0, r0, c7, c5, 0	// Invalidate Icache
		mcr	p15, 0, r0, c7, c6, 0	// Invalidate Dcache
		mcr	p15, 0, r0, c8, c7, 0	// Invalidate TLB
		b	1f
1:
		// We're now 100% certain to be non-cached
	
		@ Turn off the watchdog otherwise... we get reset
		@ before we boot.
		ldr	r0, =HW_RTC_CTRL_CLR_ADDR
		mov	r1, #BM_RTC_CTRL_SFTRST | BM_RTC_CTRL_CLKGATE
		str	r1, [r0]
		mov	r1, #BM_RTC_CTRL_WATCHDOGEN
		str	r1, [r0]


#if 0
		@ Set the machine type/etc for boot here, as the bootloader
		@ can't do this
		mov	r7, #MACH_TYPE_STMP36XX
		mov	r8, #0
#endif

#ifdef CONFIG_MACH_ARMA37
# warning "hack"
		mov	r7, #(MACH_TYPE_ARMA37 & 0xff)
		orr	r7, r7, #(MACH_TYPE_ARMA37 & 0xff00)
		mov	r8, #0
#endif

