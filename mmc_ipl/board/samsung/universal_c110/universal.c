/*
 * Copyright (c) 2010 Samsung Electronics
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

#include <common.h>
#include <asm/io.h>

#include "mmc_ipl.h"

#define EMMC_COPY_TO_MEM_ADDR	0xD0037F9C

#define BLK_COUNT		512
#define BUS_WIDTH_8BIT		8

typedef u32(*copy_emmc_to_mem)
	(u32 ack, u32 number_of_block, u32 *buf, int buswidth);

int board_mmc_init(void)
{
	unsigned int value;

	/* MASSMEMORY_EN: XMSMDATA7: GPJ2[7] output high */
	value = readl(0xe0200280);
	value &= ~(0xf << (7 << 2));
	value |= (1 << (7 << 2));
	writel(value, 0xe0200280);

	value = readl(0xe0200284);
	value |= (1 << 7);
	writel(value, 0xe0200284);

	/* GPG0[0:6] special function 2 */
	writel(0x2222022, 0xe02001a0);
	/* GPG0[0:6] pull disable */
	writel(0x10, 0xe02001a8);
	/* GPG0[0:6] drv 4x */
	writel(0x3fef, 0xe02001ac);

	return 0;
}

void board_mmc_read_block(unsigned char *buf)
{
	copy_emmc_to_mem copy_bl2 =
		(copy_emmc_to_mem) (*(u32 *) EMMC_COPY_TO_MEM_ADDR);

	copy_bl2(0, BLK_COUNT, (u32 *) buf, BUS_WIDTH_8BIT);
}
