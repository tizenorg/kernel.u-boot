/*
 * Copyright (C) 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Sanghee Kim <sh0130.kim@samsung.com>
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
 *
 */

#include <common.h>
#include <usbd.h>
#ifdef CONFIG_LCD
#include <fbutils.h>
#endif
#include <version.h>
#include <timestamp.h>
#ifdef CONFIG_USBD_PROG_IMAGE
#include <bmp_layout.h>
#endif
#include "musb.h"

#define TX_DATA_LEN	5
#define RX_DATA_LEN	1024

/* required for DMA transfers */
__attribute__ ((__aligned__ (__alignof__ (long long))))
static char tx_data[TX_DATA_LEN];
__attribute__ ((__aligned__ (__alignof__ (long long))))
static char rx_data[RX_DATA_LEN * 2];

static int downloading;

extern int musb_recv_done;
extern int musb_connected;
extern struct musb musb;

static int __board_usb_init(void)
{
	return 0;
}

int board_usb_init(void) __attribute__((weak, alias("__board_usb_init")));

extern int check_exit_key(void);

#ifdef CONFIG_GENERIC_MMC
#include <mmc.h>
static char mmc_info[64];

static void usbd_set_mmc_dev(struct usbd_ops *usbd)
{
	struct mmc *mmc;
	char mmc_name[4];

	usbd->mmc_dev = CONFIG_MMC_DEFAULT_DEV;

	mmc = find_mmc_device(usbd->mmc_dev);
	mmc_init(mmc);

	if (!mmc->read_bl_len) {
		usbd->mmc_max = 0;
		usbd->mmc_total = 0;
		return;
	}

	/* FIXME */
	usbd->mmc_max = 0xf000;
	usbd->mmc_blk = mmc->read_bl_len;
	usbd->mmc_total = mmc->capacity / mmc->read_bl_len;

	sprintf(mmc_name, "%c%c%c", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff);

	if (!strncmp(mmc_name, "SEM", 3)) {
		sprintf(mmc_info, "MMC: iNAND %d.%d%s\n",
			(mmc->version >> 4) & 0xf,
			(mmc->version & 0xf) == EXT_CSD_REV_1_5 ?
			(mmc->check_rev? 3 : 41) : (mmc->version & 0xf),
			mmc->check_rev? "+":"");
	} else {
		sprintf(mmc_info, "MMC: MoviNAND %d.%d%s\n",
			(mmc->version >> 4) & 0xf,
			(mmc->version & 0xf) == EXT_CSD_REV_1_5 ?
			(mmc->check_rev? 3 : 41) : (mmc->version & 0xf),
			mmc->check_rev? "+":"");
	}
}
#endif

/* start the usb controller */
static int usb_init(void)
{
	if (board_usb_init()) {
		puts("Failed to board_usb_init\n");
		return 0;
	}

	musb_init();

	printf("USB Start!!\n");

	while (!musb_connected) {
		musb_interrupt(&musb);

		if (check_exit_key())
			return 0;
	}

	puts("Connected!!\n");

	return 1;
}

static void usb_stop(void)
{
#ifdef CONFIG_MMC_ASYNC_WRITE
	/* We must wait until finish the writing */
	struct mmc *mmc;
	mmc = find_mmc_device(0);
	mmc_init(mmc);
#endif
#ifdef CONFIG_LCD
	if (!board_no_lcd_support()) {
		exit_font();
	}
#endif
}

static void down_start(void)
{
	downloading = 1;

#ifdef CONFIG_LCD
#ifndef CONFIG_USBD_PROG_IMAGE
	if (!board_no_lcd_support()) {
		fb_printf("Download Start\n");
		draw_progress(115, 0, FONT_WHITE);
	}
#endif
#endif
}

static void down_cancel(int mode)
{
#ifdef CONFIG_LCD
	if (!board_no_lcd_support()) {
		/* clear fb */
		fb_clear(120);
		exit_font();
	}
#endif
	switch (mode) {
	case END_BOOT:
		run_command("reset", 0);
		break;
	case END_RETRY:
		run_command("usbdown", 0);
		break;
	case END_FAIL:
		run_command("usbdown fail", 0);
		break;
	default:
		break;
	}
}

/* send the packet to host PC */
static void send_packet(char *data, int len)
{
	u8 *addr = (u8 *)data;
	int cur_len = 0;
	int ret_len = 0;

	while (cur_len < len) {
		musb.send_data = addr;
		musb.send_len = (len - cur_len) > 512 ? 512 : (len - cur_len);
		musb.send_flag = 0;

		ret_len = musb_tx(&musb, 1);
		if (ret_len > 0) {
			addr += ret_len;
			cur_len += ret_len;
		}
	}
}

/*
 * receive the packet from host PC
 * return received size
 */
static int recv_packet(void)
{
	while (1) {
		musb_interrupt(&musb);

		if (!musb_connected) {
			puts("Disconnected!!\n");
			return -1;
		}

		if (musb_recv_done) {
			musb_recv_done = 0;
			return musb.recv_len;
		}

		if (!downloading) {
			if (check_exit_key())
				return 0;
		}
	}
}

/* setup the download informations */
static void recv_setup(char *addr, int len)
{
	musb.recv_addr = (u32)addr;
	musb.recv_ptr = (u8 *)addr;
	musb.recv_len = len;
}

#ifdef CONFIG_LCD
#ifdef CONFIG_USBD_PROG_IMAGE
#include <mobile/download_rgb16_wvga_portrait.h>
#define LOGO_X			205
#define LOGO_Y			272
#define LOGO_TEXT_X		90
#define LOGO_TEXT_Y		360

#define FAIL_LOGO_X		196
#define FAIL_LOGO_Y		270
#define FAIL_LOGO_TEXT_X	70
#define FAIL_LOGO_TEXT_Y	370

#define PROG_BG_X		39
#define PROG_BG_Y		445
#define PROG_BAR_W		4

static void draw_progress_bar(int percent)
{
	int i;
	static bmp_image_t *middle = NULL;
	unsigned long len;
	int scale;
	static int scale_bak = 0;

	/* reset progress bar when downloading again */
	if (percent == 1)
		scale_bak = 0;

	if (middle == NULL)
		middle = gunzip_bmp((unsigned long)prog_middle, &len);

	scale = percent * 1; /* bg: 400p, bar: 4p -> 1:1 */

	for (i = scale_bak; i < scale; i++) {
		lcd_display_bitmap(middle,
			PROG_BG_X + 1 + PROG_BAR_W * i, PROG_BG_Y + 1);
	}

	scale_bak = i;

	/* at last bar */
	if (i == (100 / 1))
		scale_bak = 0;
}

static void draw_download_logo(void)
{
	bmp_image_t *bmp;
	unsigned long len;

	lcd_display_clear();

	bmp = gunzip_bmp((unsigned long)download_image, &len);
	lcd_display_bitmap((ulong)bmp, LOGO_X, LOGO_Y);
	free(bmp);

	bmp = gunzip_bmp((unsigned long)download_text, &len);
	lcd_display_bitmap((ulong)bmp, LOGO_TEXT_X, LOGO_TEXT_Y);
	free(bmp);

	bmp = gunzip_bmp((unsigned long)prog_base, &len);
	lcd_display_bitmap((ulong)bmp, PROG_BG_X, PROG_BG_Y);
	free(bmp);
}

void draw_download_fail_logo(void)
{
	bmp_image_t *bmp;
	unsigned long len;

	lcd_display_clear();

	bmp = gunzip_bmp((unsigned long)download_noti_image, &len);
	lcd_display_bitmap((ulong)bmp, FAIL_LOGO_X, FAIL_LOGO_Y);
	free(bmp);

	bmp = gunzip_bmp((unsigned long)download_fail_text, &len);
	lcd_display_bitmap((ulong)bmp, FAIL_LOGO_TEXT_X, FAIL_LOGO_TEXT_Y);
	free(bmp);
}
#endif

static void set_logo(char *app_ver, int fail)
{
#ifdef CONFIG_LCD
	if (!board_no_lcd_support()) {
		char rev_info[64];

		init_font();
		set_font_color(FONT_WHITE);

		if (fail) {
#ifdef CONFIG_USBD_PROG_IMAGE
			draw_download_fail_logo();
#else
			fb_printf("Update is NOT completed\n");
			fb_printf("Retry..\n");
#endif
			return;
		}

#ifdef CONFIG_USBD_PROG_IMAGE
		draw_download_logo();
#endif
		fb_printf(U_BOOT_VERSION);
		fb_printf(" (");
		fb_printf(U_BOOT_DATE);
		fb_printf(")\n");
		get_rev_info(rev_info);
		fb_printf(rev_info);

#ifdef CONFIG_GENERIC_MMC
		fb_printf(mmc_info);
		fb_printf("\n");
#endif
		fb_printf("Downloader v");
		fb_printf(app_ver);
		fb_printf("\nPress the POWERKEY to CANCEL the downloading\n");
	}
#endif
}

static void set_progress(int progress)
{
	if (progress > 100)
		progress = 100;
#ifdef CONFIG_USBD_PROG_IMAGE
	draw_progress_bar(progress);
#else
	draw_progress(115, progress, FONT_WHITE);
#endif
}
#endif

/*
 * This function is interfaced between
 * USB Device Controller and USB Downloader
 */
struct usbd_ops *usbd_set_interface(struct usbd_ops *usbd)
{
	usbd->usb_init = usb_init;
	usbd->usb_stop = usb_stop;
	usbd->send_data = send_packet;
	usbd->recv_data = recv_packet;
	usbd->recv_setup = recv_setup;
	usbd->ram_addr = CONFIG_SYS_DOWN_ADDR;
	usbd->down_cancel = down_cancel;
	usbd->down_start = down_start;
	usbd->cpu_reset = NULL;

	usbd->tx_data = tx_data;
	usbd->rx_data = rx_data;
	usbd->tx_len = TX_DATA_LEN;
	usbd->rx_len = RX_DATA_LEN;
#ifdef CONFIG_LCD
	if (!board_no_lcd_support()) {
		usbd->set_logo = set_logo;
		usbd->set_progress = set_progress;
	}
#endif
#ifdef CONFIG_GENERIC_MMC
	usbd_set_mmc_dev(usbd);
#endif
	downloading = 0;

	return usbd;
}
