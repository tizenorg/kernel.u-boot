/*
 * Copyright (C) 2008-2009 Samsung Electronics
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

#include <common.h>

#include <asm/io.h>

#include "onenand_ipl.h"

/* S5PC100 specific */
#define S5PC100_AHB_ADDR		0xB0000000
#define MEM_ADDR(fba, fpa, fsa)		((fba) << 13 | (fpa) << 7 | (fsa) << 5)
#define CMD_MAP_01(mem_addr)	(S5PC100_AHB_ADDR | (1 << 26) | (mem_addr))
#define CMD_MAP_11(addr)	(S5PC100_AHB_ADDR | (3 << 26) | ((addr) << 2))
#define onenand_ahb_readw(a)	(readl(CMD_MAP_11((a) >> 1)) & 0xffff)

static int s5pc100_onenand_read_page(ulong block, ulong page,
                                u_char * buf, int pagesize)
{
        unsigned int *p = (unsigned int *) buf;
        int mem_addr, i;

        mem_addr = MEM_ADDR(block, page, 0);

        pagesize >>= 2;

        for (i = 0; i < pagesize; i++)
                *p++ = readl(CMD_MAP_01(mem_addr));

        return 0;
}

int onenand_board_init(int *page_is_4KiB, int *page)
{
	if (((readl(0xE0000000) & 0x00FFF000) == 0x00110000) ||
			((readl(0xE0000000) & 0xFFFFF000) == 0x36442000))
		return ONENAND_USE_GENERIC;

	onenand_read_page = s5pc100_onenand_read_page;
	*page = 8;

	return ONENAND_USE_BOARD;
}
