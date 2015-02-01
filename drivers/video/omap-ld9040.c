/*
 * LCD panel driver for Board based on OMAP4430. 
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

#include <common.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <spi.h>

/* these machine specific platform data would be setting at universal.c */
struct spi_platform_data *omap_ld9040;

static const unsigned char SEQ_SWRESET[] = {
	0x01, COMMAND_ONLY,
};

static const unsigned char SEQ_USER_SETTING[] = {
	0xF0, 0x5A,

	DATA_ONLY, 0x5A,
};

static const unsigned char SEQ_ELVSS_ON[] = {
	0xB1, 0x0D,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x16,
};

static const unsigned char SEQ_TEMP_SWIRE[] = {
	0xB2, 0x06,

	DATA_ONLY, 0x06,
	DATA_ONLY, 0x06,
	DATA_ONLY, 0x06,
};

static const unsigned char SEQ_GTCON[] = {
	0xF7, 0x09,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
};

static const unsigned char SEQ_PANEL_CONDITION[] = {
	0xF8, 0x05,

	DATA_ONLY, 0x65,
	DATA_ONLY, 0x96,
	DATA_ONLY, 0x71,
	DATA_ONLY, 0x7D,
	DATA_ONLY, 0x19,
	DATA_ONLY, 0x3B,
	DATA_ONLY, 0x0D,
	DATA_ONLY, 0x19,
	DATA_ONLY, 0x7E,
	DATA_ONLY, 0x0D,
	DATA_ONLY, 0xE2,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x7E,
	DATA_ONLY, 0x7D,
	DATA_ONLY, 0x07,
	DATA_ONLY, 0x07,
	DATA_ONLY, 0x20,
	DATA_ONLY, 0x20,
	DATA_ONLY, 0x20,
	DATA_ONLY, 0x02,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_GAMMA_SET1[] = {
	0xF9, 0x00,

	DATA_ONLY, 0xA7,
	DATA_ONLY, 0xB5,
	DATA_ONLY, 0xAE,
	DATA_ONLY, 0xBF,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x91,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0xB2,
	DATA_ONLY, 0xB4,
	DATA_ONLY, 0xAA,
	DATA_ONLY, 0xBB,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0xAC,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0xB3,
	DATA_ONLY, 0xB1,
	DATA_ONLY, 0xAA,
	DATA_ONLY, 0xBC,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0xB3,
};

static const unsigned char SEQ_GAMMA_CTRL[] = {
	0xFB, 0x02,

	DATA_ONLY, 0x5A,
};

static const unsigned char SEQ_APON[] = {
	0xF3, 0x00,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x0A,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_DISPCTL[] = {
	0xF2, 0x02,

	DATA_ONLY, 0x08,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x10,
};

static const unsigned char SEQ_MANPWR[] = {
	0xB0, 0x04,
};

static const unsigned char SEQ_PWR_CTRL[] = {
	0xF4, 0x0A,

	DATA_ONLY, 0x87,
	DATA_ONLY, 0x25,
	DATA_ONLY, 0x6A,
	DATA_ONLY, 0x44,
	DATA_ONLY, 0x02,
	DATA_ONLY, 0x88,
};

static const unsigned char SEQ_SLPOUT[] = {
	0x11, COMMAND_ONLY,
};

static const unsigned char SEQ_SLPIN[] = {
	0x10, COMMAND_ONLY,
};

static const unsigned char SEQ_DISPON[] = {
	0x29, COMMAND_ONLY,
};

static const unsigned char SEQ_DISPOFF[] = {
	0x28, COMMAND_ONLY,
};

static const unsigned char SEQ_VCI1_1ST_EN[] = {
	0xF3, 0x10,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_VL1_EN[] = {
	0xF3, 0x11,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_VL2_EN[] = {
	0xF3, 0x13,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_VCI1_2ND_EN[] = {
	0xF3, 0x33,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_VL3_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_VREG1_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0x01,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_VGH_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0x11,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_VGL_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0x31,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x02,
};

static const unsigned char SEQ_VMOS_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xB1,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
};

static const unsigned char SEQ_VINT_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xF1,
	/* DATA_ONLY, 0x71,     VMOS/VBL/VBH not used */
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
	/* DATA_ONLY, 0x02,     VMOS/VBL/VBH not used */
};

static const unsigned char SEQ_VBH_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xF9,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
};

static const unsigned char SEQ_VBL_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xFD,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
};

static const unsigned char SEQ_GAM_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xFF,
	/* DATA_ONLY, 0x73,     VMOS/VBL/VBH not used */
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
	/* DATA_ONLY, 0x02,     VMOS/VBL/VBH not used */
};

static const unsigned char SEQ_SD_AMP_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xFF,
	/* DATA_ONLY, 0x73,     VMOS/VBL/VBH not used */
	DATA_ONLY, 0x80,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
	/* DATA_ONLY, 0x02,     VMOS/VBL/VBH not used */
};

static const unsigned char SEQ_GLS_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xFF,
	/* DATA_ONLY, 0x73,     VMOS/VBL/VBH not used */
	DATA_ONLY, 0x81,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
	/* DATA_ONLY, 0x02,     VMOS/VBL/VBH not used */
};

static const unsigned char SEQ_ELS_EN[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xFF,
	/* DATA_ONLY, 0x73,     VMOS/VBL/VBH not used */
	DATA_ONLY, 0x83,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
	/* DATA_ONLY, 0x02,     VMOS/VBL/VBH not used */
};

static const unsigned char SEQ_EL_ON[] = {
	0xF3, 0x37,

	DATA_ONLY, 0xFF,
	/* DATA_ONLY, 0x73,     VMOS/VBL/VBH not used */
	DATA_ONLY, 0x87,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x03,
	/* DATA_ONLY, 0x02,     VMOS/VBL/VBH not used */
};

static const unsigned char SEQ_ID1[] = {
	0xDA, COMMAND_ONLY,
};

static const unsigned char SEQ_ID2[] = {
	0xDB, COMMAND_ONLY,
};

static const unsigned char SEQ_ID3[] = {
	0xDC, COMMAND_ONLY,
};

static void ld9040_spi_write(unsigned char address, unsigned char command)
{

	if (address != DATA_ONLY)
		spi_gpio_write(omap_ld9040, (unsigned int)0x0,
			       (unsigned int)address);

	if (command != COMMAND_ONLY)
		spi_gpio_write(omap_ld9040, (unsigned int)0x1,
			       (unsigned int)command);
}

static void ld9040_panel_send_sequence(const unsigned char *wbuf,
				       unsigned int size_cmd)
{
	int i = 0;

	while (i < size_cmd) {
		ld9040_spi_write(wbuf[i], wbuf[i + 1]);
		i += 2;
	}
}

static int ld9040_spi_read(const unsigned char *wbuf, unsigned int size_cmd)
{

	ld9040_panel_send_sequence(wbuf, size_cmd);
	return spi_gpio_read(omap_ld9040);
}

void ld9040_cfg_ldo(void)
{
	udelay(10);

	ld9040_panel_send_sequence(SEQ_USER_SETTING,
				   ARRAY_SIZE(SEQ_USER_SETTING));
	ld9040_panel_send_sequence(SEQ_PANEL_CONDITION,
				   ARRAY_SIZE(SEQ_PANEL_CONDITION));
	ld9040_panel_send_sequence(SEQ_DISPCTL, ARRAY_SIZE(SEQ_DISPCTL));
	ld9040_panel_send_sequence(SEQ_MANPWR, ARRAY_SIZE(SEQ_MANPWR));
	ld9040_panel_send_sequence(SEQ_PWR_CTRL, ARRAY_SIZE(SEQ_PWR_CTRL));
	ld9040_panel_send_sequence(SEQ_ELVSS_ON, ARRAY_SIZE(SEQ_ELVSS_ON));
	ld9040_panel_send_sequence(SEQ_GTCON, ARRAY_SIZE(SEQ_GTCON));
	ld9040_panel_send_sequence(SEQ_GAMMA_SET1, ARRAY_SIZE(SEQ_GAMMA_SET1));
	ld9040_panel_send_sequence(SEQ_GAMMA_CTRL, ARRAY_SIZE(SEQ_GAMMA_CTRL));
	ld9040_panel_send_sequence(SEQ_SLPOUT, ARRAY_SIZE(SEQ_SLPOUT));

	udelay(120);
}

void ld9040_enable_ldo(unsigned int on)
{
	char ret = 0;

	if (on)
		ld9040_panel_send_sequence(SEQ_DISPON, ARRAY_SIZE(SEQ_DISPON));

	ret = ld9040_spi_read(SEQ_ID1, ARRAY_SIZE(SEQ_ID1));
	if (ret) {
		printf("OLED Module manufacturer : \t%x\n", ret);
		ret = ld9040_spi_read(SEQ_ID2, ARRAY_SIZE(SEQ_ID2));
		printf("OLED Module/driver version : \t%x\n", ret);
		ret = ld9040_spi_read(SEQ_ID3, ARRAY_SIZE(SEQ_ID3));
		printf("OLED module/driver : \t\t%x\n", ret);
	}
}

/* this function would be called at universal.c */
void ld9040_set_platform_data(struct spi_platform_data *pd)
{
	if (pd == NULL) {
		puts("pd is NULL.\n");
		return;
	}

	omap_ld9040 = pd;
}
