/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Minkyu Kang <mk7.kang@samsung.com>
 * Sanghee Kim <sh0130.kim@samsung.com>
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

#ifndef __ASM_ARCH_CPU_H
#define __ASM_ARCH_CPU_H

#define EXYNOS4_ADDR_BASE	0x10000000

/* EXYNOS4 */
#define EXYNOS4_GPIO_PART3_BASE	0x03860000
#define EXYNOS4_PRO_ID		0x10000000
#define EXYNOS4_SYSREG_BASE	0x10010000
#define EXYNOS4_POWER_BASE	0x10020000
#define EXYNOS4_INFORM_BASE	0x10020800
#define EXYNOS4_CLOCK_BASE	0x10030000
#define EXYNOS4_MCT_BASE	0x10050000
#define EXYNOS4_WDT_BASE	0x10060000
#define EXYNOS4_SECKEY_BASE	0x10100000
#if defined(CONFIG_EXYNOS4210)
#define EXYNOS4_DMC0_BASE	0x10400000
#define EXYNOS4_DMC1_BASE	0x10410000
#else
#define EXYNOS4_DMC0_BASE	0x10600000
#define EXYNOS4_DMC1_BASE	0x10610000
#endif
#define EXYNOS4_GPIO_PART2_BASE	0x11000000
#define EXYNOS4_GPIO_PART1_BASE	0x11400000
#define EXYNOS4_FIMD_BASE	0x11C00000
#define EXYNOS4_MIPI_DSIM_BASE	0x11C80000
#define EXYNOS4_USBOTG_BASE	0x12480000
#define EXYNOS4_MMC_BASE	0x12510000
#define EXYNOS4_SROMC_BASE	0x12570000
#define EXYNOS4_USBPHY_BASE	0x125B0000
#define EXYNOS4_UART_BASE	0x13800000
#if defined(CONFIG_EXYNOS4210)
#define EXYNOS4_ADC_BASE		0x13910000
#else
#define EXYNOS4_ADC_BASE	0x12150000
#endif
#define EXYNOS4_PWMTIMER_BASE	0x139D0000

/*
 * SYSREG
 */

#define EXYNOS4_LCDBLK_CFG	(EXYNOS4_SYSREG_BASE + 0x0210)

/*
 * POWER
 */

#define EXYNOS4_SWRESET		(EXYNOS4_POWER_BASE + 0x400)
#define EXYNOS4_RST_STAT	(EXYNOS4_POWER_BASE + 0x404)

#define S5P_SWRESET		EXYNOS4_SWRESET
#define S5P_RST_STAT		EXYNOS4_RST_STAT

#ifndef __ASSEMBLY__
#include <asm/io.h>

/* CPU detection macros */
extern unsigned int s5p_cpu_id;
extern unsigned int s5p_cpu_rev;

static inline int s5p_get_cpu_rev(void)
{
	return s5p_cpu_rev;
}

static inline void s5p_set_cpu_rev(unsigned int rev)
{
	s5p_cpu_rev = rev;
}

static inline void s5p_set_cpu_id(void)
{
	/*
	 * PRO_ID[31:16] = Product ID
	 * 0x4320: EXYNOS4210 EVT0
	 * 0x4321: EXYNOS4210 EVT1
	 * 0x4322: EXYNOS4212
	 * 0xE441: EXYNOS4412
	 */
	s5p_cpu_id = readl(EXYNOS4_PRO_ID) >> 16;

	switch (s5p_cpu_id) {
	case 0x4320:
		s5p_cpu_id |= 0x1;
		s5p_set_cpu_rev(0);
		break;
	case 0x4321:
		s5p_set_cpu_rev(1);
		break;
	case 0x4322:
	case 0xE441:
	default:
		/* PRO_ID[7:0] = MainRev + SubRev */
		s5p_set_cpu_rev(readl(EXYNOS4_PRO_ID) & 0xff);
	}
}

static inline int cpu_is_exynos4(void)
{
	switch (s5p_cpu_id) {
	case 0x4321:
	case 0x4322:
	case 0xE441:
		return 1;
	default:
		return 0;
	}
}

#define IS_SAMSUNG_TYPE(type, id)			\
static inline int cpu_is_##type(void)			\
{							\
	return s5p_cpu_id == id ? 1 : 0;		\
}

IS_SAMSUNG_TYPE(exynos4210, 0x4321)
IS_SAMSUNG_TYPE(exynos4212, 0x4322)
IS_SAMSUNG_TYPE(exynos4412, 0xE441)

#define SAMSUNG_BASE(device, base)				\
static inline unsigned int samsung_get_base_##device(void)	\
{								\
	if (cpu_is_exynos4())					\
		return EXYNOS4_##base;				\
	else							\
		return 0;					\
}

SAMSUNG_BASE(power, POWER_BASE)
SAMSUNG_BASE(adc, ADC_BASE)
SAMSUNG_BASE(clock, CLOCK_BASE)
SAMSUNG_BASE(fimd, FIMD_BASE)
SAMSUNG_BASE(mipi_dsim, MIPI_DSIM_BASE)
SAMSUNG_BASE(gpio_part1, GPIO_PART1_BASE)
SAMSUNG_BASE(gpio_part2, GPIO_PART2_BASE)
SAMSUNG_BASE(gpio_part3, GPIO_PART3_BASE)
SAMSUNG_BASE(pro_id, PRO_ID)
SAMSUNG_BASE(mmc, MMC_BASE)
SAMSUNG_BASE(sromc, SROMC_BASE)
SAMSUNG_BASE(swreset, SWRESET)
SAMSUNG_BASE(timer, PWMTIMER_BASE)
SAMSUNG_BASE(uart, UART_BASE)
SAMSUNG_BASE(usb_phy, USBPHY_BASE)
SAMSUNG_BASE(usb_otg, USBOTG_BASE)
SAMSUNG_BASE(systimer, MCT_BASE)
SAMSUNG_BASE(watchdog, WDT_BASE)
#endif

#endif	/* __ASM_ARCH_CPU_H */
