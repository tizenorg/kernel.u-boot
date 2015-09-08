/*
 * Lowlevel setup for MIDAS board based on EXYNOS4
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Kyungmin Park <kyungmin.park@samsung.com>
 * Sanghee Kim <sh0130.kim@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

/*
 * Register usages:
 *
 * r5 has zero always
 * r7 has GPIO part1 base 0x11400000
 * r6 has GPIO part2 base 0x11000000
 */

	.globl lowlevel_init
lowlevel_init:
	mov	r11, lr

	/* r5 has always zero */
	mov	r5, #0

	ldr	r7, =EXYNOS4_GPIO_PART1_BASE
	ldr	r6, =EXYNOS4_GPIO_PART2_BASE

	/* System Timer */
	ldr	r0, =EXYNOS4_MCT_BASE
	ldr	r1, =0x100
	str	r1, [r0, #0x240]

	/* Workaround: PMIC manual reset */
	/* nPOWER: XEINT_23: GPX2[7] */
	add	r0, r6, #0xC40			@ EXYNOS4_GPIO_X2_OFFSET
	ldr	r1, [r0, #0x0]
	bic	r1, r1, #(0xf << 28)		@ 28 = 7 * 4-bit
	orr	r1, r1, #(0x1 << 28)		@ Output
	str	r1, [r0, #0x0]

	ldr	r1, [r0, #0x4]
	orr	r1, r1, #(1 << 7)		@ 7 = 7 * 1-bit
	str	r1, [r0, #0x4]

	/* init system clock */
	bl	system_clock_init

	/* Disable Watchdog */
	ldr	r0, =EXYNOS4_WDT_BASE		@0x10060000
	str	r5, [r0]

	/* UART */
	bl	uart_asm_init

	/* PMU init */
	bl	system_power_init

/*	bl	tzpc_init
*/

	mov	lr, r11
	mov	pc, lr
	nop
	nop
	nop

/*
 * uart_asm_init: Initialize UART's pins
 */
uart_asm_init:
	/*
	 * setup UART0-UART4 GPIOs (part1)
	 * GPA1CON[3] = I2C_3_SCL (3)
	 * GPA1CON[2] = I2C_3_SDA (3)
	 */
	mov	r0, r7
	ldr	r1, =0x22222222
	str	r1, [r0, #0x00]			@ EXYNOS4_GPIO_A0_OFFSET
	ldr	r1, =0x00223322
	str	r1, [r0, #0x20]			@ EXYNOS4_GPIO_A1_OFFSET

	/* UART_SEL GPY4[7] (part2) */
	add	r0, r6, #0x1A0			@ EXYNOS4_GPIO_Y4_OFFSET
	ldr	r1, [r0, #0x0]
	bic	r1, r1, #(0xf << 28)		@ 28 = 7 * 4-bit
	orr	r1, r1, #(0x1 << 28)
	str	r1, [r0, #0x0]

	ldr	r1, [r0, #0x8]
	bic	r1, r1, #(0x3 << 14)		@ 14 = 7 * 2-bit
	orr	r1, r1, #(0x3 << 14)		@ Pull-up enabled
	str	r1, [r0, #0x8]

	ldr	r1, [r0, #0x4]
	orr	r1, r1, #(1 << 7)		@ 7 = 7 * 1-bit
	str	r1, [r0, #0x4]

#if 0
	ldr	r0, =EXYNOS4_UART_BASE
	orr     r0, r0, #0x20000		@ UART2
	mov     r1, #0x3
	str     r1, [r0, #0x000]                @ ULCON
	ldr     r1, =0x245
	str     r1, [r0, #0x004]                @ UCON
	mov     r1, #0x35
	str     r1, [r0, #0x028]                @ UBRDIV
	mov     r1, #0x4
	str     r1, [r0, #0x02C]                @ UFRACVAL

	mov     r2, #'W'
	strb    r2, [r0, #0x020]                @ UTXH
1001:
	ldrb    r3, [r0, #0x010]                @ UTRSTAT
	tst	r3, #(1 << 2)
	beq     1001b
#endif

	mov	pc, lr
	nop
	nop
	nop
	.align	5

system_clock_init:
	ldr	r0, =EXYNOS4_CLOCK_BASE

	/* CMU_CPU MUX / DIV */
	ldr	r1, =0x01000000
	ldr	r2, =0x14200			@ CLK_SRC_CPU
	str	r1, [r0, r2]
	/* wait ?us */
	mov	r1, #0x10000
1:	subs	r1, r1, #1
	bne	1b

	/* CMU_DMC MUX / DIV */
	ldr	r1, =0x00011000
	ldr	r2, =0x10200			@ CLK_SRC_DMC
	str	r1, [r0, r2]
	/* wait ?us */
	mov	r1, #0x10000
2:	subs	r1, r1, #1
	bne	2b
	ldr	r1, =0x00111113
	ldr	r2, =0x10500			@ CLK_DIV_DMC0
	str	r1, [r0, r2]
	ldr	r1, =0x05051013
	ldr	r2, =0x10504			@ CLK_DIV_DMC1
	str	r1, [r0, r2]

	/* CMU_TOP MUX / DIV */
	ldr	r1, =0x00000000
	ldr	r2, =0x0C210			@ CLK_SRC_TOP0
	str	r1, [r0, r2]
	ldr	r1, =0x00001000
	ldr	r2, =0x0C214			@ CLK_SRC_TOP1
	str	r1, [r0, r2]
	/* wait ?us */
	mov	r1, #0x10000
3:	subs	r1, r1, #1
	bne	3b
	ldr	r1, =0x01205474
	ldr	r2, =0x0C510			@ CLK_DIV_TOP
	str	r1, [r0, r2]

	/* CMU_LEFTBUS MUX / DIV */
	ldr	r1, =0x10
	ldr	r2, =0x04200			@ CLK_SRC_LEFTBUS
	str	r1, [r0, r2]
	/* wait ?us */
	mov	r1, #0x10000
4:	subs	r1, r1, #1
	bne	4b
	ldr	r1, =0x13
	ldr	r2, =0x04500			@ CLK_DIV_LEFTBUS
	str	r1, [r0, r2]

	/* CMU_RIGHTBUS MUX / DIV */
	ldr	r1, =0x10
	ldr	r2, =0x08200			@ CLK_SRC_RIGHTBUS
	str	r1, [r0, r2]
	/* wait ?us */
	mov	r1, #0x10000
5:	subs	r1, r1, #1
	bne	5b
	ldr	r1, =0x13
	ldr	r2, =0x08500			@ CLK_DIV_RIGHTBUS
	str	r1, [r0, r2]

	/* MMC[0:1] */
	ldr	r1, =0x000f000f			/* 800(MPLL) / (15 + 1) */
	ldr	r2, =0x0C544			@ CLK_DIV_FSYS1
	str	r1, [r0, r2]
	/* wait for CLK_DIV_STAT_FSYS1 */
	ldr	r2, =0x0C644
	ldr	r3, =0x01010101	/* On changing bits */
6:	ldr	r1, [r0, r2]
	and	r1, r1, r3
	cmp	r1, #0
	bne	6b

	/* MMC[2:3] */
	ldr	r1, =0x000f000f			/* 800(MPLL) / (15 + 1) */
	ldr	r2, =0x0C548			@ CLK_DIV_FSYS2
	str	r1, [r0, r2]
	/* wait for CLK_DIV_STAT_FSYS2 */
	ldr	r2, =0x0C648
	ldr	r3, =0x01010101	/* On changing bits */
7:	ldr	r1, [r0, r2]
	and	r1, r1, r3
	cmp	r1, #0
	bne	7b

	/* MMC4 */
	ldr	r1, =0x0003			/* 800(MPLL) / (3 + 1) */
	ldr	r2, =0x0C54C			@ CLK_DIV_FSYS3
	str	r1, [r0, r2]
	/* wait for CLK_DIV_STAT_FSYS3 */
	ldr	r2, =0x0C64C
	ldr	r3, =0x00000101	/* On changing bits */
8:	ldr	r1, [r0, r2]
	and	r1, r1, r3
	cmp	r1, #0
	bne	8b

	/* UART[0:5] */
	ldr	r1, =0x774777
	ldr	r2, =0x0C550			@ CLK_DIV_PERIL0
	str	r1, [r0, r2]
	/* wait for CLK_DIV_STAT_PERIL0 */
	ldr	r2, =0x0C650
	ldr	r3, =0x00111111	/* On changing bits */
9:	ldr	r1, [r0, r2]
	and	r1, r1, r3
	cmp	r1, #0
	bne	9b

	/* Set PLL locktime */
	ldr	r1, =0x03E8
	ldr	r2, =0x14000			@ APLL_LOCK
	str	r1, [r0, r2]
	ldr	r1, =0x02F1
	ldr	r2, =0x14008			@ MPLL_LOCK
	str	r1, [r0, r2]
	ldr	r1, =0x2321
	ldr	r2, =0x0C010			@ EPLL_LOCK
	str	r1, [r0, r2]
	ldr	r1, =0x2321
	ldr	r2, =0x0C020			@ VPLL_LOCK
	str	r1, [r0, r2]

/* FIXME: check this */
	/* SATA: SCLKMPLL(0), MMC[0:4]: SCLKMPLL(6) */
	ldr	r1, =0x0066666
	ldr	r2, =0x0C240			@ CLK_SRC_FSYS
	str	r1, [r0, r2]
	/* UART[0:5], PWM: SCLKMPLL(6) */
	ldr	r1, =0x6666666
	ldr	r2, =0x0C250			@ CLK_SRC_PERIL0_OFFSET
	str	r1, [r0, r2]
	/* wait ?us */
	mov	r1, #0x10000
10:	subs	r1, r1, #1
	bne	10b
/* FIXME END */

#ifdef CONFIG_CLK_ARM_800
	ldr	r1, =0x00133730
	ldr	r2, =0x14500			@ CLK_DIV_CPU0
	str	r1, [r0, r2]
	ldr	r1, =0x00000003
	ldr	r2, =0x14504			@ CLK_DIV_CPU1
	str	r1, [r0, r2]
	/* APLL */
	ldr	r1, =0x00803800
	ldr	r2, =0x14104			@ APLL_CON1
	str	r1, [r0, r2]
	ldr	r1, =0x80640300			@ 100:3:0
	ldr	r2, =0x14100			@ APLL_CON0
	str	r1, [r0, r2]
#endif
#ifdef CONFIG_CLK_ARM_1000
	ldr	r1, =0x00143730
	ldr	r2, =0x14500			@ CLK_DIV_CPU0
	str	r1, [r0, r2]
	ldr	r1, =0x00000004
	ldr	r2, =0x14504			@ CLK_DIV_CPU1
	str	r1, [r0, r2]
	/* APLL */
	ldr	r1, =0x00803800
	ldr	r2, =0x14104			@ APLL_CON1
	str	r1, [r0, r2]
	ldr	r1, =0x807D0300			@ 125:3:0
	ldr	r2, =0x14100			@ APLL_CON0
	str	r1, [r0, r2]
#endif
#ifdef CONFIG_CLK_ARM_1200
	ldr	r1, =0x00143730
	ldr	r2, =0x14500			@ CLK_DIV_CPU0
	str	r1, [r0, r2]
	ldr	r1, =0x00000005
	ldr	r2, =0x14504			@ CLK_DIV_CPU1
	str	r1, [r0, r2]
	/* APLL */
	ldr	r1, =0x00803800
	ldr	r2, =0x14104			@ APLL_CON1
	str	r1, [r0, r2]
	ldr	r1, =0x80960300			@ 150:3:0
	ldr	r2, =0x14100			@ APLL_CON0
	str	r1, [r0, r2]
#endif

	/* check C2C_CTRL enable bit */
	ldr	r3, =EXYNOS4_POWER_BASE
	ldr	r1, [r3, #0x24]
	and	r1, r1, #1
	cmp	r1, #0
	bne	skip_mpll

#ifdef CONFIG_CLK_BUS_DMC_165_330
	/* MPLL */
	ldr	r1, =0x00803800
	ldr	r2, =0x1010C			@ MPLL_CON1
	str	r1, [r0, r2]
	ldr	r1, =0x81160501			@ 667MHz (278:5:1)
	ldr	r2, =0x10108			@ MPLL_CON0
	str	r1, [r0, r2]
#endif
#ifdef CONFIG_CLK_BUS_DMC_200_400
	/* MPLL */
	ldr	r1, =0x00803800
	ldr	r2, =0x1010C			@ MPLL_CON1
	str	r1, [r0, r2]
	ldr	r1, =0x80640300			@ 800MHz (100:3:0)
	ldr	r2, =0x10108			@ MPLL_CON0
	str	r1, [r0, r2]
#endif
skip_mpll:

	/* EPLL */
	ldr	r1, =0x00000080
	ldr	r2, =0x0C118			@ EPLL_CON2
	str	r1, [r0, r2]
	ldr	r1, =0x66010000
	ldr	r2, =0x0C114			@ EPLL_CON1
	str	r1, [r0, r2]
	ldr	r1, =0x80400203			@ 96MHz (64:2:3)
	ldr	r2, =0x0C110			@ EPLL_CON0
	str	r1, [r0, r2]

	/* VPLL */
	ldr	r1, =0x00000080
	ldr	r2, =0x0C128			@ VPLL_CON2
	str	r1, [r0, r2]
	ldr	r1, =0x66010000
	ldr	r2, =0x0C124			@ VPLL_CON1
	str	r1, [r0, r2]
	ldr	r1, =0x80480203			@ 108MHz (72:2:3)
	ldr	r2, =0x0C120			@ VPLL_CON0
	str	r1, [r0, r2]

	/* wait ?us */
	mov	r1, #0x40000
11:	subs	r1, r1, #1
	bne	11b

	ldr	r1, =0x01000001
	ldr	r2, =0x14200			@ CLK_SRC_CPU
	str	r1, [r0, r2]

	ldr	r1, =0x00011000
	ldr	r2, =0x10200			@ CLK_SRC_DMC
	str     r1, [r0, r2]

	ldr	r1, =0x00000110
	ldr	r2, =0x0C210			@ CLK_SRC_TOP0
	str	r1, [r0, r2]
	ldr	r1, =0x01111000
	ldr	r2, =0x0C214			@ CLK_SRC_TOP1
	str	r1, [r0, r2]

	/* wait ?us */
	mov	r1, #0x10000
12:	subs	r1, r1, #1
	bne	12b

#if 0
	/*
	 * SMMUJPEG[11], JPEG[6], CSIS1[5]		: 0111 1001
	 * Turn off all
	 */
	ldr	r1, =0xFFF80000
	ldr	r2, =0x0C920			@ CLK_GATE_IP_CAM
	str	r1, [r0, r2]

	/* Turn off all */
	ldr	r1, =0xFFFFFFC0
	ldr	r2, =0x0C924			@ CLK_GATE_IP_VP
	str	r1, [r0, r2]

	/* Turn off all */
	ldr	r1, =0xFFFFFFFE
	ldr	r2, =0x0C928			@ CLK_GATE_IP_MFC
	str	r1, [r0, r2]

	/* Turn off all */
	ldr	r1, =0xFFFFFFFE
	ldr	r2, =0x0C92C			@ CLK_GATE_IP_G3D
	str	r1, [r0, r2]

	/* Turn off all */
	ldr	r1, =0xFFFFFC00
	ldr	r2, =0x0C930			@ CLK_GATE_IP_IMAGE
	str	r1, [r0, r2]

	/* DSIM0[3], MDNIE0[2], MIE0[1]			: 0001 */
	ldr	r1, =0xFFFFFFF1
	ldr	r2, =0x0C934			@ CLK_GATE_IP_LCD0
	str	r1, [r0, r2]

	/* Turn off all */
	ldr	r1, =0xFFFFFFC0
	ldr	r2, =0x0C938			@ CLK_GATE_IP_LCD1
	str	r1, [r0, r2]

	/*
	 * SMMUPCIE[18], NFCON[16]			: 1111 1010
	 * ONENAND[15], PCIE[14], SATA[10], SDMMC43[9:8]: 0011 1000
	 * SDMMC1[6], TSI[4], SATAPHY[3], PCIEPHY[2]	: 1010 0011
	 */
	ldr	r1, =0xFFFA38A3
	ldr	r2, =0x0C940			@ CLK_GATE_IP_FSYS
	str	r1, [r0, r2]

	/* Turn off all */
	ldr	r1, =0xFFFFFFFE
	ldr	r2, =0x0C94C			@ CLK_GATE_IP_GPS
	str	r1, [r0, r2]

	/*
	 * AC97[27], SPDIF[26], SLIMBUS[25]		: 1111 0001
	 * I2C2[8]					: 1111 1110
	 */
	ldr	r1, =0xF1FFFEFF
	ldr	r2, =0x0C950			@ CLK_GATE_IP_PERIL
	str	r1, [r0, r2]

	/*
	 * KEYIF[16]					: 1111 1110
	 */
	ldr	r1, =0xFFFEFFFF
	ldr	r2, =0x0C960			@ CLK_GATE_IP_PERIR
	str	r1, [r0, r2]

	/* GPS[7], LCD1[5], G3D[3], MFC[2], TV[1]	: 0101 0001 */
	ldr	r1, =0xFFFFFF51
	ldr	r2, =0x0C970			@ CLK_GATE_BLOCK
	str	r1, [r0, r2]
#endif
	mov	pc, lr
	nop
	nop
	nop

system_power_init:
	ldr	r0, =EXYNOS4_POWER_BASE		@ 0x10020000

	ldr	r2, =0x330C			@ PS_HOLD_CONTROL
	ldr	r1, [r0, r2]
	orr	r1, r1, #(0x3 << 8)		@ Data High, Output En
	str	r1, [r0, r2]

	mov	pc, lr
	nop
	nop
	nop

tzpc_init:
	ldr	r0, =0x10110000
	mov	r1, #0x0
	str	r1, [r0]
	mov	r1, #0xff
	str	r1, [r0, #0x0804]
	str	r1, [r0, #0x0810]
	str	r1, [r0, #0x081C]
	str	r1, [r0, #0x0828]

	ldr	r0, =0x10120000
	str	r1, [r0, #0x0804]
	str	r1, [r0, #0x0810]
	str	r1, [r0, #0x081C]
	str	r1, [r0, #0x0828]

	ldr	r0, =0x10130000
	str	r1, [r0, #0x0804]
	str	r1, [r0, #0x0810]
	str	r1, [r0, #0x081C]
	str	r1, [r0, #0x0828]

	ldr	r0, =0x10140000
	str	r1, [r0, #0x0804]
	str	r1, [r0, #0x0810]
	str	r1, [r0, #0x081C]
	str	r1, [r0, #0x0828]

	ldr	r0, =0x10150000
	str	r1, [r0, #0x0804]
	str	r1, [r0, #0x0810]
	str	r1, [r0, #0x081C]
	str	r1, [r0, #0x0828]

	mov	pc, lr
