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

#ifndef _MULTI_I2C_H
#define _MULTI_I2C_H

#define SOFT_I2C_DEV  (1 << 1)
#define HARD_I2C_DEV  (1 << 2)

enum {
	MULTI_I2C_SOFT,
	MULTI_I2C_HARD,
	MULTI_I2C_MAX,
};

struct i2c_ops {
	void (*init) (int speed, int slaveadd);
	int (*probe) (uchar chip);
	int (*read) (uchar chip, uint addr, int alen,
		uchar *buffer, int len);
	int (*read_r) (uchar chip, uint addr, int alen,
		uchar *buffer, int len);
	int (*write) (uchar chip, uint addr, int alen,
		uchar *buffer, int len);
	unsigned int (*get_bus_num) (void);
	int (*set_bus_num) (unsigned int bus);
	void (*reset) (void);
};

/* Device information */
struct multi_i2c_dev {
	struct i2c_ops *ops;
};

#ifdef CONFIG_SOFT_I2C
extern struct i2c_ops soft_i2c_ops;
#endif
#ifdef CONFIG_DRIVER_S3C24X0_I2C
extern struct i2c_ops s3c24x0_i2c_ops;
#endif

#endif /* _MULTI_I2C_H */
