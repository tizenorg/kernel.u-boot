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
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/power.h>
#include <asm/arch/clk.h>
#include <asm/arch/clock.h>
#include <asm/arch/mipi_dsim.h>
#include <asm/arch/regs-fb.h>
#include <mmc.h>
#include <fat.h>
#include <fbutils.h>
#include <max77686.h>
#include <max77693.h>
#include <mobile/misc.h>
#include <mobile/fs_type_check.h>

DECLARE_GLOBAL_DATA_PTR;

static struct exynos4_gpio_part1 *gpio1;
static struct exynos4_gpio_part2 *gpio2;

static unsigned int board_rev = -1;
static unsigned int board_type = -1;
static int boot_mode = -1;

enum {
	BOARD_M0_PRXM,
	BOARD_M0_REAL,
};

static inline int board_is_m0_prxm(void)
{
	return board_type == BOARD_M0_PRXM;
}

static inline int board_is_m0_real(void)
{
	return board_type == BOARD_M0_REAL;
}

static inline int board_is_m0(void)
{
	return (board_is_m0_prxm() || board_is_m0_real());
}

enum {
	I2C_0, I2C_1, I2C_2, I2C_3,
	I2C_4, I2C_5, I2C_6, I2C_7,
	I2C_8, I2C_9, I2C_10, I2C_NUM,
};

/* i2c7 (MAX77686)	SDA: GPD0[2] SCL: GPD0[3] */
static struct i2c_gpio_bus_data i2c_7 = {
	.sda_pin = 2,
	.scl_pin = 3,
};

/* i2c9 (IF PMIC)	SDA: GPM2[0] SCL: GPM2[1] */
static struct i2c_gpio_bus_data i2c_9 = {
	.sda_pin = 0,
	.scl_pin = 1,
};

/* i2c10 (Fuel Gauge)	SDA: GPF1[5] SCL: GPF1[4] */
static struct i2c_gpio_bus_data i2c_10 = {
	.sda_pin = 5,
	.scl_pin = 4,
};

static struct i2c_gpio_bus i2c_gpio[I2C_NUM];

static const char * const pcb_rev_m0[] = {
	"M0_PROXIMA_REV0.1_1125",
	"unknown",
	"unknown",
	"M0_PROXIMA_REV0.0_1114",
	"unknown",
	"unknown",
	"unknown",
	"M0_REAL_REV0.6_120119",
	"M0_REAL_REV0.6_A",
	"unknown",
	"unknown",
	"M0_REAL_REV1.0_120302",
	"M0_REAL_REV1.1_2nd_120413",
	"unknown",
	"unknown"
};

int check_home_key(void);

static void check_hw_revision(void)
{
	int hwrev = 0;
	int i;

	/* HW_REV[0:3]: GPM1[2:5] */
	for (i = 2; i < 6; i++) {
		gpio_cfg_pin(&gpio2->m1, i, GPIO_INPUT);
		gpio_set_pull(&gpio2->m1, i, GPIO_PULL_NONE);
	}

	udelay(1);

	for (i = 2; i < 6; i++) {
		hwrev |= (gpio_get_value(&gpio2->m1, i) << (i - 2));
		udelay(1);
	}

	board_rev = hwrev;
}

static void check_board_type(void)
{
	/*
	 * BOARD  | F2.4    | F2.7
	 * ---------------------------------------
	 * M0     | NC      | s_led_sda
	 */

	gpio_set_pull(&gpio1->f2, 4, GPIO_PULL_NONE);
	gpio_set_pull(&gpio1->f2, 7, GPIO_PULL_NONE);

	udelay(1);

	if (!gpio_get_value(&gpio1->f2, 4)) {
		if (gpio_get_value(&gpio1->f2, 7)) {
			if (board_rev >= 7)
				board_type = BOARD_M0_REAL;
			else
				board_type = BOARD_M0_PRXM;
		}
	}
}

static void show_hw_revision(void)
{
	printf("HW Revision:\t0x%x\n", board_rev);

	if (board_is_m0())
		printf("PCB Revision:\t%s\n",
			pcb_rev_m0[board_rev & 0xf]);
	else
		printf("PCB Revision:\tunknown\n");
}

u32 get_board_rev(void)
{
	return board_rev;
}

void get_rev_info(char *rev_info)
{
	if (board_is_m0())
		sprintf(rev_info, "HW Revision: 0x%x (%s)\n",
			board_rev, pcb_rev_m0[board_rev & 0xf]);
	else
		sprintf(rev_info, "HW Revision: 0x%x (%s)\n",
			board_rev, "unknown");
}

static void board_external_gpio_init(void)
{
	/*
	 * some pins which in alive block are connected with external pull-up
	 * but it's default setting is pull-down.
	 * if that pin set as input then that floated
	 */

	gpio_set_pull(&gpio2->x0, 2, GPIO_PULL_NONE);	/* PS_ALS_INT */
	gpio_set_pull(&gpio2->x0, 4, GPIO_PULL_NONE);	/* TSP_nINT */
	gpio_set_pull(&gpio2->x0, 7, GPIO_PULL_NONE);	/* AP_PMIC_IRQ */
	gpio_set_pull(&gpio2->x1, 5, GPIO_PULL_NONE);	/* IF_PMIC_IRQ */
	gpio_set_pull(&gpio2->x2, 0, GPIO_PULL_NONE);	/* VOL_UP */
	gpio_set_pull(&gpio2->x2, 1, GPIO_PULL_NONE);	/* VOL_DOWN */
	gpio_set_pull(&gpio2->x2, 3, GPIO_PULL_NONE);	/* FUEL_ALERT */
	gpio_set_pull(&gpio2->x2, 4, GPIO_PULL_NONE);	/* ADC_INT */
	gpio_set_pull(&gpio2->x2, 7, GPIO_PULL_NONE);	/* nPOWER */
	gpio_set_pull(&gpio2->x3, 0, GPIO_PULL_NONE);	/* WPC_INT */
	gpio_set_pull(&gpio2->x3, 5, GPIO_PULL_NONE);	/* OK_KEY */
	gpio_set_pull(&gpio2->x3, 7, GPIO_PULL_NONE);	/* HDMI_HPD */
}

#ifdef CONFIG_SYS_I2C_INIT_BOARD
void i2c_init_board(void)
{
	max77686_bus_init(I2C_7);
	max77693_pmic_bus_init(I2C_9);
	max77693_muic_bus_init(I2C_9);
	max77693_fg_bus_init(I2C_10);

	i2c_gpio[I2C_0].bus = NULL;
	i2c_gpio[I2C_1].bus = NULL;
	i2c_gpio[I2C_2].bus = NULL;
	i2c_gpio[I2C_3].bus = NULL;
	i2c_gpio[I2C_4].bus = NULL;
	i2c_gpio[I2C_5].bus = NULL;
	i2c_gpio[I2C_6].bus = NULL;
	i2c_gpio[I2C_7].bus = &i2c_7;
	i2c_gpio[I2C_8].bus = NULL;
	i2c_gpio[I2C_9].bus = &i2c_9;
	i2c_gpio[I2C_10].bus = &i2c_10;

	i2c_gpio[I2C_7].bus->gpio_base = (unsigned int)&gpio1->d0;
	i2c_gpio[I2C_9].bus->gpio_base = (unsigned int)&gpio2->m2;
	i2c_gpio[I2C_10].bus->gpio_base = (unsigned int)&gpio1->f1;

	i2c_gpio_init(i2c_gpio, I2C_NUM, I2C_9);
}
#endif

int board_early_init_f(void)
{
	gpio1 = (struct exynos4_gpio_part1 *)EXYNOS4_GPIO_PART1_BASE;
	gpio2 = (struct exynos4_gpio_part2 *)EXYNOS4_GPIO_PART2_BASE;

	check_hw_revision();
	check_board_type();
	board_external_gpio_init();

#ifdef CONFIG_OFFICIAL_REL
	if (!check_home_key())
		gd->flags |= GD_FLG_DISABLE_CONSOLE;
#endif

	return 0;
}

int board_init(void)
{
	gpio1 = (struct exynos4_gpio_part1 *)EXYNOS4_GPIO_PART1_BASE;
	gpio2 = (struct exynos4_gpio_part2 *)EXYNOS4_GPIO_PART2_BASE;

	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;
	if (board_is_m0())
		gd->bd->bi_arch_number = MACH_TYPE_SMDK4412 + 1;
	else
		gd->bd->bi_arch_number = MACH_TYPE_SMDK4412;

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
	writel(0, CONFIG_INFO_ADDRESS);
	writel(0, CONFIG_INFO_ADDRESS + 4);
#endif
	return 0;
}

static void init_battery(void);
static void init_pmic(void);

int dram_init(void)
{
	u32 size_mb = exynos_get_dram_size();

	gd->ram_size = size_mb << 20;

	init_pmic();
	return 0;
}

void dram_init_banksize(void)
{
	int i;
	u32 size_mb = exynos_get_dram_size();

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
	gd->bd->bi_dram[2].start = PHYS_SDRAM_3;
	gd->bd->bi_dram[2].size = PHYS_SDRAM_3_SIZE;
	gd->bd->bi_dram[3].start = PHYS_SDRAM_4;
	gd->bd->bi_dram[3].size = PHYS_SDRAM_4_SIZE;
#if defined(CONFIG_TRUSTZONE)
	gd->bd->bi_dram[3].size -= CONFIG_TRUSTZONE_RESERVED_DRAM;
#endif
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

static void check_uboot_backup()
{
	unsigned int magic_base = CONFIG_SYS_SDRAM_BASE + 0x02004000;
	unsigned int count = 0;
	char buf[64];

	run_command("mmc boot 0 1 1 2", 0);
	run_command("mmc read 0 0x42004000 0x0 0x10", 0);
	run_command("mmc boot 0 1 1 0", 0);

	if (readl(magic_base) != 0xea000013) {
		puts("Auto uboot backup start..!!\n");
		count += sprintf(buf + count, "run updatebootb; ");
	}
	if (count) {
		count += sprintf(buf + count, "reset");
		setenv("bootcmd", buf);
	}

	/* Clear the magic value */
	memset((void *)magic_base, 0, 2);
}

int check_exit_key(void)
{
	static int count = 0;

	if (max77686_check_pwrkey())
		count++;

	if (count >= 3) {
		count = 0;
		return 1;
	}

	return 0;
}

int check_volume_up(void)
{
	if (board_is_m0_real())
		return !(gpio_get_value(&gpio2->x2, 2));
	else
		return !(gpio_get_value(&gpio1->j1, 1));
}

int check_volume_down(void)
{
	if (board_is_m0_real())
		return !(gpio_get_value(&gpio2->x3, 3));
	else
		return !(gpio_get_value(&gpio1->j1, 2));
}

int check_home_key(void)
{
	return 1;
}

static void check_keypad(void)
{
	unsigned int power_key;

	power_key = max77686_check_pwron_pwrkey();
	if (power_key) {
		if (check_volume_down())
			setenv("bootcmd", "usbdown");
	}
}

static void print_msg(char *msg)
{
#ifdef CONFIG_LCD
	if (!board_no_lcd_support())
		fb_printf(msg);
#endif
	puts(msg);
}

/*
 * PMIC / MUIC
 */

static unsigned int battery_soc;	/* state of charge in % */
static unsigned int battery_uV;		/* in micro volts */
static int ta_usb_connected;		/* ta: 1, usb: 2 */

static void init_pmic(void)
{
	if (max77686_rtc_init())
		return;

	if (max77686_init())
		return;

	/* BUCK/LDO Output Voltage */
	max77686_set_ldo_voltage(21, 2800000);		/* LDO21 VTF_2.8V */
	max77686_set_ldo_voltage(23, 3300000);		/* LDO23 TSP_AVDD_3.3V */

	if ((board_is_m0() && (board_rev != 0x3)))
		max77686_set_ldo_voltage(24, 1800000);	/* LDO24 TSP_VDD_1.8V */

	/* BUCK/LDO Output Mode */
	max77686_set_buck_mode(1, OPMODE_STANDBY);	/* BUCK1 VMIF_1.1V_AP */
	max77686_set_buck_mode(2, OPMODE_ON);		/* BUCK2 VARM_1.0V_AP */
	max77686_set_buck_mode(3, OPMODE_ON);		/* BUCK3 VINT_1.0V_AP */
	max77686_set_buck_mode(4, OPMODE_ON);		/* BUCK4 VG3D_1.0V_AP */
	max77686_set_buck_mode(5, OPMODE_ON);		/* BUCK5 VMEM_1.2V_AP */
	max77686_set_buck_mode(6, OPMODE_ON);		/* BUCK6 VCC_SUB_1.35V */
	max77686_set_buck_mode(7, OPMODE_ON);		/* BUCK7 VCC_SUB_2.0V */
	max77686_set_buck_mode(8, OPMODE_OFF);		/* BUCK8 VMEM_VDDF_2.85V */
	max77686_set_buck_mode(9, OPMODE_OFF);		/* BUCK9 CAM_ISP_CORE_1.2V */

	max77686_set_ldo_mode(1, OPMODE_LPM);		/* LDO1 VALIVE_1.0V_AP */
	max77686_set_ldo_mode(2, OPMODE_STANDBY);	/* LDO2 VM1M2_1.2V_AP */
	max77686_set_ldo_mode(3, OPMODE_LPM);		/* LDO3 VCC_1.8V_AP */
	max77686_set_ldo_mode(4, OPMODE_LPM);		/* LDO4 VCC_2.8V_AP */

	if (board_is_m0_real())
		max77686_set_ldo_mode(5, OPMODE_OFF);	/* VCC_1.8V_IO has removed from rev06 */
	else
		max77686_set_ldo_mode(5, OPMODE_LPM);	/* LDO5 VCC_1.8V_IO */

	max77686_set_ldo_mode(6, OPMODE_STANDBY);	/* LDO6 VMPLL_1.0V_AP */
	max77686_set_ldo_mode(7, OPMODE_STANDBY);	/* LDO7 VPLL_1.0V_AP */
	max77686_set_ldo_mode(8, OPMODE_LPM);		/* LDO8 VMIPI_1.0V_AP */
	max77686_set_ldo_mode(9, OPMODE_OFF);		/* LDO9 CAM_ISP_MIPI_1.2V */
	max77686_set_ldo_mode(10, OPMODE_LPM);		/* LDO10 VMIPI_1.8V_AP */
	max77686_set_ldo_mode(11, OPMODE_STANDBY);	/* LDO11 VABB1_1.8V_AP */
	max77686_set_ldo_mode(12, OPMODE_LPM);		/* LDO12 VUOTG_3.0V_AP */
	max77686_set_ldo_mode(13, OPMODE_OFF);	/* LDO13 VC2C_1.8V_AP */
	max77686_set_ldo_mode(14, OPMODE_STANDBY);	/* LDO14 VABB02_1.8V_AP */
	max77686_set_ldo_mode(15, OPMODE_STANDBY);	/* LDO15 VHSIC_1.0V_AP */
	max77686_set_ldo_mode(16, OPMODE_STANDBY);	/* LDO16 VHSIC_1.8V_AP */
	max77686_set_ldo_mode(17, OPMODE_OFF);		/* LDO17 CAM_SENSOR_CORE_1.2V */
	max77686_set_ldo_mode(18, OPMODE_OFF);		/* LDO18 CAM_ISP_SEN_IO_1.8V */
	max77686_set_ldo_mode(19, OPMODE_OFF);		/* LDO19 VT_CAM_1.8V */
	max77686_set_ldo_mode(20, OPMODE_ON);		/* LDO20 VDDQ_PRE_1.8V */
	max77686_set_ldo_mode(21, OPMODE_OFF);		/* LDO21 VTF_2.8V */
	max77686_set_ldo_mode(22, OPMODE_OFF);		/* LDO22 VMEM_VDD_2.8V */
	max77686_set_ldo_mode(23, OPMODE_OFF);		/* LDO23 TSP_AVDD_3.3V */
	max77686_set_ldo_mode(24, OPMODE_OFF);		/* LDO24 TSP_VDD_1.8V */
	max77686_set_ldo_mode(25, OPMODE_OFF);		/* LDO25 VCC_3.3V_LCD */
	max77686_set_ldo_mode(26, OPMODE_OFF);		/* LDO26 VCC_3.0V_MOTOR */

	/* 32KHZ: Enable low jitter mode and active P32KH, 32KHZCP,32KHZAP */
	max77686_set_32khz(MAX77686_EN32KHZ_DFLT);

	show_pwron_source(NULL);
}

static void init_muic(void)
{
	max77693_init();
}

int pmic_has_battery(void)
{
	return max77693_charger_detbat();
}

static void init_battery(void)
{
	max77693_fg_init(BATTERY_SDI_2100, ta_usb_connected);
}

static void check_battery(void)
{
	int pwron_acokb;

	max77693_fg_probe();
	battery_soc = max77693_fg_get_soc();
	battery_uV = max77693_fg_get_vcell();

	/* When JIG is connected, it may skip */
	if (battery_uV > 3850000)
		return;

	/* check power-off condition */
	if (ta_usb_connected) {
		if (!pmic_has_battery()) {
			printf("Battery is not detected, power off\n");
			power_off();
		}
	} else {
		if (battery_uV < 3400000 || battery_soc < 1) {
			puts("Please charge the battery, power off.\n");
			power_off();
		}
	}
}

static void check_ta_usb(void)
{
	unsigned char pwron = max77686_get_reg_pwron();

	if (ta_usb_connected)
		return;

	/* check whether ta or usb cable have been attached */
	if (pwron == 0x04) {
		writel(0, EXYNOS4_WDT_BASE);
		max77686_rtc_disable_wtsr_smpl();
		power_off();
	}
}

static int init_charger(void)
{
	if (!pmic_has_battery())
		return -1;

	max77686_clear_irq();

	switch (ta_usb_connected) {
	case CHARGER_TA:
		run_command("max77693 charger start 650", 0);
		break;
	case CHARGER_TA_500:
		run_command("max77693 charger start 500", 0);
		break;
	case CHARGER_UNKNOWN:
		/* for cable which don't officially support, fall through */
	case CHARGER_USB:
		run_command("max77693 charger start 475", 0);
		break;
	case CHARGER_NO:
		run_command("max77693 charger stop", 0);
		break;
	default:
		printf("charger: not supported mode (%d)\n", ta_usb_connected);
		return 1;
	}

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
			gpio_direction_output(&gpio1->f2, 3, 1);
		else
			gpio_direction_output(&gpio1->f2, 3, 0);
	else
		/* USB_SEL (0: IF_TXD/RXD, 1:CP_D+/D-) */
		if (path)
			gpio_direction_output(&gpio1->j0, 1, 0);
		else
			gpio_direction_output(&gpio1->j0, 1, 1);
}

int get_ta_usb_status(void)
{
	return ta_usb_connected;
}

/*
 * LCD
 */

int board_no_lcd_support(void)
{
	return 0;
}

#ifdef CONFIG_LCD
int s5p_no_lcd_support(void)
{
	return 0;
}

void fimd_clk_set(void)
{
	struct exynos4_clock *clk =
		    (struct exynos4_clock *)samsung_get_base_clock();
	unsigned int cfg = 0;

	cfg = readl(EXYNOS4_LCDBLK_CFG);
	cfg |= (1 << 1);
	writel(cfg, EXYNOS4_LCDBLK_CFG);

	/* set lcd src clock */
	cfg = readl(&clk->src_lcd);
	cfg &= ~(0xf);
	cfg |= 0x6;
	writel(cfg, &clk->src_lcd);

	/* set fimd ratio */
	cfg = readl(&clk->div_lcd);
	cfg &= ~(0xf);
	cfg |= 0x1;
	writel(cfg, &clk->div_lcd);
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

static struct s5p_platform_mipi_dsim dsim_platform_data = {
	.lcd_panel_info = NULL,
	.dsim_config = &dsim_config,
};

static struct mipi_dsim_lcd_device mipi_lcd_device = {
	.name	= "s6e8ax0",
	.panel_id = "ams480gyxx-sm2",
	.id	= -1,
	.bus_id	= 0,
	.platform_data	=(void *)&dsim_platform_data,
};

static int mipi_power(void)
{
	/* LDO8 VMIPI_1.0V_AP */
	max77686_set_ldo_mode(8, OPMODE_ON);
	/* LDO10 VMIPI_1.8V_AP */
	max77686_set_ldo_mode(10, OPMODE_ON);
	return 0;
}

static int lcd_power(void)
{
	if (board_is_m0() && (board_rev != 0x3)) {
		/* LCD_2.2V_EN: GPC0[1] */
		gpio_set_pull(&gpio1->c0, 1, GPIO_PULL_UP);
		gpio_direction_output(&gpio1->c0, 1, 1);
	} else {
		/* LDO24 LCD_VDD_2.2V */
		max77686_set_ldo_voltage(24, 2200000);
		max77686_set_ldo_mode(24, OPMODE_LPM);
	}

	/* LDO25 VCC_3.1V_LCD */
	max77686_set_ldo_voltage(25, 3100000);
	max77686_set_ldo_mode(25, OPMODE_LPM);

	return 0;
}

static int lcd_reset(void)
{
	/* reset lcd */
	gpio_direction_output(&gpio1->f2, 1, 0);
	udelay(10);
	gpio_set_value(&gpio1->f2, 1, 1);

	return 0;
}
extern void s6e8ax0_init(void);
extern void s5p_set_dsim_platform_data(struct s5p_platform_mipi_dsim *dsim_pd);

void init_panel_info(vidinfo_t *vid)
{
	vid->board_logo = 1;

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
	vid->lcd_power_on = lcd_power;	/* lcd_power_on in mipi dsi driver */
	vid->reset_lcd = lcd_reset;

	vid->init_delay = 0;
	vid->power_on_delay = 25;
	vid->reset_delay = 0;
	vid->interface_mode = FIMD_RGB_INTERFACE;

	strcpy(dsim_platform_data.lcd_panel_name, mipi_lcd_device.name);
	dsim_platform_data.lcd_power = lcd_power;
	dsim_platform_data.mipi_power = mipi_power;
	dsim_platform_data.phy_enable = mipi_phy_control;
	dsim_platform_data.lcd_panel_info = (void *)vid;
	s5p_mipi_dsi_register_lcd_device(&mipi_lcd_device);

	s6e8ax0_init();

	s5p_set_dsim_platform_data(&dsim_platform_data);

	setenv("lcdinfo", "lcd=s6e8ax0");
}

#include <mobile/logo_rgb16_hd720_portrait.h>
#include <mobile/charging_rgb565_hd720_portrait.h>
#include <mobile/download_rgb16_wvga_portrait.h>

logo_info_t logo_info;

static void init_logo_info(void)
{
	int x_ofs = (720 - 480) / 2;
	int y_ofs = (1280 - 800) / 2;

	logo_info.logo_top.img = logo_top_hd720;
	logo_info.logo_top.x = logo_top_x_hd720;
	logo_info.logo_top.y = logo_top_y_hd720;
	logo_info.logo_bottom.img = logo_bottom_hd720;
	logo_info.logo_bottom.x = logo_bottom_x_hd720;
	logo_info.logo_bottom.y = logo_bottom_y_hd720;

	logo_info.charging.img = charging_animation_loading_hd720;
	logo_info.charging.x = charging_x_hd720;
	logo_info.charging.y = charging_y_hd720;

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

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	setenv("model", "GT-I8800");
	if (board_is_m0_prxm())
		setenv("board", "M0_PROXIMA");
	else if (board_is_m0_real())
		setenv("board", "M0_REAL");
	else
		setenv("board", "unknown");

	show_hw_revision();

	init_muic();

	check_keypad();

	check_auto_burn();

	ta_usb_connected = max77693_muic_check();
	check_ta_usb();

	init_battery();

	init_charger();

	check_battery();

#ifdef CONFIG_CMD_PIT
	check_pit();
#endif
	boot_mode = fixup_boot_mode(boot_mode);
	set_boot_mode(boot_mode);
#ifdef CONFIG_LCD
	init_logo_info();
	set_logo_image(boot_mode);
#endif

	return 0;
}
#endif

#ifdef CONFIG_CMD_USBDOWN
int usb_board_init(void)
{
	max77686_clear_irq();

#ifdef CONFIG_CMD_PMIC
	run_command("max77686 ldo 12 on", 0);
	run_command("max77693 safeout 1 on", 0);
#endif
	return 0;
}
#endif

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

static int do_muic(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	char cmd[64];
	int count;
	int i;

	/*
	 * muic command are used in others (e.g., fixup_uart_path)
	 * but pegasusq boards use max77686 as muic
	 * so it need to reconnect to muic
	 */
	count = sprintf(cmd, "max77693");
	for (i = 1; i < argc; i++)
		count += sprintf(cmd + count, " %s", argv[i]);

	run_command(cmd, 0);
}

U_BOOT_CMD(
	muic, 4, 1, do_muic,
	"run the max77686 cmd",
	""
);
