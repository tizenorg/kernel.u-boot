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
#include <malloc.h>
#include "usb-hs-otg.h"
#include <mobile/misc.h>

#define TX_DATA_LEN	5
#define RX_DATA_LEN	1024

/* required for DMA transfers */
__attribute__ ((__aligned__ (__alignof__ (long long))))
static char tx_data[TX_DATA_LEN];
__attribute__ ((__aligned__ (__alignof__ (long long))))
static char rx_data[RX_DATA_LEN * 2];

static int downloading;
static int uploading;

extern int s5p_receive_done;
extern int s5p_usb_connected;
extern otg_dev_t otg;

static struct usbd_ops *usbd_op;

static int __usb_board_init(void)
{
	return 0;
}

int usb_board_init(void) __attribute__((weak, alias("__usb_board_init")));

extern void s5pc_fimd_lcd_off(unsigned int win_id);
extern void s5pc_fimd_window_off(unsigned int win_id);
extern int check_exit_key(void);

#ifdef CONFIG_GENERIC_MMC
#include <mmc.h>
static char mmc_info[64];

static void usbd_set_mmc_dev(struct usbd_ops *usbd)
{
	struct mmc *mmc;
	char mmc_name[4];

	usbd->mmc_dev = 0;

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
			(mmc->check_rev? 3 : 41) :
			((mmc->version & 0xf) > EXT_CSD_REV_1_5 ?
			 (mmc->version & 0xf) - 1 :
			 (mmc->version & 0xf)),
			mmc->check_rev? "+":"");
	} else {
		sprintf(mmc_info, "MMC: MoviNAND %d.%d%s\n",
			(mmc->version >> 4) & 0xf,
			(mmc->version & 0xf) == EXT_CSD_REV_1_5 ?
			(mmc->check_rev? 3 : 41) :
			((mmc->version & 0xf) > EXT_CSD_REV_1_5 ?
			 (mmc->version & 0xf) - 1 :
			 (mmc->version & 0xf)),
			mmc->check_rev? "+":"");
	}
	usbd->mmc_info = mmc_info;
}
#endif

/* clear download informations */
static void s5p_usb_clear_dnfile_info(void)
{
	otg.dn_addr = 0;
	otg.dn_filesize = 0;
	otg.dn_ptr = 0;
}

/* start the usb controller */
static int usb_init(void)
{
	if (usb_board_init()) {
		puts("Failed to usb_board_init\n");
		return 0;
	}

	s5p_usbctl_init();

	printf("USB Start!! - %s Speed\n",
			otg.speed ? "Full" : "High");

	while (!s5p_usb_connected) {
		if (s5p_usb_detect_irq()) {
			s5p_udc_int_hndlr();
			s5p_usb_clear_irq();
		}

		if (check_exit_key())
			return 0;
	}

	s5p_usb_clear_dnfile_info();

	puts("Connected!!\n");
#ifdef CONFIG_LCD
	if (!board_no_lcd_support()) {
		fb_printf("Cable is connected\n");
	}
#endif

	return 1;
}

static void usb_stop(void)
{
#ifdef CONFIG_MMC_ASYNC_WRITE
	/* We must wait until finish the writing */
	mmc_init(find_mmc_device(0));
#endif
#ifdef CONFIG_LCD
	exit_font();

	/* it uses fb3 as default window. */
	s5pc_fimd_lcd_off(3);
	s5pc_fimd_window_off(3);
#endif
}

static void down_start(void)
{
	downloading = 1;
}

static void down_cancel(int mode)
{
#ifdef CONFIG_LCD
	/* clear fb */
	exit_font();
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

static void upload_start(void)
{
	uploading = 1;
}

static void upload_cancel(int mode)
{
#ifdef CONFIG_LCD
	/* clear fb */
	exit_font();
#endif
	switch (mode) {
	case END_BOOT:
		run_command("reset", 0);
		break;
	case END_RETRY:
	case END_FAIL:
		run_command("ramdump logo", 0);
		run_command("ramdump upload", 0);
		break;
	default:
		break;
	}
}

/*
 * receive the packet from host PC
 * return received size
 */
static int usb_receive_packet(void)
{
	s5p_usb_set_dn_addr(otg.dn_addr, otg.dn_filesize);
	while (1) {
		if (s5p_usb_detect_irq()) {
			s5p_udc_int_hndlr();
			s5p_usb_clear_irq();
		}

		if (!s5p_usb_connected) {
			puts("Disconnected!!\n");
			return -1;
		}

		if (s5p_receive_done) {
			s5p_receive_done = 0;
			return otg.dn_filesize;
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
	s5p_usb_clear_dnfile_info();

	otg.dn_addr = (u32)addr;
	otg.dn_ptr = (u8 *) addr;
	otg.dn_filesize = len;
#ifdef CONFIG_S5P_USB_DMA
	if (otg.op_mode == USB_DMA) {
		if ( len > (otg.bulkout_max_pktsize * 1023) )
			usbd_op->prepare_dma((void *)addr, (otg.bulkout_max_pktsize * 1023), BULK_OUT_TYPE);
		else
			usbd_op->prepare_dma((void *)addr, len, BULK_OUT_TYPE);
	}
#endif
}

/*
 * This function is interfaced between
 * USB Device Controller and USB Downloader
 */
struct usbd_ops *usbd_set_interface(struct usbd_ops *usbd, int mode)
{
	usbd_op = usbd;

	usbd->usb_init = usb_init;
	usbd->usb_stop = usb_stop;
	usbd->send_data = s5p_usb_tx;
	usbd->recv_data = usb_receive_packet;
	usbd->recv_setup = recv_setup;
	usbd->ram_addr = CONFIG_SYS_DOWN_ADDR;
	usbd->cpu_reset = NULL;

	usbd->tx_data = tx_data;
	usbd->rx_data = rx_data;
	usbd->tx_len = TX_DATA_LEN;
	usbd->rx_len = RX_DATA_LEN;
#ifdef CONFIG_S5P_USB_DMA
	usbd->prepare_dma = s5p_usb_setdma;
	usbd->dma_done = s5p_usb_dma_done;
#endif

	if (mode == MODE_UPLOAD) {
		usbd->start = upload_start;
		usbd->cancel = upload_cancel;
		usbd->set_logo = NULL;
		usbd->set_progress = NULL;
	} else {
		usbd->start = down_start;
		usbd->cancel = down_cancel;
		usbd->set_logo = set_download_logo;
		usbd->set_progress = draw_progress_bar;
	}
#ifdef CONFIG_GENERIC_MMC
	usbd_set_mmc_dev(usbd);
#endif
	downloading = 0;

	return usbd;
}
