/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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

#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/arch/power.h>
#include <mobile/misc.h>


void board_inform_set(unsigned int flag)
{
	unsigned int inform = readl(CONFIG_INFORM_ADDRESS);

	/* at watchdog reset */
	if (flag == 0)
		writel(0, CONFIG_INFORM_ADDRESS);

	inform = (REBOOT_MODE_PREFIX | flag);
	writel(inform, CONFIG_INFORM_ADDRESS);
}

void board_inform_clear(unsigned int flag)
{
	/* clear INFORM regardless of flags */
	writel(0, CONFIG_INFORM_ADDRESS);
	/* for normal boot */
	writel(0x12345678, CONFIG_LPM_INFORM);
}
