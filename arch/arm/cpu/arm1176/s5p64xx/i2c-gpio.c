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

#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <i2c-gpio.h>
#include <i2c.h>

static struct i2c_gpio_bus *i2c_gpio;

void i2c_gpio_init(struct i2c_gpio_bus *bus, int len, int index)
{
	int i;
	struct s5p_gpio_bank *bank;

	i2c_gpio = bus;

	/* init all i2c gpio buses */
	for (i = 0; i < len; i++) {
		bank = (struct s5p_gpio_bank *)i2c_gpio[i].bus->gpio_base;

		/* SDA */
		gpio_direction_output(bank,
				i2c_gpio[i].bus->sda_pin, 1);

		/* SCL */
		gpio_direction_output(bank,
				i2c_gpio[i].bus->scl_pin, 1);
	}

	/* set default bus */
	i2c_set_bus_num(index);
}

void i2c_gpio_set(int line, int value)
{
	struct s5p_gpio_bank *bank;
	unsigned int bus_index;

	bus_index = i2c_get_bus_num();

	bank = (struct s5p_gpio_bank *)i2c_gpio[bus_index].bus->gpio_base;

	if (line)
		line = i2c_gpio[bus_index].bus->sda_pin;
	else
		line = i2c_gpio[bus_index].bus->scl_pin;

	gpio_set_value(bank, line, value);
}

int i2c_gpio_get(void)
{
	struct s5p_gpio_bank *bank;
	unsigned int bus_index;

	bus_index = i2c_get_bus_num();

	bank = (struct s5p_gpio_bank *)i2c_gpio[bus_index].bus->gpio_base;

	return gpio_get_value(bank, i2c_gpio[bus_index].bus->sda_pin);
}

void i2c_gpio_dir(int dir)
{
	struct s5p_gpio_bank *bank;
	unsigned int bus_index;

	bus_index = i2c_get_bus_num();

	bank = (struct s5p_gpio_bank *)i2c_gpio[bus_index].bus->gpio_base;

	if (dir) {
		gpio_direction_output(bank,
				i2c_gpio[bus_index].bus->sda_pin, 0);
	} else {
		gpio_direction_input(bank,
				i2c_gpio[bus_index].bus->sda_pin);
	}
}
