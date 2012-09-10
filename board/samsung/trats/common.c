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
#include <malloc.h>
#include <asm/io.h>
#include <exports.h>
#include <mmc.h>
#include <lcd.h>
#include <pmic.h>
#include <ramoops.h>
#include <bmp_layout.h>
#include <asm/arch/power.h>
#include <mobile/pit.h>
#include <mobile/misc.h>
#include <mobile/fota.h>
#include <version.h>
#include <timestamp.h>
#ifdef CONFIG_LCD
#include <lcd.h>
#include <fbutils.h>
#endif

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	struct mmc *mmc = find_mmc_device(CONFIG_MMC_DEFAULT_DEV);

	if (mmc == NULL) {
		printf("cann't set serial number, cause no mmc.\n");
		serialnr->high = 0;
		serialnr->low = 0;
	} else {
		/* make unique ID using MMC's CID, it's unique */
		serialnr->high = mmc->cid[2];
		serialnr->low = mmc->cid[3];
	}
}
#endif

#ifdef CONFIG_CMD_PIT
extern struct pitpart_data pitparts[];
void check_pit(void)
{
	char buf[64];

	sprintf(buf, "pit read %x", CONFIG_PIT_DOWN_ADDR);
	run_command(buf, 0);

	sprintf(buf, "pit update %x", CONFIG_PIT_DOWN_ADDR);
	if (run_command(buf, 0) <= 0) {
		puts("use default partition\n");
		return;
	}
}
#endif

/* defined at driver/mobile/rb_dump.c */
#define _MAGIC (('G' << 24) | ('L' << 16) | ('O' << 8) | ('G'))

static int check_boot_mode(unsigned int inform)
{
	int boot_mode = -1;
	unsigned int upmode = 0;

#if defined(CONFIG_CMD_RAMDUMP)
	if (readl(CONFIG_SYS_SDRAM_BASE) == REBOOT_INFO_MAGIC) {
		upmode = inform & UPLOAD_CAUSE_MASK;
		if (upmode == UPLOAD_CAUSE_KERNEL_PANIC) {
			puts("(ram dump)\n");
			boot_mode = DUMP_REBOOT;
			return boot_mode;
		} else if (upmode == UPLOAD_CAUSE_FORCE_UPLOAD) {
			puts("(ram dump - force)\n");
			boot_mode = DUMP_FORCE_REBOOT;
			return boot_mode;
		} else
			printf("(unknown upload mode: 0x%x)\n", upmode);

		writel(0, CONFIG_SYS_SDRAM_BASE);
	}
#endif
	inform &= ~(REBOOT_MODE_PREFIX);
	if (inform == REBOOT_DUMP) {
		puts("(ram dump)\n");
		boot_mode = DUMP_REBOOT;
	} else if (inform == REBOOT_FUS) {
		puts("(fus reboot)\n");
		boot_mode = FUS_USB_REBOOT;
	} else if (inform == REBOOT_FOTA) {
		puts("(fota reboot)\n");
		boot_mode = FOTA_REBOOT;
	} else if (inform == REBOOT_INTENDED) {
		puts("(intended reboot)\n");
		boot_mode = NORMAL_BOOT;
	} else {
		printf("(unknown: 0x%x) - normal\n", inform);
		boot_mode = NORMAL_BOOT;
	}

	return boot_mode;
}

#define STR_CHARGER_DETECT	"charger_detect_boot"
#define STR_FOTA_UPDATE		"fota_update_boot"
#define STR_FUS_UPDATE		"fus_update_boot"
#define STR_HIBFAIL		"hibfail_boot"

static const char *reset_reason[] = {
	"Pin(Ext) Reset ",
	"Warm Reset",
	"Watchdog Reset",
	"S/W Reset"
};

extern int pmic_has_battery(void);
extern int get_ta_usb_status(void);

int get_boot_mode(void)
{
	int boot_mode = -1;
	int status = get_reset_status();
	unsigned int inform = readl(CONFIG_INFORM_ADDRESS);

	/* check inform */
	printf("Inform:\t\t0x%08x\n", inform);

	printf("Reset Status:\t");

	switch (status) {
	case EXTRESET:
		printf("%s\n", reset_reason[status]);
		boot_mode = NORMAL_BOOT;
		break;
	case WARMRESET:
	case WDTRESET:
	case SWRESET:
		puts(reset_reason[status]);
		boot_mode = check_boot_mode(inform);
		break;
	}

	return boot_mode;
}

#ifndef CONFIG_LPM_INFORM
#define CONFIG_LPM_INFORM 0x10020808      /* INFORM2 */
#endif

int fixup_boot_mode(int mode)
{
	int ret = 1;
	char *env;
	unsigned int lpm_inform = readl(CONFIG_LPM_INFORM);

	/* check update mode */
	env = getenv("updatestate");
	if (env && !strcmp(env, "going"))
		return FUS_FAIL_REBOOT;

	/* check params flag */
	printf("Change State:\t");

	/* check download mode */
	env = getenv("bootcmd");
	if (env && strstr(env, "usbdown")) {
		puts("download\n");
		return FUS_USB_REBOOT;
	}

	if (get_reset_status() == EXTRESET) {
		if (get_ta_usb_status()) {
			puts("charging ");
			mode = CHARGE_BOOT;
		}
	}
	else if ((lpm_inform == REBOOT_CHARGE) &&
			(readl(CONFIG_SYS_SDRAM_BASE) != REBOOT_INFO_MAGIC)) {
		puts("charge reboot\n");
		mode = CHARGE_BOOT;
	}
	/* check battery */
	if (mode == CHARGE_BOOT) {
		if (!get_ta_usb_status()) {
			puts("- no charger ");
			mode = NORMAL_BOOT;
		}
		if (!pmic_has_battery()) {
			puts("- no battery ");
			mode = NORMAL_BOOT;
		}
	}

	switch (mode) {
	case NORMAL_BOOT:
	case CHARGE_BOOT:
		env = getenv("SLP_FLAG_FUS");
		if (env && !strcmp(env, "2")) {
			puts("fus reboot - ums\n");
			ret = setenv("SLP_FLAG_FUS", "0");
			mode = FUS_UMS_REBOOT;
		} else if (env && !strcmp(env, "1")) {
			puts("fus reboot - usb\n");
			ret = setenv("SLP_FLAG_FUS", "0");
			mode = FUS_USB_REBOOT;
		}
		puts("maintain\n");
		break;
	default:
		puts("maintain\n");
	}

	if (ret == 0)
		saveenv();

	if (mode == CHARGE_BOOT) {
		/* When charging, DO NOT boot with hibernation snapshot */
		setenv("opts", "noresume");
	} else {
		board_inform_clear(0);
	}

	printf("Inform:\t\t0x%08x \n", readl(CONFIG_INFORM_ADDRESS));

	return mode;
}

void set_boot_mode(int mode)
{
	char buf[32];

	switch (mode) {
	case CHARGE_BOOT:
		setenv("bootmode", STR_CHARGER_DETECT);
		puts("AST_CHARGING..\n");
		break;
	case FOTA_REBOOT:
		setenv("bootmode", STR_FOTA_UPDATE);
		break;
	case FUS_USB_REBOOT:
		setenv("bootmode", STR_FUS_UPDATE);
		setenv("bootcmd", "usbdown");
		puts("AST_DLOAD..\n");
		break;
	case FUS_UMS_REBOOT:
		setenv("bootmode", STR_FUS_UPDATE);
		erase_given_area(PARTS_QBOOT, QBOOT_ERASE_SIZE);
		break;
	case FUS_FAIL_REBOOT:
		setenv("bootmode", "fus_fail");
		setenv("bootcmd", "usbdown fail");
		puts("AST_DLOAD..\n");
		break;
	case HIBFAIL_REBOOT:
		setenv("bootmode", STR_HIBFAIL);
		/* for debugging on development */
		setenv("bootdelay", "-1");
		break;
#if defined(CONFIG_CMD_RAMDUMP)
	case LOCKUP_RESET:
		setenv("bootcmd", "ramdump start");
		setenv("bootmode", "dump_lockup");
		puts("AST_UPLOAD..\n");
		break;
	case DUMP_REBOOT:
		setenv("bootcmd", "ramdump start");
		setenv("bootmode", "dump_panic");
		puts("AST_UPLOAD..\n");
		break;
	case DUMP_FORCE_REBOOT:
		setenv("bootcmd", "ramdump start");
		setenv("bootmode", "dump_force");
		puts("AST_UPLOAD..\n");
		break;
#endif
	case RTL_REBOOT:
		setenv("bootmode", "rtl");
		break;
	case NORMAL_BOOT:
		setenv("bootmode", "normal");
		break;
	default:
		sprintf(buf, "unknown(%d)", mode);
		setenv("bootmode", buf);
		break;
	}
}

static void draw_booting_logo(void)
{
#ifdef CONFIG_CMD_BMP
	int x, y;
	bmp_image_t *bmp;
	unsigned long len;

	/* top image */
	x = logo_info.logo_top.x;
	y = logo_info.logo_top.y;

	bmp = gunzip_bmp((ulong)logo_info.logo_top.img, &len);
	lcd_display_bitmap((ulong)bmp, x, y);
	free(bmp);

	/* bottom image */
	if (logo_info.logo_bottom.img == NULL)
		return;

	x = logo_info.logo_bottom.x;
	y = logo_info.logo_bottom.y;

	bmp = gunzip_bmp((ulong)logo_info.logo_bottom.img, &len);
	lcd_display_bitmap((ulong)bmp, x, y);
	free(bmp);
#endif
}

static void draw_charging_logo(void)
{
#ifdef CONFIG_CMD_BMP
	int x, y;
	bmp_image_t *bmp;
	unsigned long len;

	x = logo_info.charging.x;
	y = logo_info.charging.y;

	bmp = gunzip_bmp((ulong)logo_info.charging.img, &len);
	lcd_display_bitmap((ulong) bmp, x, y);
	free(bmp);
#endif
}

static void draw_download_logo(void)
{
	int x, y;
	bmp_image_t *bmp;
	unsigned long len;

	lcd_display_clear();

	x = logo_info.download_logo.x;
	y = logo_info.download_logo.y;

	bmp = gunzip_bmp((unsigned long)logo_info.download_logo.img, &len);
	lcd_display_bitmap((ulong)bmp, x, y);
	free(bmp);

	x = logo_info.download_text.x;
	y = logo_info.download_text.y;

	bmp = gunzip_bmp((unsigned long)logo_info.download_text.img, &len);
	lcd_display_bitmap((ulong)bmp, x, y);
	free(bmp);

	x = logo_info.download_bar.x;
	y = logo_info.download_bar.y;

	bmp = gunzip_bmp((unsigned long)logo_info.download_bar.img, &len);
	lcd_display_bitmap((ulong)bmp, x, y);
	free(bmp);
}

static void draw_download_fail_logo(void)
{
	int x, y;
	bmp_image_t *bmp;
	unsigned long len;

	lcd_display_clear();

	x = logo_info.download_fail_logo.x;
	y = logo_info.download_fail_logo.y;

	bmp = gunzip_bmp((unsigned long)logo_info.download_fail_logo.img, &len);
	lcd_display_bitmap((ulong)bmp, x, y);
	free(bmp);

	x = logo_info.download_fail_text.x;
	y = logo_info.download_fail_text.y;

	bmp = gunzip_bmp((unsigned long)logo_info.download_fail_text.img, &len);
	lcd_display_bitmap((ulong)bmp, x, y);
	free(bmp);
}

void set_download_logo(char *app_ver, char *mmc_ver, int fail)
{
	char buf[64];
	int pit_ver = pit_get_version();
	char *env;

	init_font();
	set_font_color(FONT_WHITE);

	if (fail) {
		draw_download_fail_logo();
		return;
	}

	draw_download_logo();

	fb_printf(U_BOOT_VERSION);
	fb_printf(" (");
	fb_printf(U_BOOT_DATE);
	fb_printf(")\n");
	get_rev_info(buf);
	fb_printf(buf);

	sprintf(buf, "PIT Version: %02d\n", pit_get_version());
	fb_printf(buf);

	env = getenv("kernelv");
	if (env && !strncmp(env, "fail", 4)) {
		set_font_color(FONT_RED);
		fb_printf("\nPIT successfully updated!\nDownload AGAIN Offcial Version!\n");
		set_font_color(FONT_WHITE);
	}

	fb_printf("\nPlease Press POWERKEY 3 times to CANCEL downloading\n");
}

void draw_progress_bar(int percent)
{
	int i;
	int x, y;
	static bmp_image_t *middle = NULL;
	unsigned long len;
	int scale;
	static int scale_bak = 0;

	/* limit range */
	if (percent > 100)
		percent = 100;
	/* reset progress bar when downloading again */
	if (percent == 1)
		scale_bak = 0;

	if (middle == NULL)
		middle = gunzip_bmp((unsigned long)logo_info.download_bar_middle.img, &len);

	x = logo_info.download_bar.x;
	y = logo_info.download_bar.y;
	scale = percent * 1; /* bg: 400p, bar: 4p -> 1:1 */

	for (i = scale_bak; i < scale; i++) {
		if (logo_info.rotate)
			lcd_display_bitmap((ulong)middle,
				x + 1,
				y + 1 + logo_info.download_bar_width * i);
		else
			lcd_display_bitmap((ulong)middle,
				x + 1 + logo_info.download_bar_width * i,
				y + 1);
	}

	scale_bak = i;

	/* at last bar */
	if (i == (100 / 1))
		scale_bak = 0;
}

void set_logo_image(int mode)
{
	if (board_no_lcd_support())
		return;

	switch (mode) {
	case HIBFAIL_REBOOT:
		/* for debugging on development */
#if defined(CONFIG_CMD_PIT)
		/* erase_given_area(PARTS_QBOOT, QBOOT_ERASE_SIZE); */
#endif
		/* fall through */
	case FOTA_REBOOT:
	case RTL_REBOOT:
	case NORMAL_BOOT:
		draw_booting_logo();
		break;
	case CHARGE_BOOT:
		draw_charging_logo();
		break;
	case FUS_USB_REBOOT:
	case FUS_UMS_REBOOT:
	case FUS_FAIL_REBOOT:
		/* draw logo at download module */
		break;
#if defined(CONFIG_CMD_RAMDUMP)
	case LOCKUP_RESET:
	case DUMP_REBOOT:
	case DUMP_FORCE_REBOOT:
		/* draw logo at ramdump module */
		break;
#endif
	default:
		printf("not supported logo (%d)\n", mode);
	}
}
