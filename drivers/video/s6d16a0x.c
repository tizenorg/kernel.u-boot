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

#include <common.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <spi.h>
/* these machine specific platform data would be setting at universal.c */
struct spi_platform_data *s6d16a0x;

static const unsigned char SEQ_PASSWD2_SET[] = {
	0xF1, 0x5A,
	DATA_ONLY, 0x5A,
};

static const unsigned char SEQ_DISCTL_SET[] = {
	0xf2, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x62,
	DATA_ONLY, 0x62,
	DATA_ONLY, 0x57,
	DATA_ONLY, 0x57,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
};

static const unsigned char SEQ_PWRCTL_SET[] = {
	0xf3, 0x00,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x25,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0x2D,
	DATA_ONLY, 0x2D,
	DATA_ONLY, 0x24,
	DATA_ONLY, 0x2D,
	DATA_ONLY, 0x14,
	DATA_ONLY, 0x14,
	DATA_ONLY, 0x12,
	DATA_ONLY, 0x78,
};

static const unsigned char SEQ_VCMCTL_SET[] = {
	0xf4, 0x00,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x57,
	DATA_ONLY, 0x68,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x57,
	DATA_ONLY, 0x68,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
};

static const unsigned char SEQ_SRCCTL_SET[] = {
	0xf5, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x57,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0x14,
	DATA_ONLY, 0x14,
	DATA_ONLY, 0x09,
	DATA_ONLY, 0x09,
};

static const unsigned char SEQ_PANELCTL1_SET[] = {
	0xf6, 0x02,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x80,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x44,
	DATA_ONLY, 0x00,
};

static const unsigned char SEQ_PANELCTL2_SET[] = {
	0xf7, 0x00,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0xF2,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x1F,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x07,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x4B,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x8C,
};

static const unsigned char SEQ_PANELCTL3_SET[] = {
	0xf8, 0x00,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0xF2,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x1F,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x07,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x4B,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x8C,
};

static const unsigned char SEQ_PANELCTL4_SET[] = {
	0xf9, 0x00,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x05,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x04,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x0C,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x0F,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x11,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x1F,
	DATA_ONLY, 0xFF,
	DATA_ONLY, 0xE0,
};

static const unsigned char SEQ_PANELCTL4_SET_EX[] = {
	0xf9, 0x00,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x05,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x04,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x0C,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x0F,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x11,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x1F,
	DATA_ONLY, 0xFF,
	DATA_ONLY, 0xE0,
};

static const unsigned char SEQ_PGAMMACTL_SET[] = {
	0xfa, 0x19,
	DATA_ONLY, 0x37,
	DATA_ONLY, 0x2C,
	DATA_ONLY, 0x0F,
	DATA_ONLY, 0x13,
	DATA_ONLY, 0x16,
	DATA_ONLY, 0x18,
	DATA_ONLY, 0x0C,
	DATA_ONLY, 0x07,
	DATA_ONLY, 0x1F,
	DATA_ONLY, 0x13,
	DATA_ONLY, 0x1A,
	DATA_ONLY, 0x22,
	DATA_ONLY, 0x1F,
	DATA_ONLY, 0x1E,
	DATA_ONLY, 0x13,
	DATA_ONLY, 0x37,
	DATA_ONLY, 0x2C,
	DATA_ONLY, 0x0F,
	DATA_ONLY, 0x14,
	DATA_ONLY, 0x16,
	DATA_ONLY, 0x19,
	DATA_ONLY, 0x0C,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x20,
	DATA_ONLY, 0x12,
	DATA_ONLY, 0x16,
	DATA_ONLY, 0x1D,
	DATA_ONLY, 0x18,
	DATA_ONLY, 0x17,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x37,
	DATA_ONLY, 0x2C,
	DATA_ONLY, 0x12,
	DATA_ONLY, 0x16,
	DATA_ONLY, 0x17,
	DATA_ONLY, 0x1C,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x0E,
	DATA_ONLY, 0x25,
	DATA_ONLY, 0x12,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x07,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
};

static const unsigned char SEQ_NGAMMACTL_SET[] = {
	0xFB, 0x00,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x1E,
	DATA_ONLY, 0x1D,
	DATA_ONLY, 0x3B,
	DATA_ONLY, 0x42,
	DATA_ONLY, 0x49,
	DATA_ONLY, 0x3C,
	DATA_ONLY, 0x36,
	DATA_ONLY, 0x4F,
	DATA_ONLY, 0x43,
	DATA_ONLY, 0x45,
	DATA_ONLY, 0x45,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x0D,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x28,
	DATA_ONLY, 0x24,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x3F,
	DATA_ONLY, 0x47,
	DATA_ONLY, 0x4A,
	DATA_ONLY, 0x3A,
	DATA_ONLY, 0x32,
	DATA_ONLY, 0x4C,
	DATA_ONLY, 0x43,
	DATA_ONLY, 0x44,
	DATA_ONLY, 0x42,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x0D,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x3E,
	DATA_ONLY, 0x3E,
	DATA_ONLY, 0x3E,
	DATA_ONLY, 0x55,
	DATA_ONLY, 0x4A,
	DATA_ONLY, 0x48,
	DATA_ONLY, 0x38,
	DATA_ONLY, 0x31,
	DATA_ONLY, 0x4C,
	DATA_ONLY, 0x41,
	DATA_ONLY, 0x43,
	DATA_ONLY, 0x40,
	DATA_ONLY, 0x1F,
	DATA_ONLY, 0x0D,
};

static const unsigned char SEQ_CLKCTL3_SET[] = {
	0xB7, 0x00,

	DATA_ONLY, 0x11,
	DATA_ONLY, 0x11,
};

static const unsigned char SEQ_HOSTCTL1_SET[] = {
	0xB8, 0x31,

	DATA_ONLY, 0x11,
};

static const unsigned char SEQ_HOSTCTL2_SET[] = {
	0xB9, 0x00,

	DATA_ONLY, 0x06,
};

static const unsigned char SEQ_WRDISBV[] = {
	0x51, 0xFF,
};

static const unsigned char SEQ_WRCTRLD[] = {
	0x53, 0x24,
};

static const unsigned char SEQ_WRCABC[] = {
	0x55, 0x01,		/* CABC ON */
};

static const unsigned char SEQ_WRCABCMB[] = {
	0x5E, 0x00
};

static const unsigned char SEQ_CABCCTL1[] = {
	0xC0, 0xFF,		/* RRC MAX POWER */

	DATA_ONLY, 0x80,	/* DEFAULT */
	DATA_ONLY, 0x30,	/* ONOFFDIMMEN ON */
};

static const unsigned char SEQ_TEON_SET[] = {
	COMMAND_ONLY, 0x35,
};

static const unsigned char SEQ_PASSWD_SET[] = {
	0xF1, 0xA5,

	DATA_ONLY, 0xA5,
};

static const unsigned char SEQ_CASET[] = {
	0x2A, 0x00,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0x3F,
};

static const unsigned char SEQ_PASET[] = {
	0x2B, 0x00,

	DATA_ONLY, 0x00,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0xDF,
};

static const unsigned char SEQ_COLMOD[] = {
	0x36, 0xC4,	/* MADCTL : Reverse display */
};

static const unsigned char SEQ_COLMOD_EX[] = {
	0x36, 0x40,	/* MADCTL : Reverse display */
};

static const unsigned char SEQ_WRCTRLK[] = {
	0x63, 0x00,
};

static const unsigned char SEQ_SLPOUT[] = {
	0x11, COMMAND_ONLY,
};

static const unsigned char SEQ_DISPON[] = {
	0x29, COMMAND_ONLY,
};

static const unsigned char SEQ_DISPOFF[] = {
	0x28, COMMAND_ONLY,
};

static void s6d16a0x_spi_write(unsigned char address, unsigned char command)
{
	if (address != DATA_ONLY)
		spi_gpio_write(s6d16a0x, (unsigned int) 0x0, (unsigned int) address);

	if (command != COMMAND_ONLY)
		spi_gpio_write(s6d16a0x, (unsigned int) 0x1, (unsigned int) command);
}

static void s6d16a0x_panel_send_sequence(const unsigned char *wbuf, unsigned int size_cmd)
{
	int i = 0;
	while (i < size_cmd) {
		s6d16a0x_spi_write(wbuf[i], wbuf[i+1]);
		i += 2;
	}
}

void s6d16a0x_cfg_ldo(void)
{
	s6d16a0x_panel_send_sequence(SEQ_PASSWD2_SET, ARRAY_SIZE(SEQ_PASSWD2_SET));
	s6d16a0x_panel_send_sequence(SEQ_DISCTL_SET, ARRAY_SIZE(SEQ_DISCTL_SET));
	s6d16a0x_panel_send_sequence(SEQ_PWRCTL_SET, ARRAY_SIZE(SEQ_PWRCTL_SET));
	s6d16a0x_panel_send_sequence(SEQ_VCMCTL_SET, ARRAY_SIZE(SEQ_VCMCTL_SET));
	s6d16a0x_panel_send_sequence(SEQ_SRCCTL_SET, ARRAY_SIZE(SEQ_SRCCTL_SET));
	s6d16a0x_panel_send_sequence(SEQ_PANELCTL1_SET, ARRAY_SIZE(SEQ_PANELCTL1_SET));
	s6d16a0x_panel_send_sequence(SEQ_PANELCTL2_SET, ARRAY_SIZE(SEQ_PANELCTL2_SET));
	s6d16a0x_panel_send_sequence(SEQ_PANELCTL3_SET, ARRAY_SIZE(SEQ_PANELCTL3_SET));
	if (s6d16a0x->set_rev) {
		s6d16a0x_panel_send_sequence(SEQ_PANELCTL4_SET_EX, ARRAY_SIZE(SEQ_PANELCTL4_SET_EX));
	} else {
		s6d16a0x_panel_send_sequence(SEQ_PANELCTL4_SET, ARRAY_SIZE(SEQ_PANELCTL4_SET));
	}
	s6d16a0x_panel_send_sequence(SEQ_PGAMMACTL_SET, ARRAY_SIZE(SEQ_PGAMMACTL_SET));
	s6d16a0x_panel_send_sequence(SEQ_NGAMMACTL_SET, ARRAY_SIZE(SEQ_NGAMMACTL_SET));
	s6d16a0x_panel_send_sequence(SEQ_CLKCTL3_SET, ARRAY_SIZE(SEQ_CLKCTL3_SET));
	s6d16a0x_panel_send_sequence(SEQ_HOSTCTL1_SET, ARRAY_SIZE(SEQ_HOSTCTL1_SET));
	s6d16a0x_panel_send_sequence(SEQ_HOSTCTL2_SET, ARRAY_SIZE(SEQ_HOSTCTL2_SET));
#if 1
	/* for M2 board */
	s6d16a0x_panel_send_sequence(SEQ_TEON_SET, ARRAY_SIZE(SEQ_TEON_SET));
	s6d16a0x_panel_send_sequence(SEQ_PASSWD_SET, ARRAY_SIZE(SEQ_PASSWD_SET));
#else
	s6d16a0x_panel_send_sequence(SEQ_WRCABC, ARRAY_SIZE(SEQ_WRCABC));
	s6d16a0x_panel_send_sequence(SEQ_WRCABCMB, ARRAY_SIZE(SEQ_WRCABCMB));
	s6d16a0x_panel_send_sequence(SEQ_CABCCTL1, ARRAY_SIZE(SEQ_CABCCTL1));
	s6d16a0x_panel_send_sequence(SEQ_TEON_SET, ARRAY_SIZE(SEQ_TEON_SET));
	s6d16a0x_panel_send_sequence(SEQ_PASSWD2_SET, ARRAY_SIZE(SEQ_PASSWD2_SET));
#endif
	s6d16a0x_panel_send_sequence(SEQ_CASET, ARRAY_SIZE(SEQ_CASET));
	s6d16a0x_panel_send_sequence(SEQ_PASET, ARRAY_SIZE(SEQ_PASET));

	if (s6d16a0x->set_rev) {
		s6d16a0x_panel_send_sequence(SEQ_COLMOD_EX, ARRAY_SIZE(SEQ_COLMOD_EX));
	} else {
		s6d16a0x_panel_send_sequence(SEQ_COLMOD, ARRAY_SIZE(SEQ_COLMOD));
	}
	s6d16a0x_panel_send_sequence(SEQ_WRDISBV, ARRAY_SIZE(SEQ_WRDISBV));
	s6d16a0x_panel_send_sequence(SEQ_WRCTRLD, ARRAY_SIZE(SEQ_WRCTRLD));
	s6d16a0x_panel_send_sequence(SEQ_WRCTRLK, ARRAY_SIZE(SEQ_WRCTRLK));
	s6d16a0x_panel_send_sequence(SEQ_SLPOUT, ARRAY_SIZE(SEQ_SLPOUT));
	udelay(120000);
}

void s6d16a0x_enable_ldo(unsigned int onoff)
{
	if (onoff) {
		s6d16a0x_panel_send_sequence(SEQ_DISPON, ARRAY_SIZE(SEQ_DISPON));
	}
}

/* this function would be called at universal.c */
void s6d16a0x_set_platform_data(struct spi_platform_data *pd)
{
	if (pd == NULL) {
		printf("pd is NULL.\n");
		return;
	}

	s6d16a0x = pd;
}

