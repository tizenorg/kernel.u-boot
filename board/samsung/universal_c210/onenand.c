/*
 * Copyright (C) 2010 Samsung Electronics
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
#include <linux/mtd/mtd.h>
#include <linux/mtd/onenand.h>

/* S5PC110 specific definitions */
#define S5PC110_DMA_SRC_ADDR		0x400
#define S5PC110_DMA_SRC_CFG		0x404
#define S5PC110_DMA_DST_ADDR		0x408
#define S5PC110_DMA_DST_CFG		0x40C
#define S5PC110_DMA_TRANS_SIZE		0x414
#define S5PC110_DMA_TRANS_CMD		0x418
#define S5PC110_DMA_TRANS_STATUS	0x41C
#define S5PC110_DMA_TRANS_DIR		0x420

#define S5PC110_DMA_CFG_SINGLE		(0x0 << 16)
#define S5PC110_DMA_CFG_4BURST		(0x2 << 16)
#define S5PC110_DMA_CFG_8BURST		(0x3 << 16)
#define S5PC110_DMA_CFG_16BURST		(0x4 << 16)

#define S5PC110_DMA_CFG_INC		(0x0 << 8)
#define S5PC110_DMA_CFG_CNT		(0x1 << 8)

#define S5PC110_DMA_CFG_8BIT		(0x0 << 0)
#define S5PC110_DMA_CFG_16BIT		(0x1 << 0)
#define S5PC110_DMA_CFG_32BIT		(0x2 << 0)

#define S5PC110_DMA_SRC_CFG_READ	(S5PC110_DMA_CFG_16BURST | \
					S5PC110_DMA_CFG_INC | \
					S5PC110_DMA_CFG_16BIT)
#define S5PC110_DMA_DST_CFG_READ	(S5PC110_DMA_CFG_16BURST | \
					S5PC110_DMA_CFG_INC | \
					S5PC110_DMA_CFG_32BIT)
#define S5PC110_DMA_SRC_CFG_WRITE	(S5PC110_DMA_CFG_16BURST | \
					S5PC110_DMA_CFG_INC | \
					S5PC110_DMA_CFG_32BIT)
#define S5PC110_DMA_DST_CFG_WRITE	(S5PC110_DMA_CFG_16BURST | \
					S5PC110_DMA_CFG_INC | \
					S5PC110_DMA_CFG_16BIT)

#define S5PC110_DMA_TRANS_CMD_TDC	(0x1 << 18)
#define S5PC110_DMA_TRANS_CMD_TEC	(0x1 << 16)
#define S5PC110_DMA_TRANS_CMD_TR	(0x1 << 0)

#define S5PC110_DMA_TRANS_STATUS_TD	(0x1 << 18)
#define S5PC110_DMA_TRANS_STATUS_TB	(0x1 << 17)
#define S5PC110_DMA_TRANS_STATUS_TE	(0x1 << 16)

#define S5PC110_DMA_DIR_READ		0x0
#define S5PC110_DMA_DIR_WRITE		0x1

#define S5PC210_ONENAND_DMA_ADDRESS	(CONFIG_SYS_ONENAND_BASE + 0x00600000)

extern void *memcpy32(void *dst, const void *src, int len);

static int s5pc210_dma_poll(void *dst, void *src, size_t count, int direction)
{
	unsigned long base = S5PC210_ONENAND_DMA_ADDRESS;
	int status, src_cfg, dst_cfg;
	int src_addr, dst_addr;

	src_addr = (unsigned int) src;
	dst_addr = (unsigned int) dst;

	if (direction == S5PC110_DMA_DIR_READ) {
		/* S5PC210 EVT0 workaround */
		src_addr = (unsigned long) src - CONFIG_SYS_ONENAND_BASE;
		src_cfg = S5PC110_DMA_SRC_CFG_READ;
		dst_cfg = S5PC110_DMA_DST_CFG_READ;
	} else {
		/* S5PC210 EVT0 workaround */
		dst_addr = (unsigned long) dst - CONFIG_SYS_ONENAND_BASE;
		src_cfg = S5PC110_DMA_SRC_CFG_WRITE;
		dst_cfg = S5PC110_DMA_DST_CFG_WRITE;
	}

	writel(src_addr, base + S5PC110_DMA_SRC_ADDR);
	writel(dst_addr, base + S5PC110_DMA_DST_ADDR);
	writel(src_cfg, base + S5PC110_DMA_SRC_CFG);
	writel(dst_cfg, base + S5PC110_DMA_DST_CFG);
	writel(count, base + S5PC110_DMA_TRANS_SIZE);
	writel(direction, base + S5PC110_DMA_TRANS_DIR);

	writel(S5PC110_DMA_TRANS_CMD_TR, base + S5PC110_DMA_TRANS_CMD);

	do {
		status = readl(base + S5PC110_DMA_TRANS_STATUS);
		if (status & S5PC110_DMA_TRANS_STATUS_TE) {
			writel(S5PC110_DMA_TRANS_CMD_TEC, base + S5PC110_DMA_TRANS_CMD);
			return -1;
		}
	} while (!(status & S5PC110_DMA_TRANS_STATUS_TD));

	/* XXX need some time delay. why? */
	udelay(1);

	writel(S5PC110_DMA_TRANS_CMD_TDC, base + S5PC110_DMA_TRANS_CMD);

	return 0;
}

static int s5pc210_read_bufferram(struct mtd_info *mtd, loff_t addr, int area,
				unsigned char *buffer, int offset,
				size_t count)
{
	struct onenand_chip *this = mtd->priv;
	void *p;
	int err, ofs;

	p = this->base + area;
	if (ONENAND_CURRENT_BUFFERRAM(this)) {
		if (area == ONENAND_DATARAM)
			p += this->writesize;
		else
			p += mtd->oobsize;
	}

	if (count == this->writesize) {
		err = s5pc210_dma_poll(buffer + offset, p, count, S5PC110_DMA_DIR_READ);
		if (!err)
			return count;
	}

	/* To prevent unaligned access */
	ofs = offset & (32 - 1);
	if (ofs || (count & (32 - 1))) {
		int c = roundup(count + ofs, 32);
		memcpy(this->page_buf, p + (offset - ofs), c);
		p = this->page_buf + ofs;
	} else
		p += offset;

	memcpy32(buffer, p, count);

	return 0;
}

static int s5pc210_chip_probe(struct mtd_info *mtd)
{
	return 0;
}

void onenand_board_init(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;

	this->base = (void *)CONFIG_SYS_ONENAND_BASE;
	this->options |= ONENAND_RUNTIME_BADBLOCK_CHECK;
	this->chip_probe = s5pc210_chip_probe;
	this->read_bufferram = s5pc210_read_bufferram;
}
