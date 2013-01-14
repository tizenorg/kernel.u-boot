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

#include <common.h>
#include <asm/io.h>

void reset_cpu(ulong addr)
{
	writel(0x1, samsung_get_base_swreset());
}

/*
 * return: xxxx MB
 */
int exynos_get_dram_size(void)
{
	u32 dmc_base = samsung_get_base_dmc();
	u32 memcontrol;
	u32 memconfig0, memconfig1;
	u32 chip_num;
	u32 size;

	memcontrol = readl(dmc_base + 0x4);
	memconfig0 = readl(dmc_base + 0x8);

	/* chip_num = MEMCONTROL[19:16] */
	chip_num = (memcontrol >> 16) & 0xf;
	switch (chip_num) {
	case 1:
		memconfig1 = readl(dmc_base + 0xc);
		/* chip_mask = MEMCONFIGx[23:16] */
		size = ((memconfig0 + memconfig1) >> 16) & 0xff;
		size = ((~size + 1) & 0xff) << 4;
		break;
	case 0:
	default:
		size = (memconfig0 >> 16) & 0xff;
		size = ((~size + 1) & 0xff) << 4;
		break;
	}

	return size;
}

static void exynos_chip_info(void)
{
	printf("Pro ID:\t0x%08X\n", readl(S5P_PRO_ID));
	printf("Pkg ID:\t0x%08X\n", readl(S5P_PRO_ID + 4));
}

U_BOOT_CMD(
	chipinfo, 1, 1, exynos_chip_info,
	"print chip info (product id, package id, lot id, ...)",
	"chipinfo\n"
);
