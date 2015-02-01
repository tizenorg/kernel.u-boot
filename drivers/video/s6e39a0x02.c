/* linux/drivers/video/s6e39a0x.c
 *
 * MIPI-DSI based s6e39a0x AMOLED lcd panel driver.
 *
 * Donghwa Lee, <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <common.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mipi_dsim.h>
#include <spi.h>
#include <mipi_display.h>

#define MIN_BRIGHTNESS	0
#define MAX_BRIGHTNESS	10
#define IRQ_HANDLED	1

struct s6e39a0x02 {
	struct device	*dev;
	unsigned int	power;
	unsigned int	current_brightness;
};

static void s6e39a0x02_etc_cond1(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	unsigned char data_to_send_1[3] = {
		0xf0, 0x5a, 0x5a
	};

	unsigned char data_to_send_2[3] = {
		0xf1, 0x5a, 0x5a
	};

	unsigned char data_to_send_3[3] = {
		0xfc, 0x5a, 0x5a
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_3, sizeof(data_to_send_3));
}

static void s6e39a0x02_gamma_cond(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	unsigned char data_to_send[26] = {
		0xfa, 0x02, 0x10, 0x10, 0x10, 0xf0, 0x8c, 0xed, 0xd5,
		0xca, 0xd8, 0xdc, 0xdb, 0xdc, 0xbb, 0xbd, 0xb7, 0xcb,
		0xcd, 0xc5, 0x00, 0x9c, 0x00, 0x7a, 0x00, 0xb2
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send, sizeof(data_to_send));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xfa, 0x03);
}

static void s6e39a0x02_panel_cond(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	unsigned char data_to_send_1[14] = {
		0xf8, 0x27, 0x27, 0x08, 0x08, 0x4e, 0xaa,
		0x5e, 0x8a, 0x10, 0x3f, 0x10, 0x10, 0x00
	};

	unsigned char data_to_send_2[6] = {
		0xb3, 0x63, 0x02, 0xc3, 0x32, 0xff
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xf7, 0x03);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));
}

static void s6e39a0x02_etc_cond2(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	unsigned char data_to_send_1[4] = {
		0xf6, 0x00, 0x84, 0x09
	};

	unsigned char data_to_send_2[4] = {
		0xd5, 0xa4, 0x7e, 0x20
	};

	unsigned char data_to_send_3[4] = {
		0xb1, 0x01, 0x00, 0x16
	};

	unsigned char data_to_send_4[5] = {
		0xb2, 0x15, 0x15, 0x15, 0x15
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x09);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xd5, 0x64);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x0b);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x08);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xfd, 0xf8);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x01);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xf2, 0x07);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xb0, 0x04);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xf2, 0x4d);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_3, sizeof(data_to_send_3));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_4, sizeof(data_to_send_4));

	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x11, 0x00);
}

static void s6e39a0x02_sleep_in(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x10, 0x00);
}

static void s6e39a0x02_display_on(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x29, 0x00);
}

static void s6e39a0x02_display_off(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x28, 0x00);
}

static void s6e39a0x02_memory_window_1(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	unsigned char data_to_send_1[5] = {
		0x2a, 0x00, 0x00, 0x02, 0x57
	};

	unsigned char data_to_send_2[5] = {
		0x2b, 0x00, 0x00, 0x03, 0xff
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));

	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x2C, 0x00);
}

static void s6e39a0x02_memory_window_2(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	unsigned char data_to_send_1[5] = {
		0x2a, 0x00, 0x1e, 0x02, 0x39
	};

	unsigned char data_to_send_2[5] = {
		0x2b, 0x00, 0x00, 0x03, 0xbf
	};

	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x35, 0x00);

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_1, sizeof(data_to_send_1));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int) data_to_send_2, sizeof(data_to_send_2));

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xd1, 0x8a);
}

static void s6e39a0x02_panel_init(struct mipi_dsim_device *dsim_dev)
{
	/*
	 * in case of setting gamma and panel condition at first,
	 * it shuold be setting like below.
	 * set_gamma() -> set_panel_condition()
	 */
	udelay(10000);

	s6e39a0x02_etc_cond1(dsim_dev);
	s6e39a0x02_gamma_cond(dsim_dev);
	s6e39a0x02_panel_cond(dsim_dev);
	s6e39a0x02_etc_cond2(dsim_dev);

	udelay(120000);

	s6e39a0x02_memory_window_1(dsim_dev);

	udelay(40);

	s6e39a0x02_memory_window_2(dsim_dev);
}

static int s6e39a0x02_panel_set(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_lcd_device *dsim_lcd_dev = dsim_dev->dsim_lcd_dev;

	s6e39a0x02_panel_init(dsim_dev);

	return 0;
}

static void s6e39a0x02_display_enable(struct mipi_dsim_device *dsim_dev)
{
	s6e39a0x02_display_on(dsim_dev);
}

static struct mipi_dsim_lcd_driver s6e39a0x02_dsim_ddi_driver = {
	.name = "s6e39a0x02",
	.id = -1,

	.mipi_panel_init = s6e39a0x02_panel_set,
	.mipi_display_on = s6e39a0x02_display_enable,
};

void s6e39a0x02_init(void)
{
	s5p_mipi_dsi_register_lcd_driver(&s6e39a0x02_dsim_ddi_driver);
}

