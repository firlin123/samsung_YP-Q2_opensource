/*
 * include/asm-arm/arch-stmp37xx/entry-macro.S
 *
 * Low-level IRQ helper macros for stmp37xx platforms
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

 		.macro	disable_fiq
		.endm

		.macro  get_irqnr_preamble, base, tmp
		.endm

		.macro  arch_ret_to_user, tmp1, tmp2
		.endm

		.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp

	        mov	\base, #0xf0000000			@ vm address of IRQ controller
	        @ldr	\irqnr,	[\base, #0x17c]			@ HW_ICOLL_DEBUG
	        @tst	\irqnr, #(1<<16)			@ Is an interrupt pending?	  
		ldr	\irqnr, [\base, #0x30]			@ HW_ICOLL_STAT
		cmp	\irqnr, #0x3f
		movne	\irqstat, #0				@ Ack this IRQ
		strne	\irqstat, [\base, #0x00]		@ HW_ICOLL_VECTOR
		moveqs	\irqnr, #0				@ Zero flag set for no IRQ

		.endm

