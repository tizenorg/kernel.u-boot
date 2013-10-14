#include <common.h>
#include <usbd.h>
#include <mmc.h>

#define DEBUG(level, fmt, args...) \
	if (level < debug_level) \
		printf(fmt, ##args);
#define ERROR(fmt, args...) \
	printf("error: " fmt, ##args);

static int debug_level = 2;
static int check_speed;

static int down_mode = MODE_NORMAL;

static u32 download_addr;

static struct usbd_ops usbd_ops;
static struct mmc *mmc;

struct usbd_file_info finfo;

static int thor_handshake(struct usbd_ops *usbd)
{
	int ret;
	static const char dl_key[] = "THOR";
	static const char dl_ack[] = "ROHT";

	/* setup rx buffer and transfer length */
	usbd->recv_setup(usbd->rx_data, strlen(dl_key));
	/* detect the download request from HOST PC */
	ret = usbd->recv_data();

	if (ret > 0) {
		/* recv request from HOST PC */
		if (!strncmp(usbd->rx_data, dl_key, strlen(dl_key))) {
			DEBUG(1, "Download request from the Host PC\n");

			/* send ack to HOST PC */
			strcpy(usbd->tx_data, dl_ack);
			usbd->send_data(usbd->tx_data, strlen(dl_ack));
		} else {
			DEBUG(1, "Invalid request from the HOST PC!\n");
			DEBUG(1, "(len=%lu)\n", usbd->rx_len);
			usbd->cancel(END_RETRY);

			return -1;
		}
	} else if (ret < 0) {
		usbd->cancel(END_RETRY);
		return -1;
	} else {
		usbd->cancel(END_BOOT);
		return -1;
	}

	return 0;
}

int do_usbd_down(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct usbd_ops *usbd;
	char *p;
	int ret;

	/* check debug level */
	p = getenv("usbdebug");
	if (p) {
		debug_level = simple_strtoul(p, NULL, 10);
		DEBUG(0, "debug level %d\n", debug_level);
	}

	/* download speed check option */
	if (debug_level >= 2)
		check_speed = 1;

	/* check option */
	if (argc > 1) {
		p = argv[1];
		if (!strncmp(p, "force", 5)) {
			DEBUG(1, "opt: force\n");
			down_mode = MODE_FORCE;
		} else if (!strncmp(p, "upload", 6)) {
			DEBUG(1, "opt: upload\n");
			down_mode = MODE_UPLOAD;
		}
	}

	usbd = usbd_set_interface(&usbd_ops, down_mode);
	usbd->rx_len = 256;

	if (down_mode == MODE_UPLOAD)
		goto setup_usb;

	download_addr = (u32)usbd->ram_addr;

	mmc = find_mmc_device(usbd->mmc_dev);
	mmc_init(mmc);

	/* get gpt info */
setup_usb:

	/* init the usb controller */
	if (!usbd->usb_init()) {
		usbd->cancel(END_BOOT);
		return 0;
	}

	/* thor handsake */
	ret = thor_handshake(usbd);

	if (!ret)
		DEBUG(1, "Hand shake complete!\n");

	printf("USB DOWN\n");

	return 0;
}

U_BOOT_CMD(usbdown, CONFIG_SYS_MAXARGS, 1, do_usbd_down,
	   "USB Downloader for SLP",
	   "usbdown <force> - download binary images (force - don't check target)\n"
	   "usbdown upload - upload debug info"
);
