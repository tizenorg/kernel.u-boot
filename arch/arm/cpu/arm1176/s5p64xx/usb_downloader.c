/*
 * Copyright (C) 2009 Samsung Electronics
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
 *
 */

#include <common.h>
#include "usbd.h"
#include "usb-hs-otg.h"
#ifdef CONFIG_S5PC1XXFB
#include <fbutils.h>
#endif

static char tx_data[8] = "MPL";
static long tx_len = 4;

static char rx_data[2048];
static long rx_len = 64;

extern int s5p_receive_done;
extern int s5p_usb_connected;
extern otg_dev_t otg;

static int __usb_board_init(void)
{
	return 0;
}

int usb_board_init(void) __attribute__((weak, alias("__usb_board_init")));

/* clear download informations */
static void s5p_usb_clear_dnfile_info(void)
{
	otg.dn_addr = 0;
	otg.dn_filesize = 0;
	otg.dn_ptr = 0;
}

/* clear upload informations */
static void s5p_usb_clear_upfile_info(void)
{
	otg.up_addr = 0;
	otg.up_size = 0;
	otg.up_ptr = 0;
}

/* start the usb controller */
static void usb_init(void)
{
	if (usb_board_init()) {
		printf("Failed to usb_board_init\n");
		return;
	}

#ifdef CONFIG_S5PC1XXFB
	init_font();
	set_font_color(FONT_WHITE);
	fb_printf("Ready to USB Connection\n");
#endif

	s5p_usbctl_init();
	s5p_usbc_activate();

	printf("USB Start!! - %s Speed\n",
			otg.speed ? "Full" : "High");

	while (!s5p_usb_connected) {
		if (s5p_usb_detect_irq()) {
			s5p_udc_int_hndlr();
			s5p_usb_clear_irq();
		}
	}

	s5p_usb_clear_dnfile_info();

	printf("Connected!!\n");

#ifdef CONFIG_S5PC1XXFB
	fb_printf("Download Start\n");
	draw_progress(40, 0, FONT_WHITE);
#endif
}

static void usb_stop(void)
{
	s5p_usb_stop();
#ifdef CONFIG_S5PC1XXFB
	exit_font();

	/* it uses fb3 as default window. */
	s5pc_fimd_lcd_off(3);
	s5pc_fimd_window_off(3);
#endif
}

/*
 * receive the packet from host PC
 * return received size
 */
static int usb_receive_packet(void)
{
	while (1) {
		if (s5p_usb_detect_irq()) {
			s5p_udc_int_hndlr();
			s5p_usb_clear_irq();
		}

		if (s5p_receive_done) {
			s5p_receive_done = 0;
			return otg.dn_filesize;
		}
	}
}

/* setup the download informations */
static void recv_setup(char *addr, int len)
{
	s5p_usb_clear_dnfile_info();

	otg.dn_addr = (u32)addr;
	otg.dn_ptr = (u8 *) addr;
	otg.dn_filesize = len;
}

#ifdef CONFIG_GENERIC_MMC
#include <mmc.h>

static void usbd_set_mmc_dev(struct usbd_ops *usbd)
{
	struct mmc *mmc;

	usbd->mmc_dev = 0;
	/* FIX 0x400 */
	usbd->mmc_max = 0x400;
	/* get from mmc->capacity?? */
	usbd->mmc_total = 0xf50000;	/* 8GB / 0x200  */

	mmc = find_mmc_device(usbd->mmc_dev);
	mmc_init(mmc);

	usbd->mmc_blk = mmc->read_bl_len;
}
#endif

#ifdef CONFIG_S5PC1XXFB
static void set_progress(int progress)
{
	draw_progress(40, progress, FONT_WHITE);
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
	usbd->send_data = s5p_usb_tx;
	usbd->recv_data = usb_receive_packet;
	usbd->recv_setup = recv_setup;
	usbd->tx_data = tx_data;
	usbd->rx_data = rx_data;
	usbd->tx_len = tx_len;
	usbd->rx_len = rx_len;
	usbd->ram_addr = CONFIG_SYS_DOWN_ADDR;
#ifdef CONFIG_S5PC1XXFB
	usbd->set_progress = set_progress;
#endif
#ifdef CONFIG_GENERIC_MMC
	usbd_set_mmc_dev(usbd);
#endif

	return usbd;
}
