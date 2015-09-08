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

#define MIN_BRIGHTNESS	0
#define MAX_BRIGHTNESS	10
#define IRQ_HANDLED	1

static struct mipi_ddi_platform_data *ddi_pd;

/* This is only used Goni Rev0.4 */
extern int s5p_dsim_register_lcd_driver(struct mipi_lcd_driver *lcd_drv);

struct s6e39a0x {
	struct device	*dev;
	unsigned int	power;
	unsigned int	current_brightness;
};

static void s6e39a0x_etc_cond(void)
{
	unsigned char data_to_send[3] = {
		0xf0, 0x5a, 0x5a
	};

	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_LONG_WR,
		(unsigned int) data_to_send, sizeof(data_to_send));
}

static void s6e39a0x_gamma_cond(void)
{
	unsigned char data_to_send[26] = {
		0xfa, 0x02, 0x18, 0x08, 0x24, 0xe0, 0xd9, 0xd5, 0xc8,
		0xcf, 0xc0, 0xd3, 0xd9, 0xcc, 0xa9, 0xb2, 0x9d, 0xbb,
		0xc1, 0xb3, 0x00, 0x9e, 0x00, 0x9a, 0x00, 0xd3
	};

	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_LONG_WR,
		(unsigned int) data_to_send, sizeof(data_to_send));
}

static void s6e39a0x_gamma_update(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_WR_1_PARA, 0xfa, 0x03);
}

static void s6e39a0x_panel_cond(void)
{
	unsigned char data_to_send[14] = {
		0xf8, 0x28, 0x28, 0x08, 0x08, 0x40, 0xb0,
		0x50, 0x90, 0x10, 0x30, 0x10, 0x00, 0x00
	};

	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_LONG_WR,
		(unsigned int) data_to_send, sizeof(data_to_send));
}
static void s6e39a0x_new1_cond(void)
{
	unsigned char data_to_send[4] = {
		0xf6, 0x00, 0x84, 0x09
	};

	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_LONG_WR,
		(unsigned int) data_to_send, sizeof(data_to_send));
}
static void s6e39a0x_etc_cond2(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_WR_1_PARA, 0xb0, 0x01);
}

static void s6e39a0x_etc_cond3(void)
{
	unsigned char data_to_send[3] = {
		0xc0, 0x00, 0x00
	};

	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_LONG_WR,
		(unsigned int) data_to_send, sizeof(data_to_send));
}

static void s6e39a0x_etc_cond4(void)
{
	unsigned char data_to_send[5] = {
		0x2a, 0x00, 0x00, 0x02, 0x57
	};

	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_LONG_WR,
		(unsigned int) data_to_send, sizeof(data_to_send));
}

static void s6e39a0x_etc_cond5(void)
{
	unsigned char data_to_send[5] = {
		0x2b, 0x00, 0x00, 0x03, 0xff
	};

	ddi_pd->cmd_write((void *)ddi_pd->dsim_data, DCS_LONG_WR,
		(unsigned int) data_to_send, sizeof(data_to_send));
}

static void s6e39a0x_etc_cond6(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xb0, 0x09);
}

static void s6e39a0x_etc_cond7(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xd5, 0x64);
}

static void s6e39a0x_etc_cond8(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xb0, 0x0b);
}

static void s6e39a0x_etc_cond9(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xd5, 0xa4);
}

static void s6e39a0x_etc_cond10(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xb0, 0x0c);
}

static void s6e39a0x_etc_cond11(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xd5, 0x7e);
}

static void s6e39a0x_etc_cond12(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xf7, 0x03);
}

static void s6e39a0x_etc_cond13(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xb0, 0x02);
}

static void s6e39a0x_etc_cond14(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xb3, 0xc3);
}

static void s6e39a0x_sleep_in(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_NO_PARA, 0x10, 0x00);
}

static void s6e39a0x_sleep_out(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_NO_PARA, 0x11, 0x00);
}

void s6e39a0x_display_on(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_NO_PARA, 0x29, 0x00);
}

static void s6e39a0x_display_off(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_NO_PARA, 0x28, 0x00);
}

static void s6e39a0x_te_on(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_NO_PARA, 0x35, 0x00);
}

static void s6e39a0x_global(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xb0, 0x01);
}

static void s6e39a0x_dispctl(void)
{
	ddi_pd->cmd_write((void *)ddi_pd->dsim_data,
		DCS_WR_1_PARA, 0xf2, 0x01);
}

int s6e39a0x_panel_init(void)
{
	/* 
	 * in case of setting gamma and panel condition at first,
	 * it shuold be setting like below.
	 * set_gamma() -> set_panel_condition()
	 */
	udelay(10000);
	s6e39a0x_etc_cond();
	s6e39a0x_gamma_cond();
	s6e39a0x_gamma_update();
	s6e39a0x_panel_cond();
	s6e39a0x_new1_cond();
	s6e39a0x_etc_cond2();
	s6e39a0x_etc_cond3();
	s6e39a0x_etc_cond4();
	s6e39a0x_etc_cond5();
	s6e39a0x_etc_cond6();
	s6e39a0x_etc_cond7();
	s6e39a0x_etc_cond8();
	s6e39a0x_etc_cond9();
	s6e39a0x_etc_cond10();
	s6e39a0x_etc_cond11();
	s6e39a0x_etc_cond12();
	s6e39a0x_etc_cond13();
	s6e39a0x_etc_cond14();
	s6e39a0x_global();
	s6e39a0x_dispctl();
	s6e39a0x_te_on();
	s6e39a0x_sleep_out();
	udelay(120000);

	return 0;
}

int s6e39a0x_set_link(struct mipi_ddi_platform_data *pd)
{
	if (pd == NULL) {
		printf("mipi_ddi_platform_data pointer is NULL.\n");
		return -1;
	}

	/* check callback functions. */
	if (pd->cmd_write == NULL) {
		printf("cmd_write function is null.\n");
		return -1;
	}

	if (pd->get_dsim_frame_done == NULL) {
		printf("dsim_frame_done function is null.\n");
		return -1;
	}

	if (pd->clear_dsim_frame_done == NULL) {
		printf("clear_dsim_frame_done function is null.\n");
		return -1;
	}

	if (pd->change_dsim_transfer_mode == NULL)
		printf("change_dsim_transfer_mode is null.\n");

	if (pd->get_fb_frame_done == NULL)
		printf("change_dsim_transfer_mode is null.\n");

	if (pd->trigger == NULL)
		printf("trigger is null.\n");

	ddi_pd = pd;
	return 0;
}

/* this function would be called at universal.c */
void s6e39a0x_set_platform_data(struct mipi_ddi_platform_data *pd)
{
	if (pd == NULL) {
		printf("pd is NULL.\n");
		return;
	}

	ddi_pd = pd;
}

static struct mipi_lcd_driver s6e39a0x_mipi_driver = {
	.name = "s6e39a0x",

	.init = s6e39a0x_panel_init,
	.display_on = s6e39a0x_display_on,
	.set_link = s6e39a0x_set_link,
};

int s6e39a0x_init(void)
{
	s5p_dsim_register_lcd_driver(&s6e39a0x_mipi_driver);

	return 0;
}
