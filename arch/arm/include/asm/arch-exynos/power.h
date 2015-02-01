/*
 * Copyright (c) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
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
 *
 */

#ifndef __ASM_ARM_ARCH_POWER_H_
#define __ASM_ARM_ARCH_POWER_H_

/* 
 * Power control
 */
#define EXYNOS4_MIPI_PHY0_CONTROL	0x10020710
#define EXYNOS4_MIPI_PHY1_CONTROL	0x10020714
#define S5P_MIPI_PHY_ENABLE		(1 << 0)
#define S5P_MIPI_PHY_SRESETN		(1 << 1)
#define S5P_MIPI_PHY_MRESETN		(1 << 2)

#ifndef __ASSEMBLY__

/* RESET */

enum reset_status {
	EXTRESET,
	WARMRESET,
	WDTRESET,
	SWRESET,
};

#define RESET_SW		(0x1 << 29)
#define RESET_WARM		(0x1 << 28)
#define RESET_WDT		(0x1 << 20)
#define RESET_PIN		(0x1 << 16)

static inline unsigned int get_reset_status(void)
{
	unsigned int val;

	val = readl(EXYNOS4_POWER_BASE + 0x404);

	if (val & RESET_SW)
		return SWRESET;
	else if (val & RESET_WARM)
		return WARMRESET;
	else if (val & RESET_WDT)
		return WDTRESET;
	else if (val & RESET_PIN)
		return EXTRESET;

	return val;
}

#define EXYNOS4_PS_HOLD		(EXYNOS4_POWER_BASE + 0x330C)

static inline void power_off(void)
{
	unsigned int val;

	val = readl(EXYNOS4_PS_HOLD);
	val |= (1 << 9);	/* set output */
	val &= ~(1 << 8);	/* set low */
	writel(val, EXYNOS4_PS_HOLD);

	while(1);
	/*NOTREACHED*/
}

#endif

#endif
