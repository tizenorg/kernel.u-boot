/*
 * LCD panel driver for Board based on S5PC100 and S5PC110. 
 *
 * Author: InKi Dae  <inki.dae@samsung.com>
 *
 * Derived from drivers/video/omap/lcd-apollon.c
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

#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>

#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

#define S5PCFB_C100_CS_LOW	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) & 0xdf, S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C100_CS_HIGH	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) | 0x20, S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C100_CLK_LOW	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) & 0xbf, S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C100_CLK_HIGH	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) | 0x40, S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C100_SDA_LOW	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) & 0x7f, S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C100_SDA_HIGH	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) | 0x80, S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))

#define S5PCFB_C110_CS_LOW	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_1_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) & 0xfd, S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_1_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C110_CS_HIGH	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_1_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) | 0x02, S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_1_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C110_CLK_LOW	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) & 0xfd, S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C110_CLK_HIGH	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) | 0x02, S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C110_SDA_LOW	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) & 0xf7, S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))
#define S5PCFB_C110_SDA_HIGH	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET)) | 0x08, S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+\
		S5PC1XX_GPIO_DAT_OFFSET))

const unsigned char SEQ_DISPLAY_ON[] = {
	0x14, 0x03,
};

const unsigned char SEQ_DISPLAY_OFF[] = {
	0x14, 0x00,
};

const unsigned char SEQ_STANDBY_ON[] = {
	0x1D, 0xA1,
};

const unsigned char SEQ_STANDBY_OFF[] = {
	0x1D, 0xA0,
};


const unsigned char SEQ_SETTING[] = {
	0x31, 0x08, /* panel setting */
	0x32, 0x14,
	0x30, 0x02,
	0x27, 0x01,
	0x12, 0x08,
	0x13, 0x08,
	0x15, 0x10,
	0x16, 0x00, /* 24bit line and 16M color */

	0xA1, 0x00,
	DATA_ONLY, 0xC4,
	0xEF, 0xD0, /* pentile key setting */
	DATA_ONLY, 0xE8,

	0x39, 0x44, /* gamma setting : 300cd */

	0x40, 0x0A,
	0x41, 0x00,
	0x42, 0x12,
	0x43, 0x18,
	0x44, 0x18,
	0x45, 0x14,
	0x46, 0x52,

	0x50, 0x0A,
	0x51, 0x00,
	0x52, 0x00,
	0x53, 0x1A,
	0x54, 0x1A,
	0x55, 0x17,
	0x56, 0x3A,

	0x60, 0x00,
	0x61, 0x00,
	0x62, 0x29,
	0x63, 0x29,
	0x64, 0x26,
	0x65, 0x20,
	0x66, 0x3D,

	0x17, 0x22, /* power setting */
	0x18, 0x33,
	0x19, 0x03,
	0x1A, 0x01,
	0x22, 0xA4, /* VXX x 0.60 */
	0x23, 0x00,
	0x26, 0xA0,
};

static void tl2796_c100_spi_write_byte(unsigned char address, unsigned char command)
{
	int     j;
	unsigned char DELAY=1;
	unsigned short data;

	data = (address << 8) + command;

	S5PCFB_C100_CS_HIGH;
	S5PCFB_C100_SDA_HIGH;
	S5PCFB_C100_CLK_HIGH;
	udelay(DELAY);

	S5PCFB_C100_CS_LOW;
	udelay(DELAY);

	for (j = 15; j >= 0; j--)
	{
		S5PCFB_C100_CLK_LOW;

		/* data high or low */
		if ((data >> j) & 0x0001)
			S5PCFB_C100_SDA_HIGH;
		else
			S5PCFB_C100_SDA_LOW;

		udelay(DELAY);

		S5PCFB_C100_CLK_HIGH;
		udelay(DELAY);
	}

	S5PCFB_C100_CS_HIGH;
	udelay(DELAY);
}

static void tl2796_c110_spi_write_byte(unsigned char address, unsigned char command)
{
	int     j;
	unsigned char DELAY=1;
	unsigned short data;

	data = (address << 8) + command;

	S5PCFB_C110_CS_HIGH;
	S5PCFB_C110_SDA_HIGH;
	S5PCFB_C110_CLK_HIGH;
	udelay(DELAY);

	S5PCFB_C110_CS_LOW;
	udelay(DELAY);

	for (j = 15; j >= 0; j--)
	{
		S5PCFB_C110_CLK_LOW;

		/* data high or low */
		if ((data >> j) & 0x0001)
			S5PCFB_C110_SDA_HIGH;
		else
			S5PCFB_C110_SDA_LOW;

		udelay(DELAY);

		S5PCFB_C110_CLK_HIGH;
		udelay(DELAY);
	}

	S5PCFB_C110_CS_HIGH;
	udelay(DELAY);
}


static void tl2796_spi_write(unsigned char address, unsigned char command)
{
	if (cpu_is_s5pc110()) {
		if(address != DATA_ONLY)
			tl2796_c110_spi_write_byte(0x70, address);

		tl2796_c110_spi_write_byte(0x72, command);
	} else {
		if(address != DATA_ONLY)
			tl2796_c100_spi_write_byte(0x70, address);

		tl2796_c100_spi_write_byte(0x72, command);
	}
}

static void tl2796_panel_send_sequence(const unsigned char *wbuf, unsigned int size)
{
	int i = 0;
	while (i < size) {
		if (wbuf[i] == 0x1D && wbuf[i + 1] == 0xA1)
			udelay(200000);
		else if (wbuf[i] == 0x1D && wbuf[i + 1] == 0xA0)
			udelay(250000);
		s6e63m0_spi_write(wbuf[i], wbuf[i+1]);
		i += 2;
	}
}

void tl2796_c100_panel_power_on(void)
{
	/* set gpio data for MLCD_RST to HIGH */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x80,
		S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+4));
	/* set gpio data for MLCD_ON to HIGH */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_J1_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x8,
		S5PC100_GPIO_BASE(S5PC100_GPIO_J1_OFFSET+4));
	udelay(25000);

	/* set gpio data for MLCD_RST to LOW */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) & 0x7f,
		S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	udelay(20);
	/* set gpio data for MLCD_RST to HIGH */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x80,
		S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));

	udelay(20000);

	tl2796_panel_send_sequence(SEQ_SETTING, ARRAY_SIZE(SEQ_SETTING));
	tl2796_panel_send_sequence(SEQ_STANDBY_OFF, ARRAY_SIZE(SEQ_STANBY_OFF));
}

void lcd_panel_power_on(void)
{
	/* set gpio data for MLCD_RST to HIGH */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0xdf,
		S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	/* set gpio data for MLCD_ON to HIGH */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_J1_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x8,
		S5PC110_GPIO_BASE(S5PC110_GPIO_J1_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	udelay(25000);

	/* set gpio data for MLCD_RST to LOW */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) & 0xdf,
		S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	udelay(20);
	/* set gpio data for MLCD_RST to HIGH */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x20,
		S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));

	udelay(20000);

	tl2796_panel_send_sequence(SEQ_SETTING, ARRAY_SIZE(SEQ_SETTING));
	tl2796_panel_send_sequence(SEQ_STANDBY_OFF, ARRAY_SIZE(SEQ_STANBY_OFF));
}

static inline void tl2796_c100_panel_hw_reset(void)
{
	/* set gpio pin for MLCD_RST to LOW */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) & 0x7f,
		S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	udelay(1);	/* Shorter than 5 usec */
	/* set gpio pin for MLCD_RST to HIGH */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x80,
		S5PC100_GPIO_BASE(S5PC100_GPIO_H1_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	udelay(10000);
}

static inline void tl2796_c110_panel_hw_reset(void)
{
	/* set gpio pin for MLCD_RST to LOW */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) & 0xdf,
		S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	udelay(1);	/* Shorter than 5 usec */
	/* set gpio pin for MLCD_RST to HIGH */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x20,
		S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_5_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	udelay(10000);
}

void lcd_panel_enable(void)
{
	tl2796_panel_send_sequence(SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
}

static void tl2796_panel_disable(void)
{
	tl2796_panel_send_sequence(SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
}

void tl2796_c100_panel_init(void)
{
	/* set gpio pin for DISPLAY_CS to HIGH */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x20,
		S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	/* set gpio pin for DISPLAY_CLK to HIGH */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x40,
		S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	/* set gpio pin for DISPLAY_SI to HIGH */
	writel(readl(S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x80,
		S5PC100_GPIO_BASE(S5PC100_GPIO_K3_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
}

void lcd_panel_init(void)
{
	/* set gpio pin for DISPLAY_CS to HIGH */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_1_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x02,
		S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_1_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	/* set gpio pin for DISPLAY_CLK to HIGH */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x02,
		S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
	/* set gpio pin for DISPLAY_SI to HIGH */
	writel(readl(S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+
			S5PC1XX_GPIO_DAT_OFFSET)) | 0x08,
		S5PC110_GPIO_BASE(S5PC110_GPIO_MP0_4_OFFSET+
		    S5PC1XX_GPIO_DAT_OFFSET));
}
