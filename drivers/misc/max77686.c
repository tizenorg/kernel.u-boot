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
#include <max77686.h>

/*
 * MAX77686 PMIC Registers
 */
#define MAX77686_DEVICE_ID	0x00
#define MAX77686_INTSRC		0x01
#define MAX77686_INT1		0x02
#define MAX77686_INT2		0x03
#define MAX77686_INT1MSK	0x04
#define MAX77686_INT2MSK	0x05
#define MAX77686_STATUS1	0x06
#define MAX77686_STATUS2	0x07
#define MAX77686_PWRON		0x08
#define MAX77686_ONOFF_DELAY	0x09
#define MAX77686_MRSTB		0x0A

/* INT1 Bit Map */
#define MAX77686_INT1_MRSTB	(1 << 7)
#define MAX77686_INT1_ONKEY1S	(1 << 6)
#define MAX77686_INT1_ACOKBR	(1 << 5)
#define MAX77686_INT1_ACOKBF	(1 << 4)
#define MAX77686_INT1_PWRONR	(1 << 1)
#define MAX77686_INT1_PWRONF	(1 << 0)
/* PWRON Bit Map */
#define MAX77686_PWRON_WTSRON	(1 << 7)
#define MAX77686_PWRON_SMPLON	(1 << 6)
#define MAX77686_PWRON_RTCA2	(1 << 5)
#define MAX77686_PWRON_RTCA1	(1 << 4)
#define MAX77686_PWRON_MRSTB	(1 << 3)
#define MAX77686_PWRON_ACOKB	(1 << 2)
#define MAX77686_PWRON_JIGONB	(1 << 1)
#define MAX77686_PWRON_PWRON	(1 << 0)


/*
 * MAX77686 RTC Registers
 */
#define MAX77686_RTC_INT	0x0

struct pmic_opmode {
	/* not supported mode has '0xff' */
	unsigned char off;
	unsigned char standby;
	unsigned char lpm;
	unsigned char on;
	unsigned char mask;
};

struct max77686_data {
	unsigned char reg;
	union opmode {
		struct pmic_opmode opmode;
		unsigned char opmodes[5];
	} u;
};

static const char *opmode_str[] = { "OFF", "STANDBY", "LPM", "ON" };

static const struct max77686_data bucks[] = {
	{.reg = 0xff,.u.opmode = {0xff, 0xff, 0xff, 0xff, 0xff},},	/* dummy */
	{.reg = 0x10,.u.opmode = {0x00, 0x01, 0xff, 0x03, 0x03},},	/* BUCK1 */
	{.reg = 0x12,.u.opmode = {0x00, 0x10, 0x20, 0x30, 0x30},},	/* BUCK2 */
	{.reg = 0x1c,.u.opmode = {0x00, 0x10, 0x20, 0x30, 0x30},},	/* BUCK3 */
	{.reg = 0x26,.u.opmode = {0x00, 0x10, 0x20, 0x30, 0x30},},	/* BUCK4 */
	{.reg = 0x30,.u.opmode = {0x00, 0xff, 0xff, 0x03, 0x03},},	/* BUCK5 */
	{.reg = 0x32,.u.opmode = {0x00, 0xff, 0xff, 0x03, 0x03},},	/* BUCK6 */
	{.reg = 0x34,.u.opmode = {0x00, 0xff, 0xff, 0x03, 0x03},},	/* BUCK7 */
	{.reg = 0x36,.u.opmode = {0x00, 0xff, 0xff, 0x03, 0x03},},	/* BUCK8 */
	{.reg = 0x38,.u.opmode = {0x00, 0xff, 0xff, 0x03, 0x03},},	/* BUCK9 */
};

static const struct max77686_data ldos[] = {
	{.reg = 0xff,.u.opmode = {0xff, 0xff, 0xff, 0xff, 0xff},},	/* dummy */
	{.reg = 0x40,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO1 */
	{.reg = 0x41,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO2 */
	{.reg = 0x42,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO3 */
	{.reg = 0x43,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO4 */
	{.reg = 0x44,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO5 */
	{.reg = 0x45,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO6 */
	{.reg = 0x46,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO7 */
	{.reg = 0x47,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO8 */
	{.reg = 0x48,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO9 */
	{.reg = 0x49,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO10 */
	{.reg = 0x4a,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO11 */
	{.reg = 0x4b,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO12 */
	{.reg = 0x4c,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO13 */
	{.reg = 0x4d,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO14 */
	{.reg = 0x4e,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO15 */
	{.reg = 0x4f,.u.opmode = {0x00, 0x40, 0x80, 0xc0, 0xc0},},	/* LDO16 */
	{.reg = 0x50,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO17 */
	{.reg = 0x51,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO18 */
	{.reg = 0x52,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO19 */
	{.reg = 0x53,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO20 */
	{.reg = 0x54,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO21 */
	{.reg = 0x55,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO22 */
	{.reg = 0x56,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO23 */
	{.reg = 0x57,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO24 */
	{.reg = 0x58,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO25 */
	{.reg = 0x59,.u.opmode = {0x00, 0xff, 0x80, 0xc0, 0xc0},},	/* LDO26 */
};

static const char * pwron_source[] = {
	"PWRON",
	"JIGONB",
	"ACOKB",
	"MRSTB",
	"RTCA1",
	"RTCA2",
	"SMPLON",
	"WTSRON"
};

static unsigned int pmic_bus = -1;

static unsigned char reg_int1 = -1;
static unsigned char reg_pwron = -1;

static ulong max77686_hex_to_voltage(int buck, int ldo, int hex);

void max77686_bus_init(int bus_num)
{
	pmic_bus = bus_num;
}

void show_pwron_source(char *buf)
{
	int i;
	unsigned char addr = MAX77686_PMIC_ADDR;
	unsigned char val;

	i2c_read(addr, MAX77686_PWRON, 1, &val, 1);
	for (i = 0; i < 8; i++) {
		if (val & (0x1 << i)) {
			printf("PWRON:\t%s (0x%02x)\n", pwron_source[i], val);
			if (buf)
				sprintf(buf, "PWRON: %s (0x%02x)", pwron_source[i], val);
		}
	}
}

/*
 * RTC
 */
static int max77686_rtc_probe(void)
{
	unsigned char addr = MAX77686_RTC_ADDR;

	i2c_set_bus_num(pmic_bus);

	if (i2c_probe(addr)) {
		puts("Can't found max77686-rtc\n");
		return -1;
	}

	return 0;
}

int max77686_rtc_init(void)
{
	unsigned char addr = MAX77686_RTC_ADDR;
	unsigned char val;

	if (max77686_rtc_probe())
		return -1;

	/* clear int flag */
	i2c_read(addr, MAX77686_RTC_INT, 1, &val, 1);

	return 0;
}

/*
 * PMIC
 */
static int max77686_probe(void)
{
	unsigned char addr = MAX77686_PMIC_ADDR;

	i2c_set_bus_num(pmic_bus);

	if (i2c_probe(addr)) {
		puts("Can't found max77686\n");
		return -1;
	}

	return 0;
}

int max77686_init(void)
{
	unsigned char addr = MAX77686_PMIC_ADDR;

	reg_int1 = 0;
	reg_pwron = 0;

	if (max77686_probe())
		return -1;

	i2c_read(addr, MAX77686_INT1, 1, &reg_int1, 1);
	i2c_read(addr, MAX77686_PWRON, 1, &reg_pwron, 1);

	return 0;
}

int max77686_check_pwron_pwrkey(void)
{
	/* check int1, not pwron */
	return !!(reg_int1 & MAX77686_INT1_PWRONR);
}

int max77686_check_pwron_wtsr(void)
{
	return !!(reg_pwron & MAX77686_PWRON_WTSRON);
}

int max77686_check_pwron_smpl(void)
{
	return !!(reg_pwron & MAX77686_PWRON_SMPLON);
}

int max77686_check_acokb_pwron(void)
{
	unsigned char addr = MAX77686_PMIC_ADDR;
	unsigned char val;

	if (max77686_probe())
		return -1;

	i2c_read(addr, MAX77686_PWRON, 1, &val, 1);

	return !!(val & MAX77686_PWRON_ACOKB);
}

int max77686_check_pwrkey(void)
{
	unsigned char addr = MAX77686_PMIC_ADDR;
	unsigned char val;

	if (max77686_probe())
		return -1;

	i2c_read(addr, MAX77686_INT1, 1, &val, 1);

	return !!(val & MAX77686_INT1_PWRONR);
}

int max77686_clear_irq(void)
{
	unsigned char addr = MAX77686_PMIC_ADDR;
	unsigned char val[2];

	if (max77686_probe())
		return -1;

	i2c_read(addr, MAX77686_INT1, 1, val, 2);

	return 0;
}

static int max77686_status(void)
{
	unsigned char addr = MAX77686_PMIC_ADDR;
	unsigned char val[2];
	unsigned char opmode;
	int i;

	if (max77686_probe())
		return -1;

	for (i = 1; i <= 9; i++) {
		printf("BUCK%d\t: ", i);

		if (2 <= i && i <= 4)
			i2c_read(addr, bucks[i].reg + 2, 1, val, 1);
		else
			i2c_read(addr, bucks[i].reg + 1, 1, val, 1);

		printf("%7uuV, ", max77686_hex_to_voltage(i, 0, val[0]));

		i2c_read(addr, bucks[i].reg, 1, val, 1);

		opmode = val[0] & bucks[i].u.opmode.mask;
		if (opmode == bucks[i].u.opmode.off)
			puts("OFF\n");
		else if (opmode == bucks[i].u.opmode.standby)
			puts("STANDBY (ON/OFF by PWRREQ)\n");
		else if (opmode == bucks[i].u.opmode.lpm)
			puts("LP\n");
		else if (opmode == bucks[i].u.opmode.on)
			puts("ON\n");
		else
			printf("unknown (0x%x)\n", val[0]);
	}

	for (i = 1; i <= 26; i++) {
		printf("LDO%d\t: ", i);

		i2c_read(addr, ldos[i].reg, 1, val, 1);

		printf("%7uuV, ", max77686_hex_to_voltage(0, i, val[0] & ~ldos[i].u.opmode.mask));

		opmode = val[0] & ldos[i].u.opmode.mask;
		if (opmode == ldos[i].u.opmode.off)
			puts("OFF\n");
		else if (opmode == ldos[i].u.opmode.standby)
			puts("STANDBY (ON/OFF by PWRREQ)\n");
		else if (opmode == ldos[i].u.opmode.lpm)
			puts("LP (ON with LPM by PWRREQ)\n");
		else if (opmode == ldos[i].u.opmode.on)
			puts("ON\n");
		else
			printf("unknown (0x%x)\n", val[0]);
	}

	return 0;
}

static ulong max77686_hex_to_voltage(int buck, int ldo, int hex)
{
	ulong uV;

	if (ldo) {
		switch (ldo) {
		case 1:
		case 2:
		case 6:
		case 7:
		case 8:
		case 15:
			/* 0x00..0x3f: 0.800V ~ 2.375V */
			if (hex > 0x3f) {
				printf("warn: out of range (0x%x<0x3f)\n", hex);
				hex = 0x3f;
			}

			uV = 800000 + hex * 25000;
			break;
		default:
			/* 0x00..0x3f: 0.800V ~ 3.950V*/
			if (hex > 0x3f) {
				printf("warn: out of range (0x%x<0x3f)\n", hex);
				hex = 0x3f;
			}

			uV = 800000 + hex * 50000;
			break;
		}

		return uV;
	}

	if (buck) {
		switch (buck) {
		case 2:
		case 3:
		case 4:
			/* 0x00..0xff: 0.600V ~ 3.7875V */
			if (hex > 0xff) {
				printf("warn: out of range (0x%x<0xff)\n", hex);
				hex = 0xff;
			}

			uV = 600000 + hex * 12500;
			break;
		default:
			/* 0x00..0x3f: 0.750V ~ 3.900V*/
			if (hex > 0x3f) {
				printf("warn: out of range (0x%x<0x3f)\n", hex);
				hex = 0x3f;
			}

			uV = 750000 + hex * 50000;
			break;
		}

		return uV;
	}
}

static int max77686_voltage_to_hex(int buck, int ldo, ulong uV)
{
	unsigned int set = 0;

	if (ldo) {
		switch (ldo) {
		case 1:
		case 2:
		case 6:
		case 7:
		case 8:
		case 15:
			if (uV < 800000)
				set = 0x0;
			else if (uV > 2375000)
				set = 0x3f;
			else
				set = (uV - 800000) / 25000;
			break;
		default:
			if (uV < 800000)
				set = 0x0;
			else if (uV > 3950000)
				set = 0x3f;
			else
				set = (uV - 800000) / 50000;
		}

		return set;
	}

	if (buck) {
		switch (buck) {
		case 2:
		case 3:
		case 4:
			if (uV < 600000)
				set = 0;
			else if (uV > 3787500)
				set = 0xff;
			else
				set = (uV - 600000) / 12500;
			break;
		default:
			if (uV < 750000)
				set = 0;
			else if (uV > 3900000)
				set = 0x3f;
			else
				set = (uV - 750000) / 50000;
		}

		return set;
	}
}

static int max77686_set_voltage(int buck, int ldo, ulong uV)
{
	unsigned char addr = MAX77686_PMIC_ADDR;
	unsigned char val[2];
	unsigned int reg = 0, set = 0, mask = 0;

	if (ldo) {
		if (ldo < 1 || ldo > 26)
			return -1;

		reg = ldos[ldo].reg;
		mask = 0x3f;
	} else if (buck) {
		if (buck < 1 || buck > 9)
			return -1;

		switch (buck) {
		case 2:
		case 3:
		case 4:
			reg = bucks[buck].reg + 2;
			mask = 0xff;
			break;
		default:
			reg = bucks[buck].reg + 1;
			mask = 0x3f;
		}
	} else
		return -1;

	set = max77686_voltage_to_hex(buck, ldo, uV) & mask;

	if (max77686_probe())
		return -1;

	i2c_read(addr, reg, 1, val, 1);
	val[0] &= ~mask;
	val[0] |= set;
	i2c_write(addr, reg, 1, val, 1);
	i2c_read(addr, reg, 1, val, 1);
	debug("MAX77686 REG %2.2xh has %2.2xh\n",
			reg, val[0]);

	return 0;
}

int max77686_set_ldo_voltage(int ldo, ulong uV)
{
	return max77686_set_voltage(0, ldo, uV);
}

int max77686_set_buck_voltage(int buck, ulong uV)
{
	return max77686_set_voltage(buck, 0, uV);
}

static int max77686_set_output(int buck, int ldo, opmode_type mode)
{
	unsigned char addr = MAX77686_PMIC_ADDR;
	unsigned char val[2];
	unsigned int reg, set, mask;

	if (ldo) {
		if (ldo < 1 || ldo > 26)
			return -1;
		reg = ldos[ldo].reg;
		mask = ldos[ldo].u.opmode.mask;
		set = ldos[ldo].u.opmodes[mode];
	} else if (buck) {
		if (buck < 1 || buck > 9)
			return -1;
		reg = bucks[buck].reg;
		mask = bucks[buck].u.opmode.mask;
		set = bucks[buck].u.opmodes[mode];
	} else
		return -1;

	if (max77686_probe())
		return -1;

	if (set == 0xff) {
		printf("%s is not supported on LDO%d\n", opmode_str[mode], ldo);
		return -1;
	}

	i2c_read(addr, reg, 1, val, 1);
	val[0] &= ~mask;
	val[0] |= set;
	i2c_write(addr, reg, 1, val, 1);
	i2c_read(addr, reg, 1, val, 1);

	return 0;
}

int max77686_set_ldo_mode(int ldo, opmode_type mode)
{
	return max77686_set_output(0, ldo, mode);
}

int max77686_set_buck_mode(int buck, opmode_type mode)
{
	return max77686_set_output(buck, 0, mode);
}

int max77686_set_32khz(unsigned char mode)
{
	unsigned char addr = MAX77686_PMIC_ADDR;
	unsigned char val[2];

	if (max77686_probe())
		return -1;

	val[0] = mode;
	i2c_write(addr, 0x7f, 1, val, 1);

	return 0;
}

static int do_max77686(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int buck = 0, ldo = 0, on = -1;
	int ret = 0;

	switch (argc) {
	case 2:
		if (strncmp(argv[1], "status", 6) == 0)
			return max77686_status();
		break;
		return ret;
	case 4:
		if (argv[1][0] == 'l')
			ldo = simple_strtoul(argv[2], NULL, 10);
		else if (argv[1][0] == 'b')
			buck = simple_strtoul(argv[2], NULL, 10);
		else {
			printf("Option = \"%s\" ? \n", argv[1]);
			break;
		}

		if (!strncmp(argv[3], "on", 2))
			ret = max77686_set_output(buck, ldo, OPMODE_ON);
		else if (!strncmp(argv[3], "lpm", 3))
			ret = max77686_set_output(buck, ldo, OPMODE_LPM);
		else if (!strncmp(argv[3], "standby", 3))
			ret = max77686_set_output(buck, ldo, OPMODE_STANDBY);
		else if (!strncmp(argv[3], "off", 3))
			ret = max77686_set_output(buck, ldo, OPMODE_OFF);
		else {
			ulong volt = simple_strtoul(argv[3], NULL, 10);
			printf("volt = %uuV\n", volt);
			if (volt <= 0)
				ret = -1;
			else
				ret = max77686_set_voltage(buck, ldo, volt);
		}

		if (!ret)
			printf("%s %s %s\n", argv[1], argv[2], argv[3]);
		if (ret < 0)
			printf("Error.\n");
		return ret;
	default:
		break;
	}

	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(
	max77686, 4, 1, do_max77686,
	"PMIC LDO & BUCK control for MAX77686",
	"status - Display PMIC LDO & BUCK status\n"
	"max77686 ldo num volt - Set LDO voltage\n"
	"max77686 ldo num on/lpm/standby/off - Set LDO output mode\n"
	"max77686 buck num volt - Set BUCK voltage\n"
	"max77686 buck num on/lpm/standby/off - Set BUCK output mode\n"
	"	volt is voltage in uV\n"
);
