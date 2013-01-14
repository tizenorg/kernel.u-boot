/*
 * Copyright (c) 2012 Samsung Electronics.
 * Abhilash Kesavan <a.kesavan@samsung.com>
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
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/sromc.h>

static void exynos4_i2c_config(int peripheral, int flags)
{
	struct exynos4_gpio_part1 *gpio1 =
		(struct exynos4_gpio_part1 *) samsung_get_base_gpio_part1();

	switch (peripheral) {
	case PERIPH_ID_I2C0:
		gpio_cfg_pin(&gpio1->d1, 0, GPIO_FUNC(0x2));
		gpio_cfg_pin(&gpio1->d1, 1, GPIO_FUNC(0x2));
		break;
	case PERIPH_ID_I2C1:
		gpio_cfg_pin(&gpio1->d1, 2, GPIO_FUNC(0x2));
		gpio_cfg_pin(&gpio1->d1, 3, GPIO_FUNC(0x2));
		break;
	case PERIPH_ID_I2C2:
		gpio_cfg_pin(&gpio1->a0, 6, GPIO_FUNC(0x3));
		gpio_cfg_pin(&gpio1->a0, 7, GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C3:
		gpio_cfg_pin(&gpio1->a1, 2, GPIO_FUNC(0x3));
		gpio_cfg_pin(&gpio1->a1, 3, GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C4:
		gpio_cfg_pin(&gpio1->b, 0, GPIO_FUNC(0x3));
		gpio_cfg_pin(&gpio1->b, 1, GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C5:
		gpio_cfg_pin(&gpio1->b, 2, GPIO_FUNC(0x3));
		gpio_cfg_pin(&gpio1->b, 3, GPIO_FUNC(0x3));
		break;
	case PERIPH_ID_I2C6:
		gpio_cfg_pin(&gpio1->c1, 3, GPIO_FUNC(0x4));
		gpio_cfg_pin(&gpio1->c1, 4, GPIO_FUNC(0x4));
		break;
	case PERIPH_ID_I2C7:
		gpio_cfg_pin(&gpio1->d0, 2, GPIO_FUNC(0x3));
		gpio_cfg_pin(&gpio1->d0, 3, GPIO_FUNC(0x3));
		break;
	}
}

static int exynos4_pinmux_config(int peripheral, int flags)
{
	switch (peripheral) {
	case PERIPH_ID_I2C0:
	case PERIPH_ID_I2C1:
	case PERIPH_ID_I2C2:
	case PERIPH_ID_I2C3:
	case PERIPH_ID_I2C4:
	case PERIPH_ID_I2C5:
	case PERIPH_ID_I2C6:
	case PERIPH_ID_I2C7:
		exynos4_i2c_config(peripheral, flags);
		break;
	default:
		debug("%s: invalid peripheral %d", __func__, peripheral);
		return -1;
	}

	return 0;
}

int exynos_pinmux_config(int peripheral, int flags)
{
	if (cpu_is_exynos4())
		return exynos4_pinmux_config(peripheral, flags);
	else {
		debug("pinmux functionality not supported\n");
		return -1;
	}
}
