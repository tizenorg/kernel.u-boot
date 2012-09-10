/*
 * Copyright (c) 2010 Samsung Electronics
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

#include "mmc_ipl.h"

static int __board_mmc_init(void)
{
	return 0;
}

int board_mmc_init(void) __attribute__((weak, alias("__board_mmc_init")));

int mmc_init(void)
{
	if (board_mmc_init())
		return 1;

	return 0;
}

void mmc_read_block(unsigned char *buf)
{
	board_mmc_read_block(buf);
}
