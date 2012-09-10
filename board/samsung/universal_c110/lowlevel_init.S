/*
 * Memory Setup stuff - taken from blob memsetup.S
 *
 * Copyright (C) 2009 Samsung Electronics
 * Kyungmin Park <kyungmin.park@samsung.com>
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
#include <asm/arch/clock.h>
#include <asm/arch/power.h>

#define DEBUG_PM_C110
#undef DEBUG_PM_C110

/*
 * Register usages:
 *
 * r5 has zero always
 * r7 has S5PC100 GPIO base, 0xE0300000
 * r6 has real GPIO base, 0xE0300000, 0xE0200000 at S5PC100, S5PC110 repectively
 * r9 has Mobile DDR size, 1 means 1GiB, 2 means 2GiB and so on
 */

	.globl lowlevel_init
lowlevel_init:
	mov	r11, lr

	/* r5 has always zero */
	mov	r5, #0

	ldr	r7, =S5PC100_GPIO_BASE
	ldr	r6, =S5PC100_GPIO_BASE
	/* Read CPU ID */
	ldr	r2, =S5PC100_PRO_ID
	ldr	r0, [r2]
	mov	r1, #0x00010000
	and	r0, r0, r1
	cmp	r0, r5
	beq	100f
	ldr	r6, =S5PC110_GPIO_BASE
100:
	/* Turn on KEY_LED_ON [GPJ4(1)] XMSMWEN */
	cmp	r7, r6
	beq	skip_check_didle			@Support C110 only

	ldr	r0, =S5PC110_RST_STAT
	ldr	r1, [r0]
	and	r1, r1, #S5PC110_RST_STAT_WAKEUP_MODE_MASK
	cmp	r1, #S5PC110_DEEPIDLE_WAKEUP
	beq	didle_wakeup
	cmp	r7, r6
skip_check_didle:

#ifndef DEBUG_PM_C110
	addeq	r0, r6, #0x280				@S5PC100_GPIO_J4_OFFSET
	addne	r0, r6, #0x2C0				@S5PC110_GPIO_J4_OFFSET
	ldr	r1, [r0, #0x0]				@S5PC1XX_GPIO_CON_OFFSET
	bic	r1, r1, #(0xf << 4)			@ 1 * 4-bit
	orr	r1, r1, #(0x1 << 4)
	str	r1, [r0, #0x0]				@S5PC1XX_GPIO_CON_OFFSET

	ldr	r1, [r0, #0x4]				@S5PC1XX_GPIO_DAT_OFFSET
#ifdef CONFIG_PRELOADER
	orr	r1, r1, #(1 << 1)			@ 1 * 1-bit
#else
	bic	r1, r1, #(1 << 1)
#endif
	str	r1, [r0, #0x4]				@S5PC1XX_GPIO_DAT_OFFSET
#endif

	/* Don't setup at s5pc100 */
	beq	100f

	/*
	 * Initialize Async Register Setting for EVT1
	 * Because we are setting EVT1 as the default value of EVT0,
	 * setting EVT0 as well does not make things worse.
	 * Thus, for the simplicity, we set for EVT0, too
	 *
	 * The "Async Registers" are:
	 *	0xE0F0_0000
	 *	0xE1F0_0000
	 *	0xF180_0000
	 *	0xF190_0000
	 *	0xF1A0_0000
	 *	0xF1B0_0000
	 *	0xF1C0_0000
	 *	0xF1D0_0000
	 *	0xF1E0_0000
	 *	0xF1F0_0000
	 *	0xFAF0_0000
	 */
	ldr     r0, =0xe0f00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xe1f00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xf1800000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xf1900000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xf1a00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xf1b00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xf1c00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xf1d00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xf1e00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xf1f00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	ldr     r0, =0xfaf00000
	ldr     r1, [r0]
	bic     r1, r1, #0x1
	str     r1, [r0]

	/*
	 * Diable ABB block to reduce sleep current at low temperature
	 * Note that it's hidden register setup don't modify it
	 */
	ldr	r0, =0xE010C300
	ldr	r1, =0x00800000
	str	r1, [r0]

100:
	/* IO retension release */
	ldreq	r0, =S5PC100_OTHERS			@0xE0108200
	ldrne	r0, =S5PC110_OTHERS			@0xE010E000
	ldr	r1, [r0]
	ldreq	r2, =(1 << 31)				@IO_RET_REL
	ldrne	r2, =((1 << 31) | (1 << 30) | (1 << 29) | (1 << 28))	@ GPIO, UART_IO
	orr	r1, r1, r2
	/* Do not release retention here for S5PC110 */
	streq	r1, [r0]

#ifndef CONFIG_PRELOADER
	/* Disable Watchdog */
	ldreq	r0, =S5PC100_WATCHDOG_BASE		@0xEA200000
	ldrne	r0, =S5PC110_WATCHDOG_BASE		@0xE2700000
	str	r5, [r0]

	/* setting SRAM */
	ldreq	r0, =S5PC100_SROMC_BASE
	ldrne	r0, =S5PC110_SROMC_BASE
	ldr	r1, =0x9
	str	r1, [r0]
#endif

	/* S5PC100 has 3 groups of interrupt sources */
	ldreq	r0, =S5PC100_VIC0_BASE			@0xE4000000
	ldrne	r0, =S5PC110_VIC0_BASE			@0xF2000000
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
	bl	uart_asm_init

	bl	internal_ram_init
#endif

#ifdef CONFIG_PRELOADER
	/* init system clock */
	bl	system_clock_init

	/* OneNAND Sync Read Support at S5PC110 only
	 * RM[15]	: Sync Read
	 * BRWL[14:12]	: 7 CLK
	 * BL[11:9]	: Continuous
	 * VHF[3]	: Very High Frequency Enable (Over 83MHz)
	 * HF[2]	: High Frequency Enable (Over 66MHz)
	 * WM[1]	: Sync Write
	 */
	cmp	r7, r6
	ldrne	r1, =0xE006
	ldrne	r0, =0xB001E442
	strneh	r1, [r0]

	/*
	 * GCE[26]	: Gated Clock Enable
	 * RPE[17]	: Enables Read Prefetch
	 */
	ldrne	r1, =((1 << 26) | (1 << 17) | 0xE006)
	ldrne	r0, =0xB0600000
	strne	r1, [r0, #0x100]			@ ONENAND_IF_CTRL
	ldrne	r1, =0x1212
	strne	r1, [r0, #0x108]

	/* Board detection to set proper memory configuration */
	cmp	r7, r6
	moveq	r9, #1		/* r9 has 1Gib default at s5pc100 */
	movne	r9, #2		/* r9 has 2Gib default at s5pc110 */

	/*
	 * Aquila Rev 0.3 : 4G2G1G x16 for Infineon ES3.0
	 * Aquila Rev 0.4 : 4G2G1G x16 for Infineon ES3.1 (same as Rev 0.7)
	 * Aquila Rev 0.5 : 4G3G1G x16 for Infineon ES3.1
	 * Aquila Rev 0.6 : 4G1G1G x32 for MSM6290
	 * Aquila Rev 0.7 : 4G2G1G x16 for Infineon ES3.1 (same as Rev 0.4)
	 * Aquila Rev 0.8 : 4G3G1G x16 for Infineon ES3.1
	 * Aquila Rev 0.9 : 4G3G1G x16 for Infineon ES3.1
	 */
	ldr	r2, =0xE0200200
	ldr	r4, [r2, #0x48]
	/*
	 * HWREV0[2], HWREV1[3], HWREV2[4]
	 */
	bic	r1, r4, #(0x3F << 4)	/* PULLUP_DISABLE: 3 * 2-bit */
	/* HWREV3[1] */
	bic	r1, r1, #(0x3 << 2)	/* PULLUP_DISABLE: 2 * 2-bit */
	/* HWREV3[7] */
	bic	r1, r1, #(0x3 << 14)	/* PULLUP_DISABLE: 2 * 2-bit */
	str	r1, [r2, #0x48]
	/* For write completion */
	nop
	nop

	ldr	r3, [r2, #0x44]
	and	r1, r3, #(0x7 << 2)	/* HWREV: 2 3 4 */
	mov	r1, r1, lsr #2
	cmp	r1, #0x5
	moveq	r9, #3
	cmp	r1, #0x6
	moveq	r9, #1
	cmp	r1, #0x7
	moveq	r9, #2
	and	r0, r3, #(0x1 << 1)	/* HWREV3[1] */
	mov	r0, r0, lsr #1
	orr	r1, r1, r0, lsl #3
	cmp	r1, #0x8		/* Rev 0.8 */
	moveq	r9, #3
	and	r1, r3, #(0x7 << 2)	/* HWREV: 2 3 4 */
	mov	r1, r1, lsr #2
	and	r0, r3, #(0x1 << 7)	/* HWREV3[7] */
	mov	r0, r0, lsr #7
	orr	r1, r1, r0, lsl #3
	cmp	r1, #0x9		/* Rev 0.9 */
	moveq	r9, #3
	str	r4, [r2, #0x48]		/* Restore PULLUP configuration */

#ifdef KESSLER_S1
	mov	r9, #2			/* Rev 0.9 */
#endif

	bl	mem_ctrl_asm_init

	/* Wakeup support. Don't know if it's going to be used, untested. */
	ldreq	r0, =S5PC100_RST_STAT
	ldrne	r0, =S5PC110_RST_STAT
	ldr	r1, [r0]
	biceq	r1, r1, #0xfffffff7
	moveq	r2, #(1 << 3)
	bicne	r1, r1, #0xfffeffff
	movne	r2, #(1 << 16)
	cmp	r1, r2
	bne	1f
wakeup:
#ifdef DEBUG_PM_C110
	mov	r0, r6
	ldr	r1, =0x22222222
	str	r1, [r0, #0x0]			@S5PC100_GPIO_A0_OFFSET
	ldr	r1, =0x00002222
	str	r1, [r0, #0x20]			@S5PC100_GPIO_A1_OFFSET

	/* UART_SEL MP0_5[7] at S5PC110 */
	add	r0, r6, #0x360			@S5PC110_GPIO_MP0_5_OFFSET
	ldr	r1, [r0, #0x0]			@S5PC1XX_GPIO_CON_OFFSET
	bic	r1, r1, #(0xf << 28)		@ 28 = 7 * 4-bit
	orr	r1, r1, #(0x1 << 28)		@ Output
	str	r1, [r0, #0x0]			@S5PC1XX_GPIO_CON_OFFSET

	ldr	r1, [r0, #0x8]			@S5PC1XX_GPIO_PULL_OFFSET
	bic	r1, r1, #(0x3 << 14)		@ 14 = 7 * 2-bit
	orr	r1, r1, #(0x2 << 14)		@ Pull-up enabled
	str	r1, [r0, #0x8]			@S5PC1XX_GPIO_PULL_OFFSET

	ldr	r1, [r0, #0x4]			@S5PC1XX_GPIO_DAT_OFFSET
	orr	r1, r1, #(1 << 7)		@ 7 = 7 * 1-bit
	str	r1, [r0, #0x4]			@S5PC1XX_GPIO_DAT_OFFSET

	ldr	r0, =0xE2900000			@ S5PC110_PA_UART
	orr	r0, r0, #0x800
	mov	r1, #0x3
	str	r1, [r0, #0x000]		@ ULCON
	ldr	r1, =0x245
	str	r1, [r0, #0x004]		@ UCON
	mov	r1, #0x23
	str	r1, [r0, #0x028]		@ UBRDIV
	mov	r1, #0x3
	str	r1, [r0, #0x02C]		@ UDIVSLOT

	mov	r2, #'W'
	strb	r2, [r0, #0x020]		@ UTXH
1001:
	ldrb	r3, [r0, #0x010]		@ UTRSTAT
	tst	r3, #(1 << 2)
	beq	1001b
#endif

	/* turn off L2 cache */
	bl	l2_cache_disable

	cmp	r7, r6
	ldreq	r0, =0xC100
	ldrne	r0, =0xC110

	/* invalidate L2 cache also */
	bl	invalidate_dcache

	/* turn on L2 cache */
	bl	l2_cache_enable

	cmp	r7, r6
	/* Load return address and jump to kernel */
	ldreq	r0, =S5PC100_INFORM0
	ldrne	r0, =S5PC110_INFORM0

	/* r1 = physical address of s5pc1xx_cpu_resume function */
	ldr	r1, [r0]

	/* Jump to kernel (sleep-s5pc1xx.S) */
	mov	pc, r1
	nop
	nop
#else
	cmp	r7, r6
	/* Clear wakeup status register */
	ldreq	r0, =S5PC100_WAKEUP_STAT
	ldrne	r0, =S5PC110_WAKEUP_STAT
	ldr	r1, [r0]
	str	r1, [r0]

	/* IO retension release */
	ldreq	r0, =S5PC100_OTHERS			@0xE0108200
	ldrne	r0, =S5PC110_OTHERS			@0xE010E000
	ldr	r1, [r0]
	ldreq	r2, =(1 << 31)				@IO_RET_REL
	ldrne	r2, =((1 << 31) | (1 << 30) | (1 << 29) | (1 << 28))
	orr	r1, r1, r2
	str	r1, [r0]

#endif
	b	1f

didle_wakeup:
	/* Wait when APLL is locked */
	ldr	r0, =S5PC110_APLL_CON
lockloop:
	ldr	r1, [r0]
	and	r1, r1, #S5PC110_APLL_CON_LOCKED
	cmp	r1, #S5PC110_APLL_CON_LOCKED
	bne	lockloop

	ldr	r0, =S5PC110_INFORM0
	ldr	r1, [r0]
	mov	pc, r1
	nop
	nop
	nop
	nop
	nop


1:
	mov	lr, r11
	mov	pc, lr

/*
 * system_clock_init: Initialize core clock and bus clock.
 * void system_clock_init(void)
 */
system_clock_init:
	ldr	r0, =S5PC100_CLOCK_BASE		@ 0xE0100000

	/* Check S5PC100 */
	cmp	r7, r6
	bne	110f
100:
#ifndef DEBUG_PM_C110
	/* Set Lock Time */
	ldr	r1, =0xe10			@ Locktime : 0xe10 = 3600
	str	r1, [r0, #0x000]		@ S5PC100_APLL_LOCK
	str	r1, [r0, #0x004]		@ S5PC100_MPLL_LOCK
	str	r1, [r0, #0x008]		@ S5PC100_EPLL_LOCK
	str	r1, [r0, #0x00C]		@ S5PC100_HPLL_LOCK

	/* S5P_APLL_CON */
#ifdef CONFIG_CLK_667_166_83
	ldr	r1, =0x81bc0400		@ SDIV 0, PDIV 4, MDIV 444 (1333MHz)
#elif defined(CONFIG_CLK_666_166_66)
	ldr	r1, =0x814d0301		@ SDIV 1, PDIV 3, MDIV 333 (1333MHz)
#elif defined(CONFIG_CLK_600_150_75)
	ldr	r1, =0x812C0300		@ SDIV 0, PDIV 3, MDIV 300 (1200MHz)
#elif defined(CONFIG_CLK_533_133_66)
	ldr	r1, =0x810b0300		@ SDIV 0, PDIV 3, MDIV 267 (1066MHz)
#elif defined(CONFIG_CLK_500_166_66)
	ldr	r1, =0x81f40301		@ SDIV 1, PDIV 3, MDIV 500 (1000MHz)
#elif defined(CONFIG_CLK_467_117_59)
	ldr	r1, =0x826E0401		@ SDIV 1, PDIV 4, MDIV 622 (933MHz)
#elif defined(CONFIG_CLK_400_100_50)
	ldr	r1, =0x81900301		@ SDIV 1, PDIV 3, MDIV 400 (800MHz)
#else
#error you should set the correct clock configuration
#endif
	str	r1, [r0, #0x100]
	/* S5P_MPLL_CON */
	ldr	r1, =0x80590201		@ SDIV 1, PDIV 2, MDIV 89 (267MHz)
	str	r1, [r0, #0x104]
	/* S5P_EPLL_CON */
	ldr	r1, =0x80870303		@ SDIV 3, PDIV 3, MDIV 135 (67.5MHz)
	str	r1, [r0, #0x108]
	/* S5P_HPLL_CON */
	ldr	r1, =0x80600603		@ SDIV 3, PDIV 6, MDIV 96
	str	r1, [r0, #0x10C]

	ldr     r1, [r0, #0x300]
	ldr     r2, =0x00003fff
	bic     r1, r1, r2
#ifdef CONFIG_CLK_800_166_66
	ldr     r2, =0x00011401
#elif defined(CONFIG_CLK_500_166_66)
	ldr     r2, =0x00011201
#elif defined(CONFIG_CLK_666_166_66)
	ldr     r2, =0x00011300
#else
	ldr     r2, =0x00011301
#endif
	orr	r1, r1, r2
	str	r1, [r0, #0x300]
	ldr     r1, [r0, #0x304]
	ldr     r2, =0x00011110
	orr     r1, r1, r2
	str     r1, [r0, #0x304]
	ldr     r1, =0x00000001
	str     r1, [r0, #0x308]

	/* Set Source Clock */
	ldr	r1, =0x00001111			@ A, M, E, HPLL Muxing
	str	r1, [r0, #0x200]		@ S5PC1XX_CLK_SRC0
#endif
	b	200f
110:
	ldr	r0, =S5PC110_PWR_CFG		@ 0xE010C000

	/* Set OSC_FREQ value */
	ldr	r1, =0xf
	str	r1, [r0, #0x100]		@ S5PC110_OSC_FREQ

	/* Set MTC_STABLE value */
	ldr	r1, =0xffffffff
	str	r1, [r0, #0x110]		@ S5PC110_MTC_STABLE

	/* Set CLAMP_STABLE value */
	ldr	r1, =0x3ff03ff
	str	r1, [r0, #0x114]		@ S5PC110_CLAMP_STABLE

	ldr	r0, =S5PC100_CLOCK_BASE		@ 0xE0100000

	/* Set Clock divider */
	ldr	r1, =0x14131330			@ 1:1:4:4, 1:4:5
	str	r1, [r0, #0x300]
	ldr	r1, =0x11110111			@ UART[3210]: MMC[3210]
	str	r1, [r0, #0x310]

	/* Set Lock Time */
	ldr	r1, =0x2cf			@ Locktime : 30us
	str	r1, [r0, #0x000]		@ S5PC110_APLL_LOCK
	ldr	r1, =0xe10			@ Locktime : 0xe10 = 3600
	str	r1, [r0, #0x008]		@ S5PC110_MPLL_LOCK
	str	r1, [r0, #0x010]		@ S5PC110_EPLL_LOCK
	str	r1, [r0, #0x020]		@ S5PC110_VPLL_LOCK

	/* S5PC110_APLL_CON */
	ldr	r1, =0x80C80601			@ 800MHz
	str	r1, [r0, #0x100]
	/* S5PC110_MPLL_CON */
	ldr	r1, =0x829B0C01			@ 667MHz
	str	r1, [r0, #0x108]
	/* S5PC110_EPLL_CON */
	ldr	r1, =0x80600602			@  96MHz VSEL 0 P 6 M 96 S 2
	str	r1, [r0, #0x110]
	/* S5PC110_VPLL_CON */
	ldr	r1, =0x806C0603			@  54MHz
	str	r1, [r0, #0x120]

	/* Set Source Clock */
	ldr	r1, =0x10001111			@ A, M, E, VPLL Muxing
	str	r1, [r0, #0x200]		@ S5PC1XX_CLK_SRC0

	ldr	r1, =0x66667777			@ S5PC110_CLK_SRC4 (UART/MMC)
	str	r1, [r0, #0x210]

	/* OneDRAM(DMC0) clock setting */
	ldr	r1, =0x01000000			@ ONEDRAM_SEL[25:24] 1 SCLKMPLL
	str	r1, [r0, #0x218]		@ S5PC110_CLK_SRC6
	ldr	r1, =0x30000000			@ ONEDRAM_RATIO[31:28] 3 + 1
	str	r1, [r0, #0x318]		@ S5PC110_CLK_DIV6

	/* XCLKOUT = XUSBXTI 24MHz */
	add	r2, r0, #0xE000			@ S5PC110_OTHERS
	ldr     r1, [r2]
	orr	r1, r1, #(0x3 << 8)		@ CLKOUT[9:8] 3 XUSBXTI
	str	r1, [r2]

	/* CLK_IP0 */
	ldr	r1, =0x8fefeeb			@ DMC[1:0] PDMA0[3] IMEM[5]
	str	r1, [r0, #0x460]		@ S5PC110_CLK_IP0

	/* CLK_IP1 */
	ldr	r1, =0xe9fdf0fd			@ FIMD[0] DSIM[2] USBOTG[16]
						@ NANDXL[24]
	str	r1, [r0, #0x464]		@ S5PC110_CLK_IP1

	/* CLK_IP2 */
	ldr	r1, =0xf75f7fc			@ CORESIGHT[8] MODEM[9]
						@ HOSTIF[10] HSMMC0[16]
						@ HSMMC2[18] VIC[27:24]
	str	r1, [r0, #0x468]		@ S5PC110_CLK_IP2

	/* CLK_IP3 */
	ldr	r1, =0x8eff038c			@ I2C[8:6]
						@ SYSTIMER[16] UART0[17]
						@ UART1[18] UART2[19]
						@ UART3[20] WDT[22]
						@ PWM[23] GPIO[26] SYSCON[27]
	str	r1, [r0, #0x46c]		@ S5PC110_CLK_IP3

	/* CLK_IP4 */
	ldr	r1, =0xfffffff1			@ CHIP_ID[0] TZPC[8:5]
	str	r1, [r0, #0x470]		@ S5PC110_CLK_IP3

200:
	/* wait at least 200us to stablize all clock */
	mov	r2, #0x10000
1:	subs	r2, r2, #1
	bne	1b

	mov	pc, lr

#ifndef CONFIG_PRELOADER
internal_ram_init:
	ldreq	r0, =0xE3800000
	ldrne	r0, =0xF1500000
	ldr	r1, =0x0
	str	r1, [r0]

	mov	pc, lr
#endif

#ifndef CONFIG_PRELOADER
/*
 * uart_asm_init: Initialize UART's pins
 */
uart_asm_init:
	/* set GPIO to enable UART0-UART4 */
	mov	r0, r6
	ldr	r1, =0x22222222
	str	r1, [r0, #0x0]			@S5PC100_GPIO_A0_OFFSET
	ldr	r1, =0x00002222
	str	r1, [r0, #0x20]			@S5PC100_GPIO_A1_OFFSET

	/* Check S5PC100 */
	cmp	r7, r6
	bne	110f

#ifndef DEBUG_PM_C110
	/* UART_SEL GPK0[5] at S5PC100 */
	add	r0, r6, #0x2A0			@S5PC100_GPIO_K0_OFFSET
	ldr	r1, [r0, #0x0]			@S5PC1XX_GPIO_CON_OFFSET
	bic	r1, r1, #(0xf << 20)		@ 20 = 5 * 4-bit
	orr	r1, r1, #(0x1 << 20)		@ Output
	str	r1, [r0, #0x0]			@S5PC1XX_GPIO_CON_OFFSET

	ldr	r1, [r0, #0x8]			@S5PC1XX_GPIO_PULL_OFFSET
	bic	r1, r1, #(0x3 << 10)		@ 10 = 5 * 2-bit
	orr	r1, r1, #(0x2 << 10)		@ Pull-up enabled
	str	r1, [r0, #0x8]			@S5PC1XX_GPIO_PULL_OFFSET

	ldr	r1, [r0, #0x4]			@S5PC1XX_GPIO_DAT_OFFSET
	orr	r1, r1, #(1 << 5)		@ 5 = 5 * 1-bit
	str	r1, [r0, #0x4]			@S5PC1XX_GPIO_DAT_OFFSET
#endif
	b	200f
110:
	/*
	 * Note that the following address
	 * 0xE020'0360 is reserved address at S5PC100
	 */
	/* UART_SEL MP0_5[7] at S5PC110 */
	add	r0, r6, #0x360			@S5PC110_GPIO_MP0_5_OFFSET
	ldr	r1, [r0, #0x0]			@S5PC1XX_GPIO_CON_OFFSET
	bic	r1, r1, #(0xf << 28)		@ 28 = 7 * 4-bit
	orr	r1, r1, #(0x1 << 28)		@ Output
	str	r1, [r0, #0x0]			@S5PC1XX_GPIO_CON_OFFSET

	ldr	r1, [r0, #0x8]			@S5PC1XX_GPIO_PULL_OFFSET
	bic	r1, r1, #(0x3 << 14)		@ 14 = 7 * 2-bit
	orr	r1, r1, #(0x2 << 14)		@ Pull-up enabled
	str	r1, [r0, #0x8]			@S5PC1XX_GPIO_PULL_OFFSET

	ldr	r1, [r0, #0x4]			@S5PC1XX_GPIO_DAT_OFFSET
	orr	r1, r1, #(1 << 7)		@ 7 = 7 * 1-bit
	str	r1, [r0, #0x4]			@S5PC1XX_GPIO_DAT_OFFSET
200:
	mov	pc, lr
#endif
