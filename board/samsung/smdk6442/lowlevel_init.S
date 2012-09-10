/*
 * Copyright (C) 2009 Samsung Electronics
 * Kyungmin Park <kyungmin.park@samsung.com>
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <version.h>
#include <asm/arch/cpu.h>
#include <asm/arch/power.h>

/*
 * Register usages:
 *
 * r5 has zero always
 */

_TEXT_BASE:
	.word	TEXT_BASE

	.globl lowlevel_init
lowlevel_init:
	mov	r9, lr

	/* r5 has always zero */
	mov	r5, #0

	ldr	r8, =S5P6442_GPIO_BASE			@0xE0200000

	/* IO retension release */
	ldr	r0, =S5P6442_OTHERS			@0xE010E000
	ldr	r1, [r0]
	ldr	r2, =(1 << 31)				@IO_RET_REL
	orr	r1, r1, r2
	str	r1, [r0]

#ifndef CONFIG_PRELOADER
	/* Disable Watchdog */
	ldr	r0, =S5P6442_WATCHDOG_BASE		@0xEA200000
	orr	r0, r0, #0x0
	str	r5, [r0]

	/* setting SRAM */
	ldr	r0, =S5P6442_SROMC_BASE			@0xE7000000
	ldr	r1, =0x9
	str	r1, [r0]
#endif

	/* S5P6442 has 3 groups of interrupt sources */
	ldr	r0, =S5P6442_VIC0_BASE			@0xE4000000
	add	r1, r0, #0x00100000
	add	r2, r0, #0x00200000

	/* Disable all interrupts (VIC0, VIC1 and VIC2) */
	mvn	r3, #0x0
	str	r3, [r0, #0x14]				@INTENCLEAR
	str	r3, [r1, #0x14]				@INTENCLEAR
	str	r3, [r2, #0x14]				@INTENCLEAR

#ifndef CONFIG_PRELOADER
	/* Set all interrupts as IRQ */
	str	r5, [r0, #0xc]				@INTSELECT
	str	r5, [r1, #0xc]				@INTSELECT
	str	r5, [r2, #0xc]				@INTSELECT

	/* Pending Interrupt Clear */
	str	r5, [r0, #0xf00]			@INTADDRESS
	str	r5, [r1, #0xf00]			@INTADDRESS
	str	r5, [r2, #0xf00]			@INTADDRESS
#endif

#ifndef CONFIG_PRELOADER
	/* for UART */
	bl uart_asm_init
#endif

#ifdef CONFIG_PRELOADER
	/* init system clock */
	bl	system_clock_init

	bl	mem_ctrl_asm_init

	/* OneNAND Sync Read Support at S5PC110 only
	 * RM[15]       : Sync Read
	 * BRWL[14:12]  : 7 CLK
	 * BL[11:9]     : Continuous
	 * VHF[3]       : Very High Frequency Enable (Over 83MHz)
	 * HF[2]        : High Frequency Enable (Over 66MHz)
	 */
	ldr	r1, =0xE00C

	ldr	r0, =0xB001E442
	strh	r1, [r0]

	ldr	r0, =0xB0600000
	str	r1, [r0, #0x100]			@ ONENAND_IF_CTRL

	/* Wakeup support. Don't know if it's going to be used, untested. */
	ldr	r0, =S5P6442_RST_STAT
	ldr	r1, [r0]
	bic	r1, r1, #0xfffffff7
	cmp	r1, #0x8
	beq	wakeup_reset
#endif

1:
	mov	lr, r9
	mov	pc, lr

#ifdef CONFIG_PRELOADER
wakeup_reset:

	/* Clear wakeup status register */
	ldr	r0, =S5P6442_WAKEUP_STAT
	ldr	r1, [r0]
	str	r1, [r0]

	/* Load return address and jump to kernel */
	ldr	r0, =S5P6442_INFORM0

	/* r1 = physical address of s5p6442_cpu_resume function */
	ldr	r1, [r0]

	/* Jump to kernel (sleep.S) */
	mov	pc, r1
	nop
	nop
#endif

/*
 * system_clock_init: Initialize core clock and bus clock.
 * void system_clock_init(void)
 */
system_clock_init:
	ldr	r8, =S5P64XX_CLOCK_BASE		@ 0xE0100000

	/* Set Lock Time */
	ldr	r1, =0xe10			@ Locktime : 0xe10 = 3600
	str	r1, [r8, #0x000]		@ APLL_LOCK
	str	r1, [r8, #0x008]		@ MPLL_LOCK
	str	r1, [r8, #0x010]		@ EPLL_LOCK
	str	r1, [r8, #0x020]		@ VPLL_LOCK

	/* APLL_CON */
	ldr	r1, =0x829b0c01		@ SDIV 1, PDIV 0xc, MDIV 0x29b
	str	r1, [r8, #0x100]
	/* MPLL_CON */
	ldr	r1, =0x810a0303		@ SDIV 3, PDIV 3, MDIV 0x10a
	str	r1, [r8, #0x108]
	/* EPLL_CON */
	ldr	r1, =0x80600303		@ SDIV 3, PDIV 3, MDIV 0x60
	str	r1, [r8, #0x110]
	/* VPLL_CON */
	ldr	r1, =0x806c0303		@ SDIV 3, PDIV 3, MDIV 0x6c
	str	r1, [r8, #0x120]

	/* Set Source Clock */
	ldr	r1, =0x1111			@ A, M, E, VPLL Muxing
	str	r1, [r8, #0x200]		@ CLK_SRC0

	/* Set Clock divider */
	ldr	r1, =0x10100000
	str	r1, [r8, #0x300]

	/* wait at least 200us to stablize all clock */
	mov	r2, #0x10000
1:	subs	r2, r2, #1
	bne	1b

	mov	pc, lr

#ifndef CONFIG_PRELOADER
/*
 * uart_asm_init: Initialize UART's pins
 */
uart_asm_init:
	mov	r0, r8
	ldr	r1, =0x22222222
	str	r1, [r0, #0x0]			@ GPA0_CON
	ldr	r1, =0x00000022
	str	r1, [r0, #0x20]			@ GPA1_CON

	mov	pc, lr
#endif
