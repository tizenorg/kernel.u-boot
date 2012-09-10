/*
 *  Copyright (C) 2010 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *  Kyungmin Park <kyungmin.park@samsung.com>
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
#include <i2c.h>
#include <lcd.h>
#include <spi.h>
#include <mmc.h>
#include <asm/io.h>
#include <asm/arch/adc.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/power.h>
#include <asm/arch/clk.h>
#include <ramoops.h>
#include <info_action.h>
#include <pmic.h>

#include <asm/arch/hs_otg.h>
#include <asm/arch/regs-otg.h>
#include "usb_mass_storage.h"

DECLARE_GLOBAL_DATA_PTR;

static struct exynos4_gpio_part1 *gpio1;
static struct exynos4_gpio_part2 *gpio2;

static unsigned int battery_soc;
static unsigned int board_rev;

u32 get_board_rev(void)
{
	return board_rev;
}

static int get_hwrev(void)
{
	return board_rev & 0xFF;
}

enum {
	I2C_0, I2C_1, I2C_2, I2C_3,
	I2C_4, I2C_5, I2C_6, I2C_7,
	I2C_8, I2C_9, I2C_10, I2C_11,
	I2C_12, I2C_13, I2C_NUM,
};

/* i2c0 (CAM)	SDA: GPD1[0] SCL: GPD1[1] */
static struct i2c_gpio_bus_data i2c_0 = {
	.sda_pin	= 0,
	.scl_pin	= 1,
};

/* i2c1 (Gryo)	SDA: GPD1[2] SCL: GPD1[3] */
static struct i2c_gpio_bus_data i2c_1 = {
	.sda_pin	= 2,
	.scl_pin	= 3,
};

/* i2c3 (TSP)	SDA: GPA1[2] SCL: GPA1[3] */
static struct i2c_gpio_bus_data i2c_3 = {
	.sda_pin	= 2,
	.scl_pin	= 3,
};

/* i2c4		SDA: GPB[2] SCL: GPB[3] */
static struct i2c_gpio_bus_data i2c_4 = {
	.sda_pin	= 2,
	.scl_pin	= 3,
};

/* i2c5 (PMIC)	SDA: GPB[6] SCL: GPB[7] */
static struct i2c_gpio_bus_data i2c_5 = {
	.sda_pin	= 6,
	.scl_pin	= 7,
};

/* i2c6 (CODEC)	SDA: GPC1[3] SCL: GPC1[4] */
static struct i2c_gpio_bus_data i2c_6 = {
	.sda_pin	= 3,
	.scl_pin	= 4,
};

/* i2c7		SDA: GPD0[2] SCL: GPD0[3] */
static struct i2c_gpio_bus_data i2c_7 = {
	.sda_pin	= 2,
	.scl_pin	= 3,
};

/* i2c9		SDA: SPY4[0] SCL: SPY4[1] */
static struct i2c_gpio_bus_data i2c_9 = {
	.sda_pin	= 0,
	.scl_pin	= 1,
};

/* i2c10	SDA: SPE1[0] SCL: SPE1[1] */
static struct i2c_gpio_bus_data i2c_10 = {
	.sda_pin	= 0,
	.scl_pin	= 1,
};

/* i2c12	SDA: SPE4[0] SCL: SPE4[1] */
static struct i2c_gpio_bus_data i2c_12 = {
	.sda_pin	= 0,
	.scl_pin	= 1,
};

/* i2c13	SDA: SPE4[2] SCL: SPE4[3] */
static struct i2c_gpio_bus_data i2c_13 = {
	.sda_pin	= 2,
	.scl_pin	= 3,
};

static struct i2c_gpio_bus i2c_gpio[I2C_NUM];

static void check_battery(int mode);
static void check_micro_usb(int intr);
static void init_pmic_lp3974(void);
static void init_pmic_max8952(void);

void i2c_init_board(void)
{
	gpio1 = (struct exynos4_gpio_part1 *) EXYNOS4_GPIO_PART1_BASE;
	gpio2 = (struct exynos4_gpio_part2 *) EXYNOS4_GPIO_PART2_BASE;

	i2c_gpio[I2C_0].bus = &i2c_0;
	i2c_gpio[I2C_1].bus = &i2c_1;
	i2c_gpio[I2C_2].bus = NULL;
	i2c_gpio[I2C_3].bus = &i2c_3;
	i2c_gpio[I2C_4].bus = &i2c_4;
	i2c_gpio[I2C_5].bus = &i2c_5;
	i2c_gpio[I2C_6].bus = &i2c_6;
	i2c_gpio[I2C_7].bus = &i2c_7;
	i2c_gpio[I2C_8].bus = NULL;
	i2c_gpio[I2C_9].bus = &i2c_9;
	i2c_gpio[I2C_10].bus = &i2c_10;
	i2c_gpio[I2C_11].bus = NULL;
	i2c_gpio[I2C_12].bus = &i2c_12;
	i2c_gpio[I2C_13].bus = &i2c_13;

	i2c_gpio[I2C_0].bus->gpio_base = (unsigned int)&gpio1->d1;
	i2c_gpio[I2C_1].bus->gpio_base = (unsigned int)&gpio1->d1;
	i2c_gpio[I2C_3].bus->gpio_base = (unsigned int)&gpio1->a1;
	i2c_gpio[I2C_4].bus->gpio_base = (unsigned int)&gpio1->b;
	i2c_gpio[I2C_5].bus->gpio_base = (unsigned int)&gpio1->b;
	i2c_gpio[I2C_6].bus->gpio_base = (unsigned int)&gpio1->c1;
	i2c_gpio[I2C_7].bus->gpio_base = (unsigned int)&gpio1->d0;
	i2c_gpio[I2C_9].bus->gpio_base = (unsigned int)&gpio2->y4;
	i2c_gpio[I2C_10].bus->gpio_base = (unsigned int)&gpio1->e1;
	i2c_gpio[I2C_12].bus->gpio_base = (unsigned int)&gpio1->e4;
	i2c_gpio[I2C_13].bus->gpio_base = (unsigned int)&gpio1->e4;

	i2c_gpio_init(i2c_gpio, I2C_NUM, I2C_5);
}

static void check_hw_revision(void);

int board_init(void)
{
	gpio1 = (struct exynos4_gpio_part1 *) EXYNOS4_GPIO_PART1_BASE;
	gpio2 = (struct exynos4_gpio_part2 *) EXYNOS4_GPIO_PART2_BASE;

	gd->bd->bi_arch_number = MACH_TYPE;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	check_hw_revision();

	return 0;
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE + PHYS_SDRAM_2_SIZE;

	/* Early init for i2c devices - Where these funcions should go?? */

	/* Reset on max17040 */
	check_battery(1);

	/* pmic init */
	init_pmic_lp3974();
	init_pmic_max8952();

	/* Reset on fsa9480 */
	check_micro_usb(1);

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
}

static void check_auto_burn(void)
{
	unsigned int magic_base = CONFIG_SYS_SDRAM_BASE + 0x02000000;
	unsigned int count = 0;
	char buf[64];

	/* OneNAND */
	if (readl(magic_base) == 0x426f6f74) {		/* ASICC: Boot */
		puts("Auto buring bootloader\n");
		count += sprintf(buf + count, "run updateb; ");
	}
	/* MMC */
	if (readl(magic_base) == 0x654D4D43) {		/* ASICC: eMMC */
		puts("Auto buring bootloader (eMMC)\n");
		count += sprintf(buf + count, "run updatemmc; ");
	}
	if (readl(magic_base + 0x4) == 0x4b65726e) {	/* ASICC: Kern */
		puts("Auto buring kernel\n");
		count += sprintf(buf + count, "run updatek; ");
	}
	/* Backup u-boot in eMMC */
	if (readl(magic_base + 0x8) == 0x4261636B) {	/* ASICC: Back */
		puts("Auto buring u-boot image (boot partition2 in eMMC)\n");
		count += sprintf(buf + count, "run updatebackup; ");
	}

	if (count) {
		count += sprintf(buf + count, "reset");
		setenv("bootcmd", buf);
	}

	/* Clear the magic value */
	memset((void *)magic_base, 0, 2);
}

static void check_battery(int mode)
{
	unsigned char val[2];
	unsigned char addr = 0x36;	/* max17040 fuel gauge */

	i2c_set_bus_num(I2C_9);

	if (i2c_probe(addr)) {
		puts("Can't found max17040 fuel gauge\n");
		return;
	}

	/* mode 0: check mode / 1: enable mode */
	if (mode) {
		val[0] = 0x40;
		val[1] = 0x00;
		i2c_write(addr, 0xfe, 1, val, 2);
	} else {
		i2c_read(addr, 0x04, 1, val, 1);
		printf("battery:\t%d%%\n", val[0]);
		battery_soc = val[0];
	}
}

static int fsa9480_probe(void)
{
	unsigned char addr = 0x25;

	i2c_set_bus_num(I2C_10);

	if (i2c_probe(addr)) {
		puts("Can't found fsa9480\n");
		return 1;
	}

	return 0;
}

static void charger_en(int enable);
static void into_charge_mode(int charger_speed);
static void check_micro_usb(int intr)
{
	unsigned char addr;
	unsigned char val[2];
	static int started_charging_once = 0;
	char *path;

	if (fsa9480_probe())
		return;

	addr = 0x25;	/* fsa9480 */

	/* Clear Interrupt */
	if (intr) {
		i2c_read(addr, 0x03, 1, val, 2);
		return;
	}

	/* Read Device Type 1 */
	i2c_read(addr, 0x0a, 1, val, 1);

#define FSA_DEV1_CHARGER	(1 << 6)
#define FSA_DEV1_UART		(1 << 3)
#define FSA_DEV1_USB		(1 << 2)
#define FSA_DEV2_JIG_USB_OFF	(1 << 1)
#define FSA_DEV2_JIG_USB_ON	(1 << 0)

	/* disable the charger related feature */
	/*
	 * If USB, use default 475mA
	 * If Charger, use 600mA and go to charge mode
	 */
	if ((val[0] & FSA_DEV1_CHARGER) && !started_charging_once) {
		started_charging_once = 1;

		/* If it's full, do not charge. */
		if (battery_soc < 100) {
			charger_en(600);
			into_charge_mode(600);
		} else
			charger_en(0);
	} else if (val[0] & FSA_DEV1_USB) {
		if (battery_soc < 100)
			charger_en(500); /* enable charger and keep booting */
		else
			charger_en(0);
	}

	/* If reset status is watchdog reset then skip it */
	if (get_reset_status() != SWRESET) {
		/* If Factory Mode is Boot ON-USB, go to download mode */
		i2c_read(addr, 0x07, 1, val, 1);

#define FSA_ADC_FAC_USB_OFF	0x18
#define FSA_ADC_FAC_USB_ON	0x19
#define FSA_ADC_FAC_UART	0x1d

		if (val[0] == FSA_ADC_FAC_USB_ON ||
			val[0] == FSA_ADC_FAC_USB_OFF)
			setenv("bootcmd", "usbdown");
	}

	path = getenv("usb");

	if (!strncmp(path, "cp", 2))
		run_command("microusb cp", 0);
}

static void micro_usb_switch(int path)
{
	unsigned char addr;
	unsigned char val[2];

	if (fsa9480_probe())
		return;

	addr = 0x25;		/* fsa9480 */

	if (path)
		val[0] = 0x90;	/* VAUDIO */
	else
		val[0] = (1 << 5) | (1 << 2);	/* DHOST */

	i2c_write(addr, 0x13, 1, val, 1);	/* MANSW1 */

	i2c_read(addr, 0x2, 1, val, 1);
	val[0] &= ~(1 << 2);			/* Manual switching */
	i2c_write(addr, 0x2, 1, val, 1);
}

#define FSA_REG_CONTROL 0x02
#define FSA_REG_INT1 0x03
#define FSA_REG_DEV_TYPE1 0x0A

#define FSA_INTB (1 << 0)
#define FSA_INT1_DETACH (1 << 1)
/* Both functions use SW polling technique to obtain the state of FSA9480 device */
/* AFAIK - HW interrupts aren't used on u-boot (at least for this purpose) */

int micro_usb_attached(void)
{
	unsigned char addr;
	unsigned char val;

	if (fsa9480_probe())
		return -1;

	addr = 0x25;	/* fsa9480 */

	/* Setup control register */
	i2c_read(addr, FSA_REG_CONTROL, 1, &val, 1);

	val |= FSA_INTB; /* Mask INTB interrupt */
	i2c_write(addr, FSA_REG_CONTROL, 1, &val, 1);

	i2c_read(addr, FSA_REG_DEV_TYPE1, 1, &val, 1);

	if (val & FSA_DEV1_USB)
		return 1;

	return 0;
}

int micro_usb_detach(void)
{
	unsigned char addr;
	unsigned char val[2];

	if (fsa9480_probe())
		return -1;

	addr = 0x25;	/* fsa9480 */

	/* Read interrupt status register */
	i2c_read(addr, FSA_REG_INT1, 1, val, 2);

	if (val[0] & FSA_INT1_DETACH) {
		puts("USB cable detached !!!\n");
		return 1;
	}

	return 0;
}


#define LP3974_REG_IRQ1		0x00
#define LP3974_REG_IRQ2		0x01
#define LP3974_REG_IRQ3		0x02
#define LP3974_REG_ONOFF1	0x11
#define LP3974_REG_ONOFF2	0x12
#define LP3974_REG_ONOFF3	0x13
#define LP3974_REG_ONOFF4	0x14
#define LP3974_REG_LDO7		0x21
#define LP3974_REG_LDO17	0x29
#define LP3974_REG_UVLO		0xB9
#define LP3974_REG_MODCHG	0xEF
/* ONOFF1 */
#define LP3974_LDO3		(1 << 2)
/* ONOFF2 */
#define LP3974_LDO6		(1 << 7)
#define LP3974_LDO7		(1 << 6)
#define LP3974_LDO8		(1 << 5)
#define LP3974_LDO9		(1 << 4)
#define LP3974_LDO10		(1 << 3)
#define LP3974_LDO11		(1 << 2)
#define LP3974_LDO12		(1 << 1)
#define LP3974_LDO13		(1 << 0)
/* ONOFF3 */
#define LP3974_LDO14		(1 << 7)
#define LP3974_LDO15		(1 << 6)
#define LP3974_LDO16		(1 << 5)
#define LP3974_LDO17		(1 << 4)

static int lp3974_probe(void)
{
	unsigned char addr = 0xCC >> 1;

	i2c_set_bus_num(I2C_5);

	if (i2c_probe(addr)) {
		puts("Can't found lp3974\n");
		return 1;
	}

	return 0;
}

static int max8952_probe(void)
{
	unsigned char addr = 0xC0 >> 1;

	i2c_set_bus_num(I2C_5);

	if (i2c_probe(addr)) {
		puts("Cannot find MAX8952\n");
		return 1;
	}

	return 0;
}

static void init_pmic_lp3974(void)
{
	unsigned char addr;
	unsigned char val[2];

	addr = 0xCC >> 1; /* LP3974 */
	if (lp3974_probe())
		return;

	/* LDO2 1.2V LDO3 1.1V */
	val[0] = 0x86; /* (((1200 - 800) / 50) << 4) | (((1100 - 800) / 50)) */
	i2c_write(addr, 0x1D, 1, val, 1);

	/* LDO4 3.3V */
	val[0] = 0x11; /* (3300 - 1600) / 100; */
	i2c_write(addr, 0x1E, 1, val, 1);

	/* LDO5 2.8V */
	val[0] = 0x0c; /* (2800 - 1600) / 100; */
	i2c_write(addr, 0x1F, 1, val, 1);

	/* LDO6 not used: minimum */
	val[0] = 0;
	i2c_write(addr, 0x20, 1, val, 1);

	/* LDO7 1.8V */
	val[0] = 0x02; /* (1800 - 1600) / 100; */
	i2c_write(addr, 0x21, 1, val, 1);

	/* LDO8 3.3V LDO9 2.8V*/
	val[0] = 0x30; /* (((3300 - 3000) / 100) << 4) | (((2800 - 2800) / 100) << 0); */
	i2c_write(addr, 0x22, 1, val, 1);

	/* LDO10 1.1V LDO11 3.3V */
	val[0] = 0x71; /* (((1100 - 950) / 50) << 5) | (((3300 - 1600) / 100) << 0); */
	i2c_write(addr, 0x23, 1, val, 1);

	/* LDO12 2.8V */
	val[0] = 0x14; /* (2800 - 1200) / 100 + 4; */
	i2c_write(addr, 0x24, 1, val, 1);

	/* LDO13 1.2V */
	val[0] = 0x4; /* (1200 - 1200) / 100 + 4; */
	i2c_write(addr, 0x25, 1, val, 1);

	/* LDO14 1.8V */
	val[0] = 0x6; /* (1800 - 1200) / 100; */
	i2c_write(addr, 0x26, 1, val, 1);

	/* LDO15 1.2V */
	val[0] = 0; /* (1200 - 1200) / 100; */
	i2c_write(addr, 0x27, 1, val, 1);

	/* LDO16 2.8V */
	val[0] = 0xc; /* (2800 - 1600) / 100; */
	i2c_write(addr, 0x28, 1, val, 1);

	/* LDO17 3.0V */
	val[0] = 0xe; /* (3000 - 1600) / 100; */
	i2c_write(addr, 0x29, 1, val, 1);

	/*
	 * Because the data sheet of LP3974 does NOT mention default
	 * register values of ONOFF1~4 (ENABLE1~4), we ignore the given
	 * default values and set as we want
	 */

	/* Note: To remove USB detect warning, Turn off LDO 8 first */

	/*
	 * ONOFF2
	 * LDO6 OFF, LDO7 ON, LDO8 OFF, LDO9 ON,
	 * LDO10 OFF, LDO11 OFF, LDO12 OFF, LDO13 OFF
	 */
	val[0] = 0x50;
	i2c_write(addr, LP3974_REG_ONOFF2, 1, val, 1);

	/*
	 * ONOFF1
	 * Buck1 ON, Buck2 OFF, Buck3 ON, Buck4 ON
	 * LDO2 ON, LDO3 OFF, LDO4 ON, LDO5 ON
	 */
	val[0] = 0xBB;
	i2c_write(addr, LP3974_REG_ONOFF1, 1, val, 1);

	/*
	 * ONOFF3
	 * LDO14 OFF, LDO15 OFF, LGO16 OFF, LDO17 ON,
	 * EPWRHOLD OFF, EBATTMON OFF, ELBCNFG2 OFF, ELBCNFG1 OFF
	 */
	val[0] = 0x10;
	i2c_write(addr, LP3974_REG_ONOFF3, 1, val, 1);

	/*
	 * ONOFF4
	 * EN32kAP ON, EN32kCP ON, ENVICHG ON, ENRAMP ON,
	 * RAMP 12mV/us (fastest)
	 */
	val[0] = 0xFB;
	i2c_write(addr, LP3974_REG_ONOFF4, 1, val, 1);

	/*
	 * CHGCNTL1
	 * ICHG: 500mA (0x3) / 600mA (0x5)
	 * RESTART LEVEL: 100mA (0x1)
	 * EOC LEVEL: 30% (0x4) / 25% (0x3) : both 150mA of ICHG
	 * Let's start with slower charging mode and let micro usb driver
	 * determine whether we can do it fast or not. Thus, using the slower
	 * setting...
	 */
	val[0] = 0x8B;
	i2c_write(addr, 0xC, 1, val, 1);

	/*
	 * CHGCNTL2
	 * CHARGER DISABLE: Enable (0x0)
	 * TEMP CONTROL: 105C (0x0)
	 * BATT SEL: 4.2V (0x0)
	 * FULL TIMEOUT: 5hr (0x0)
	 * ESAFEOUT2: ON (0x1)
	 * ESAFEOUT1: OFF (0x0)
	 */
	val[0] = 0x40;
	i2c_write(addr, 0xD, 1, val, 1);

	val[0] = 0x0E; /* 1.1V @ DVSARM1(VINT) */
	i2c_write(addr, 0x15, 1, val, 1);
	val[0] = 0x0E; /* 1.1V @ DVSARM2(VINT) */
	i2c_write(addr, 0x16, 1, val, 1);
	val[0] = 0x0E; /* 1.1V @ DVSARM3(VINT) */
	i2c_write(addr, 0x17, 1, val, 1);
	val[0] = 0x0A; /* 1.0V @ DVSARM4(VINT) */
	i2c_write(addr, 0x18, 1, val, 1);
	val[0] = 0x12; /* 1.2V @ DVSINT1(VG3D) */
	i2c_write(addr, 0x19, 1, val, 1);
	val[0] = 0x0E; /* 1.1V @ DVSINT2(VG3D) */
	i2c_write(addr, 0x1A, 1, val, 1);

	val[0] = 0x2; /* 1.8V for BUCK3 VCC 1.8V PDA */
	i2c_write(addr, 0x1B, 1, val, 1);
	val[0] = 0x4; /* 1.2V for BUCK4 VMEM 1.2V C210 */
	i2c_write(addr, 0x1C, 1, val, 1);

	/* Use DVSARM1 for VINT */
	gpio_direction_output(&gpio2->x0, 5, 0);
	gpio_direction_output(&gpio2->x0, 6, 0);
	/* Use DVSINT2 for VG3D */
	gpio_direction_output(&gpio1->e2, 0, 1);

	/*
	 * Default level of UVLO.
	 * UVLOf = 2.7V (0x3 << 4), UVLOr = 3.1V (0xB)
	 * set UVLOf to 2.55V (0 << 4).
	 */
	val[0] = 0x2C;
	i2c_write(addr, LP3974_REG_MODCHG, 1, val, 1);
	val[0] = 0x58;
	i2c_write(addr, LP3974_REG_MODCHG, 1, val, 1);
	val[0] = 0xB1;
	i2c_write(addr, LP3974_REG_MODCHG, 1, val, 1);

	i2c_read_r(addr, LP3974_REG_UVLO, 1, val, 1);
	val[0] = (val[0] & 0xf) | (0 << 4);
	i2c_write(addr, LP3974_REG_UVLO, 1, val, 1);

	val[0] = 0x00;
	i2c_write(addr, LP3974_REG_MODCHG, 1, val, 1);
}

int check_exit_key(void)
{
	return pmic_get_irq(PWRON1S);
}

static void check_keypad(void)
{
	unsigned int val = 0;
	unsigned int power_key, auto_download = 0;

	val = ~(gpio_get_value(&gpio2->x2, 1));

	power_key = pmic_get_irq(PWRONR);

	if (power_key && (val & 0x1))
		auto_download = 1;

	if (auto_download)
		setenv("bootcmd", "usbdown");
}

/*
 * charger_en(): set lp3974 pmic's charger mode
 * enable 0: disable charger
 * 600: 600mA
 * 500: 500mA
 */
static void charger_en(int enable)
{
	unsigned char addr = 0xCC >> 1; /* LP3974 */
	unsigned char val[2];

	if (lp3974_probe())
		return;

	switch (enable) {
	case 0:
		puts("Disable the charger.\n");
		i2c_read(addr, 0x0D, 1, val, 1);
		val[0] |= 0x1;
		i2c_write(addr, 0xD, 1, val, 1);
		break;
	case 500:
		puts("Enable the charger @ 500mA\n");
		/*
		 * CHGCNTL1
		 * ICHG: 500mA (0x3) / 600mA (0x5)
		 * RESTART LEVEL: 100mA (0x1)
		 * EOC LEVEL: 30% (0x4) / 25% (0x3) : both 150mA of ICHG
		 * Let's start with slower charging mode and
		 * let micro usb driver determine whether we can do it
		 * fast or not. Thus, using the slower setting...
		 */
		val[0] = 0x8B;
		i2c_write(addr, 0x0C, 1, val, 1);
		i2c_read(addr, 0x0D, 1, val, 1);
		val[0] &= ~(0x1);
		i2c_write(addr, 0x0D, 1, val, 1);
		break;
	case 600:
		puts("Enable the charger @ 600mA\n");
		val[0] = 0x6D;
		i2c_write(addr, 0x0C, 1, val, 1);
		i2c_read(addr, 0x0D, 1, val, 1);
		val[0] &= ~(0x1);
		i2c_write(addr, 0x0D, 1, val, 1);
		break;
	default:
		puts("Incorrect charger setting.\n");
	}
}

struct thermister_stat {
	short centigrade;
	unsigned short adc;
};

static struct thermister_stat adc_to_temperature_data[] = {
	{ .centigrade = -20,    .adc = 1856, },
	{ .centigrade = -15,    .adc = 1799, },
	{ .centigrade = -10,    .adc = 1730, },
	{ .centigrade = -5,     .adc = 1649, },
	{ .centigrade = 0,      .adc = 1556, },
	{ .centigrade = 5,      .adc = 1454, },
	{ .centigrade = 10,     .adc = 1343, },
	{ .centigrade = 15,     .adc = 1227, },
	{ .centigrade = 20,     .adc = 1109, },
	{ .centigrade = 25,     .adc = 992, },
	{ .centigrade = 30,     .adc = 880, },
	{ .centigrade = 35,     .adc = 773, },
	{ .centigrade = 40,     .adc = 675, },
	{ .centigrade = 45,     .adc = 586, },
	{ .centigrade = 50,     .adc = 507, },
	{ .centigrade = 55,     .adc = 436, },
	{ .centigrade = 58,     .adc = 399, },
	{ .centigrade = 63,     .adc = 343, },
	{ .centigrade = 65,     .adc = 322, },
};

#ifndef USHRT_MAX
#define USHRT_MAX	0xFFFFU
#endif

static int adc_to_temperature_centigrade(unsigned short adc)
{
	int i;
	int approximation;
	/* low_*: Greatest Lower Bound,
	 *          *          *          * high_*: Smallest Upper Bound */
	int low_temp = 0, high_temp = 0;
	unsigned short low_adc = 0, high_adc = USHRT_MAX;
	for (i = 0; i < ARRAY_SIZE(adc_to_temperature_data); i++) {
		if (adc_to_temperature_data[i].adc <= adc &&
				adc_to_temperature_data[i].adc >= low_adc) {
			low_temp = adc_to_temperature_data[i].centigrade;
			low_adc = adc_to_temperature_data[i].adc;
		}
		if (adc_to_temperature_data[i].adc >= adc &&
				adc_to_temperature_data[i].adc <= high_adc) {
			high_temp = adc_to_temperature_data[i].centigrade;
			high_adc = adc_to_temperature_data[i].adc;
		}
	}

	/* Linear approximation between cloest low and high,
	 * which is the weighted average of the two. */

	/* The following equation is correct only when the two are different */
	if (low_adc == high_adc)
		return low_temp;
	if (ARRAY_SIZE(adc_to_temperature_data) < 2)
		return 20; /* The room temperature */
	if (low_adc == 0)
		return high_temp;
	if (high_adc == USHRT_MAX)
		return low_temp;

	approximation = low_temp * (adc - low_adc) +
		high_temp * (high_adc - adc);
	approximation /= high_adc - low_adc;

	return approximation;
}

static int adc_get_average_ambient_temperature(void)
{
	unsigned short min = USHRT_MAX;
	unsigned short max = 0;
	unsigned int sum = 0;
	unsigned int measured = 0;
	int i;

	for (i = 0; i < 7; i++) {
		/* XADCAIN6 */
		unsigned short measurement = get_adc_value(6);
		sum += measurement;
		measured++;
		if (min > measurement)
			min = measurement;
		if (max < measurement)
			max = measurement;
	}
	if (measured >= 3) {
		measured -= 2;
		sum -= min;
		sum -= max;
	}
	sum /= measured;
	printf("Average Ambient Temperature = %d(ADC=%d)\n",
			adc_to_temperature_centigrade(sum), sum);
	return adc_to_temperature_centigrade(sum);
}

enum temperature_level {
	_TEMP_OK,
	_TEMP_OK_HIGH,
	_TEMP_OK_LOW,
	_TEMP_TOO_HIGH,
	_TEMP_TOO_LOW,
};

static enum temperature_level temperature_check(void)
{
	int temp = adc_get_average_ambient_temperature();
	if (temp < -5)
		return _TEMP_TOO_LOW;
	if (temp < 0)
		return _TEMP_OK_LOW;
	if (temp > 63)
		return _TEMP_TOO_HIGH;
	if (temp > 58)
		return _TEMP_OK_HIGH;
	return _TEMP_OK;
}

/*
 * into_charge_mode()
 * Run a charge loop with animation and temperature check with sleep
**/
static void into_charge_mode(int charger_speed)
{
	int i, j, delay;
	enum temperature_level previous_state = _TEMP_OK;
	unsigned int wakeup_stat = 0;

	/* 1. Show Animation */
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			printf(".");
			for (delay = 0; delay < 1000; delay++)
				udelay(1000);
		}
		printf("\n");
	}

	/* 2. Loop with temperature check and sleep */
	do {
		/* TODO: 2.A. Setup wakeup source and rtc tick */

		/* TODO: 2.B. Go to sleep */
		for (delay = 0; delay < 4000; delay++)
			udelay(1000);

		/* 2.C. Check the temperature */
		switch (temperature_check()) {
		case _TEMP_OK:
			charger_en(charger_speed);
			previous_state = _TEMP_OK;
			break;
		case _TEMP_TOO_LOW:
			charger_en(0);
			previous_state = _TEMP_TOO_LOW;
			break;
		case _TEMP_TOO_HIGH:
			charger_en(0);
			previous_state = _TEMP_TOO_HIGH;
			break;
		case _TEMP_OK_LOW:
			if (previous_state == _TEMP_TOO_LOW) {
				charger_en(0);
			} else {
				charger_en(charger_speed);
				previous_state = _TEMP_OK;
			}
			break;
		case _TEMP_OK_HIGH:
			if (previous_state == _TEMP_TOO_HIGH) {
				charger_en(0);
			} else {
				charger_en(charger_speed);
				previous_state = _TEMP_OK;
			}
			break;
		}
	} while (wakeup_stat == 0x04);
}

static void init_pmic_max8952(void)
{
	unsigned char addr;
	unsigned char val[2];

	addr = 0xC0 >> 1; /* MAX8952 */
	if (max8952_probe())
		return;

	/* MODE0: 1.10V: Default */
	val[0] = 33;
	i2c_write(addr, 0x00, 1, val, 1);
	/* MODE1: 1.20V */
	val[0] = 43;
	i2c_write(addr, 0x01, 1, val, 1);
	/* MODE2: 1.05V */
	val[0] = 28;
	i2c_write(addr, 0x02, 1, val, 1);
	/* MODE3: 0.95V */
	val[0] = 18;
	i2c_write(addr, 0x03, 1, val, 1);

	/*
	 * Note: use the default setting and configure pins high
	 * to generate the 1.1V
	 */
	/* VARM_OUTPUT_SEL_A / VID_0 / XEINT_3 (GPX0[3]) = default 0 */
	gpio_direction_output(&gpio2->x0, 3, 0);
	/* VARM_OUTPUT_SEL_B / VID_1 / XEINT_4 (GPX0[4]) = default 0 */
	gpio_direction_output(&gpio2->x0, 4, 0);

	/* CONTROL: Disable PULL_DOWN */
	val[0] = 0;
	i2c_write(addr, 0x04, 1, val, 1);

	/* SYNC: Do Nothing */
	/* RAMP: As Fast As Possible: Default: Do Nothing */
}

#ifdef CONFIG_LCD

void fimd_clk_set(void)
{
	struct exynos4_clock *clk =
		(struct exynos4_clock *)samsung_get_base_clock();

	/* workaround */
	unsigned long display_ctrl = 0x10010210;
	unsigned int cfg = 0;

	/* LCD0_BLK FIFO S/W reset */
	cfg = readl(display_ctrl);
	cfg |= (1 << 9);
	writel(cfg, display_ctrl);

	cfg = 0;

	/* FIMD of LBLK0 Bypass Selection */
	cfg = readl(display_ctrl);
	cfg &= ~(1 << 9);
	cfg |= (1 << 1);
	writel(cfg, display_ctrl);

	/* set lcd src clock */
	cfg = readl(&clk->src_lcd0);
	cfg &= ~(0xf);
	cfg |= 0x6;
	writel(cfg, &clk->src_lcd0);

	/* set fimd ratio */
	cfg = readl(&clk->div_lcd0);
	cfg &= ~(0xf);
	cfg |= 0x1;
	writel(cfg, &clk->div_lcd0);
}

extern void ld9040_set_platform_data(struct spi_platform_data *pd);

struct spi_platform_data spi_pd;

static void lcd_cfg_gpio(void)
{
	unsigned int i;

	for (i = 0; i < 8; i++) {
		/* set GPF0,1,2[0:7] for RGB Interface and Data lines (32bit) */
		gpio_cfg_pin(&gpio1->f0, i, GPIO_FUNC(2));
		gpio_cfg_pin(&gpio1->f1, i, GPIO_FUNC(2));
		gpio_cfg_pin(&gpio1->f2, i, GPIO_FUNC(2));
		/* pull-up/down disable */
		gpio_set_pull(&gpio1->f0, i, GPIO_PULL_NONE);
		gpio_set_pull(&gpio1->f1, i, GPIO_PULL_NONE);
		gpio_set_pull(&gpio1->f2, i, GPIO_PULL_NONE);

		/* drive strength to max (24bit) */
		gpio_set_drv(&gpio1->f0, i, GPIO_DRV_4X);
		gpio_set_rate(&gpio1->f0, i, GPIO_DRV_SLOW);
		gpio_set_drv(&gpio1->f1, i, GPIO_DRV_4X);
		gpio_set_rate(&gpio1->f1, i, GPIO_DRV_SLOW);
		gpio_set_drv(&gpio1->f2, i, GPIO_DRV_4X);
		gpio_set_rate(&gpio1->f2, i, GPIO_DRV_SLOW);
	}

	for (i = 0; i < 4; i++) {
		/* set GPF3[0:3] for RGB Interface and Data lines (32bit) */
		gpio_cfg_pin(&gpio1->f3, i, GPIO_PULL_UP);
		/* pull-up/down disable */
		gpio_set_pull(&gpio1->f3, i, GPIO_PULL_NONE);
		/* drive strength to max (24bit) */
		gpio_set_drv(&gpio1->f3, i, GPIO_DRV_4X);
		gpio_set_rate(&gpio1->f3, i, GPIO_DRV_SLOW);
	}

	/* gpio pad configuration for LCD reset. */
	gpio_direction_output(&gpio2->y4, 5, 1);

	/*
	 * gpio pad configuration for
	 * DISPLAY_CS, DISPLAY_CLK, DISPLAY_SO, DISPLAY_SI.
	 */
	gpio_cfg_pin(&gpio2->y4, 3, GPIO_OUTPUT);
	gpio_cfg_pin(&gpio2->y3, 1, GPIO_OUTPUT);
	gpio_cfg_pin(&gpio2->y3, 3, GPIO_OUTPUT);

	spi_pd.cs_bank = &gpio2->y4;
	spi_pd.cs_num = 3;
	spi_pd.clk_bank = &gpio2->y3;
	spi_pd.clk_num = 1;
	spi_pd.si_bank = &gpio2->y3;
	spi_pd.si_num = 3;

	spi_pd.mode = SPI_MODE_3;

	spi_pd.cs_active = ACTIVE_LOW;
	spi_pd.word_len = 8;

	ld9040_set_platform_data(&spi_pd);

	return;
}

extern void ld9040_cfg_ldo(void);
extern void ld9040_enable_ldo(unsigned int onoff);

int s5p_no_lcd_support(void)
{
	return 0;
}

void init_panel_info(vidinfo_t *vid)
{
	vid->vl_freq	= 60;
	vid->vl_col	= 480;
	vid->vl_row	= 800;
	vid->vl_width	= 480;
	vid->vl_height	= 800;
	vid->vl_clkp	= CONFIG_SYS_HIGH;
	vid->vl_hsp	= CONFIG_SYS_HIGH;
	vid->vl_vsp	= CONFIG_SYS_HIGH;
	vid->vl_dp	= CONFIG_SYS_HIGH;

	vid->vl_bpix	= 32;
	/* disable dual lcd mode. */
	vid->dual_lcd_enabled = 0;

	/* LD9040 LCD Panel */
	vid->vl_hspw	= 2;
	vid->vl_hbpd	= 16;
	vid->vl_hfpd	= 16;

	vid->vl_vspw	= 2;
	vid->vl_vbpd	= 6;
	vid->vl_vfpd	= 4;

	vid->cfg_gpio = lcd_cfg_gpio;
	vid->backlight_on = NULL;
	vid->lcd_power_on = NULL;	/* Don't need the poweron squence */
	vid->reset_lcd = NULL;		/* Don't need the reset squence */

	vid->cfg_ldo = ld9040_cfg_ldo;
	vid->enable_ldo = ld9040_enable_ldo;

	vid->init_delay = 0;
	vid->power_on_delay = 0;
	vid->reset_delay = 0;
	vid->interface_mode = FIMD_RGB_INTERFACE;

	/* board should be detected at here. */

	/* for LD8040. */
	vid->pclk_name = MPLL;
	vid->sclk_div = 1;

	setenv("lcdinfo", "lcd=ld9040");
}
#endif

static unsigned int get_hw_revision(void)
{
	int hwrev, mode0, mode1;

	mode0 = get_adc_value(1);		/* HWREV_MODE0 */
	mode1 = get_adc_value(2);		/* HWREV_MODE1 */

	/*
	 * XXX Always set the default hwrev as the latest board
	 * ADC = (voltage) / 3.3 * 4096
	 */
	hwrev = 3;

#define IS_RANGE(x, min, max)	((x) > (min) && (x) < (max))
	if (IS_RANGE(mode0, 80, 200) && IS_RANGE(mode1, 80, 200))
		hwrev = 0x0;		/* 0.01V	0.01V */
	if (IS_RANGE(mode0, 750, 1000) && IS_RANGE(mode1, 80, 200))
		hwrev = 0x1;		/* 610mV	0.01V */
	if (IS_RANGE(mode0, 1300, 1700) && IS_RANGE(mode1, 80, 200))
		hwrev = 0x2;		/* 1.16V	0.01V */
	if (IS_RANGE(mode0, 2000, 2400) && IS_RANGE(mode1, 80, 200))
		hwrev = 0x3;		/* 1.79V	0.01V */
#undef IS_RANGE

	debug("mode0: %d, mode1: %d, hwrev 0x%x\n", mode0, mode1, hwrev);

	return hwrev;
}

static const char * const pcb_rev[] = {
	"UNIV_0.0",
	"UNIV_0.1",
	"AQUILA_1.7",
	"AQUILA_1.9",
};

static void check_hw_revision(void)
{
	int hwrev;

	hwrev = get_hw_revision();

	board_rev |= hwrev;
}

static void show_hw_revision(void)
{
	printf("HW Revision:\t0x%x\n", board_rev);
	printf("PCB Revision:\t%s\n", pcb_rev[board_rev & 0xf]);
}

void get_rev_info(char *rev_info)
{
	sprintf(rev_info, "HW Revision: 0x%x (%s)\n",
			board_rev, pcb_rev[board_rev & 0xf]);
}

static void check_reset_status(void)
{
	int status = get_reset_status();

	puts("Reset Status: ");

	switch (status) {
	case EXTRESET:
		puts("Pin(Ext) Reset\n");
		break;
	case WARMRESET:
		puts("Warm Reset\n");
		break;
	case WDTRESET:
		puts("Watchdog Reset\n");
		break;
	case SWRESET:
		puts("S/W Reset\n");
		break;
	default:
		printf("Unknown (0x%x)\n", status);
	}
}

#ifdef CONFIG_CMD_RAMOOPS
static void show_dump_msg(void)
{
	int ret;

	ret = ramoops_init(samsung_get_base_modem());

	if (!ret)
		setenv("bootdelay", "-1");
}
#endif

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	check_reset_status();
#ifdef CONFIG_CMD_RAMOOPS
	show_dump_msg();
#endif

	show_hw_revision();

	pmic_bus_init(I2C_5);

	check_keypad();

	check_auto_burn();

	/* check max17040 */
	check_battery(0);

	/* check fsa9480 */
	check_micro_usb(0);

#ifdef CONFIG_INFO_ACTION
	info_action_check();
#endif

#ifdef CONFIG_CMD_PMIC
	run_command("pmic ldo 4 off", 0);	/* adc off */
#endif

	return 0;
}
#endif


static int s5pc2xx_phy_control(int on)
{
	static int status;
	if (on && !status) {
#ifdef CONFIG_CMD_PMIC
		run_command("pmic ldo 8 on", 0);
		run_command("pmic ldo 3 on", 0);
		run_command("pmic safeout 1 on", 0);
#endif
		/* USB Path to AP */
		micro_usb_switch(0);
		status = 1;
	} else if (!on && status) {
#ifdef CONFIG_CMD_PMIC
		run_command("pmic ldo 3 off", 0);
		run_command("pmic ldo 8 off", 0);
		run_command("pmic safeout 1 off", 0);
#endif
		status = 0;
	}
	udelay(10000);

	return 0;
}

struct s3c_plat_otg_data exynos4_otg_data = {
	.phy_control = s5pc2xx_phy_control,
	.regs_phy = EXYNOS4_USBPHY_BASE,
	.regs_otg = EXYNOS4_USBOTG_BASE,
};


#ifdef CONFIG_CMD_USB_MASS_STORAGE

static int ums_read_sector(struct ums_device *ums_dev, unsigned int n, void *buf)
{
	if (ums_dev->mmc->block_dev.block_read(ums_dev->dev_num,
	                                       n + ums_dev->offset, 1, buf) != 1)
		return -1;

	return 0;
}

static int ums_write_sector(struct ums_device *ums_dev, unsigned int n, void *buf)
{
	if (ums_dev->mmc->block_dev.block_write(ums_dev->dev_num,
	                                        n + ums_dev->offset, 1, buf) != 1)
		return -1;

	return 0;
}

static void ums_get_capacity(struct ums_device *ums_dev, long long int *capacity)
{
	long long int tmp_capacity;
	
	tmp_capacity = (long long int) ((ums_dev->offset + ums_dev->part_size) * SECTOR_SIZE);
	*capacity = ums_dev->mmc->capacity - tmp_capacity;
}

static struct ums_board_info ums_board = {
	.read_sector = ums_read_sector,
	.write_sector = ums_write_sector,
	.get_capacity = ums_get_capacity,
	.name = "SLP UMS disk",
	.ums_dev = {
		.mmc = NULL,
		.dev_num = 0,
		.offset = 0,
		.part_size = 0.
	},
};

struct ums_board_info *board_ums_init(unsigned int dev_num, unsigned int offset, 
                                      unsigned int part_size)
{
	struct mmc *mmc;
	
	mmc = find_mmc_device(dev_num);
	if (!mmc)
		return NULL;

	ums_board.ums_dev.mmc = mmc;
	ums_board.ums_dev.dev_num = dev_num;
	ums_board.ums_dev.offset = offset;
	ums_board.ums_dev.part_size = part_size;
	
	/* Init MMC */
	mmc_init(mmc);

	s3c_udc_probe(&exynos4_otg_data);
	return &ums_board;
}
#endif

#ifdef CONFIG_USBDOWNLOAD_GADGET
void usbd_gadget_udc_probe(void) {
	puts("USB_udc_probe\n");
	s3c_udc_probe(&exynos4_otg_data);
}
#endif

#ifdef CONFIG_CMD_USBDOWN
int usb_board_init(void)
{
	/* interrupt clear */
	pmic_get_irq(PWRON1S);

#ifdef CONFIG_CMD_PMIC
	run_command("pmic ldo 8 on", 0);
	run_command("pmic ldo 3 on", 0);
	run_command("pmic safeout 1 on", 0);
#endif
	return 0;
}
#endif

#ifdef CONFIG_GENERIC_MMC
int s5p_no_mmc_support(void)
{
	return 0;
}

int board_mmc_init(bd_t *bis)
{
	int i, err;

	switch (get_hwrev()) {
	case 0:
		/*
		 * Set the low to enable LDO_EN
		 * But when you use the test board for eMMC booting
		 * you should set it HIGH since it removes the inverter
		 */
		/* MASSMEMORY_EN: XMDMDATA_6: GPE3[6] */
		gpio_direction_output(&gpio1->e3, 6, 0);
		break;
	default:
		/*
		 * Default reset state is High and there's no inverter
		 * But set it as HIGH to ensure
		 */
		/* MASSMEMORY_EN: XMDMADDR_3: GPE1[3] */
		gpio_direction_output(&gpio1->e1, 3, 1);
		break;
	}

#ifdef CONFIG_MMC_BOOT_EVT0
	/*
	 * In MMC boot test board don't have inverter
	 */
	gpio_direction_output(&gpio1->e3, 6, 1);
#endif

	/*
	 * eMMC GPIO:
	 * SDR 8-bit@48MHz at MMC0
	 * GPK0[0]	SD_0_CLK(2)
	 * GPK0[1]	SD_0_CMD(2)
	 * GPK0[2]	SD_0_CDn	-> Not used
	 * GPK0[3:6]	SD_0_DATA[0:3](2)
	 * GPK1[3:6]	SD_0_DATA[0:3](3)
	 *
	 * DDR 4-bit@26MHz at MMC4
	 * GPK0[0]	SD_4_CLK(3)
	 * GPK0[1]	SD_4_CMD(3)
	 * GPK0[2]	SD_4_CDn	-> Not used
	 * GPK0[3:6]	SD_4_DATA[0:3](3)
	 * GPK1[3:6]	SD_4_DATA[4:7](4)
	 */
	for (i = 0; i < 7; i++) {
		if (i == 2)
			continue;
		/* GPK0[0:6] special function 2 */
		gpio_cfg_pin(&gpio2->k0, i, 0x2);
		/* GPK0[0:6] pull disable */
		gpio_set_pull(&gpio2->k0, i, GPIO_PULL_NONE);
		/* GPK0[0:6] drv 4x */
		gpio_set_drv(&gpio2->k0, i, GPIO_DRV_4X);
	}

	for (i = 3; i < 7; i++) {
		/* GPK1[3:6] special function 3 */
		gpio_cfg_pin(&gpio2->k1, i, 0x3);
		/* GPK1[3:6] pull disable */
		gpio_set_pull(&gpio2->k1, i, GPIO_PULL_NONE);
		/* GPK1[3:6] drv 4x */
		gpio_set_drv(&gpio2->k1, i, GPIO_DRV_4X);
	}

	/* T-flash detect */
	gpio_cfg_pin(&gpio2->x3, 4, 0xf);
	gpio_set_pull(&gpio2->x3, 4, GPIO_PULL_UP);

	/*
	 * MMC device init
	 * mmc0	 : eMMC (8-bit buswidth)
	 * mmc2	 : SD card (4-bit buswidth)
	 */
	err = s5p_mmc_init(0, 8);

	/*
	 * Check the T-flash  detect pin
	 * GPX3[4] T-flash detect pin
	 */
	if (!gpio_get_value(&gpio2->x3, 4)) {
		/*
		 * SD card GPIO:
		 * GPK2[0]	SD_2_CLK(2)
		 * GPK2[1]	SD_2_CMD(2)
		 * GPK2[2]	SD_2_CDn	-> Not used
		 * GPK2[3:6]	SD_2_DATA[0:3](2)
		 */
		for (i = 0; i < 7; i++) {
			if (i == 2)
				continue;
			/* GPK2[0:6] special function 2 */
			gpio_cfg_pin(&gpio2->k2, i, 0x2);
			/* GPK2[0:6] pull disable */
			gpio_set_pull(&gpio2->k2, i, GPIO_PULL_NONE);
			/* GPK2[0:6] drv 4x */
			gpio_set_drv(&gpio2->k2, i, GPIO_DRV_4X);
		}
		err = s5p_mmc_init(2, 4);
	}

	return err;

}
#endif

#ifdef CONFIG_CMD_DEVICE_POWER
static int do_microusb(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	switch (argc) {
	case 2:
		if (strncmp(argv[1], "cp", 2) == 0) {
			micro_usb_switch(1);
			run_command("pmic safeout 2 on", 0);
			setenv("usb", "cp");
		} else if (strncmp(argv[1], "ap", 2) == 0) {
			micro_usb_switch(0);
			run_command("pmic safeout 2 off", 0);
			setenv("usb", "ap");
		}
		break;
	default:
		cmd_usage(cmdtp);
		return 1;
	}

	saveenv();

	printf("USB Path is set to %s\n", getenv("usb"));

	return 0;
}

U_BOOT_CMD(
	microusb,		CONFIG_SYS_MAXARGS,	1, do_microusb,
	"Micro USB Switch",
	"cp - switch to CP\n"
	"microusb ap - switch to AP\n"
);
#endif
