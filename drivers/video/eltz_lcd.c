/*
 * SUB LCD panel driver for the Samsung TickerTape board
 *
 * Author: InKi Dae  <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <common.h>
#include <stdarg.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>

/*
 * Register Definition
 */
/* SROM Controller */
#define SMC_BW		0xE7000000
#define SMC_BC0		0xE7000004

/* GPIO for interfacing with SROMC */
#define GPK0_CFG	0xE03002A0
#define GPL2_CFG	0xE0300360
#define GPL3_CFG	0xE0300380
#define GPL4_CFG	0xE03003A0

/* GPIO for LCD_ON  */
#define GPH0_CFG	0xE0300C00

#define GPK0_DATA	0xE03002A4
#define GPJ2_DATA	0xE0300244
#define GPH0_DATA	0xE0300C04

/* 
 * GPIO Definition
 */
/* register select */
#define SUBLCD_RS_LOW		__raw_writel(__raw_readl(GPK0_DATA) & 0xFD, GPK0_DATA)
#define SUBLCD_RS_HIGH		__raw_writel(__raw_readl(GPK0_DATA) | 0x02, GPK0_DATA)

/* reset */
#define SUBLCD_RESETB_LOW	__raw_writel(__raw_readl(GPK0_DATA) & 0xF7, GPK0_DATA)
#define SUBLCD_RESETB_HIGH	__raw_writel(__raw_readl(GPK0_DATA) | 0x08, GPK0_DATA)

/* sublcd on */
#define SUBLCD_ON_LOW		__raw_writel(__raw_readl(GPH0_DATA) & 0xF7, GPH0_DATA)
#define SUBLCD_ON_HIGH		__raw_writel(__raw_readl(GPH0_DATA) | 0x08, GPH0_DATA)

#define SUBLCD_BASE	(0x80000000)
#define SUBLCD_BASE_W	__REGw(0x80000000)
#define SUBLCD_BASE_B	__REGb(0x80000000)

static unsigned short makepixel565(char r, char g, char b)
{
	return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static void read_image16(unsigned short* pImg, int x1pos, int y1pos, int x2pos,
	int y2pos, unsigned short pixel)
{
	int i, j;

	for(i = y1pos; i < y2pos; i++) {
		for(j = x1pos; j < x2pos; j++) {
			*(pImg) = pixel;
		}
	}

	SUBLCD_RS_LOW;
	SUBLCD_BASE_B = 0x0000;
}

static void sublcd_write_register_16(unsigned char address, unsigned short data)
{
	SUBLCD_RS_LOW;
	SUBLCD_BASE_B = address;

	SUBLCD_RS_HIGH;
	SUBLCD_BASE_W = data;
}

static void sublcd_write_GRAM(void)
{
	/* vertical start address */
	sublcd_write_register_16(0x35, 0x0000);
	/* vertical end address */
	sublcd_write_register_16(0x36, 0x013f);
	/* horzontal start and end address */
	sublcd_write_register_16(0x37, 0x00ef);

	/* start address set */
	sublcd_write_register_16(0x20, 0x00);
	sublcd_write_register_16(0x21, 0x00);

	/* index write */
	SUBLCD_RS_LOW;
	SUBLCD_BASE_B = 0x22;

	SUBLCD_RS_HIGH;
}

static void sublcd_power_on(void)
{
	SUBLCD_ON_HIGH;

	udelay(5000);

	/* power-on Reset */
	SUBLCD_RESETB_HIGH;
	SUBLCD_RESETB_LOW;
	udelay(15);
	SUBLCD_RESETB_HIGH;

	udelay(5000);

	/* change to I80 18bit mode */
	SUBLCD_RS_LOW;
	SUBLCD_BASE_B = 0x23;

	/* set data bus width to 16bit for SROMC*/
	__raw_writel(__raw_readl(SMC_BW) | 0x00000003, SMC_BW);

	/* change to I80 16bit mode */
	sublcd_write_register_16(0x03, 0x0030);

	/* Power Setting Sequence */
	sublcd_write_register_16(0x11, 0x000f);
	sublcd_write_register_16(0x12, 0x0008);
	sublcd_write_register_16(0x13, 0x056a);
	sublcd_write_register_16(0x14, 0x4202);

	/* Initializing Sequence */
	sublcd_write_register_16(0x01, 0x8800);
	sublcd_write_register_16(0x18, 0x0012);
	sublcd_write_register_16(0x1a, 0x0003);
	sublcd_write_register_16(0x06, 0x0002);
	sublcd_write_register_16(0x07, 0x0400);
	sublcd_write_register_16(0x08, 0x0404);
	sublcd_write_register_16(0x09, 0x7411);
	sublcd_write_register_16(0x35, 0x0000);
	sublcd_write_register_16(0x36, 0x013f);
	sublcd_write_register_16(0x37, 0x00ef);

	/* gamma */
	sublcd_write_register_16(0x82, 0x0000);
	sublcd_write_register_16(0x70, 0x5d25);
	sublcd_write_register_16(0x71, 0x7325);
	sublcd_write_register_16(0x72, 0x6f25);
	sublcd_write_register_16(0x73, 0x2821);
	sublcd_write_register_16(0x74, 0x2d27);
	sublcd_write_register_16(0x75, 0x2425);
	sublcd_write_register_16(0x76, 0x2527);
	sublcd_write_register_16(0x77, 0x2621);
	sublcd_write_register_16(0x78, 0x2b26);

	/* set Gamma updata command */
	SUBLCD_RS_LOW;
	SUBLCD_BASE_B = 0x83;

	sublcd_write_register_16(0x20, 0x0000);
	sublcd_write_register_16(0x21, 0x0000);
	SUBLCD_RS_LOW;
	SUBLCD_BASE_B = 0x22;

	/* stand-by off */
	sublcd_write_register_16(0x10, 0x0000);
	udelay(140000);

	/* EL ON */
	sublcd_write_register_16(0x0B, 0x0001);

	/* DISPLAY ON */
	sublcd_write_register_16(0x05, 0x0001);

	/* initialize GRAM region to black color*/
	sublcd_write_GRAM();
	read_image16((char *)SUBLCD_BASE, 0, 0, 240, 320, makepixel565(0,0,0));
}

static void srom_write_setup(void)
{
	/* set access cycle of SROM Controller for SROM BANK0 */
	__raw_writel(__raw_readl(SMC_BC0) & ~0xFFFFFF00, SMC_BC0);
	__raw_writel(__raw_readl(SMC_BC0) | 0x22411300, SMC_BC0);
}


static void sublcd_gpio_init(void)
{
	/* set gpio pin for CSB, RS, RESETB, RDB, WRB */
	__raw_writel(__raw_readl(GPK0_CFG) & ~0xFF00F0FF, GPK0_CFG);
	__raw_writel(__raw_readl(GPK0_CFG) | 0x22001012, GPK0_CFG);

	/* set gpio pin for 16bit DATA Lines */
	__raw_writel(__raw_readl(GPL2_CFG) & ~0xFFF00000, GPL2_CFG);
	__raw_writel(__raw_readl(GPL2_CFG) | 0x22200000, GPL2_CFG);
	__raw_writel(__raw_readl(GPL3_CFG) & ~0xFFFFFFFF, GPL3_CFG);
	__raw_writel(__raw_readl(GPL3_CFG) | 0x22222222, GPL3_CFG);
	__raw_writel(__raw_readl(GPL4_CFG) & ~0x000FFFFF, GPL4_CFG);
	__raw_writel(__raw_readl(GPL4_CFG) | 0x00022222, GPL4_CFG);

	/* set gpio bin for SUBLCD_ON to output */
	__raw_writel(__raw_readl(GPH0_CFG) & ~0x0000F000, GPH0_CFG);
	__raw_writel(__raw_readl(GPH0_CFG) | 0x00001000, GPH0_CFG);

	/* set Data bus width to 8-bit for SROM Controller */
	__raw_writel(__raw_readl(SMC_BW) & ~0xFFFFFFFF, SMC_BW);
	__raw_writel(__raw_readl(SMC_BW) | 0x00000000, SMC_BW);
}

static void draw_test(void)
{
	static int i = 0;

	sublcd_write_GRAM();

	read_image16(SUBLCD_BASE, 0, 0, 240, 320,
		makepixel565((i * 12345678) & 255, (i * 50) & 255, (i * 87654321) & 255));

	if (++i > 255) i = 0;
}

void sublcd_panel_test(void)
{
	sublcd_gpio_init();

	srom_write_setup();

	sublcd_power_on();
}
