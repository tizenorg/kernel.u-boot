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

#ifndef _MOBILE_MISC_H_
#define _MOBILE_MISC_H_

/* reboot */

enum boot_status {
	NORMAL_BOOT,
	CHARGE_BOOT,
	DUMP_REBOOT,
	DUMP_FORCE_REBOOT,
	FOTA_REBOOT,
	FUS_USB_REBOOT,
	FUS_UMS_REBOOT,
	FUS_FAIL_REBOOT,
	HIBFAIL_REBOOT,
	RTL_REBOOT,
	LOCKUP_RESET,
	EXCEPTION_BOOT
};

/* INFORM3
 * bit field: 0x1234567x
 */
#define REBOOT_MODE_PREFIX  0x12345670
#define REBOOT_INTENDED	0
#define REBOOT_FUS		1
#define REBOOT_DUMP		2
#define REBOOT_RECOVERY	4
#define REBOOT_FOTA		5
#define REBOOT_SET_PREFIX   0xabc00000
#define REBOOT_SET_DEBUG    0x000d0000
#define REBOOT_SET_SWSEL    0x000e0000
#define REBOOT_SET_SUD      0x000f0000

/* INFORM2
 * 0x0: enter LP charging mode
 * 0x12345678: Don't enter LPM mode
 */
#define REBOOT_CHARGE	0
#define REBOOT_NPM		8

#define REBOOT_INFO_MAGIC	0x66262564
#define UPLOAD_CAUSE_INIT	0xcafebabe
#define UPLOAD_CAUSE_MASK	0x000000ff
#define UPLOAD_CAUSE_KERNEL_PANIC	0x000000c8
#define UPLOAD_CAUSE_FORCE_UPLOAD	0x00000022


/* logo */

struct img_info {
	const unsigned char *img;
	int x;
	int y;
};

typedef struct logo_info {
	int rotate;
	struct img_info logo_top;
	struct img_info logo_bottom;
	struct img_info charging;
	struct img_info download_logo;
	struct img_info download_text;
	struct img_info download_fail_logo;
	struct img_info download_fail_text;
	struct img_info download_bar;
	struct img_info download_bar_middle;
	int download_bar_width;
} logo_info_t;

extern logo_info_t logo_info;

/* board/.. */
void get_rev_info(char *rev_info);
int board_no_lcd_support(void);
void board_inform_set(unsigned int flag);
void board_inform_clear(unsigned int flag);
void draw_progress_bar(int percent);
void set_download_logo(char *app_ver, char *mmc_ver, int fail);

#endif
