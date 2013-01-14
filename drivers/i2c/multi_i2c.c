/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd. All rights reserved.
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

#include <common.h>
#include <asm/errno.h>
#include <multi_i2c.h>


static struct multi_i2c_dev multi_i2c_devs[MULTI_I2C_MAX];
static struct multi_i2c_dev *active_dev;
static int count_devices;

static struct multi_i2c_dev *multi_i2c_get_by_bus(const unsigned int bus)
{
	/* HARD I2C MAX NUM */
	if (bus > CONFIG_MAX_I2C_NUM)
		return &multi_i2c_devs[MULTI_I2C_SOFT];

	return &multi_i2c_devs[MULTI_I2C_HARD];
}

static void multi_i2c_register(struct i2c_ops *ops, u32 flags)
{
	int index;

	if (flags & SOFT_I2C_DEV)
		index = MULTI_I2C_SOFT;
	else
		index = MULTI_I2C_HARD;

	multi_i2c_devs[index].ops = ops;
	active_dev = &multi_i2c_devs[index];
	count_devices++;
}

void i2c_init(int speed, int slaveadd)
{
	int i;

	count_devices = 0;

#ifdef CONFIG_SOFT_I2C
	multi_i2c_register(&soft_i2c_ops, SOFT_I2C_DEV);
#endif
#ifdef CONFIG_DRIVER_S3C24X0_I2C
	multi_i2c_register(&s3c24x0_i2c_ops, HARD_I2C_DEV);
#endif
	for (i = 0; i < count_devices; i++) {
		if (multi_i2c_devs[i].ops->init) {
			multi_i2c_devs[i].ops->init(speed, slaveadd);
		}
	}
}

int i2c_probe(uchar chip)
{
	return active_dev->ops->probe(chip);
}

int i2c_read(uchar chip, uint addr, int alen,
		uchar *buffer, int len)
{
	return active_dev->ops->read(chip, addr, alen,
			buffer, len);
}

int i2c_write(uchar chip, uint addr, int alen,
		uchar *buffer, int len)
{
	return active_dev->ops->write(chip, addr, alen,
			buffer, len);
}

unsigned int i2c_get_bus_num(void)
{
	return active_dev->ops->get_bus_num();
}

int i2c_set_bus_num(unsigned int bus)
{
	struct multi_i2c_dev *dev;

	if (bus < 0 || bus > CONFIG_SYS_MAX_I2C_BUS)
		return -1;

	dev = multi_i2c_get_by_bus(bus);
	if (dev) {
		active_dev = dev;
		return dev->ops->set_bus_num(bus);
	}

	printf("%s: Can not find I2C dev for %d bus\n",
				__func__, bus);
	return -EINVAL;
}

void i2c_reset(void)
{
	int i;

	for (i = 0; i < count_devices; i++) {
		if (multi_i2c_devs[i].ops->reset) {
			multi_i2c_devs[i].ops->reset();
		}
	}
}
