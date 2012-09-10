/*
 * Copyright (C) 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __I2C_GPIO_H_
#define __I2C_GPIO_H_

#ifndef __ASSEMBLY__
struct i2c_gpio_bus_data {
	unsigned int gpio_base;
	unsigned int sda_pin;
	unsigned int scl_pin;
};

struct i2c_gpio_bus {
	struct i2c_gpio_bus_data *bus;
};

void i2c_gpio_init(struct i2c_gpio_bus *bus, int len, int index);
void i2c_gpio_dir(int dir);
void i2c_gpio_set(int line, int value);
int i2c_gpio_get(void);
void i2c_gpio_set_bus(int index);

#define I2C_SDA(x)		i2c_gpio_set(1, x)
#define I2C_SCL(x)		i2c_gpio_set(0, x)
#define I2C_READ		(!!i2c_gpio_get())
#define I2C_ACTIVE		i2c_gpio_dir(1)
#define I2C_TRISTATE		i2c_gpio_dir(0)
#define I2C_DELAY		udelay(5)

#endif
#endif
