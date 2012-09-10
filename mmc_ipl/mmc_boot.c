/*
 * Copyright (c) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * Derived from onenand_ipl
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

#include <common.h>
#include "mmc_ipl.h"

typedef int (init_fnc_t)(void);

void start_mmcboot(void)
{
	uchar *buf;

	if (mmc_init())
		hang();

	buf = (uchar *)CONFIG_SYS_LOAD_ADDR;

	mmc_read_block(buf);

	((init_fnc_t *)CONFIG_SYS_LOAD_ADDR)();

	/* should never come here */
}

void hang(void)
{
       for (;;);
}
