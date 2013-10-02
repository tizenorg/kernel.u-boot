#include <common.h>
#include <usbd.h>
#include <malloc.h>
#include <asm/arch/usb-hs-otg.h>
#include <asm/arch-exynos/power.h>

#define TX_DATA_LEN	128
#define RX_DATA_LEN	1024

/* required for DMA transfers */
__aligned(__alignof__(long long))
static char tx_data[TX_DATA_LEN];
__aligned(__alignof__(long long))
static char rx_data[RX_DATA_LEN * 2];

static struct usbd_ops *usbd_op;

static int downloading;
static int uploading;

int s5p_usb_connected;

void set_usb_connected(int value)
{
	s5p_usb_connected = value;
}

int s5p_receive_done;

void set_s5p_receive_done(int value)
{
	s5p_receive_done = value;
}

otg_dev_t otg;

otg_dev_t *get_otg(void)
{
	return &otg;
}

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

	usbd->mmc_max = 0xf000;
	usbd->mmc_blk = mmc->read_bl_len;
	usbd->mmc_total = mmc->capacity / mmc->read_bl_len;

	sprintf(mmc_name, "%c%c%c", mmc->cid[0] & 0xff,
		(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff);
	sprintf(mmc_info, "MMC: MoviNAND %d,%d\n",
		mmc->version == MMC_VERSION_4_5 ? 4 : 0,
		mmc->version == MMC_VERSION_4_5 ? 5 : 0);

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
	}

	s5p_usb_clear_dnfile_info();

	puts("Connected!!\n");

	return 1;
}

static void usb_stop(void)
{
#ifdef CONFIG_MMC_ASYNC_WRITE
	/* We must wait until finish the writing */
	mmc_init(find_mmc_device(0));
#endif
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
	}
}

/* setup the download informations */
static void recv_setup(char *addr, int len)
{
	s5p_usb_clear_dnfile_info();

	otg.dn_addr = (u32)addr;
	otg.dn_ptr = (u8 *)addr;
	otg.dn_filesize = len;
#ifdef CONFIG_S5P_USB_DMA
	if (otg.op_mode == USB_DMA) {
		if (len > (otg.bulkout_max_pktsize * 1023))
			usbd_op->prepare_dma((void *)addr,
					     (otg.bulkout_max_pktsize * 1023),
					     BULK_OUT_TYPE);
		else
			usbd_op->prepare_dma((void *)addr, len, BULK_OUT_TYPE);
	}
#endif
}

static void upload_start(void)
{
	uploading = 1;
}

static void upload_cancel(int mode)
{
	switch (mode) {
	case END_BOOT:
		run_command("reset", 0);
		break;
	case END_RETRY:
	case END_FAIL:
		break;
	default:
		break;
	}
}

static void down_start(void)
{
	downloading = 1;
}

static void down_cancel(int mode)
{
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
#if CONFIG_SHOW_USBDOWN_LOGO
		usbd->set_logo = set_download_logo;
		usbd->set_progress = draw_progress_bar;
#endif
	}
#ifdef CONFIG_GENERIC_MMC
	usbd_set_mmc_dev(usbd);
#endif
	downloading = 0;

	return usbd;
}
