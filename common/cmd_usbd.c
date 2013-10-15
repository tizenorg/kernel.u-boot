#include <common.h>
#include <usbd.h>
#include <mmc.h>
#include <thor-proto.h>
#include <asm/arch/power.h>

/* download packet size */
#define PKT_DOWNLOAD_SIZE			(1 << 20)
#define PKT_DOWNLOAD_CHUNK_SIZE	(32 << 20)

#define DEBUG(level, fmt, args...) \
	if (level < debug_level) \
		printf(fmt, ##args);
#define ERROR(fmt, args...) \
	printf("error: " fmt, ##args);

static int debug_level = 2;
static int check_speed;

static int down_mode = MODE_NORMAL;

static u32 download_addr;
static u32 download_addr_ofs;

static struct usbd_ops usbd_ops;
static struct mmc *mmc;

struct usbd_file_info finfo;

static inline int pkt_download(void *dest, const int size)
{
	usbd_ops.recv_setup((char *)dest, size);
	return usbd_ops.recv_data();
}

static inline void pkt_upload(void *src, const int size)
{
	usbd_ops.send_data((char *)src, size);
}

/* send ACK to Host PC */
static void send_rsp(const struct res_pkt *rsp)
{
	memcpy(usbd_ops.tx_data, rsp, sizeof(struct res_pkt));
	pkt_upload(usbd_ops.tx_data, sizeof(struct res_pkt));
	DEBUG(1, "-RSP: %d, %d\n", rsp->id, rsp->sub_id);
}

/* send ACK about recv data to Host PC */
static void send_data_rsp(s32 ack, s32 count)
{
	struct data_res_pkt rsp;

	rsp.ack = ack;
	rsp.cnt = count;

	memcpy(usbd_ops.tx_data, &rsp, sizeof(struct data_res_pkt));
	pkt_upload(usbd_ops.tx_data, sizeof(struct data_res_pkt));

	DEBUG(3, "-DATA RSP: %d, %d\n", ack, count);
}

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

static inline void set_async_buffer(void)
{
	if (download_addr_ofs)
		download_addr_ofs = 0;
	else
		download_addr_ofs = PKT_DOWNLOAD_CHUNK_SIZE + (5 << 20);

	download_addr = usbd_ops.ram_addr + download_addr_ofs;
	DEBUG(1, "download addr: 0x%8x\n", download_addr);
}

static int recv_data(void)
{
	u32 dn_addr;
	u32 buffered = 0;
	u32 progressed = 0;

	int ret = 1;
	int count = 0;
	int final = 0;

	u32 remained = finfo.file_size;

	while (!final) {
		dn_addr = download_addr + buffered;
		ret = pkt_download((void *)dn_addr, finfo.download_packet_size);

		if (ret <= 0)
			return ret;

		buffered += finfo.download_packet_size;
		progressed += finfo.download_packet_size;

		/* check last packet */
		if (progressed >= finfo.file_size)
			final = 1;

		/* fusing per chunk */
		if ((buffered >= PKT_DOWNLOAD_CHUNK_SIZE) || final) {
			/* fusing recv data */
			/*ret = fusing_recv_data();*/

			if (ret <= 0)
				return ret;

			set_async_buffer();

			remained -= buffered;
			buffered = 0;
		}

		send_data_rsp(0, count++);

		/* set progress bar */
	}

	return ret;
}

static int thor_download_start(const struct res_pkt *rsp)
{
	/* send ACK to thor */
	send_rsp(rsp);

	return recv_data();
}

static int thor_request_download(const struct rqt_pkt *rqt)
{
	struct res_pkt rsp = {0, };

	rsp.id = rqt->id;
	rsp.sub_id = rqt->sub_id;

	switch (rqt->sub_id) {
	case RQT_DL_INIT:
		/* thor, send the size of the entire */
		finfo.download_total_size = rqt->int_data[0];
		finfo.download_unit = finfo.download_total_size / 100;
		finfo.download_prog = 0;
		finfo.download_prog_check = finfo.download_unit;

		DEBUG(1, "INIT: total %d bytes\n", finfo.download_total_size);

		download_addr_ofs = 0;

		/* send ACK to thor */
		send_rsp(&rsp);

		break;
	case RQT_DL_FILE_INFO:
		/* thor, send the file info */
		finfo.file_type = rqt->int_data[0];
		finfo.file_size = rqt->int_data[1];
		finfo.file_name = (char *)rqt->str_data[0];
		finfo.file_offset = 0;

		DEBUG(1, "INFO: name(%s),size(%d), type(%d)\n",
		      finfo.file_name, finfo.file_size, finfo.file_type);

		rsp.int_data[0] = PKT_DOWNLOAD_SIZE;
		finfo.download_packet_size = PKT_DOWNLOAD_SIZE;

		/* send ACK to thor */
		send_rsp(&rsp);

		break;
	case RQT_DL_FILE_START:
		return thor_download_start(&rsp);
	case RQT_DL_FILE_END:
		/* send ACK to thor */
		send_rsp(&rsp);

		break;
	case RQT_DL_EXIT:
		/* send ACK to thor */
		send_rsp(&rsp);

		break;
	default:
		return 0;
	}

	return 1;
}

static int thor_request_cmd(const struct rqt_pkt *rqt)
{
	struct res_pkt rsp = {0, };

	rsp.id = rqt->id;
	rsp.sub_id = rqt->sub_id;

	switch (rqt->sub_id) {
	case RQT_CMD_REBOOT:
		DEBUG(1, "Target Reset\n");
		/* send ACK to thor */
		send_rsp(&rsp);

		/* set normal boot */
		board_inform_clear();
		run_command("reset", 0);
		break;
	case RQT_CMD_POWEROFF:
		DEBUG(1, "Target PowerOff\n");
		/* send ACK to thor */
		send_rsp(&rsp);

		/* power_off(); */
		break;
	default:
		return 0;
	}

	return 1;
}

static int thor_request_info(const struct rqt_pkt *rqt)
{
	struct res_pkt rsp = {0, };

	rsp.id = rqt->id;
	rsp.sub_id = rqt->sub_id;

	switch (rqt->sub_id) {
	case RQT_INFO_VER_PROTOCOL:
		rsp.int_data[0] = VER_PROTOCOL_MAJOR;
		rsp.int_data[1] = VER_PROTOCOL_MINOR;
		break;
	case RQT_INFO_VER_HW:
		snprintf(rsp.str_data[0], sizeof(rsp.str_data[0]),
			 "%x", checkboard());
		break;
	case RQT_INFO_VER_BOOT:
		sprintf(rsp.str_data[0], "%s", getenv("ver"));
		break;
	case RQT_INFO_VER_KERNEL:
		sprintf(rsp.str_data[0], "%s", "k unknown");
		break;
	case RQT_INFO_VER_PLATFORM:
		sprintf(rsp.str_data[0], "%s", "p unknown");
		break;
	case RQT_INFO_VER_CSC:
		sprintf(rsp.str_data[0], "%s", "c unknown");
		break;
	default:
		return 0;
	}

	send_rsp(&rsp);
	return 1;
}

static int thor_download_data(struct usbd_ops *usbd)
{
	struct rqt_pkt rqt;
	int ret = 1;

	/* receive thor request */
	memset(&rqt, 0, sizeof(struct rqt_pkt));
	memcpy(&rqt, usbd->rx_data, sizeof(rqt));

	DEBUG(1, "+RQT: %d, %d\n", rqt.id, rqt.sub_id);

	switch (rqt.id) {
	case RQT_INFO:
		ret = thor_request_info(&rqt);
		break;
	case RQT_CMD:
		ret = thor_request_cmd(&rqt);
		break;
	case RQT_DL:
		ret = thor_request_download(&rqt);
		break;
	case RQT_UL:
		break;
	default:
		ERROR("unknown request: %d, %d\n", rqt.id, rqt.sub_id);
		ret = 0;
	}

	/*
	 * > 0 : success
	 * = 0 : error -> exit
	 * < 0 : error -> retry
	 */

	return ret;
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

	usbd->start();
	finfo.file_offset = 0;

	/* receive the data from Host PC */
	while (1) {
		usbd->recv_setup(usbd->rx_data, usbd->rx_len);

		ret = usbd->recv_data();
		if (ret > 0) {
			ret = thor_download_data(usbd);

			if (ret < 0) {
				usbd->cancel(END_RETRY);
				return 0;
			} else if (ret == 0) {
				usbd->cancel(END_NORMAL);
				return 0;
			}
		} else if (ret < 0) {
			usbd->cancel(END_FAIL);
			return 0;
		} else {
			usbd->cancel(END_BOOT);
			return 0;
		}
	}

	printf("USB DOWN\n");

	return 0;
}

U_BOOT_CMD(usbdown, CONFIG_SYS_MAXARGS, 1, do_usbd_down,
	   "USB Downloader for SLP",
	   "usbdown <force> - download binary images (force - don't check target)\n"
	   "usbdown upload - upload debug info"
);
