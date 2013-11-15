/*
 * (C) Copyright 2012 Samsung Electronics
 * Donghwa Lee <dh09.lee@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <lcd.h>
#include <fbutils.h>
#include <malloc.h>
#include <pit.h>
#include <lcd.h>
#include <version.h>
#include <libtizen.h>

#include "tizen_hd_logo.h"
#include "tizen_hd_logo_data.h"
#include "download_rgb16_wvga_portrait.h"

#define LOGO_X_OFS	((720 - 480) / 2)
#define LOGO_Y_OFS	((1280 - 800) / 2)
#define PROGRESS_BAR_WIDTH	4

void get_tizen_logo_info(vidinfo_t *vid)
{
	switch (vid->resolution) {
	case HD_RESOLUTION:
		vid->logo_width = TIZEN_HD_LOGO_WIDTH;
		vid->logo_height = TIZEN_HD_LOGO_HEIGHT;
		vid->logo_addr = (ulong)tizen_hd_logo;
		break;
	default:
		break;
	}
}

void draw_download_fail_logo(void)
{
	int x, y;

	lcd_clear();

	x = 196 + LOGO_X_OFS;
	y = 280 + LOGO_Y_OFS;

	bmp_display((unsigned long)download_noti_image, x, y);

	x = 70 + LOGO_X_OFS;
	y = 370 + LOGO_Y_OFS;

	bmp_display((unsigned long)download_fail_text, x, y);
}

void draw_download_logo(void)
{
	int x, y;

	lcd_clear();

	x = 205 + LOGO_X_OFS;
	y = 272 + LOGO_Y_OFS;

	bmp_display((unsigned long)download_image, x, y);

	x = 90 + LOGO_X_OFS;
	y = 360 + LOGO_Y_OFS;

	bmp_display((unsigned long)download_text, x, y);

	x = 39 + LOGO_X_OFS;
	y = 445 + LOGO_Y_OFS;

	bmp_display((unsigned long)prog_base, x, y);
}

void set_download_logo(int fail)
{
	char buf[64];
	char *env;

	init_font();
	set_font_color(FONT_WHITE);

	if (fail) {
		draw_download_fail_logo();
		return;
	}

	draw_download_logo();

	sprintf(buf, "%s (%s)\n", U_BOOT_VERSION, U_BOOT_DATE);
	fb_printf(buf);

	sprintf(buf, "PIT Version: %02d\n", pit_get_version());
	fb_printf(buf);

	env = getenv("kernelv");
	if (env && !strncmp(env, "fail", 4)) {
		set_font_color(FONT_RED);
		fb_printf(
		 "\nPIT successfully updated!\nDownload AGAIN Offcial Version!\n");
		set_font_color(FONT_WHITE);
	}

	fb_printf("\nPlease Press POWERKEY 3 times to CANCEL downloading\n");
}

void draw_progress(
		unsigned long long int total_file_size,
		unsigned long long int downloaded_file_size)
{
	int i;
	int x, y;
	unsigned long len;
	int scale;
	static int scale_bak;
	static bmp_image_t *progress_bar_bmp;
	void *alloc_addr = NULL;
	static int percent_1;
	static int progress_percent;
	int tmp_percent = 0;

	if (total_file_size == 0)
		return;

	if (percent_1 == 0)
		percent_1 = total_file_size / 100;

	/* because of performance issue,
	   division must be implemented using subtraction */
	for (i = 0; i < 100; i++) {
		downloaded_file_size -= percent_1;
		tmp_percent++;
		if (downloaded_file_size <= 0)
			break;
	}

	if (progress_percent == tmp_percent)
		return;

	progress_percent = tmp_percent;

	/* reset progress bar when downloading again */
	if (progress_percent == 1)
		scale_bak = 0;

	x = 39 + LOGO_X_OFS;
	y = 445 + LOGO_Y_OFS;
	scale = progress_percent * 1; /* bg: 400p, bar: 4p -> 1:1 */

	if (progress_bar_bmp == NULL)
		progress_bar_bmp = gunzip_bmp(
				(unsigned long)prog_middle,
				&len, &alloc_addr);

	for (i = scale_bak; i < scale; i++) {
		lcd_display_bitmap((unsigned long)progress_bar_bmp,
				   x + 1 + PROGRESS_BAR_WIDTH * i,
				   y + 1);
	}

	scale_bak = i;

	/* at last bar */
	if (i == (100 / 1))
		scale_bak = 0;
}
