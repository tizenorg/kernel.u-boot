/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Sanghee Kim <sh0130.kim@samsung.com>
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
#include <lcd.h>
#include <spi.h>
#include <swi.h>
#include <asm/io.h>
#include <asm/arch/adc.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/power.h>
#include <asm/arch/clk.h>
#include <asm/arch/clock.h>
#include <asm/arch/mipi_dsim.h>
#include <asm/arch/regs-fb.h>
#include <pmic.h>
#include <mmc.h>
#include <fat.h>
#include <max17042.h>
#include <mobile/misc.h>
#include <mobile/fs_type_check.h>
#include "common.h"

#ifdef CONFIG_S5PC1XXFB
#include <fbutils.h>
#endif

#define REV_MASK	0x0000ffff

DECLARE_GLOBAL_DATA_PTR;

static struct exynos4_gpio_part1 *gpio1;
static struct exynos4_gpio_part2 *gpio2;

static unsigned int battery_soc;
static unsigned int battery_uV;	/* in micro volts */
static unsigned int battery_cap;	/* in mAh */
static unsigned int board_rev;
static int boot_mode = -1;

u32 get_board_rev(void)
{
	return board_rev;
}

static int hwrevision(int rev)
{
	return (board_rev & 0xf) == rev;
}

enum {
	I2C_0, I2C_1, I2C_2, I2C_3,
	I2C_4, I2C_5, I2C_6, I2C_7,
	I2C_8, I2C_9, I2C_NUM,
};

/* i2c0 (CAM)	SDA: GPD1[0] SCL: GPD1[1] */
static struct i2c_gpio_bus_data i2c_0 = {
	.sda_pin = 0,
	.scl_pin = 1,
};

/* i2c1 (Gryo)	SDA: GPD1[2] SCL: GPD1[3] */
static struct i2c_gpio_bus_data i2c_1 = {
	.sda_pin = 2,
	.scl_pin = 3,
};

/* i2c3 (TSP)	SDA: GPA1[2] SCL: GPA1[3] */
static struct i2c_gpio_bus_data i2c_3 = {
	.sda_pin = 2,
	.scl_pin = 3,
};

/* i2c5 (PMIC)	SDA: GPB[6] SCL: GPB[7] */
static struct i2c_gpio_bus_data i2c_5 = {
	.sda_pin = 6,
	.scl_pin = 7,
};

/* i2c6 (CODEC)	SDA: GPC1[3] SCL: GPC1[4] */
static struct i2c_gpio_bus_data i2c_6 = {
	.sda_pin = 3,
	.scl_pin = 4,
};

/* i2c7		SDA: GPD0[2] SCL: GPD0[3] */
static struct i2c_gpio_bus_data i2c_7 = {
	.sda_pin = 2,
	.scl_pin = 3,
};

/* i2c9 (Fuel Gauge)	SDA: SPY4[0] SCL: SPY4[1] */
static struct i2c_gpio_bus_data i2c_9 = {
	.sda_pin = 0,
	.scl_pin = 1,
};

static struct i2c_gpio_bus i2c_gpio[I2C_NUM];

static void init_battery_max17042(void);
static void init_pmic_max8997(void);
static void check_hw_revision(void);
static int into_charge_boot(int mode);

void i2c_init_board(void)
{
	i2c_gpio[I2C_0].bus = &i2c_0;
	i2c_gpio[I2C_1].bus = &i2c_1;
	i2c_gpio[I2C_2].bus = NULL;
	i2c_gpio[I2C_3].bus = &i2c_3;
	i2c_gpio[I2C_4].bus = NULL;
	i2c_gpio[I2C_5].bus = &i2c_5;
	i2c_gpio[I2C_6].bus = &i2c_6;
	i2c_gpio[I2C_7].bus = &i2c_7;
	i2c_gpio[I2C_8].bus = NULL;
	i2c_gpio[I2C_9].bus = &i2c_9;

	i2c_gpio[I2C_0].bus->gpio_base = (unsigned int)&gpio1->d1;
	i2c_gpio[I2C_1].bus->gpio_base = (unsigned int)&gpio1->d1;
	i2c_gpio[I2C_3].bus->gpio_base = (unsigned int)&gpio1->a1;
	i2c_gpio[I2C_5].bus->gpio_base = (unsigned int)&gpio1->b;
	i2c_gpio[I2C_6].bus->gpio_base = (unsigned int)&gpio1->c1;
	i2c_gpio[I2C_7].bus->gpio_base = (unsigned int)&gpio1->d0;
	i2c_gpio[I2C_9].bus->gpio_base = (unsigned int)&gpio2->y4;

	i2c_gpio_init(i2c_gpio, I2C_NUM, I2C_5);
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
int check_home_key(void);
int board_early_init_f(void)
{
	gpio1 = (struct exynos4_gpio_part1 *)EXYNOS4_GPIO_PART1_BASE;
	gpio2 = (struct exynos4_gpio_part2 *)EXYNOS4_GPIO_PART2_BASE;
#ifdef CONFIG_OFFICIAL_REL
	if (!check_home_key())
		gd->flags |= GD_FLG_DISABLE_CONSOLE;
#endif
	return 0;
}
#endif

int board_init(void)
{
	gpio1 = (struct exynos4_gpio_part1 *)EXYNOS4_GPIO_PART1_BASE;
	gpio2 = (struct exynos4_gpio_part2 *)EXYNOS4_GPIO_PART2_BASE;

	check_hw_revision();

	gd->bd->bi_arch_number = MACH_TYPE;
	if (hwrevision(1) || hwrevision(2))
		gd->bd->bi_arch_number += 1;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	/* Check reboot reason */
	boot_mode = get_boot_mode();
	/* Set frame buffer */
	switch (boot_mode) {
	case LOCKUP_RESET:
	case DUMP_REBOOT:
	case DUMP_FORCE_REBOOT:
		gd->fb_base = CONFIG_SYS_FB2_ADDR;
		lcd_base = (void *)(gd->fb_base);
		break;
	default:
		break;
	}
#ifdef CONFIG_SBOOT
	/* workaround: clear INFORM4..5 */
	memset((void *)CONFIG_INFO_ADDRESS, 0x0, sizeof(u32) * 2);
#endif
	return 0;
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE + PHYS_SDRAM_2_SIZE +
	    PHYS_SDRAM_3_SIZE + PHYS_SDRAM_4_SIZE;

	check_hw_revision();

	/* Early init for i2c devices - Where these funcions should go?? */

	/* pmic init */
	init_pmic_max8997();

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
	gd->bd->bi_dram[2].start = PHYS_SDRAM_3;
	gd->bd->bi_dram[2].size = PHYS_SDRAM_3_SIZE;
	gd->bd->bi_dram[3].start = PHYS_SDRAM_4;
	gd->bd->bi_dram[3].size = PHYS_SDRAM_4_SIZE;
}

static void check_auto_burn(void)
{
	unsigned int magic_base = CONFIG_SYS_SDRAM_BASE + 0x02000000;
	unsigned int count = 0;
	char buf[64];

	/* Initial Setting */
	if (readl(magic_base) == 0x534E5344) {	/* ASICC: SNSD */
		puts("Auto buring intiail Setting (boot image)\n");
		count += sprintf(buf + count, "run setupboot; ");
		goto done;
	}
	/* MMC */
	if (readl(magic_base) == 0x654D4D43) {	/* ASICC: eMMC */
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

 done:
	if (count) {
		count += sprintf(buf + count, "reset");
		setenv("bootcmd", buf);
		setenv("updatestate", NULL);
	}

	/* Clear the magic value */
	memset((void *)magic_base, 0, 2);
}

static unsigned char inline max8997_reg_ldo(int uV)
{
	unsigned char ret;
	if (uV <= 800000)
		return 0;
	if (uV >= 3950000)
		return 0x3f;
	ret = (uV - 800000) / 50000;
	if (ret > 0x3f) {
		printf("MAX8997 LDO SETTING ERROR (%duV) -> %u\n", uV, ret);
		ret = 0x3f;
	}

	return ret;
}

int pmic_has_battery(void)
{
	return pmic_charger_detbat();
}

/*
 * when ta connected is 1, usb is 2
 */
static int ta_usb_connected = 0;

int get_ta_usb_status(void)
{
	return ta_usb_connected;
}

static void into_minimum_power(void)
{
	struct exynos4_clock *clk =
	    (struct exynos4_clock *)samsung_get_base_clock();
	unsigned int reg;
	unsigned char addr = 0xCC >> 1;
	unsigned char val[2];

	/* Turn the core1 off */
	writel(0x0, 0x10022080);

	/* Slow down the CPU: 100MHz */
	/* 1. Set APLL_CON0 @ 100MHZ */
	writel(0xa0c80604, &clk->apll_con0);
	/* 2. Change system clock dividers */
	writel(0x00000100, &clk->div_cpu0);	/* CLK_DIV_CPU0 */
	do {
		reg = readl(&clk->div_stat_cpu0);	/* CLK_DIV_STAT_CPU0 */
	} while (reg & 0x1111111);
	/* skip CLK_DIV_CPU1: no change */
	writel(0x13113117, &clk->div_dmc0);	/* CLK_DIV_DMC0 */
	do {
		reg = readl(&clk->div_stat_dmc0);
	} while (reg & 0x11111111);
	/* skip CLK_DIV_TOP: no change */
	/* skip CLK_DIV_LEFT/RIGHT BUS: no change */

	if (pmic_probe())
		return;

	/* LDO1 VADC: 3.3V */
	val[0] = max8997_reg_ldo(3300000) | 0x00;	/* OFF */
	i2c_write(addr, 0x3b, 1, val, 1);
	/* LDO5 VHSIC: 1.2V */
	val[0] = max8997_reg_ldo(1200000) | 0x00;	/* OFF */
	i2c_write(addr, 0x3f, 1, val, 1);
	/* LDO7 CAM_ISP: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x41, 1, val, 1);
	/* LDO8 VDAC/VUSB: 3.3V */
	val[0] = max8997_reg_ldo(3300000) | 0x00;	/* OFF */
	i2c_write(addr, 0x42, 1, val, 1);
	/* LDO11 LVDS: 3.3V */
	val[0] = max8997_reg_ldo(3300000) | 0x00;	/* OFF */
	i2c_write(addr, 0x45, 1, val, 1);
	/* LDO12 VTCAM: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x46, 1, val, 1);
	/* LDO14 MOTOR: 3.0V */
	val[0] = max8997_reg_ldo(3000000) | 0x00;	/* OFF */
	i2c_write(addr, 0x48, 1, val, 1);
	/* LDO16 CAM_SENSOR: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x4a, 1, val, 1);
	/* LDO18 VTOUCH 2.8V */
	val[0] = max8997_reg_ldo(2800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x4c, 1, val, 1);

	/* Turn off unnecessary power domains */
	writel(0x0, 0x10023420);	/* XXTI */
	writel(0x0, 0x10023C00);	/* CAM */
	writel(0x0, 0x10023C20);
	writel(0x0, 0x10023C40);
	writel(0x0, 0x10023C60);
	writel(0x0, 0x10023CC0);
	writel(0x0, 0x10023CE0);
	writel(0x0, 0x10023D00);	/* GPS_ALIVE */

	/* Turn off unnecessary clocks */
	writel(0x0, &clk->gate_ip_cam);	/* CAM */
	writel(0x0, &clk->gate_ip_tv);	/* TV */
	writel(0x0, &clk->gate_ip_mfc);	/* MFC */
	writel(0x0, &clk->gate_ip_g3d);	/* G3D */
	writel(0x0, &clk->gate_ip_image);	/* IMAGE */
	writel(0x0, &clk->gate_ip_gps);	/* GPS */
}

static void init_battery_max17042(void)
{
	unsigned char addr = MAX8997_FG_ADDR;

	i2c_set_bus_num(I2C_9);

	if (i2c_probe(addr)) {
		puts("Can't found max17042 fuel gauge\n");
		return;
	}

	init_fuel_gauge(ta_usb_connected);
}

static void lpm_charge(void)
{
	unsigned char val[2] = { 0, };
	unsigned char addr = MAX8997_FG_ADDR;
	unsigned int vcell = 0, battery_soc = 0;

	into_minimum_power();
	into_charge_boot(ta_usb_connected);

#ifdef CONFIG_S5PC1XXFB
	if (!board_no_lcd_support()) {
		lcd_display_clear();
		init_font();
		set_font_color(FONT_WHITE);

		fb_printf("LPM Charging start..\n");
		fb_printf("It will take 5mins, Please wait for a while.\n");
	}
#endif

	while (1) {
		udelay(10000000);
		puts(".");
#ifdef CONFIG_S5PC1XXFB
		fb_printf(".");
#endif

		i2c_set_bus_num(I2C_9);
		addr = MAX8997_FG_ADDR;

		if (i2c_probe(addr)) {
			puts("Can't found max17042 fuel gauge\n");
			continue;
		}

		i2c_read(addr, 0x09, 1, val, 2);
		vcell = (val[0] >> 3) + (val[1] << 5);
		vcell = (vcell * 625);

		i2c_read(addr, 0xFF, 1, val, 2);
		battery_soc = val[1];

		if (battery_soc >= 5) {
#ifdef CONFIG_S5PC1XXFB
			fb_printf("\nLPM Charging finished.\n");
#endif
			puts("\nLPM Charging finished.\n");
			board_inform_set(REBOOT_CHARGE);
			do_reset(NULL, 0, 0, NULL);
		}

#ifdef CONFIG_CHARGER_SS6000
/*
 * using TA_nCONNECTED for SS6000 Charger.(not MAX8997 MUIC)
 */
		if (gpio_get_value(&gpio2->l2, 5)) {
#else
		if (!ta_usb_connected) {
#endif
#ifdef CONFIG_S5PC1XXFB
			fb_printf("\nUSB/TA cable has ejected.\n");
#endif
			puts("\nUSB/TA cable has ejected.\n");
			power_off();
		}
	}
}

static void check_battery(void)
{
	unsigned char val[2];
	unsigned char addr = MAX8997_FG_ADDR;

	i2c_set_bus_num(I2C_9);

	if (i2c_probe(addr)) {
		puts("Can't found max17042 fuel gauge\n");
		return;
	}

	i2c_read(addr, 0xFF, 1, val, 2);
	battery_soc = val[1];
	printf("battery:\t%d%%\n", battery_soc);

	i2c_read(addr, 0x09, 1, val, 2);
	battery_uV = (val[0] >> 3) + (val[1] << 5);
	battery_uV = (battery_uV * 625);

	i2c_read(addr, 0x05, 1, val, 2);
	battery_cap = (val[0] + val[1] * 256) / 2;
	printf("voltage:\t%d.%6.6d V (expected to be %dmAh)\n",
	       battery_uV / 1000000, battery_uV % 1000000, battery_cap);

	/* When JIG is connected, it may skip */
	if (battery_uV > 3850000)
		return;

	if (battery_uV < 3600000 || battery_soc < 5) {
		if (ta_usb_connected) {
			printf("Enter LPM Charging Mode\n");
			lpm_charge();
		} else {
			/* threshold can be changed */
			if (battery_soc < 2) {
#ifdef CONFIG_LCD
				if (!board_no_lcd_support()) {
					init_font();
					set_font_color(FONT_YELLOW);
					fb_printf("\nPlease Charge the battery");
				}
#endif
				puts("\nPlease Charge the battery\n");
				udelay(3000000);
				power_off();
			}
		}
	}
}

static void init_rtc_max8997(void)
{
	unsigned char addr = MAX8997_RTC_ADDR;
	unsigned char val[2] = {0,};

	i2c_set_bus_num(I2C_5);

	if (i2c_probe(addr)) {
		puts("Can't found max8997-rtc\n");
		return;
	}

	/* Disable SMPL & WTSR */
	i2c_write(addr, 0x6, 1, val, 1);
	val[0] = 1;
	i2c_write(addr, 0x4, 1, val, 1);
}

#if defined(CONFIG_EXTERNAL_CHARGER)
#define MAX8997_CHARGER_ENABLE 0xC0
static void disable_charger_max8997(void)
{
	unsigned char addr = MAX8997_PMIC_ADDR;
	unsigned char val[2] = {0,};

	/*
	 * In case of using external charger(currently SS6000 in U1)
	 * we need to Disable MAX8997-Charger.
	 */
	i2c_read(addr, 0x51, 1, val, 1);
	val[0] &= ~MAX8997_CHARGER_ENABLE;
	i2c_write(addr, 0x51, 1, val, 1);
}
#endif

static void init_pmic_max8997(void)
{
	unsigned char addr = MAX8997_PMIC_ADDR;
	unsigned char val[2] = {0,};
	struct exynos4_clock *clk =
		(struct exynos4_clock *)samsung_get_base_clock();
	int i = 0, ret = 0;

	if (pmic_probe())
		return;

	/* BUCK1 VARM: 1.2V */
	val[0] = (1200000 - 650000) / 25000;
	i2c_write(addr, 0x19, 1, val, 1);
	val[0] = 0x09;		/* DVS OFF */
	i2c_write(addr, 0x18, 1, val, 1);

	/* BUCK2 VINT: 1.1V */
	val[0] = (1100000 - 650000) / 25000;
	i2c_write(addr, 0x22, 1, val, 1);
	val[0] = 0x09;		/* DVS OFF */
	i2c_write(addr, 0x21, 1, val, 1);

	/* BUCK3 G3D: 1.1V - OFF */
	i2c_read(addr, 0x2a, 1, &val[0], 1);
	val[0] &= 0xfe;
	i2c_write(addr, 0x2a, 1, val, 1);

	val[0] = (1100000 - 750000) / 50000;
	i2c_write(addr, 0x2b, 1, val, 1);

	/* BUCK4 CAMISP: 1.2V - OFF */
	i2c_read(addr, 0x2c, 1, &val[0], 1);
	val[0] &= 0xfe;
	i2c_write(addr, 0x2c, 1, val, 1);

	val[0] = (1200000 - 650000) / 25000;
	i2c_write(addr, 0x2d, 1, val, 1);

	/* BUCK5 VMEM: 1.2V */
	val[0] = (1200000 - 650000) / 25000;
	for (i = 0; i < 8; i++) {
		i2c_write(addr, (0x2f + i), 1, val, 1);
	}

	val[0] = 0x09;		/* DVS OFF */
	i2c_write(addr, 0x2e, 1, val, 1);

	/* BUCK6 CAM AF: 2.8V */
	/* No Voltage Setting Register */
	/* GNSLCT 3.0X */
	val[0] = 0x04;
	i2c_write(addr, 0x37, 1, val, 1);

	/* BUCK7 VCC_SUB: 2.0V */
	val[0] = (2000000 - 750000) / 50000;
	i2c_write(addr, 0x3a, 1, val, 1);

	/* LDO1 VADC: 3.3V */
	val[0] = max8997_reg_ldo(3300000) | 0x00;	/* OFF */
	i2c_write(addr, 0x3b, 1, val, 1);

	/* LDO1 Disable active dsicharging */
	i2c_read(addr, 0x80, 1, &val[0], 1);
	val[0] &= 0xfd;
	i2c_write(addr, 0x80, 1, val, 1);


	/* LDO2 VALIVE: 1.1V */
	val[0] = max8997_reg_ldo(1100000) | 0xC0;
	i2c_write(addr, 0x3c, 1, val, 1);

	/* LDO3 VUSB/MIPI: 1.1V */
	val[0] = max8997_reg_ldo(1100000) | 0x00;	/* OFF */
	i2c_write(addr, 0x3d, 1, val, 1);

	/* LDO4 VMIPI: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x3e, 1, val, 1);

	/* LDO5 VHSIC: 1.2V */
	val[0] = max8997_reg_ldo(1200000) | 0x00;	/* OFF */
	i2c_write(addr, 0x3f, 1, val, 1);

	/* LDO6 VCC_1.8V_PDA: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0xC0;
	i2c_write(addr, 0x40, 1, val, 1);

	/* LDO7 CAM_ISP: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x41, 1, val, 1);

	/* LDO8 VDAC/VUSB: 3.3V */
	val[0] = max8997_reg_ldo(3300000) | 0x00;	/* OFF */
	i2c_write(addr, 0x42, 1, val, 1);

	/* LDO9 VCC_2.8V_PDA: 2.8V */
	val[0] = max8997_reg_ldo(2800000) | 0xC0;
	i2c_write(addr, 0x43, 1, val, 1);

	/* LDO10 VPLL: 1.1V */
	val[0] = max8997_reg_ldo(1100000) | 0xC0;
	i2c_write(addr, 0x44, 1, val, 1);

	/* LDO11 TOUCH: 2.8V */
	val[0] = max8997_reg_ldo(2800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x45, 1, val, 1);

	/* LDO12 VTCAM: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x46, 1, val, 1);

	/* LDO13 VCC_3.0_LCD: 3.0V */
	val[0] = max8997_reg_ldo(3000000) | 0xC0;
	i2c_write(addr, 0x47, 1, val, 1);

	/* LDO14 MOTOR: 3.0V */
	val[0] = max8997_reg_ldo(3000000) | 0x00;	/* OFF */
	i2c_write(addr, 0x48, 1, val, 1);

	/* LDO15 LED_A: 2.8V */
	val[0] = max8997_reg_ldo(2800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x49, 1, val, 1);

	/* LDO16 CAM_SENSOR: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x4a, 1, val, 1);

	/* LDO17 VTF: 2.8V */
	val[0] = max8997_reg_ldo(2800000) | 0x00;	/* OFF */
	i2c_write(addr, 0x4b, 1, val, 1);

	/* LDO18 TOUCH_LED 3.3V */
	val[0] = max8997_reg_ldo(3300000) | 0x00;	/* OFF */
	i2c_write(addr, 0x4c, 1, val, 1);

	/* LDO21 VDDQ: 1.2V */
	val[0] = max8997_reg_ldo(1200000) | 0xC0;
	i2c_write(addr, 0x4d, 1, val, 1);

	/* SAFEOUT for both 1 and 2: 4.9V, Active discharge, Enable */
	val[0] =
	    (0x1 << 0) | (0x1 << 2) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
	i2c_write(addr, 0x5a, 1, val, 1);

	/* Turn off unnecessary power domains */
	writel(0x0, 0x10023420);	/* XXTI */
	writel(0x0, 0x10023C00);	/* CAM */
	writel(0x0, 0x10023C20);	/* TV */
	writel(0x0, 0x10023C40);	/* MFC */
	writel(0x0, 0x10023C60);	/* G3D */
	writel(0x0, 0x10023CE0);	/* GPS */
	writel(0x0, 0x10023D00);	/* GPS_ALIVE */

	/* Turn off unnecessary clocks */
	writel(0x0, &clk->gate_ip_cam);	/* CAM */
	writel(0x0, &clk->gate_ip_tv);	/* TV */
	writel(0x0, &clk->gate_ip_mfc);	/* MFC */
	writel(0x0, &clk->gate_ip_g3d);	/* G3D */
	writel(0x0, &clk->gate_ip_image);	/* IMAGE */
	writel(0x0, &clk->gate_ip_gps);	/* GPS */

	/* enable MAX8997 charger */
	val[0] = 0xd4;
	ret = i2c_write(addr, 0x51, 1, val, 1);
	if (ret)
		puts("Charger enbled failed.\n");
}

static void init_muic(void)
{
	muic_init();
}

#ifdef CONFIG_CHARGER_SS6000
void charger_en(int enable)
{
	if (!enable) {
		gpio_direction_output(&gpio2->l2, 2, 0);
		return;
	}

	if(enable == 395) {
		printf("Enable USB Charger\n");
		gpio_direction_output(&gpio2->l2, 2, 1);
	} else {
		printf("Enable TA Charger\n");
		gpio_direction_output(&gpio2->l2, 2, 1);
		udelay(5000);
		gpio_direction_output(&gpio2->l2, 2, 0);
		udelay(300);
		gpio_direction_output(&gpio2->l2, 2, 1);
	}
}
#endif

/* TODO add Jig control */
static int into_charge_boot(int mode)
{
	int chg_current;

	if (!pmic_has_battery())
		return -1;
	/* interrupt clear */
	pmic_get_irq(PWRON1S);

	/* U1 has 1650mAh battery, charging with 650mA/TA, 395mA/USB */
	switch (mode) {
	case CHARGER_TA:
		chg_current = 650;
		break;
	case CHARGER_TA_500:
		chg_current = 500;
		break;
	case CHARGER_UNKNOWN:
		/* for cable which don't officially support, fall through */
	case CHARGER_USB:
		chg_current = 450;
		break;
	case CHARGER_NO:
		chg_current = 0;
		break;
	default:
		printf("charger: not supported mode (%d)\n", mode);
		return 1;
	}

#ifdef CONFIG_CHARGER_SS6000
	charger_en(chg_current);
#else
	pmic_charger_en(chg_current);
#endif

	return 0;
}

void board_muic_gpio_control(int output, int path)
{
	/*
	 * output - 0:usb, 1:uart
	 * path   - 0:cp,  1:ap
	 */
	if (output)
		/* UART_SEL (0: CP_TXD/RDX, 1: AP_TXD/RDX) */
		if (path)
			gpio_direction_output(&gpio2->y4, 7, 1);
		else
			gpio_direction_output(&gpio2->y4, 7, 0);
	else
		/* USB_SEL (0: IF_TXD/RXD, 1:CP_D+/D-) */
		if (path)
			gpio_direction_output(&gpio2->l0, 6, 1);
		else
			gpio_direction_output(&gpio2->l0, 6, 0);
}

int check_exit_key(void)
{
	static int count = 0;

	if (pmic_get_irq(PWRONR))
		count++;

	if (count >= 3) {
		count = 0;
		return 1;
	}

	return 0;
}

int check_volume_up(void)
{
	return !(gpio_get_value(&gpio2->x2, 0));
}

int check_volume_down(void)
{
	return !(gpio_get_value(&gpio2->x2, 1));
}

int check_home_key(void)
{
	return !(gpio_get_value(&gpio2->x3, 5));
}

static void check_keypad(void)
{
	unsigned int val = 0;
	unsigned int power_key, auto_download = 0;

	val = check_volume_down();

	power_key = pmic_get_irq_booton(PWRONR);

	if (power_key && val)
		auto_download = 1;

	if (auto_download)
		setenv("bootcmd", "usbdown");
}

#ifdef CONFIG_LCD
void fimd_clk_set(void)
{
	struct exynos4_clock *clk =
	    (struct exynos4_clock *)samsung_get_base_clock();

	/* workaround */
	unsigned long display_ctrl = EXYNOS4_LCDBLK_CFG;
	unsigned int cfg = 0;

	if (hwrevision(1) || hwrevision(2)) {
		cfg = readl(display_ctrl);
		cfg |= (1 << 1);
		writel(cfg, display_ctrl);
	} else {
		/* LCD0_BLK FIFO S/W reset */
		cfg = readl(display_ctrl);
		cfg |= (1 << 9);
		writel(cfg, display_ctrl);

		/* FIMD of LBLK0 Bypass Selection */
		cfg = readl(display_ctrl);
		cfg &= ~(1 << 9);
		cfg |= (1 << 1);
		writel(cfg, display_ctrl);
	}

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
		gpio_cfg_pin(&gpio1->f3, i, GPIO_FUNC(2));
		/* pull-up/down disable */
		gpio_set_pull(&gpio1->f3, i, GPIO_PULL_NONE);
		/* drive strength to max (24bit) */
		gpio_set_drv(&gpio1->f3, i, GPIO_DRV_4X);
		gpio_set_rate(&gpio1->f3, i, GPIO_DRV_SLOW);
	}

	/* gpio pad configuration for LCD reset. */
	gpio_direction_output(&gpio2->y4, 5, GPIO_OUTPUT);

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
}

static void lcd_reset(void)
{
	gpio_direction_output(&gpio2->y4, 5, 1);
	gpio_set_value(&gpio2->y4, 5, 0);
	udelay(20);
	gpio_set_value(&gpio2->y4, 5, 1);
}

static int mipi_phy_control(int on, u32 reset)
{
	unsigned int addr = EXYNOS4_MIPI_PHY0_CONTROL;
	u32 cfg;

	cfg = readl(addr);
	cfg = on ? (cfg | reset) : (cfg & ~reset);
	writel(cfg, addr);

	if (on)
		cfg |= S5P_MIPI_PHY_ENABLE;
	else if (!(cfg & (S5P_MIPI_PHY_SRESETN |
			    S5P_MIPI_PHY_MRESETN) & ~reset)) {
		cfg &= ~S5P_MIPI_PHY_ENABLE;
	}

	writel(cfg, addr);

	return 0;
}

static struct mipi_dsim_config dsim_config = {
	.e_interface		= DSIM_VIDEO,
	.e_virtual_ch		= DSIM_VIRTUAL_CH_0,
	.e_pixel_format		= DSIM_24BPP_888,
	.e_burst_mode		= DSIM_BURST_SYNC_EVENT,
	.e_no_data_lane		= DSIM_DATA_LANE_4,
	.e_byte_clk		= DSIM_PLL_OUT_DIV8,
	.hfp			= 1,

	.p			= 3,
	.m			= 120,
	.s			= 1,

	/* D-PHY PLL stable time spec :min = 200usec ~ max 400usec */
	.pll_stable_time	= 500,

	/* escape clk : 10MHz */
	.esc_clk		= 20 * 1000000,

	/* stop state holding counter after bta change count 0 ~ 0xfff */
	.stop_holding_cnt	= 0x7ff,
	/* bta timeout 0 ~ 0xff */
	.bta_timeout		= 0xff,
	/* lp rx timeout 0 ~ 0xffff */
	.rx_timeout		= 0xffff,
};

static struct s5p_platform_mipi_dsim s6e8ax0_platform_data = {
	.lcd_panel_info = NULL,
	.dsim_config = &dsim_config,
};

static struct mipi_dsim_lcd_device mipi_lcd_device = {
	.name	= "s6e8ax0",
	.panel_id = "ams465gs01-m3",
	.id	= -1,
	.bus_id	= 0,
	.platform_data	=(void *)&s6e8ax0_platform_data,
};

static int mipi_power(void)
{
	unsigned char addr = MAX8997_PMIC_ADDR;
	unsigned char val[2];

	/* LDO3 VUSB/MIPI: 1.1V */
	val[0] = max8997_reg_ldo(1100000) | 0xC0;
	i2c_write(addr, 0x3d, 1, val, 1);

	/* LDO4 VMIPI: 1.8V */
	val[0] = max8997_reg_ldo(1800000) | 0xC0;
	i2c_write(addr, 0x3e, 1, val, 1);

	return 0;
}

static int lcd_power(void)
{
	unsigned char addr = MAX8997_PMIC_ADDR;
	unsigned char val[2];

	/* LDO15 LCD_VDD_2.2V */
	val[0] = max8997_reg_ldo(2200000) | 0xC0;
	i2c_write(addr, 0x49, 1, val, 1);

	if (hwrevision(1) || hwrevision(2)) {
		/* LDO13 VCC_3.1V_LCD */
		val[0] = max8997_reg_ldo(3100000) | 0xC0;
		i2c_write(addr, 0x47, 1, val, 1);
	} else {
		/* LDO13 VCC_3.3V_LCD */
		val[0] = max8997_reg_ldo(3300000) | 0xC0;
		i2c_write(addr, 0x47, 1, val, 1);
	}

	/* reset lcd */
	gpio_direction_output(&gpio2->y4, 5, 0);
	udelay(10);
	gpio_direction_output(&gpio2->y4, 5, 1);

	return 0;
}

extern void s6e8ax0_init(void);
extern void s5p_set_dsim_platform_data(struct s5p_platform_mipi_dsim *dsim_pd);
extern void ld9040_cfg_ldo(void);
extern void ld9040_enable_ldo(unsigned int onoff);

int s5p_no_lcd_support(void)
{
	return 0;
}

int board_no_lcd_support(void)
{
	return s5p_no_lcd_support();
}

void init_panel_info(vidinfo_t * vid)
{
	vid->board_logo = 1;

	if (hwrevision(1) || hwrevision(2)) {
		vid->vl_freq	= 60;
		vid->vl_col	= 720;
		vid->vl_row	= 1280;
		vid->vl_width	= 720;
		vid->vl_height	= 1280;
		vid->vl_clkp	= CONFIG_SYS_HIGH;
		vid->vl_hsp	= CONFIG_SYS_LOW;
		vid->vl_vsp	= CONFIG_SYS_LOW;
		vid->vl_dp	= CONFIG_SYS_LOW;

		vid->vl_bpix	= 32;
		vid->dual_lcd_enabled = 0;

		/* s6e8ax0 Panel */
		vid->vl_hspw	= 5;
		vid->vl_hbpd	= 10;
		vid->vl_hfpd	= 10;

		vid->vl_vspw	= 2;
		vid->vl_vbpd	= 1;
		vid->vl_vfpd	= 13;
		vid->vl_cmd_allow_len = 0xf;

		vid->cfg_gpio = NULL;
		vid->backlight_on = NULL;
		vid->lcd_power_on = NULL;	/* lcd_power_on in mipi dsi driver */
		vid->reset_lcd = NULL;

		vid->init_delay = 0;
		vid->power_on_delay = 0;
		vid->reset_delay = 0;
		vid->interface_mode = FIMD_RGB_INTERFACE;

		/* board should be detected at here. */

		if (hwrevision(2)) {
			mipi_lcd_device.panel_id = "ams465gs01-sm2";
			mipi_lcd_device.reverse = 1;
		}
		strcpy(s6e8ax0_platform_data.lcd_panel_name, mipi_lcd_device.name);
		s6e8ax0_platform_data.lcd_power = lcd_power;
		s6e8ax0_platform_data.mipi_power = mipi_power;
		s6e8ax0_platform_data.phy_enable = mipi_phy_control;
		s6e8ax0_platform_data.lcd_panel_info = (void *)vid;
		s5p_mipi_dsi_register_lcd_device(&mipi_lcd_device);
		s6e8ax0_init();
		s5p_set_dsim_platform_data(&s6e8ax0_platform_data);

		setenv("lcdinfo", "lcd=s6e8ax0");
	} else {
		vid->vl_freq = 60;
		vid->vl_col = 480;
		vid->vl_row = 800;
		vid->vl_width = 480;
		vid->vl_height = 800;
		vid->vl_clkp = CONFIG_SYS_HIGH;
		vid->vl_hsp = CONFIG_SYS_HIGH;
		vid->vl_vsp = CONFIG_SYS_HIGH;
		vid->vl_dp = CONFIG_SYS_HIGH;

		vid->vl_bpix = 32;
		/* disable dual lcd mode. */
		vid->dual_lcd_enabled = 0;

		/* LD9040 LCD Panel */
		vid->vl_hspw = 2;
		vid->vl_hbpd = 16;
		vid->vl_hfpd = 16;

		vid->vl_vspw = 2;
		vid->vl_vbpd = 6;
		vid->vl_vfpd = 4;

		vid->cfg_gpio = lcd_cfg_gpio;
		vid->backlight_on = NULL;
		vid->lcd_power_on = NULL;	/* Don't need the poweron squence */
		vid->reset_lcd = lcd_reset;

		vid->cfg_ldo = ld9040_cfg_ldo;
		vid->enable_ldo = ld9040_enable_ldo;

		vid->init_delay = 0;
		vid->power_on_delay = 0;
		vid->reset_delay = 10 * 1000;
		vid->interface_mode = FIMD_RGB_INTERFACE;

		/* board should be detected at here. */

		/* for LD8040. */
		vid->pclk_name = MPLL;
		vid->sclk_div = 1;

		setenv("lcdinfo", "lcd=ld9040");
	}
}

#include <mobile/logo_rgb16_hd720_portrait.h>
#include <mobile/logo_rgb16_wvga_portrait.h>
#include <mobile/charging_rgb16_wvga_portrait.h>
#include <mobile/charging_rgb565_hd720_portrait.h>
#include <mobile/download_rgb16_wvga_portrait.h>

logo_info_t logo_info;

static void init_logo_info(void)
{
	/* apply when used wvga image, not hd720 */
	int x_ofs = 0;
	int y_ofs = 0;

	if (hwrevision(1) || hwrevision(2)) {
		logo_info.logo_top.img = logo_top_hd720;
		logo_info.logo_top.x = logo_top_x_hd720;
		logo_info.logo_top.y = logo_top_y_hd720;
		logo_info.logo_bottom.img = logo_bottom_hd720;
		logo_info.logo_bottom.x = logo_bottom_x_hd720;
		logo_info.logo_bottom.y = logo_bottom_y_hd720;

		logo_info.charging.img = charging_animation_loading_hd720;
		logo_info.charging.x = charging_x_hd720;
		logo_info.charging.y = charging_y_hd720;

		x_ofs = (720 - 480) / 2;
		y_ofs = (1280 - 800) / 2;
	} else {
		logo_info.logo_top.img = logo_top;
		logo_info.logo_top.x = 50 + 1;
		logo_info.logo_top.y = 180 + 5;
		logo_info.logo_bottom.img = logo_bottom;
		logo_info.logo_bottom.x = 140 + 1;
		logo_info.logo_bottom.y = 640;

		logo_info.charging.img = battery_charging_animation_loading;
		logo_info.charging.x = 60;
		logo_info.charging.y = 270;
	}

	logo_info.download_logo.img = download_image;
	logo_info.download_logo.x = 205 + x_ofs;
	logo_info.download_logo.y = 272 + y_ofs;
	logo_info.download_text.img = download_text;
	logo_info.download_text.x = 90 + x_ofs;
	logo_info.download_text.y = 360 + y_ofs;

	logo_info.download_fail_logo.img = download_noti_image;
	logo_info.download_fail_logo.x = 196 + x_ofs;
	logo_info.download_fail_logo.y = 270 + y_ofs;
	logo_info.download_fail_text.img = download_fail_text;
	logo_info.download_fail_text.x = 70 + x_ofs;
	logo_info.download_fail_text.y = 370 + y_ofs;

	logo_info.download_bar.img = prog_base;
	logo_info.download_bar.x = 39 + x_ofs;
	logo_info.download_bar.y = 445 + y_ofs;
	logo_info.download_bar_middle.img = prog_middle;
	logo_info.download_bar_width = 4;
	logo_info.rotate = 0;
}
#endif

static unsigned int get_label(void)
{
	int label = 0;

	return label;
}

static unsigned int get_hw_revision(void)
{
	int hwrev = 0;
	int i;

	/* HW_REV[0:3]: GPE1[0:3] */
	for (i = 0; i < 4; i++) {
		gpio_cfg_pin(&gpio1->e1, i, GPIO_INPUT);
		gpio_set_pull(&gpio1->e1, i, GPIO_PULL_NONE);
	}

	udelay(1);

	for (i = 0; i < 4; i++)
		hwrev |= (gpio_get_value(&gpio1->e1, i) << i);

	return hwrev;
}

/*
 * define board_rev (32bit) grobal variable like below.
 * label field is used to distinguish between boards with
 * same machine ID and H/W revision.
 * |	Lable (16bit)	|	H/W rev. (16bit)	|
 */
static void check_hw_revision(void)
{
	unsigned int label = 0;
	unsigned int hwrev = 0;

	label = get_label() & ~REV_MASK;
	hwrev = get_hw_revision() & REV_MASK;

	/* U1HD board rev will forcely set 0x1,
	   although board rev actually is 0x0 */
	if ((hwrev & 0xf) == 0)
		board_rev = (label | hwrev | 0x1);
	else
		board_rev = (label | hwrev);
}

static void show_hw_revision(void)
{
	printf("HW Revision:\t0x%x\n", board_rev);
}

void get_rev_info(char *rev_info)
{
	sprintf(rev_info, "HW Revision: 0x%x\n", board_rev);
}

#ifdef CONFIG_CHARGER_SS6000
void charger_init(void)
{
	printf("Charger init\n");
	gpio_cfg_pin(&gpio2->l2, 2, GPIO_OUTPUT);
	gpio_set_pull(&gpio2->l2, 2, GPIO_PULL_NONE);
	gpio_set_value(&gpio2->l2, 2, 0);

	gpio_cfg_pin(&gpio2->l2, 4, GPIO_INPUT);
	gpio_set_pull(&gpio2->l2, 4, GPIO_PULL_NONE);

	gpio_cfg_pin(&gpio2->l2, 5, GPIO_INPUT);
	gpio_set_pull(&gpio2->l2, 5, GPIO_PULL_NONE);

	gpio_direction_output(&gpio2->l2, 2, 1);
}
#endif

static int fixup_loadkernel_cmd(void)
{
	char buf[128];
	char *fs;
	struct mmc *mmc = find_mmc_device(CONFIG_MMC_DEFAULT_DEV);
	int fstype;

	fstype_register_device(&mmc->block_dev, 2);

	fstype = fstype_check(&mmc->block_dev);
	if (fstype == FS_TYPE_EXT4)
		fs = "ext4load";
	else if (fstype == FS_TYPE_VFAT)
		fs = "fatload";
	else {
		puts("can't found valid partition on /boot\n");
		setenv("kernelv", "fail");
		setenv("bootcmd", "usbdown");
		return 0;
	}

	sprintf(buf, "%s mmc 0:2 0x40007FC0 %s", fs, getenv("kernelname"));
	setenv("loaduimage", buf);
	return 0;
}

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	show_hw_revision();

	init_muic();

	check_keypad();

	check_auto_burn();

	ta_usb_connected = muic_check_type();
#ifdef CONFIG_CMD_PIT
	check_pit();
#endif
	boot_mode = fixup_boot_mode(boot_mode);
	set_boot_mode(boot_mode);
#ifdef CONFIG_LCD
	init_logo_info();
	set_logo_image(boot_mode);
#endif
#ifdef CONFIG_CHARGER_SS6000
	charger_init();
#endif
	init_battery_max17042();

	check_battery();

	into_charge_boot(ta_usb_connected);

	init_rtc_max8997();

#if defined(CONFIG_EXTERNAL_CHARGER)
	disable_charger_max8997();
#endif

	fixup_loadkernel_cmd();

	return 0;
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

int board_mmc_init(bd_t * bis)
{
	int i, err;

	/* eMMC_EN: SD_0_CDn: GPK0[2] Output High */
	gpio_direction_output(&gpio2->k0, 2, 1);
	gpio_set_pull(&gpio2->k0, 2, GPIO_PULL_NONE);

	/*
	 * eMMC GPIO:
	 * SDR 8-bit@48MHz at MMC0
	 * GPK0[0]      SD_0_CLK(2)
	 * GPK0[1]      SD_0_CMD(2)
	 * GPK0[2]      SD_0_CDn        -> Not used
	 * GPK0[3:6]    SD_0_DATA[0:3](2)
	 * GPK1[3:6]    SD_0_DATA[0:3](3)
	 *
	 * DDR 4-bit@26MHz at MMC4
	 * GPK0[0]      SD_4_CLK(3)
	 * GPK0[1]      SD_4_CMD(3)
	 * GPK0[2]      SD_4_CDn        -> Not used
	 * GPK0[3:6]    SD_4_DATA[0:3](3)
	 * GPK1[3:6]    SD_4_DATA[4:7](4)
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

	/*
	 * MMC device init
	 * mmc0  : eMMC (8-bit buswidth)
	 * mmc2  : SD card (4-bit buswidth)
	 */
	err = s5p_mmc_init(0, 8);

	/* T-flash detect */
	gpio_cfg_pin(&gpio2->x3, 4, 0xf);
	gpio_set_pull(&gpio2->x3, 4, GPIO_PULL_UP);

	/*
	 * Check the T-flash  detect pin
	 * GPX3[3] T-flash detect pin
	 */
	if (!gpio_get_value(&gpio2->x3, 4)) {
		/*
		 * SD card GPIO:
		 * GPK2[0]      SD_2_CLK(2)
		 * GPK2[1]      SD_2_CMD(2)
		 * GPK2[2]      SD_2_CDn        -> Not used
		 * GPK2[3:6]    SD_2_DATA[0:3](2)
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

static int do_charge(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	if (argc == 2) {
		if (!strncmp(argv[1], "start", 5))
			pmic_charger_en(500);
		else if (!strncmp(argv[1], "stop", 4))
			pmic_charger_en(0);
		else if (!strncmp(argv[1], "status", 7)) {
			check_battery();
		} else {
			goto charge_cmd_usage;
		}
	} else {
		goto charge_cmd_usage;
	}
	return 0;

 charge_cmd_usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	charge, 2, 1, do_charge,
	"battery charging, voltage/current measurements",
	"status - display battery voltage and current\n"
	"charge start - start charging via USB\n"
	"charge stop - stop charging\n"
);
