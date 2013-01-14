/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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
#include <i2c.h>
#include <asm/arch/gpio.h>
#include <max77693.h>

/*
 * PMIC REGISTER
 */

#define MAX77693_PMIC_PREFIX	"max77693-pmic:"

#define MAX77693_PMIC_ID1	0x20
#define MAX77693_PMIC_ID2	0x21

/* MAX77693_PMIC_ID1 */
#define MAX77693_PMIC_ID	0x93

/* MAX77693_PMIC_ID2 */
#define MAX77693_PMIC_REV_PASS1	0x0
#define MAX77693_PMIC_REV_PASS2	0x1
#define MAX77693_PMIC_REV_PASS3	0x2

/*
 * PMIC(CHARGER) REGISTER
 */

#define MAX77693_CHG_PREFIX	"max77693-chg:"

#define MAX77693_CHG_BASE	0xB0
#define MAX77693_CHG_INT_OK	0xB2
#define MAX77693_CHG_CNFG_00	0xB7
#define MAX77693_CHG_CNFG_02	0xB9
#define MAX77693_CHG_CNFG_06	0xBD
#define MAX77693_SAFEOUT	0xC6

/* MAX77693_CHG_INT_OK */
#define MAX77693_CHG_DETBAT	(0x1 << 7)

/* MAX77693_CHG_CNFG_00 */
#define MAX77693_CHG_MODE_ON	0x05

/* MAX77693_CHG_CNFG_02	*/
#define MAX77693_CHG_CC		0x3F

/* MAX77693_CHG_CNFG_06	*/
#define MAX77693_CHG_LOCK	(0x0 << 2)
#define MAX77693_CHG_UNLOCK	(0x3 << 2)

/*
 * MUIC REGISTER
 */

#define MAX77693_MUIC_PREFIX	"max77693-muic:"

#define MAX77693_MUIC_STATUS1	0x04
#define MAX77693_MUIC_STATUS2	0x05
#define MAX77693_MUIC_CONTROL1	0x0C

/* MAX77693_MUIC_STATUS1 */
#define MAX77693_MUIC_ADC_MASK	0x1F

/* MAX77693_MUIC_STATUS2 */
#define MAX77693_MUIC_CHG_NO	0x00
#define MAX77693_MUIC_CHG_USB	0x01
#define MAX77693_MUIC_CHG_USB_D	0x02
#define MAX77693_MUIC_CHG_TA	0x03
#define MAX77693_MUIC_CHG_TA_500 0x04
#define MAX77693_MUIC_CHG_TA_1A	0x05
#define MAX77693_MUIC_CHG_MASK	0x07

/* MAX77693_MUIC_CONTROL1 */
#define MAX77693_MUIC_CTRL1_DN1DP2	((0x1 << 3) | 0x1)
#define MAX77693_MUIC_CTRL1_UT1UR2	((0x3 << 3) | 0x3)
#define MAX77693_MUIC_CTRL1_ADN1ADP2	((0x4 << 3) | 0x4)
#define MAX77693_MUIC_CTRL1_AUT1AUR2	((0x5 << 3) | 0x5)
#define MAX77693_MUIC_CTRL1_MASK	0xC0

/* MUIC INFO - passing to kernel driver by pmic_info field of cmdline */
#define MUIC_PATH_USB	0
#define MUIC_PATH_UART	1

#define MUIC_PATH_CP	0
#define MUIC_PATH_AP	1

enum muic_path {
	MUIC_PATH_USB_CP,
	MUIC_PATH_USB_AP,
	MUIC_PATH_UART_CP,
	MUIC_PATH_UART_AP,
};

static unsigned int pmic_bus = -1;
static unsigned int muic_bus = -1;

static unsigned int revision;

static int usb_path = MUIC_PATH_AP;

static void set_muic_path_info(void)
{
	int val = 0;
	char *path;
	char buf[32];

	path = getenv("uartpath");
	if (path && !strncmp(path, "cp", 2))
		val |= MUIC_PATH_CP << MUIC_PATH_UART;
	else
		val |= MUIC_PATH_AP << MUIC_PATH_UART;

	path = getenv("usbpath");
	if (path && !strncmp(path, "cp", 2))
		val |= MUIC_PATH_CP << MUIC_PATH_USB;
	else
		val |= MUIC_PATH_AP << MUIC_PATH_USB;

	sprintf(buf, "pmic_info=%d", val);
	setenv("muicpathinfo", buf);
}

void max77693_pmic_bus_init(int bus_num)
{
	pmic_bus = bus_num;
}

int max77693_pmic_probe(void)
{
	unsigned char addr = MAX77693_PMIC_ADDR;

	i2c_set_bus_num(pmic_bus);

	if (i2c_probe(addr)) {
		puts("Can't found max77693 pmic\n");
		return 1;
	}

	return 0;
}

int max77693_init(void)
{
	unsigned char addr = MAX77693_PMIC_ADDR;
	unsigned char val[2];

	if (max77693_pmic_probe())
		return -1;

	/* revision */
	i2c_read(addr, MAX77693_PMIC_ID1, 1, val, 2);

	debug(MAX77693_PMIC_PREFIX "\tID:0x%x, REV:0x%x, VER:0x%x\n",
			val[0], val[1] & 0x7, val[1] & 0xF8);

	revision = val[1] & 0x3;

	/* muic path */
	set_muic_path_info();
	return 0;
}

static int max77693_charger_en(int set_current)
{
	unsigned char addr = MAX77693_PMIC_ADDR;
	unsigned char val[2];

	if (max77693_pmic_probe())
		return -1;

	/* unlock write capability */
	val[0] = MAX77693_CHG_UNLOCK;
	i2c_write(addr, MAX77693_CHG_CNFG_06, 1, val, 1);

	if (!set_current) {
		puts(MAX77693_CHG_PREFIX "\tdisabled\n");
		i2c_read(addr, MAX77693_CHG_CNFG_00, 1, val, 1);
		val[0] &= ~0x01;
		i2c_write(addr, MAX77693_CHG_CNFG_00, 1, val, 1);
		return 0;
	}

	/* set charging current */
	i2c_read(addr, MAX77693_CHG_CNFG_02, 1, val, 1);
	val[0] &= ~MAX77693_CHG_CC;
	val[0] |= set_current * 10 / 333;	/* 0.1A/3 steps */
	i2c_write(addr, MAX77693_CHG_CNFG_02, 1, val, 1);

	/* enable charging */
	val[0] = MAX77693_CHG_MODE_ON;
	i2c_write(addr, MAX77693_CHG_CNFG_00, 1, val, 1);

	/* check charging current */
	i2c_read(addr, MAX77693_CHG_CNFG_02, 1, val, 1);
	val[0] &= 0x3f;
	printf(MAX77693_CHG_PREFIX "\tenabled @ %d mA\n", val[0] * 333 / 10);

	return 0;
}

int max77693_charger_detbat(void)
{
	unsigned char addr = MAX77693_PMIC_ADDR;
	unsigned char val[2];

	if (max77693_pmic_probe())
		return -1;

	i2c_read(addr, MAX77693_CHG_INT_OK, 1, val, 1);

	if (val[0] & MAX77693_CHG_DETBAT)
		return 0;
	else
		return 1;
}

int max77693_charger_status(void)
{
	unsigned char addr = MAX77693_PMIC_ADDR;
	unsigned char base = MAX77693_CHG_BASE;
	unsigned char val[0x10];

	if (max77693_pmic_probe())
		return -1;

	i2c_read(addr, base, 1, val, 0x10);

	printf("\n");
	printf("INT    (%02Xh) = 0x%02x\n", base + 0x00, val[0x0]);
	printf("INT_MSK(%02Xh) = 0x%02x\n", base + 0x01, val[0x1]);
	printf("INT_OK (%02Xh) = 0x%02x\n", base + 0x02, val[0x2]);
	printf("DTLS00 (%02Xh) = 0x%02x\n", base + 0x03, val[0x3]);
	printf("DTLS01 (%02Xh) = 0x%02x\n", base + 0x04, val[0x4]);
	printf("DTLS02 (%02Xh) = 0x%02x\n", base + 0x05, val[0x5]);
	printf("CNFG00 (%02Xh) = 0x%02x\n", base + 0x07, val[0x7]);
	printf("CNFG01 (%02Xh) = 0x%02x\n", base + 0x08, val[0x8]);
	printf("CNFG02 (%02Xh) = 0x%02x\n", base + 0x09, val[0x9]);
	printf("CNFG03 (%02Xh) = 0x%02x\n", base + 0x0a, val[0xa]);
	printf("CNFG04 (%02Xh) = 0x%02x\n", base + 0x0b, val[0xb]);
	printf("CNFG06 (%02Xh) = 0x%02x\n", base + 0x0d, val[0xd]);

	return 0;
}

void max77693_muic_bus_init(int bus_num)
{
	muic_bus = bus_num;
}

int max77693_muic_probe(void)
{
	unsigned char addr = MAX77693_MUIC_ADDR;

	i2c_set_bus_num(muic_bus);

	if (i2c_probe(addr)) {
		puts("Can't found max77693 muic\n");
		return 1;
	}

	return 0;
}

int max77693_muic_check(void)
{
	unsigned char addr = MAX77693_MUIC_ADDR;
	unsigned char val[2];
	unsigned char charge_type, charger;

	/* if probe failed, return cable none */
	if (max77693_muic_probe())
		return CHARGER_NO;

	i2c_read(addr, MAX77693_MUIC_STATUS2, 1, val, 1);

	charge_type = val[0] & MAX77693_MUIC_CHG_MASK;

	switch (charge_type) {
	case MAX77693_MUIC_CHG_NO:
		charger = CHARGER_NO;
		break;
	case MAX77693_MUIC_CHG_USB:
	case MAX77693_MUIC_CHG_USB_D:
		charger = CHARGER_USB;
		break;
	case MAX77693_MUIC_CHG_TA:
	case MAX77693_MUIC_CHG_TA_1A:
		charger = CHARGER_TA;
		break;
	case MAX77693_MUIC_CHG_TA_500:
		charger = CHARGER_TA_500;
		break;
	default:
		charger = CHARGER_UNKNOWN;
		break;
	}

	return charger;
}

void __board_muic_gpio_control(int sw, int path) { }
void board_muic_gpio_control(int sw, int path)
	__attribute__((weak, alias("__board_muic_gpio_control")));

static int max77693_muic_switch(enum muic_path path)
{
	unsigned char addr = MAX77693_MUIC_ADDR;
	unsigned char val;

	if (max77693_muic_probe())
		return -1;

	i2c_read(addr, MAX77693_MUIC_CONTROL1, 1, &val, 1);

	val &= MAX77693_MUIC_CTRL1_MASK;

	if (revision >= MAX77693_PMIC_REV_PASS2)
		switch (path) {
		case MUIC_PATH_USB_AP:
			val |= MAX77693_MUIC_CTRL1_DN1DP2;
			break;
		case MUIC_PATH_USB_CP:
			val |= MAX77693_MUIC_CTRL1_ADN1ADP2;
			break;
		case MUIC_PATH_UART_AP:
			val |= MAX77693_MUIC_CTRL1_UT1UR2;
			break;
		case MUIC_PATH_UART_CP:
			val |= MAX77693_MUIC_CTRL1_AUT1AUR2;
			break;
		}
	else
		switch (path) {
		case MUIC_PATH_USB_AP:
			val |= MAX77693_MUIC_CTRL1_DN1DP2;
			break;
		case MUIC_PATH_USB_CP:
			val |= MAX77693_MUIC_CTRL1_UT1UR2;
			board_muic_gpio_control(MUIC_PATH_USB, MUIC_PATH_CP);
			break;
		case MUIC_PATH_UART_AP:
			val |= MAX77693_MUIC_CTRL1_UT1UR2;
			board_muic_gpio_control(MUIC_PATH_UART, MUIC_PATH_AP);
			board_muic_gpio_control(MUIC_PATH_USB, MUIC_PATH_AP);
			break;
		case MUIC_PATH_UART_CP:
			val |= MAX77693_MUIC_CTRL1_UT1UR2;
			board_muic_gpio_control(MUIC_PATH_UART, MUIC_PATH_CP);
			board_muic_gpio_control(MUIC_PATH_USB, MUIC_PATH_AP);
			break;
		}

	i2c_write(addr, MAX77693_MUIC_CONTROL1, 1, &val, 1);

		return 0;
}

/* To detect the attached accessory type on anyway jig box */
int max77693_muic_get_adc(void)
{
	unsigned char addr = MAX77693_MUIC_ADDR;
	unsigned char val[2];

	if (max77693_muic_probe())
		return -1;

	i2c_read(addr, MAX77693_MUIC_STATUS1, 1, val, 1);

	return (val[0] & MAX77693_MUIC_ADC_MASK);
}

static int max77693_status(void)
{
	unsigned char addr = MAX77693_PMIC_ADDR;
	unsigned char val[2];

	if (max77693_pmic_probe())
		return -1;

	i2c_read(addr, MAX77693_SAFEOUT, 1, val, 1);
	printf("SAFEOUT1 %s\n", (val[0] & (1 << 6)) ? "on" : "off");
	printf("SAFEOUT2 %s\n", (val[0] & (1 << 7)) ? "on" : "off");

	return 0;
}

static int max77693_safeout_control(int safeout, int on)
{
	unsigned char addr = MAX77693_PMIC_ADDR;
	unsigned char val[2];
	unsigned int reg, set;

	if (safeout < 1 || safeout > 2)
		return -1;
	reg = MAX77693_SAFEOUT;
	set = 1 << (5 + safeout);

	if (max77693_pmic_probe())
		return -1;

	i2c_read(addr, reg, 1, val, 1);
	if (on)
		val[0] |= set;
	else
		val[0] &= ~set;

	i2c_write(addr, reg, 1, val, 1);
	i2c_read(addr, reg, 1, val, 1);

	return 0;
}

static int do_max77693(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	int safeout;
	enum muic_path path;

	if (argc < 2)
		return cmd_usage(cmdtp);

	if (!strncmp(argv[1], "status", 6))
		return max77693_status();

	if (!strncmp(argv[1], "safeout", 7)) {
		if (argc != 4)
			return cmd_usage(cmdtp);

		safeout = simple_strtoul(argv[2], NULL, 10);

		if (!strncmp(argv[3], "on", 2))
			ret = max77693_safeout_control(safeout, 1);
		else if (!strncmp(argv[3], "off", 3))
			ret = max77693_safeout_control(safeout, 0);
		else
			return cmd_usage(cmdtp);

		if (!ret)
			printf("%s %s %s\n", argv[1], argv[2], argv[3]);
		else if (ret < 0)
			printf("Error.\n");
		return ret;
	}

	if (!strncmp(argv[1], "usb", 4)) {
		if (!strncmp(argv[2], "cp", 2))
			path = MUIC_PATH_USB_CP;
		else if (!strncmp(argv[2], "ap", 2))
			path = MUIC_PATH_USB_AP;
		else
			return cmd_usage(cmdtp);

		max77693_muic_switch(path);

		setenv("usbpath", argv[2]);
		set_muic_path_info();
		return 0;
	}

	if (!strncmp(argv[1], "uart", 4)) {
		if (!strncmp(argv[2], "cp", 2))
			path = MUIC_PATH_UART_CP;
		else if (!strncmp(argv[2], "ap", 2))
			path = MUIC_PATH_UART_AP;
		else
			return cmd_usage(cmdtp);

		max77693_muic_switch(path);

		setenv("uartpath", argv[2]);
		set_muic_path_info();
		return 0;
	}

	if (!strncmp(argv[1], "charger", 1)) {
		if (!strncmp(argv[2], "start", 5)) {
			int current;
			if (argc != 4)
				return cmd_usage(cmdtp);
			current = simple_strtoul(argv[3], NULL, 10);
			max77693_charger_en(current);
		} else if (!strncmp(argv[2], "stop", 4)) {
			max77693_charger_en(0);
		} else if (!strncmp(argv[2], "status", 6)) {
			max77693_fg_show_battery();
		} else {
			return cmd_usage(cmdtp);
		}

		return 0;
	}

	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	max77693, 4, 1, do_max77693,
	"PMIC/MUIC control for MAX77693",
	"status\n"
	"max77693 safeout num on/off - Turn on/off the SAFEOUT\n"
	"max77693 charger start/stop/status current\n"
	"max77693 uart ap/cp - Switching UART path between AP/CP\n"
	"max77693 usb ap/cp - Switching USB path between AP/CP\n"
);

