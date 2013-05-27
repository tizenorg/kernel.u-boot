/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Gwuieon Jin <ge.jin@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <ubi_uboot.h>
#include <common.h>
#include <lcd.h>
#include <mipi_ddi.h>
#include <mipi_display.h>
#include <linux/errno.h>
#include <asm/arch/regs-dsim.h>
#include <asm/arch/mipi_dsim.h>
#include <asm/arch/power.h>
#include <asm/arch/cpu.h>
#include <linux/types.h>
#include <linux/list.h>

#include "s5p_mipi_dsi_lowlevel.h"
#include "s5p_mipi_dsi_common.h"

/* To expand supported LCD */
enum panel_type {
	TYPE_ACX445AKM,	/* REDWOOD */
};

struct s6d6aa1_device_id {
	char name[16];
	enum panel_type type;
};

static struct s6d6aa1_device_id s6d6aa1_ids[] = {
	{
		.name = "acx445akm",
		.type = TYPE_ACX445AKM,	/* REDWOOD */
	}, { },
};

const enum panel_type s6d6aa1_get_device_type(const struct mipi_dsim_lcd_device *lcd_dev)
{
	struct s6d6aa1_device_id *id = s6d6aa1_ids;

	while (id->name[0]) {
		if (!strcmp(lcd_dev->panel_id, id->name))
			return id->type;
		id++;
	}
	return -1;
}

static void s6d6aa1_sleep_out(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x11, 0x00);
}

static void s6d6aa1_disp_ctl(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0x36, 0xC0);
}

static void s6d6aa1_write_disbv(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0x51, 0x7f);
}

static void s6d6aa1_write_ctrld(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0x53, 0x2C);
}

static void s6d6aa1_write_cabc(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	/* WM_MODE_NORMAL */
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0x55, 0x00);
}

static void s6d6aa1_display_on(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x29, 0x00);
}

static void s6d6aa1_panel_init(struct mipi_dsim_device *dsim_dev)
{
	enum panel_type panel = dsim_dev->dsim_lcd_dev->panel_type;

	s6d6aa1_sleep_out(dsim_dev);
	/* 140msec */
	udelay(140 * 1000);
	s6d6aa1_disp_ctl(dsim_dev);
}

static int s6d6aa1_panel_set(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_lcd_device *dsim_lcd_dev = dsim_dev->dsim_lcd_dev;
	dsim_lcd_dev->panel_type = s6d6aa1_get_device_type(dsim_lcd_dev);
	if (dsim_lcd_dev->panel_type == -1)
		printf("error: can't found panel type on s6d6aa1\n");

	s6d6aa1_panel_init(dsim_dev);
	return 0;
}

static void s6d6aa1_display_enable(struct mipi_dsim_device *dsim_dev)
{
	s6d6aa1_write_disbv(dsim_dev);
	s6d6aa1_write_ctrld(dsim_dev);
	s6d6aa1_write_cabc(dsim_dev);
	s6d6aa1_display_on(dsim_dev);
}

static struct mipi_dsim_lcd_driver s6d6aa1_dsim_ddi_driver = {
	.name = "s6d6aa1",
	.id = -1,

	.mipi_panel_init = s6d6aa1_panel_set,
	.mipi_display_on = s6d6aa1_display_enable,
};

void s6d6aa1_init(void)
{
	s5p_mipi_dsi_register_lcd_driver(&s6d6aa1_dsim_ddi_driver);
}
