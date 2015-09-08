/* linux/drivers/video/s5p_mipi_dsi.c
 *
 * Samsung SoC MIPI-DSIM driver.
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * InKi Dae, <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <ubi_uboot.h>
#include <common.h>
#include <lcd.h>
#include <mipi_ddi.h>
#include <asm/errno.h>
#include <asm/arch/regs-dsim.h>
#include <asm/arch/mipi_dsim.h>
#include <asm/arch/power.h>
#include <asm/arch/cpu.h>
#include <linux/types.h>
#include <linux/list.h>

#include "s5p_mipi_dsi_lowlevel.h"
#include "s5p_mipi_dsi_common.h"

#define master_to_driver(a)	(a->dsim_lcd_drv)
#define master_to_device(a)	(a->dsim_lcd_dev)

static struct s5p_platform_mipi_dsim *dsim_pd = NULL;
//static struct mipi_dsim_device *dsim = NULL;

struct mipi_dsim_ddi {
	int				bus_id;
	struct list_head		list;
	struct mipi_dsim_lcd_device	*dsim_lcd_dev;
	struct mipi_dsim_lcd_driver	*dsim_lcd_drv;
};

static LIST_HEAD(dsim_ddi_list);
static LIST_HEAD(dsim_lcd_dev_list);

int s5p_mipi_dsi_register_lcd_device(struct mipi_dsim_lcd_device *lcd_dev)
{
	struct mipi_dsim_ddi *dsim_ddi;

	if (!lcd_dev) {
		udebug("mipi_dsim_lcd_device is NULL.\n");
		return -EFAULT;
	}

	if (!lcd_dev->name) {
		udebug("dsim_lcd_device name is NULL.\n");
		return -EFAULT;
	}

	dsim_ddi = kzalloc(sizeof(struct mipi_dsim_ddi), GFP_KERNEL);
	if (!dsim_ddi) {
		udebug("failed to allocate dsim_ddi object.\n");
		return -EFAULT;
	}

	dsim_ddi->dsim_lcd_dev = lcd_dev;

	list_add_tail(&dsim_ddi->list, &dsim_ddi_list);

	return 0;
}

struct mipi_dsim_ddi
	*s5p_mipi_dsi_find_lcd_device(struct mipi_dsim_lcd_driver *lcd_drv)
{
	struct mipi_dsim_ddi *dsim_ddi;
	struct mipi_dsim_lcd_device *lcd_dev;

	list_for_each_entry(dsim_ddi, &dsim_ddi_list, list) {
		lcd_dev = dsim_ddi->dsim_lcd_dev;
		if (!lcd_dev)
			continue;

		if (lcd_drv->id >= 0) {
			if ((strcmp(lcd_drv->name, lcd_dev->name)) == 0 &&
					lcd_drv->id == lcd_dev->id) {
				/**
				 * bus_id would be used to identify
				 * connected bus.
				 */
				dsim_ddi->bus_id = lcd_dev->bus_id;

				return dsim_ddi;
			}
		} else {
			if ((strcmp(lcd_drv->name, lcd_dev->name)) == 0) {
				/**
				 * bus_id would be used to identify
				 * connected bus.
				 */
				dsim_ddi->bus_id = lcd_dev->bus_id;

				return dsim_ddi;
			}
		}

		kfree(dsim_ddi);
		list_del(&dsim_ddi_list);
	}


	return NULL;
}

int s5p_mipi_dsi_register_lcd_driver(struct mipi_dsim_lcd_driver *lcd_drv)
{
	struct mipi_dsim_ddi *dsim_ddi;

	if (!lcd_drv) {
		udebug("mipi_dsim_lcd_driver is NULL.\n");
		return -EFAULT;
	}

	if (!lcd_drv->name) {
		udebug("dsim_lcd_driver name is NULL.\n");
		return -EFAULT;
	}

	dsim_ddi = s5p_mipi_dsi_find_lcd_device(lcd_drv);
	if (!dsim_ddi) {
		udebug("mipi_dsim_ddi object not found.\n");
		return -EFAULT;
	}

	dsim_ddi->dsim_lcd_drv = lcd_drv;

	udebug("registered panel driver(%s) to mipi-dsi driver.\n",
		lcd_drv->name);

	return 0;

}

struct mipi_dsim_ddi
	*s5p_mipi_dsi_bind_lcd_ddi(struct mipi_dsim_device *dsim,
			const char *name)
{
	struct mipi_dsim_ddi *dsim_ddi;
	struct mipi_dsim_lcd_driver *lcd_drv;
	struct mipi_dsim_lcd_device *lcd_dev;
	int ret;

	list_for_each_entry(dsim_ddi, &dsim_ddi_list, list) {
		lcd_drv = dsim_ddi->dsim_lcd_drv;
		lcd_dev = dsim_ddi->dsim_lcd_dev;
		if (!lcd_drv || !lcd_dev)
				continue;

		udebug("lcd_drv->id = %d, lcd_dev->id = %d\n", lcd_drv->id, lcd_dev->id);

		if ((strcmp(lcd_drv->name, name) == 0)) {
			lcd_dev->master = dsim;

			dsim->dsim_lcd_dev = lcd_dev;
			dsim->dsim_lcd_drv = lcd_drv;

			return dsim_ddi;
		}
	}

	return NULL;
}

/* define MIPI-DSI Master operations. */
static struct mipi_dsim_master_ops master_ops = {
	.cmd_write			= s5p_mipi_dsi_wr_data,
	.get_dsim_frame_done		= s5p_mipi_dsi_get_frame_done_status,
	.clear_dsim_frame_done		= s5p_mipi_dsi_clear_frame_done,
};

#ifdef CONFIG_SLP_NAPLES
extern void s3cfb_set_trigger(void);
#endif

int s5p_mipi_dsi_init(void)
{
	struct mipi_dsim_device *dsim;
	struct mipi_dsim_config *dsim_config;
	struct mipi_dsim_ddi *dsim_ddi;
	int ret = -EINVAL;

	dsim = kzalloc(sizeof(struct mipi_dsim_device), GFP_KERNEL);
	if (!dsim) {
		udebug("failed to allocate dsim object.\n");
		return -EFAULT;
	}

	/* get mipi_dsim_config. */
	dsim_config = dsim_pd->dsim_config;
	if (dsim_config == NULL) {
		udebug("failed to get dsim config data.\n");
		return -EFAULT;
	}

	dsim->pd = dsim_pd;
	dsim->dsim_config = dsim_config;
	dsim->master_ops = &master_ops;


	/* bind lcd ddi matched with panel name. */
	dsim_ddi = s5p_mipi_dsi_bind_lcd_ddi(dsim, dsim_pd->lcd_panel_name);
	if (!dsim_ddi) {
		udebug("mipi_dsim_ddi object not found.\n");
		return -ENOSYS;
	}
	if (dsim_pd->lcd_power)
		dsim_pd->lcd_power();

	if (dsim_pd->mipi_power)
		dsim_pd->mipi_power();

	if (dsim_pd->phy_enable)
		dsim_pd->phy_enable(1, S5P_MIPI_PHY_MRESETN);

	s5p_mipi_dsi_init_dsim(dsim);
	s5p_mipi_dsi_init_link(dsim);
	s5p_mipi_dsi_set_hs_enable(dsim);

	/* set display timing. */
	s5p_mipi_dsi_set_display_mode(dsim, dsim->dsim_config);

	/* initialize mipi-dsi client(lcd panel). */
	if (dsim_ddi->dsim_lcd_drv && dsim_ddi->dsim_lcd_drv->mipi_panel_init) {
		dsim_ddi->dsim_lcd_drv->mipi_panel_init(dsim);
		dsim_ddi->dsim_lcd_drv->mipi_display_on(dsim);
	}

#ifdef CONFIG_SLP_NAPLES
	/* set trigger. */
	if (dsim_config->e_interface == DSIM_COMMAND)
		s3cfb_set_trigger();
#endif

	udebug("mipi-dsi driver(%s mode) has been probed.\n",
		(dsim_config->e_interface == DSIM_COMMAND) ?
			"CPU" : "RGB");

	return 0;
}

void s5p_set_dsim_platform_data(struct s5p_platform_mipi_dsim *pd)
{
	if (pd == NULL) {
		udebug("pd is NULL\n");
		return;
	}

	dsim_pd = pd;
}
