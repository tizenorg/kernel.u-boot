/* linux/drivers/video/s6e8ax0.c
 *
 * MIPI-DSI based s6e8ax0 AMOLED panel driver.
 *
 * Inki Dae, <inki.dae@samsung.com>
 * Donghwa Lee, <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <ubi_uboot.h>
#include <common.h>
#include <lcd.h>
#include <mipi_ddi.h>
#include <mipi_display.h>
#include <asm/errno.h>
#include <asm/arch/regs-dsim.h>
#include <asm/arch/mipi_dsim.h>
#include <asm/arch/power.h>
#include <asm/arch/cpu.h>
#include <linux/types.h>
#include <linux/list.h>

#include "s5p_mipi_dsi_lowlevel.h"
#include "s5p_mipi_dsi_common.h"

#define DSIM_PM_STABLE_TIME	(10)
#define MIN_BRIGHTNESS		(0)
#define MAX_BRIGHTNESS		(10)

#define lcd_to_master(a)	(a->dsim_dev->master)
#define lcd_to_master_ops(a)	((lcd_to_master(a))->master_ops)

enum panel_type {
	TYPE_AMS465GS01_M3,	/* U1HD */
	TYPE_AMS465GS01_SM2,	/* M0_PROXIMA */
	TYPE_AMS465GS02,	/* MIDAS */
	TYPE_AMS480GYXX_SM2,	/* M0_REAL_PROXIMA */
	TYPE_AMS529HA01,	/* Q1 */
	TYPE_AMS767KC01,	/* P8 */
};

enum {
	DSIM_NONE_STATE = 0,
	DSIM_RESUME_COMPLETE = 1,
	DSIM_FRAME_DONE = 2,
};

struct s6e8ax0 {
	unsigned int			power;
	unsigned int			updated;
	unsigned int			gamma;
	unsigned int			resume_complete;

	struct mipi_dsim_lcd_device	*lcd_dev;
	struct lcd_platform_data	*ddi_pd;
};

struct s6e8ax0_device_id {
	char name[16];
	enum panel_type type;
};

static struct s6e8ax0_device_id s6e8ax0_ids[] = {
	{
		.name = "ams465gs01-m3",
		.type = TYPE_AMS465GS01_M3,	/* U1, U1HD */
	}, {
		.name = "ams465gs01-sm2",
		.type = TYPE_AMS465GS01_SM2,	/* U1HD_5INCH, M0_PROXIMA */
	}, {
		.name = "ams465gs02",
		.type = TYPE_AMS465GS02,	/* MIDAS */
	}, {
		.name = "ams480gyxx-sm2",
		.type = TYPE_AMS480GYXX_SM2,	/* M0_REAL_PROXIMA */
	}, {
		.name = "ams529ha01",
		.type = TYPE_AMS529HA01,	/* Q1 */
	}, {
		.name = "ams767kc01",
		.type = TYPE_AMS767KC01,	/* P8 */
	}, { },
};

const enum panel_type s6e8ax0_get_device_type(const struct mipi_dsim_lcd_device *lcd_dev)
{
	struct s6e8ax0_device_id *id = s6e8ax0_ids;

	while (id->name[0]) {
		if (!strcmp(lcd_dev->panel_id, id->name))
			return id->type;
		id++;
	}
	return -1;
}

static void s6e8ax0_panel_cond(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	int reverse = dsim_dev->dsim_lcd_dev->reverse;

	const unsigned char data_to_send_ams465gs01[] = {
		0xf8, 0x3d, 0x35, 0x00, 0x00, 0x00, 0x8d, 0x00, 0x4c,
		0x6e, 0x10, 0x27, 0x7d, 0x3f, 0x10, 0x00, 0x00, 0x20,
		0x04, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08,
		0x23, 0x23, 0xc0, 0xc8, 0x08, 0x48, 0xc1, 0x00, 0xc3,
		0xff, 0xff, 0xc8
	};

	/* U1HD_5INCH - scan direction is reversed
	   against U1, U1HD, PROXIMA */
	const unsigned char data_to_send_reverse_ams465gs01[] = {
		0xf8, 0x19, 0x35, 0x00, 0x00, 0x00, 0x93, 0x00, 0x3c,
		0x7d, 0x08, 0x27, 0x7d, 0x3f, 0x00, 0x00, 0x00, 0x20,
		0x04, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08,
		0x23, 0x23, 0xc0, 0xc1, 0x01, 0x41, 0xc1, 0x00, 0xc1,
		0xf6, 0xf6, 0xc1
	};

	const unsigned char data_to_send_ams465gs02[] = {
		0xf8, 0x19, 0x35, 0x00, 0x00, 0x00, 0x93, 0x00, 0x3c,
		0x7d, 0x08, 0x27, 0x7d, 0x3f, 0x00, 0x00, 0x00, 0x20,
		0x04, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08,
		0x23, 0x23, 0xc0, 0xc1, 0x01, 0x41, 0xc1, 0x00, 0xc1,
		0xf6, 0xf6, 0xc1
	};

	const unsigned char data_to_send_ams480gyxx[] = {
		0xf8, 0x19, 0x35, 0x00, 0x00, 0x00, 0x93, 0x00, 0x3c,
		0x7d, 0x08, 0x27, 0x7d, 0x3f, 0x00, 0x00, 0x00, 0x20,
		0x04, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08,
		0x23, 0x23, 0xc0, 0xc1, 0x01, 0x41, 0xc1, 0x00, 0xc1,
		0xf6, 0xf6, 0xc1
	};

	const unsigned char data_to_send_ams529ha01[] = {
		0xf8, 0x25, 0x34, 0x00, 0x00, 0x00, 0x95, 0x00, 0x3c,
		0x7d, 0x08, 0x27, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08,
		0x23, 0x63, 0xc0, 0xc1, 0x01, 0x81, 0xc1, 0x00, 0xc8,
		0xc1, 0xd3, 0x01
	};

	const unsigned char data_to_send_ams767kc01[] = {
		0xf8, 0x01, 0x8e, 0x00, 0x00, 0x00, 0xac, 0x00, 0x9e,
		0x8d, 0x1f, 0x4e, 0x9c, 0x7d, 0x3f, 0x10, 0x00, 0x20,
		0x02, 0x10, 0x7d, 0x10, 0x00, 0x00, 0x02, 0x08, 0x10,
		0x34, 0x34, 0x34, 0xc0, 0xc1, 0x01, 0x00, 0xc1, 0x82,
		0x00, 0xc8, 0xc1, 0xe3, 0x01
	};

	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS465GS02:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams465gs02,
			ARRAY_SIZE(data_to_send_ams465gs02));
		break;
	case TYPE_AMS529HA01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams529ha01,
			ARRAY_SIZE(data_to_send_ams529ha01));
		break;
	case TYPE_AMS767KC01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01,
			ARRAY_SIZE(data_to_send_ams767kc01));
		break;
	case TYPE_AMS480GYXX_SM2:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams480gyxx,
			ARRAY_SIZE(data_to_send_ams767kc01));
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS01_SM2:
	default:
		if (reverse)
			ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
				(unsigned int)data_to_send_reverse_ams465gs01,
				ARRAY_SIZE(data_to_send_reverse_ams465gs01));
		else
			ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
				(unsigned int)data_to_send_ams465gs01,
				ARRAY_SIZE(data_to_send_ams465gs01));
		break;
	}
}

static void s6e8ax0_display_cond(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	const unsigned char data_to_send_default[] = {
		0xf2, 0x80, 0x03, 0x0d
	};
	const unsigned char data_to_send_ams767kc01[] = {
		0xf2, 0xc8, 0x05, 0x0d
	};

	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS767KC01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01,
			ARRAY_SIZE(data_to_send_ams767kc01));
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS01_SM2:
	case TYPE_AMS465GS02:
	case TYPE_AMS480GYXX_SM2:
	case TYPE_AMS529HA01:
	default:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_default,
			ARRAY_SIZE(data_to_send_default));
	}
}

static void s6e8ax0_gamma_cond(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	/* 7500K 2.2 Set (M3, 300cd) */
	const unsigned char data_to_send_default[] = {
		0xfa, 0x01, 0x0f, 0x00, 0x0f, 0xda, 0xc0, 0xe4, 0xc8,
		0xc8, 0xc6, 0xd3, 0xd6, 0xd0, 0xab, 0xb2, 0xa6, 0xbf,
		0xc2, 0xb9, 0x00, 0x93, 0x00, 0x86, 0x00, 0xd1
	};

	/* 7500K 2.2 Set (SM2, 300cd) */
	const unsigned char data_to_send_ams465gs01_sm2[] = {
		0xfa, 0x01, 0x58, 0x1f, 0x63, 0xac, 0xb4, 0x99, 0xad,
		0xba, 0xa3, 0xc0, 0xc8, 0xbb, 0x93, 0x9f, 0x8b, 0xad,
		0xb4, 0xa7, 0x00, 0xbe, 0x00, 0xab, 0x00, 0xe7
	};

	const unsigned char data_to_send_ams767kc01[] = {
		0xfa, 0x36, 0x10, 0x48, 0xb8, 0xaa, 0xa9, 0xb8, 0xc3,
		0xb7, 0xc6, 0xd2, 0xc1, 0x9a, 0xaa, 0x91, 0xb5, 0xc0,
		0xab, 0x00, 0x8c, 0x00, 0x8d, 0x00, 0xc9
	};

	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS465GS01_SM2:
	case TYPE_AMS480GYXX_SM2:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams465gs01_sm2,
			ARRAY_SIZE(data_to_send_ams465gs01_sm2));
		break;
	case TYPE_AMS767KC01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01,
			ARRAY_SIZE(data_to_send_ams767kc01));
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS02:
	case TYPE_AMS529HA01:
	default:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_default,
			ARRAY_SIZE(data_to_send_default));
	}
}

static void s6e8ax0_gamma_update(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	unsigned char param;

	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS767KC01:
		param = 0x02;
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS01_SM2:
	case TYPE_AMS465GS02:
	case TYPE_AMS480GYXX_SM2:
	case TYPE_AMS529HA01:
	default:
		param = 0x03;
	}

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xf7, param);
}

static void s6e8ax0_etc_source_control(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	const unsigned char data_to_send_default[] = {
		0xf6, 0x00, 0x02, 0x00
	};

	const unsigned char data_to_send_ams767kc01[] = {
		0xd1, 0xfe, 0x80, 0x00, 0x01, 0x0b, 0x00, 0x00, 0x40,
		0x0d, 0x00, 0x00
	};

	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS767KC01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01,
			ARRAY_SIZE(data_to_send_ams767kc01));
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS01_SM2:
	case TYPE_AMS465GS02:
	case TYPE_AMS480GYXX_SM2:
	case TYPE_AMS529HA01:
	default:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_default,
			ARRAY_SIZE(data_to_send_default));
	}
}

static void s6e8ax0_etc_pentile_control(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	const unsigned char data_to_send_default[] = {
		0xb6, 0x0c, 0x02, 0x03, 0x32, 0xff, 0x44, 0x44, 0xc0,
		0x00
	};

	const unsigned char data_to_send_ams767kc01[] = {
		0xd9, 0x14, 0x5c, 0x20, 0x0c, 0x0f, 0x41, 0x00, 0x10,
		0x11, 0x12, 0xa8, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x80,
		0xcb, 0xed, 0x64, 0xaf
	};

	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS767KC01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01,
			ARRAY_SIZE(data_to_send_ams767kc01));
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS01_SM2:
	case TYPE_AMS465GS02:
	case TYPE_AMS480GYXX_SM2:
	case TYPE_AMS529HA01:
	default:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_default,
			ARRAY_SIZE(data_to_send_default));
	}
}

static void s6e8ax0_elvss_nvm_set(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	unsigned char data_to_send[] = {
		0xd9, 0x14, 0x40, 0x0c, 0xcb, 0xce, 0x6e, 0xc4, 0x0f,
		0x40, 0x41, 0xd0, 0x00, 0x60, 0x19
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8ax0_etc_mipi_control1(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	const unsigned char data_to_send_default[] = {
		0xe1, 0x10, 0x1c, 0x17, 0x08, 0x1d
	};

	const unsigned char data_to_send_ams767kc01[] = {
		0xf4, 0x0b, 0x0a, 0x06, 0x0b, 0x33, 0x02
	};

	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS767KC01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01,
			ARRAY_SIZE(data_to_send_ams767kc01));
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS01_SM2:
	case TYPE_AMS465GS02:
	case TYPE_AMS480GYXX_SM2:
	case TYPE_AMS529HA01:
	default:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_default,
			ARRAY_SIZE(data_to_send_default));
	}
}

static void s6e8ax0_etc_mipi_control2(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	const unsigned char data_to_send_default[] = {
		0xe2, 0xed, 0x07, 0xc3, 0x13, 0x0d, 0x03
	};

	const unsigned char data_to_send_ams767kc01[] = {
		0xf6, 0x04, 0x00, 0x02
	};

	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS767KC01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01,
			ARRAY_SIZE(data_to_send_ams767kc01));
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS01_SM2:
	case TYPE_AMS465GS02:
	case TYPE_AMS480GYXX_SM2:
	case TYPE_AMS529HA01:
	default:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_default,
			ARRAY_SIZE(data_to_send_default));
	}
}

static void s6e8ax0_etc_power_control(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	const unsigned char data_to_send[] = {
		0xf4, 0xcf, 0x0a, 0x12, 0x10, 0x19, 0x33, 0x02
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}
static void s6e8ax0_etc_mipi_control3(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xe3, 0x40);
}

static void s6e8ax0_etc_mipi_control4(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	const unsigned char data_to_send[] = {
		0xe4, 0x00, 0x00, 0x14, 0x80, 0x00, 0x00, 0x00
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8ax0_elvss_set(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;

	const unsigned char data_to_send_default[] = {
		0xb1, 0x04, 0x00
	};

	const unsigned char data_to_send_ams767kc01_1[] = {
		0xb2, 0x04, 0x04, 0x04, 0x04, 0x04
	};

	const unsigned char data_to_send_ams767kc01_2[] = {
		0xb1, 0x42, 0x00
	};


	switch (dsim_dev->dsim_lcd_dev->panel_type) {
	case TYPE_AMS767KC01:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01_1,
			ARRAY_SIZE(data_to_send_ams767kc01_1));
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_ams767kc01_2,
			ARRAY_SIZE(data_to_send_ams767kc01_2));
		break;
	case TYPE_AMS465GS01_M3:
	case TYPE_AMS465GS01_SM2:
	case TYPE_AMS465GS02:
	case TYPE_AMS480GYXX_SM2:
	case TYPE_AMS529HA01:
	default:
		ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
			(unsigned int)data_to_send_default,
			ARRAY_SIZE(data_to_send_default));
	}
}

static void s6e8ax0_display_on(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x29, 0x00);
}

static void s6e8ax0_sleep_in(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x10, 0x00);
}

static void s6e8ax0_sleep_out(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x11, 0x00);
}

static void s6e8ax0_display_off(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0x28, 0x00);
}

static void s6e8ax0_apply_level1_key(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	const unsigned char data_to_send[] = {
		0xf0, 0x5a, 0x5a
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8ax0_apply_level2_key(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	const unsigned char data_to_send[] = {
		0xfc, 0x5a, 0x5a
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8ax0_apply_mtp_key(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	const unsigned char data_to_send[] = {
		0xf1, 0x5a, 0x5a
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

/* Full white 50% reducing setting */
static void s6e8ax0_acl_ctrl_set(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	const unsigned char data_to_send[] = {
		0xc1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x03, 0x1f,
		0x00, 0x00, 0x04, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x0f, 0x16, 0x1d, 0x24, 0x2a, 0x31, 0x38,
		0x3f, 0x46
	};

	ops->cmd_write(dsim_dev, MIPI_DSI_DCS_LONG_WRITE,
		(unsigned int)data_to_send, ARRAY_SIZE(data_to_send));
}

static void s6e8ax0_acl_on(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0xc0, 0x01);
}

static void s6e8ax0_acl_off(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_master_ops *ops = dsim_dev->master_ops;
	ops->cmd_write(dsim_dev,
		MIPI_DSI_DCS_SHORT_WRITE, 0xc0, 0x00);
}

static void s6e8ax0_panel_init(struct mipi_dsim_device *dsim_dev)
{
	enum panel_type panel = dsim_dev->dsim_lcd_dev->panel_type;

	/*
	 * in case of setting gamma and panel condition at first,
	 * it shuold be setting like below.
	 * set_gamma() -> set_panel_condition()
	 */

	s6e8ax0_apply_level1_key(dsim_dev);
	if (panel == TYPE_AMS529HA01)
		s6e8ax0_apply_level2_key(dsim_dev);
	else
		s6e8ax0_apply_mtp_key(dsim_dev);

	s6e8ax0_sleep_out(dsim_dev);
	udelay(5 * 1000);
	s6e8ax0_panel_cond(dsim_dev);
	s6e8ax0_display_cond(dsim_dev);
	s6e8ax0_gamma_cond(dsim_dev);
	s6e8ax0_gamma_update(dsim_dev);

	s6e8ax0_etc_source_control(dsim_dev);
	s6e8ax0_elvss_set(dsim_dev);
	s6e8ax0_etc_pentile_control(dsim_dev);
	s6e8ax0_elvss_nvm_set(dsim_dev);
	s6e8ax0_etc_mipi_control1(dsim_dev);
	s6e8ax0_etc_mipi_control2(dsim_dev);
	if (panel != TYPE_AMS767KC01) {
		s6e8ax0_etc_power_control(dsim_dev);
		s6e8ax0_etc_mipi_control3(dsim_dev);
		s6e8ax0_etc_mipi_control4(dsim_dev);
	}
}

static int s6e8ax0_panel_set(struct mipi_dsim_device *dsim_dev)
{
	struct mipi_dsim_lcd_device *dsim_lcd_dev = dsim_dev->dsim_lcd_dev;
	dsim_lcd_dev->panel_type = s6e8ax0_get_device_type(dsim_lcd_dev);
	if (dsim_lcd_dev->panel_type == -1)
		printf("error: can't found panel type on s6e8ax0\n");

	s6e8ax0_panel_init(dsim_dev);
	return 0;
}

static void s6e8ax0_display_enable(struct mipi_dsim_device *dsim_dev)
{
	s6e8ax0_display_on(dsim_dev);
}

static struct mipi_dsim_lcd_driver s6e8ax0_dsim_ddi_driver = {
	.name = "s6e8ax0",
	.id = -1,

	.mipi_panel_init = s6e8ax0_panel_set,
	.mipi_display_on = s6e8ax0_display_enable,
};

void s6e8ax0_init(void)
{
	s5p_mipi_dsi_register_lcd_driver(&s6e8ax0_dsim_ddi_driver);
}
