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

#ifndef __ASSEMBLY__
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

static int get_reset_status(void)
{
	unsigned int val;

	val = readl(S5PC210_POWER_BASE + 0x404);

	if (val & RESET_SW)
		return SWRESET;
	else if (val & RESET_WARM)
		return WARMRESET;
	else if (val & RESET_WDT)
		return WDTRESET;
	else if (val & RESET_PIN)
		return EXTRESET;

	return -1;
}
#endif

#endif
