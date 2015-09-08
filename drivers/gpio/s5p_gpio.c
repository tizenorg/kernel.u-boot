/*
 * (C) Copyright 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
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
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>

#define CON_MASK(x)		(0xf << ((x) << 2))
#define CON_SFR(x, v)		((v) << ((x) << 2))

#define DAT_MASK(x)		(0x1 << (x))
#define DAT_SET(x)		(0x1 << (x))

#define PULL_MASK(x)		(0x3 << ((x) << 1))
#define PULL_MODE(x, v)		((v) << ((x) << 1))

#define DRV_MASK(x)		(0x3 << ((x) << 1))
#define DRV_SET(x, m)		((m) << ((x) << 1))
#define RATE_MASK(x)		(0x1 << (x + 16))
#define RATE_SET(x)		(0x1 << (x + 16))

void gpio_cfg_pin(struct s5p_gpio_bank *bank, int gpio, int cfg)
{
	unsigned int value;

	value = readl(&bank->con);
	value &= ~CON_MASK(gpio);
	value |= CON_SFR(gpio, cfg);
	writel(value, &bank->con);
	if (s5p_get_cpu_rev() == 0)
		value = readl(&bank->con);
}

void gpio_direction_output(struct s5p_gpio_bank *bank, int gpio, int en)
{
	unsigned int value;

	value = readl(&bank->dat);
	value &= ~DAT_MASK(gpio);
	if (en)
		value |= DAT_SET(gpio);
	writel(value, &bank->dat);
	if (s5p_get_cpu_rev() == 0)
		value = readl(&bank->dat);

	gpio_cfg_pin(bank, gpio, GPIO_OUTPUT);
}

void gpio_direction_input(struct s5p_gpio_bank *bank, int gpio)
{
	gpio_cfg_pin(bank, gpio, GPIO_INPUT);
}

void gpio_set_value(struct s5p_gpio_bank *bank, int gpio, int en)
{
	unsigned int value;

	value = readl(&bank->dat);
	value &= ~DAT_MASK(gpio);
	if (en)
		value |= DAT_SET(gpio);
	writel(value, &bank->dat);
	if (s5p_get_cpu_rev() == 0)
		value = readl(&bank->dat);
}

unsigned int gpio_get_value(struct s5p_gpio_bank *bank, int gpio)
{
	unsigned int value;

	value = readl(&bank->dat);
	return !!(value & DAT_MASK(gpio));
}

void gpio_set_pull(struct s5p_gpio_bank *bank, int gpio, int mode)
{
	unsigned int value;

	value = readl(&bank->pull);
	value &= ~PULL_MASK(gpio);

	switch (mode) {
	case GPIO_PULL_DOWN:
	case GPIO_PULL_UP:
		value |= PULL_MODE(gpio, mode);
		break;
	default:
		break;
	}

	writel(value, &bank->pull);
	if (s5p_get_cpu_rev() == 0)
		value = readl(&bank->pull);
}

void gpio_set_drv(struct s5p_gpio_bank *bank, int gpio, int mode)
{
	unsigned int value;

	value = readl(&bank->drv);
	value &= ~DRV_MASK(gpio);

	switch (mode) {
	case GPIO_DRV_1X:
	case GPIO_DRV_2X:
	case GPIO_DRV_3X:
	case GPIO_DRV_4X:
		value |= DRV_SET(gpio, mode);
		break;
	default:
		return;
	}

	writel(value, &bank->drv);
	if (s5p_get_cpu_rev() == 0)
		value = readl(&bank->drv);
}

void gpio_set_rate(struct s5p_gpio_bank *bank, int gpio, int mode)
{
	unsigned int value;

	value = readl(&bank->drv);
	value &= ~RATE_MASK(gpio);

	switch (mode) {
	case GPIO_DRV_FAST:
	case GPIO_DRV_SLOW:
		value |= RATE_SET(gpio);
		break;
	default:
		return;
	}

	writel(value, &bank->drv);
	if (s5p_get_cpu_rev() == 0)
		value = readl(&bank->drv);
}

#ifdef CONFIG_CMD_GPIO
static char *gpio_name[] = {
	"GPA0", "GPA1", "GPB", "GPC0", "GPC1", "GPD0", "GPD1", "GPE0", "GPE1",
	"GPF0", "GPF1", "GPF2", "GPF3", "GPG0", "GPG1", "GPG2", "GPG3", "GPI",
	"GPJ0", "GPJ1", "GPJ2", "GPJ3", "GPJ4", "MP01", "MP02", "MP03", "MP04",
	"MP05", "MP06", "MP07", "MP10", "MP11", "MP12", "MP13", "MP14", "MP15",
	"MP16", "MP17", "MP18", "MP20", "MP21", "MP22", "MP23", "MP24", "MP25",
	"MP26", "MP27", "MP28",
};

static char *gpio_name1[] = {
	"GPH0", "GPH1", "GPH2", "GPH3",
};

static int do_gpio(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct s5pc110_gpio *gpio =
		(struct s5pc110_gpio *)samsung_get_base_gpio();
	int i = 0;
	int j;

	if (argc == 1) {
		cmd_usage(cmdtp);
		return 1;
	}

	if (strcmp(argv[1], "show") == 0) {
		while (1) {
			printf("%s\n", gpio_name[i]);

			for (j = 0; j < 8; j++) {
				printf("[%d] %s", j,
					gpio_get_value(&gpio->gpio_a0 + i, j) ?
					"hi" : "lo");
				if ((j + 1) & (8 - 1))
					puts("\t");
				else
					puts("\n");
			}
			puts("\n");

			i++;
			if ((&gpio->gpio_a0 + i) == &gpio->res1)
				break;
		}

		for (i = 0; i < 4; i++) {
			printf("%s\n", gpio_name1[i]);

			for (j = 0; j < 8; j++) {
				printf("[%d] %s", j,
					gpio_get_value(&gpio->gpio_h0 + i, j) ?
					"hi" : "lo");
				if ((j + 1) & (8 - 1))
					puts("\t");
				else
					puts("\n");
			}
			puts("\n");
		}

		return 1;
	} else if (strcmp(argv[1], "set") == 0) {
		int num, value;

		if (argc != 5) {
			cmd_usage(cmdtp);
			return 1;
		}

		if (strcmp(argv[2], "GPH0") == 0) {
			i = 48 + 48 + 0;
		} else if (strcmp(argv[2], "GPH1") == 0) {
			i = 48 + 48 + 1;
		} else if (strcmp(argv[2], "GPH2") == 0) {
			i = 48 + 48 + 2;
		} else if (strcmp(argv[2], "GPH3") == 0) {
			i = 48 + 48 + 3;
		} else {
			while (1) {
				if (strcmp(argv[2], gpio_name[i]) == 0)
					break;
				i++;

				if ((&gpio->gpio_a0 + i) == &gpio->res1) {
					printf("Can't found %s bank\n", argv[2]);
					return 1;
				}
			}
		}
		num = simple_strtoul(argv[3], NULL, 10);
		value = simple_strtoul(argv[4], NULL, 10);

		gpio_set_value(&gpio->gpio_a0 + i, num, value);

		printf("%s[%d] set to %s\n", argv[2], num,
				value ? "hi" : "lo");

		return 1;
	}

	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(
	gpio,		CONFIG_SYS_MAXARGS,	1, do_gpio,
	"GPIO Control",
	"show - show all banks\n"
	"gpio set bank num value - set gpio value\n"
);
#endif
