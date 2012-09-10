/*
 * USB Downloader
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 * PROPRIETARY/CONFIDENTIAL
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information").
 * You shall not disclose such Confidential Information and shall
 * use it only in accordance with the terms of the license agreement
 * you entered into with SAMSUNG ELECTRONICS.  SAMSUNG make no
 * representations or warranties about the suitability
 * of the software, either express or implied, including but not
 * limited to the implied warranties of merchantability, fitness for
 * a particular purpose, or non-infringement.
 * SAMSUNG shall not be liable for any damages suffered by licensee as
 * a result of using, modifying or distributing this software or its derivatives.
 *
 */

#include <common.h>
#include <usbd.h>
#include <asm/errno.h>
#include <asm/arch/power.h>
#include <malloc.h>
#include <mbr.h>
#include <mmc.h>
#include <fat.h>
#include <part.h>
#include <fbutils.h>
#include <mobile/header.h>
#include <mobile/pit.h>
#include <mobile/fota.h>
#include <mobile/misc.h>
#include <mobile/fs_type_check.h>
#include <mobile/stopwatch.h>
#ifndef CONFIG_CMD_PIT
#error "USB Downloader need to define the CONFIG_CMD_PIT"
#endif
#ifndef CONFIG_GENERIC_MMC
#error "USB Downloader need to define the CONFIG_GENERIC_MMC"
#endif

#define DEBUG(level, fmt, args...) \
	if (level < debug_level) printf(fmt, ##args);
#define ERROR(fmt, args...) \
	printf("error: " fmt, ##args);

#ifdef CONFIG_MMC_ASYNC_WRITE
#define mmc_async_on(d, c)	mmc_async_on(d, c)
#else
#define mmc_async_on(d, c)	do { } while(0)
#endif

/* version of USB Downloader Application */
#define APP_VERSION		"1.0"

/* download/upload packet size */
#define PKT_DOWNLOAD_SIZE 	(1 << 20)
#define PKT_DOWNLOAD_CHUNK_SIZE	(32 << 20)
#define PKT_UPLOAD_SIZE 	(512 << 10)

enum {
	OPS_READ,
	OPS_WRITE,
	OPS_ERASE,
};

enum {
	FILE_TYPE_NORMAL,
	FILE_TYPE_PIT,
};

static struct usbd_ops usbd_ops;
static struct mmc *mmc;

static int down_mode = MODE_NORMAL;
static int debug_level = 2;
static int check_speed = 0;

static u32 download_addr;
static u32 download_addr_ofs;

static u32 download_total_size;
static u32 download_packet_size;
static u32 download_unit;
static u32 download_prog;
static u32 download_prog_check;

static int dump_start;

static int file_size;
static int file_type;
static int file_offset;

extern struct pitpart_data pitparts[];

static int pkt_download(void *dest, const int size);
static void send_data_rsp(s32 ack, s32 count);

#ifdef CONFIG_ENV_UPDATE_WITH_DL
static unsigned char update_flag = 0;
#endif


static void set_update_state(int on)
{
	static int state = 0;

	if (state == on)
		return;
	else
		state = on;

	if (on)
		setenv("updatestate", "going");
	else
		setenv("updatestate", NULL);

	saveenv();
}

static void set_update_flag(void)
{
	int i;
#ifdef CONFIG_ENV_UPDATE_WITH_DL
	int count = 0;
	char buf[512] = {0, };
	char *paramlist[] = { "SLP_KERNEL_NEW", "SLP_ROOTFS_NEW" };
#endif
	if (update_flag & (1 << SLP_KERNEL_NEW))
		setenv("SLP_KERNEL_NEW","1");
	if (update_flag & (1 << SLP_ROOTFS_NEW))
		setenv("SLP_ROOTFS_NEW","1");
#ifdef CONFIG_ENV_UPDATE_WITH_DL
	for (i = 0; i < ARRAY_SIZE(paramlist); i++) {
		count += sprintf(buf + count, "%s=%s;",
				paramlist[i], getenv(paramlist[i]));
		DEBUG(3, "buf: %s\n", buf);
	}

	setenv("updated", "1");
	setenv("updateparam", buf);

	DEBUG(1, "update env: %s(%d)\n", buf, strlen(buf));
#endif
	saveenv();
}

static int mmc_cmd(int ops, int dev_num, ulong start, lbaint_t cnt, void *addr)
{
	int ret = 0;

	DEBUG(1, "mmc %s 0 0x%x 0x%x 0x%x\n", ops ? "write" : "read",
			(u32)addr, (u32)start, (u32)cnt);

	if (ops == OPS_WRITE)
		ret = mmc->block_dev.block_write(dev_num, start, cnt, addr);
	else if (ops == OPS_READ)
		ret = mmc->block_dev.block_read(dev_num, start, cnt, addr);

	if (ret > 0)
		ret = 1;
	else
		ret = 0;

	return ret;
}

static int mmc_write(struct usbd_ops *usbd, unsigned int len, int part_num)
{
	int ret = 0;
	int pit_idx;
	int blk_len = len / usbd->mmc_blk;
	int fstype;
	int ums_id;

	if (gpt_parts <= part_num) {
		ERROR("Partition table have %d partitions (request %d)\n",
				gpt_parts, part_num);
		return 0;
	}

	/* check partition overwrite */
	if ((gpt_offset[part_num] + file_offset + blk_len) > gpt_offset[part_num + 1]
		&& (part_num + 1) < gpt_parts) {
		ERROR("download img is too large (blk size: 0x%x)\n",
			gpt_offset[part_num + 1] - gpt_offset[part_num]);
#ifdef CONFIG_LCD
		set_font_color(FONT_YELLOW);
		if (!board_no_lcd_support()) {
			fb_printf("\nError: download img is too large\n");
		}
#endif
		return -1;
	}

	ret = mmc_cmd(OPS_WRITE, usbd->mmc_dev,
			gpt_offset[part_num] + file_offset,
			blk_len,
			(void *)download_addr);

	file_offset += blk_len;

	return ret;
}

static int fs_write(struct usbd_ops *usbd, char *file_name, int part_id, int len)
{
	int ret;
	char cmd[12], arg1[12], arg2[128], arg3[128], arg4[12];
	char * const argv[] = {cmd, "mmc", arg1, arg2, arg3, arg4};
	int fstype;

	ret = fstype_register_device(&mmc->block_dev, part_id);
	if (ret < 0) {
		ERROR("fstype_register_divce\n");
		return 0;
	}

	sprintf(arg1, "%d:%d", usbd->mmc_dev, part_id);
	fstype = fstype_check(&mmc->block_dev);

	/* detect filesystem */
	if (fstype == FS_TYPE_EXT4) {
#if defined(CONFIG_CMD_EXT4)
		sprintf(cmd, "ext4write");
		sprintf(arg2, "/%s", file_name);
		sprintf(arg3, "0x%x", (u32)download_addr);
		sprintf(arg4, "%d", len);

		if (do_ext4_write(NULL, 0, 6, argv)) {
			ERROR("writing %s\n", file_name);
			return 0;
		}
		return 1;
#endif
	} else if (fstype == FS_TYPE_VFAT) {
#if defined(CONFIG_FAT_WRITE)
		sprintf(cmd, "fatwrite");
		sprintf(arg2, "0x%x", (u32)download_addr);
		sprintf(arg3, "%s", file_name);
		sprintf(arg4, "%d", len);

		if (do_fat_fswrite(NULL, 0, 6, argv)) {
			ERROR("writing %s\n", file_name);
			return 0;
		}
		return 1;
#endif
	}

	/* change filesystem by CONFIG define
	   fs priority: ext4 -> vfat */
#if defined(CONFIG_CMD_EXT4)
	if (fstype != FS_TYPE_EXT4) {
		sprintf(cmd, "ext4format");
		if (do_ext4_format(NULL, 0, 3, argv)) {
			ERROR("format with ext4\n");
			return 0;
		}
	}

	sprintf(cmd, "ext4write");
	sprintf(arg2, "/%s", file_name);
	sprintf(arg3, "0x%x", (u32)download_addr);
	sprintf(arg4, "%d", len);

	ret = do_ext4_write(NULL, 0, 6, argv);
#elif defined(CONFIG_FAT_WRITE)
	if (fstype != FS_TYPE_VFAT) {
		sprintf(cmd, "fatformat");
		if (do_fat_format(NULL, 0, 3, argv)) {
			ERROR("format with vfat\n");
			return 0;
		}
	}

	sprintf(cmd, "fatwrite");
	sprintf(arg2, "0x%x", (u32)download_addr);
	sprintf(arg3, "%s", file_name);
	sprintf(arg4, "%d", len);

	ret = do_fat_fswrite(NULL, 0, 6, argv);
#else
	ERROR("doesn't support filesystem\n");
	return 0;
#endif

	if (ret) {
		ERROR("writing %s\n", file_name);
		return 0;
	}

	return 1;
}

static int fusing_mmc(u32 pit_idx, u32 addr, u32 len, u32 is_last)
{
	int ret = -1;
	char buf[64];
	char *name = pitparts[pit_idx].name;
	int part_id = pitparts[pit_idx].id;
	int part_type = pitparts[pit_idx].part_type;
	int part_fstype = pitparts[pit_idx].filesys;

	name = pitparts[pit_idx].name;

	if (name == NULL)
		name = "?";

	DEBUG(3, "mmc (%d, %s) len(0x%x)\n", part_id, name, len);

	if (part_id >= PIT_BOOTPART0_ID) {
		sprintf(buf, "mmc boot 0 1 1 %d", PIT_BOOTPARTn_GET(part_id));
		if (run_command(buf, 0) > 0) {
			sprintf(buf, "mmc write 0 0x%x 0 0x%x", addr,
				(u32)pitparts[pit_idx].size / usbd_ops.mmc_blk);
			ret = run_command(buf, 0);
		} else
			ERROR("s-boot update failed\n");

		run_command("mmc boot 0 1 1 0", 0);

		goto fusing_mmc_end;
	}

	if (part_type != PART_TYPE_DATA) {
		ERROR("not supported type: %d\n", part_type);
		return ret;
	}

	if (part_fstype == PART_FS_TYPE_BASIC) {
		DEBUG(3, "at hidden partition\n");
		ret = mmc_cmd(OPS_WRITE, usbd_ops.mmc_dev,
				pitparts[pit_idx].offset / usbd_ops.mmc_blk,
				pitparts[pit_idx].size / usbd_ops.mmc_blk,
				(void *)addr);
	} else {
		DEBUG(3, "at normal partition as image\n");
		int part_base = pit_get_partition_base();
		ret = mmc_write(&usbd_ops, len, (pit_idx - part_base));
	}

fusing_mmc_end:
	DEBUG(3, "mmc done\n");

	return ret;
}

static int fusing_fs(u32 pit_idx, u32 addr, u32 len, u32 is_last)
{
	int ret = 0;
	int part_id;
	int tmp_id;
	char *name = pitparts[pit_idx].name;
	static u32 offset = 0; /* At last, offset is file size */

	DEBUG(3, "file (%d, %s) len(0x%x)\n", pitparts[pit_idx].id, name, len);

	memcpy(CONFIG_SYS_BACKUP_ADDR + offset, download_addr, len);
	offset += len;

	if (!is_last)
		return 1;

	download_addr = CONFIG_SYS_BACKUP_ADDR;

	if (!strncmp(name, "install-", 8) ||
	    !strncmp(name, "fota-delta", 10)) {
		/* need to erase for hibernation */
		erase_given_area(PARTS_QBOOT, QBOOT_ERASE_SIZE);

		tmp_id = get_pitpart_id(PARTS_UMS);
	} else
		tmp_id = get_pitpart_id(PARTS_BOOT);

	part_id = pit_adjust_id_on_mmc(tmp_id);
	ret = fs_write(&usbd_ops, pitparts[pit_idx].file_name, part_id, offset);
	offset = 0;

#ifdef CONFIG_FOTA_DELTA_DOWNLOAD
	if (!ret && !strncmp(name, "fota-delta", 10)) {
		DEBUG(3, "set FOTA flag\n");
		board_inform_set(REBOOT_FOTA);
	}
#endif
	DEBUG(3, "file done\n");

	return ret;
}

static int update_pit(void)
{
	char arg1[12], arg2[12];
	char *argv[] = {"pit", arg1, arg2};
	u32 boot_idx;
	u32 boot_offset = 0;
	u32 boot_size = 0;

	boot_idx = get_pitpart_id(PARTS_BOOT);
	if (boot_idx >= 0) {
		boot_offset = pitparts[boot_idx].offset;
		boot_size = pitparts[boot_idx].size;
	}

	sprintf(arg1, "update");
	sprintf(arg2, "%x", (u32)download_addr);
	do_pit(NULL, 0, 3, argv);

	sprintf(arg1, "write");
	sprintf(arg2, "%x", (u32)download_addr);
	do_pit(NULL, 0, 3, argv);

	if (!pit_no_partition_support()) {
		char *mbr = getenv("mbrparts");
		int len = strlen(mbr) + 1;
		int mbr_idx = pit_get_partition_base();
		memcpy((void *)download_addr, (void *)mbr, len);
		set_gpt_info(&mmc->block_dev, (char *)download_addr, len,
				pitparts[mbr_idx].offset / usbd_ops.mmc_blk);
	}

	/*
	 * boot partition format, that is only formatted at bootloader.
	 * format conditions are,
	 * - can't detect filesystem (e.g., not formatted, broken)
	 * - boot partition size or location is changed.
	 */
	boot_idx = get_pitpart_id(PARTS_BOOT);
	if ((boot_offset != pitparts[boot_idx].offset) ||
			(boot_size != pitparts[boot_idx].size)) {
		char cmd[32];
		/* just erase partition head to format at writing */
		DEBUG(1, "erase /boot partition to format");
		sprintf(cmd, "mmc erase 0 0x%x 4",
			pitparts[boot_idx].offset / usbd_ops.mmc_blk);
		run_command(cmd, 0);
	}

	printf("Update pit is done!\n");

	return 1;
}

static int fusing_recv_data(u32 addr, int pit_idx, s32 size, int final)
{
	int ret = 1;
	char *name;
	int uboot_index = -1;

	if (file_type == FILE_TYPE_PIT)
		return update_pit();

	name = pitparts[pit_idx].name;


	switch (pitparts[pit_idx].dev_type) {
	case PIT_DEVTYPE_MMC:
		ret = fusing_mmc(pit_idx, addr, size, final);
		break;
	case PIT_DEVTYPE_FILE:
		ret = fusing_fs(pit_idx, addr, size, final);
		break;
	default:
		DEBUG(1, "unknown device type(%d). plz check PIT\n", pitparts[pit_idx].dev_type);
		ret = -1;
	}

	return ret;
}

static inline void set_async_buffer(void)
{
	if (download_addr_ofs)
		download_addr_ofs = 0;
	else
		download_addr_ofs = PKT_DOWNLOAD_CHUNK_SIZE + (5 << 20);

	download_addr = usbd_ops.ram_addr + download_addr_ofs;
	DEBUG(3, "download addr: 0x%8x\n", download_addr);
}

static void recv_data_end(int pit_idx)
{
	char *name;

	if (pit_idx < 0) {
		ERROR("invalid pit index\n");
		return;
	}

	name = pitparts[pit_idx].name;

#ifndef CONFIG_ENV_UPDATE_WITH_DL
	if (!strncmp(name, PARTS_BOOTLOADER, 6)) {
		erase_given_area(PARTS_PARAMS, 0);
	}
#endif
	if (!strncmp(name, PARTS_QBOOT, 5)) {
		qboot_erase = 1;
	} else if (!strncmp(name, PARTS_KERNEL, 6)) {
		update_flag |= 1 << SLP_KERNEL_NEW;
		erase_given_area(PARTS_QBOOT, QBOOT_ERASE_SIZE);
	} else if (!strncmp(name, PARTS_ROOT, 8) ||
		!strncmp(name, PARTS_DATA,4)) {
		update_flag |= 1 << SLP_ROOTFS_NEW;
		erase_given_area(PARTS_QBOOT, QBOOT_ERASE_SIZE);
	} else if (!strncmp(name, PARTS_CSC,3)) {
		update_flag |= 1 << SLP_ROOTFS_NEW;
	} else if (!strncmp(name, PARTS_UBI, 3)) {
		erase_given_area(PARTS_QBOOT, QBOOT_ERASE_SIZE);
	}
}

static int recv_data(int pit_idx)
{
	u32 dn_addr;
	u32 buffered = 0;
	u32 progressed = 0;
	s32 remained = file_size;
	int ret = 1;
	int count = 1;
	int final = 0;
	unsigned long long sw_size;
	u32 sw_check = 0;
	u32 sw_bak = 0;
	u32 sw_time = 0;

	if ((pit_idx < 0) && (file_type != FILE_TYPE_PIT)) {
		ERROR("invalid pit index because of wrong file name\n");
		return -ENOENT;
	}

	if (check_speed)
		sw_size = (unsigned long long) download_packet_size *
				stopwatch_tick_clock() / 1024; /* KB */

	while(!final) {
		if (check_speed) {
			sw_bak = sw_check;
			sw_check = stopwatch_tick_count();

			if (!sw_time && sw_bak)
				sw_time = sw_check - sw_bak;
			else
				sw_time = (sw_time + (sw_check - sw_bak)) / 2;
		}

		dn_addr = download_addr + buffered;
		ret = pkt_download((void *)dn_addr, download_packet_size);
		if (ret <= 0)
			return ret;

		buffered += download_packet_size;
		progressed += download_packet_size;

		/* check last packet */
		if (progressed >= file_size)
			final = 1;

		/* fusing per chunk */
		if ((buffered >= PKT_DOWNLOAD_CHUNK_SIZE) || final) {
			if (check_speed)
				printf("[%d MB/s]\n", (u32)(sw_size / sw_time / 1024));

			set_update_state(1);
			ret = fusing_recv_data(download_addr, pit_idx, MIN(buffered, remained), final);
			if (ret <= 0)
				return ret;

			set_async_buffer();
			remained -= buffered;
			buffered = 0;
		}

		if (final)
			recv_data_end(pit_idx);

		send_data_rsp(0, count++);

		/* progress bar */
		download_prog += download_packet_size;
		if (download_prog > download_prog_check) {
			if (usbd_ops.set_progress)
				usbd_ops.set_progress(download_prog / download_unit);
			download_prog_check += max(download_unit, download_packet_size);
		}
	}

	return ret;
}

/*
 * DOWNLOAD
 */

#define VER_PROTOCOL_MAJOR	4
#define VER_PROTOCOL_MINOR	0

enum rqt {
	RQT_INFO = 200,
	RQT_CMD,
	RQT_DL,
	RQT_UL,
};

enum rqt_data {
	/* RQT_INFO */
	RQT_INFO_VER_PROTOCOL = 1,
	RQT_INIT_VER_HW,
	RQT_INIT_VER_BOOT,
	RQT_INIT_VER_KERNEL,
	RQT_INIT_VER_PLATFORM,
	RQT_INIT_VER_CSC,

	/* RQT_CMD */
	RQT_CMD_REBOOT = 1,
	RQT_CMD_POWEROFF,
	RQT_CMD_EFSCLEAR,

	/* RQT_DL */
	RQT_DL_INIT = 1,
	RQT_DL_FILE_INFO,
	RQT_DL_FILE_START,
	RQT_DL_FILE_END,
	RQT_DL_EXIT,

	/* RQT_UL */
	RQT_UL_INIT = 1,
	RQT_UL_START,
	RQT_UL_END,
	RQT_UL_EXIT,
};

typedef struct _rqt_box {	/* total: 256B */
	s32 rqt;		/* request id */
	s32 rqt_data;		/* request data id */
	s32 int_data[14];	/* int data */
	char str_data[5][32];	/* string data */
	char md5[32];		/* md5 checksum */
} __attribute__((packed)) rqt_box;

typedef struct _rsp_box {	/* total: 128B */
	s32 rsp;		/* response id (= request id) */
	s32 rsp_data;		/* response data id */
	s32 ack;		/* ack */
	s32 int_data[5];	/* int data */
	char str_data[3][32];	/* string data */
} __attribute__((packed)) rsp_box;

typedef struct _data_rsp_box {	/* total: 8B */
	s32 ack;		/* response id (= request id) */
	s32 count;		/* response data id */
} __attribute__((packed)) data_rsp_box;


static const char dl_key[] = "THOR";
static const char dl_ack[] = "ROHT";

static inline int pkt_download(void *dest, const int size)
{
	usbd_ops.recv_setup((char *)dest, size);
	return usbd_ops.recv_data();
}

static inline void pkt_upload(void *src, const int size)
{
	usbd_ops.send_data((char *)src, size);
}

static void send_rsp(const rsp_box *rsp)
{
	/* should be copy on dma duffer */
	memcpy(usbd_ops.tx_data, rsp, sizeof(rsp_box));
	pkt_upload(usbd_ops.tx_data, sizeof(rsp_box));

	DEBUG(1, "-RSP: %d, %d\n", rsp->rsp, rsp->rsp_data);
}

static void send_data_rsp(s32 ack, s32 count)
{
	data_rsp_box rsp;

	rsp.ack = ack;
	rsp.count = count;

	/* should be copy on dma duffer */
	memcpy(usbd_ops.tx_data, &rsp, sizeof(data_rsp_box));
	pkt_upload(usbd_ops.tx_data, sizeof(data_rsp_box));

	DEBUG(3, "-DATA RSP: %d, %d\n", ack, count);
}

static int process_rqt_info(const rqt_box *rqt)
{
	rsp_box rsp = {0, };

	rsp.rsp = rqt->rqt;
	rsp.rsp_data = rqt->rqt_data;

	switch (rqt->rqt_data) {
	case RQT_INFO_VER_PROTOCOL:
		rsp.int_data[0] = VER_PROTOCOL_MAJOR;
		rsp.int_data[1] = VER_PROTOCOL_MINOR;
		break;
	case RQT_INIT_VER_HW:
		sprintf(rsp.str_data[0], "%x", get_board_rev());
		break;
	case RQT_INIT_VER_BOOT:
		sprintf(rsp.str_data[0], "%s", getenv("ver"));
		break;
	case RQT_INIT_VER_KERNEL:
		sprintf(rsp.str_data[0], "%s", "k unknown");
		break;
	case RQT_INIT_VER_PLATFORM:
		sprintf(rsp.str_data[0], "%s", "p unknown");
		break;
	case RQT_INIT_VER_CSC:
		sprintf(rsp.str_data[0], "%s", "c unknown");
		break;
	default:
		return 0;
	}

	send_rsp(&rsp);
	return 1;
}

static int process_rqt_cmd(const rqt_box *rqt)
{
	rsp_box rsp = {0, };

	rsp.rsp = rqt->rqt;
	rsp.rsp_data = rqt->rqt_data;

	switch (rqt->rqt_data) {
	case RQT_CMD_REBOOT:
		DEBUG(1, "TARGET RESET\n");
		send_rsp(&rsp);

		board_inform_clear(0);
		board_inform_set(0);

		run_command("reset", 0);
		break;
	case RQT_CMD_POWEROFF:
		DEBUG(1, "TARGET POWEROFF\n");
		send_rsp(&rsp);

		power_off();
		break;
	case RQT_CMD_EFSCLEAR:
		/* DEBUG(1, "EFS CLEAR\n"); */
		/* erase /csa/nv/nvdata.bin */
		fs_write(&usbd_ops, "nv/nvdata.bin", 1, 0);
		send_rsp(&rsp);
		break;
	default:
		return 0;
	}

	return 1;
}

static int process_rqt_download(const rqt_box *rqt)
{
	rsp_box rsp = {0, };
	int ret;
	char *file_name;
	static int pit_idx;

	rsp.rsp = rqt->rqt;
	rsp.rsp_data = rqt->rqt_data;

	switch (rqt->rqt_data) {
	case RQT_DL_INIT:
		download_total_size = rqt->int_data[0];
		download_unit = download_total_size / 100;
		download_prog = 0;
		download_prog_check = 0 + download_unit;

		DEBUG(1, "INIT: total %d bytes\n", download_total_size);

		download_addr_ofs = 0;
		mmc_async_on(mmc, 1);
		break;
	case RQT_DL_FILE_INFO:
		file_type = rqt->int_data[0];
		file_size = rqt->int_data[1];
		file_name = rqt->str_data[0];
		file_offset = 0;
		if (file_type == FILE_TYPE_PIT) {
			int pit_is_up = pit_check_version(file_name);
			/*
			 * Partition policy is changed from 08 version.
			 * When 08 version is installed, we will block migrating
			 * to previous version by pit.
			 */
			if ((pit_get_version() == 8) && (pit_is_up < 0)) {
				lcd_display_clear();
				init_font();
				set_font_color(FONT_RED);
				fb_printf("[Error]\n\n");
				set_font_color(FONT_WHITE);
				fb_printf("Must download pit version larger than 8.\n");
				fb_printf("Press power key retry to download again.\n");
				while(!check_exit_key());
				return -1;
			}
			pit_idx = get_pitpart_id_for_pit(file_name);
		} else
			pit_idx = get_pitpart_id_by_filename(file_name);

		DEBUG(1, "INFO: name(%s, %d), size(%d), type(%d)\n", file_name, pit_idx, file_size, file_type);

		rsp.int_data[0] = PKT_DOWNLOAD_SIZE;
		download_packet_size = PKT_DOWNLOAD_SIZE;
		break;
	case RQT_DL_FILE_START:
		send_rsp(&rsp);
		return recv_data(pit_idx);
	case RQT_DL_FILE_END:
		break;
	case RQT_DL_EXIT:
		mmc_async_on(mmc, 0);
		set_update_state(0);
		set_update_flag();
		break;
	default:
		return 0;
	}

	send_rsp(&rsp);
	return 1;
}

static int process_rqt_upload(const rqt_box *rqt)
{
	rsp_box rsp = {0, };

	rsp.rsp = rqt->rqt;
	rsp.rsp_data = rqt->rqt_data;

	switch (rqt->rqt_data) {
	case RQT_UL_INIT:
		break;
	case RQT_UL_START:
		break;
	case RQT_UL_END:
		break;
	case RQT_UL_EXIT:
		break;
	default:
		return 0;
	}

	send_rsp(&rsp);
	return 1;
}

static int process_download_data(struct usbd_ops *usbd)
{
	rqt_box rqt;
	int ret = 1;

	memset(&rqt, 0, sizeof(rqt));
	memcpy(&rqt, usbd->rx_data, sizeof(rqt));

	DEBUG(1, "+RQT: %d, %d\n", rqt.rqt, rqt.rqt_data);

	switch (rqt.rqt) {
	case RQT_INFO:
		ret = process_rqt_info(&rqt);
		break;
	case RQT_CMD:
		ret = process_rqt_cmd(&rqt);
		break;
	case RQT_DL:
		ret = process_rqt_download(&rqt);
		break;
	case RQT_UL:
		ret = process_rqt_upload(&rqt);
		break;
	default:
		ERROR("unknown request (%d)\n", rqt.rqt);
		ret = 0;
	}

	/*
	 * > 0 : success
	 * = 0 : error -> exit
	 * < 0 : error -> retry
	 */
	return ret;
}

/*
 * UPLOAD for RDX
 */

static const char rdx_preamble[] = "PrEaMbLe";
static const char rdx_postamble[] = "PoStAmBlE";
static const char rdx_ack[] = "AcKnOwLeDgMeNt";

static const char rdx_probe[] = "PrObE";
static const char rdx_xfer[] = "DaTaXfEr";

extern int rdx_info_get(void **info_buf, int *info_size);

static int process_upload_data(struct usbd_ops *usbd)
{
	static u32 dump_start, dump_end, upstate = 0;
	char *rx = usbd->rx_data;
	char *tx = usbd->tx_data;
	u32 chunk_size;
	void *buf;
	int size;

	if (!strncmp(rx, rdx_probe, strlen(rdx_probe))) {
		DEBUG(1, "up: information upload\n");

		rdx_info_get(&buf, &size);
		memcpy(tx, buf , size);
		usbd->send_data(usbd_ops.tx_data, size);
		return 1;
	} else if (!strncmp(rx, rdx_preamble, strlen(rdx_preamble))) {
		DEBUG(1, "up: preamble\n");
		upstate = 1;

		strcpy(tx, rdx_ack);
		usbd->send_data(tx, sizeof(rdx_ack));
		return 1;
	}

	/* get dump start, end address */
	if ((upstate == 1 || upstate == 2) && (strlen(rx) == 8)) {
		if (upstate == 1)
			dump_start = simple_strtoul(rx, NULL, 16);
		else if (upstate == 2)
			dump_end = simple_strtoul(rx, NULL, 16);
		else
			DEBUG(1, "up: oops!\n");
		upstate++;

		if (upstate == 3)
			DEBUG(1, "up: start(0x%08x), end(0x%08x)\n",
				dump_start, dump_end);

		strcpy(tx, rdx_ack);
		usbd->send_data(tx, sizeof(rdx_ack));
		return 1;
	}

	if ((upstate == 3)
	    && (!strncmp(rx, rdx_xfer, strlen(rdx_xfer))
		|| !strncmp(rx, rdx_ack, strlen(rdx_ack)))) {
		if (dump_start > dump_end) {
			upstate = 0;
			strcpy(tx, rdx_postamble);
			usbd->send_data(tx, sizeof(rdx_postamble));
			return 1;
		}

		/* dummy command for system reset */
		if ((dump_start == 0xfffffffc) && (dump_end == 0xffffffff))
			usbd->cancel(END_BOOT);

		chunk_size = min(PKT_UPLOAD_SIZE, (dump_end - dump_start + 1));
		usbd->send_data(dump_start, chunk_size);
		dump_start += chunk_size;

		if (!(dump_start % 0x1000000))
			DEBUG(1, "up: uploaded 0x%08x / 0x%08x\n",
				dump_start, dump_end);
		return 1;
	}

	upstate = 0;
	return 1;
}

int do_usbd_down(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct usbd_ops *usbd;
	int (*process_data) (struct usbd_ops *);
	int recv_size;
	char *p;
	int ret;
	int update_fail = 0;

	/* check debug level */
	if (p = getenv("usbdebug")) {
		debug_level = simple_strtoul(p, NULL, 10);
		DEBUG(0, "debug level %d\n", debug_level);
	}

	/* download speed check option */
	if (debug_level >= 2)
		check_speed = 1;

	/* check option */
	if (argc > 1) {
		p = argv[1];
		if (!strncmp(p, "fail", 4)) {
			DEBUG(1, "opt: fail\n");
			update_fail = 1;
		} else if (!strncmp(p, "force", 5)) {
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

	if (usbd->set_logo)
		usbd->set_logo(APP_VERSION, usbd->mmc_info, update_fail);

	mmc = find_mmc_device(usbd->mmc_dev);
	mmc_init(mmc);

	/* get gpt info */
	set_gpt_dev(usbd->mmc_dev);
	gpt_parts = get_gpt_table(&mmc->block_dev, gpt_offset);
	if (!gpt_parts) {
		char buf[32];
		sprintf(buf, "gpt default %d", usbd->mmc_dev);
		run_command(buf, 0);
	}

	DEBUG(1, "Downloader v%s %s\n",
		APP_VERSION,
		down_mode == MODE_FORCE ? "- Force" : "");

setup_usb:
	/* init the usb controller */
	if (!usbd->usb_init()) {
		usbd->cancel(END_BOOT);
		return 0;
	}

	/* refresh download logo since downloading fail */
	if (update_fail) {
		if (usbd->set_logo)
			usbd->set_logo(APP_VERSION, usbd->mmc_info, 0);

	}

setup_retry:
	usbd->recv_setup(usbd->rx_data, strlen(dl_key));
	/* detect the download request from Host PC */
	ret = usbd->recv_data();
	if (ret > 0) {
		if (!strncmp(usbd->rx_data, dl_key, strlen(dl_key))) {
			DEBUG(1, "Download request from the Host PC\n");
			msleep(30);

			strcpy(usbd->tx_data, dl_ack);
			usbd->send_data(usbd->tx_data, strlen(dl_ack));
		} else if (!strncmp(usbd->rx_data, rdx_preamble, strlen(rdx_preamble))) {
			DEBUG(1, "Upload request from the Host\n");
			msleep(30);

			strcpy(usbd->tx_data, rdx_ack);
			usbd->send_data(usbd->tx_data, sizeof(rdx_ack));
		} else if (strncmp(usbd->rx_data, "AT", 2) == 0) {
			int i;
			char atcmd_rev[10];
			const char aterr_key[13] = {
				0x0d, 0x0a,
				'C' , 'M' , 'E' , ' ' ,'E', 'R', 'R', 'O', 'R',
				0x0d, 0x0a
			};

			strncpy(atcmd_rev, usbd->rx_data, sizeof(atcmd_rev));
			for (i = 2; i < sizeof(atcmd_rev); i++) {
				if (atcmd_rev[i] == 0x0d) {
					atcmd_rev[i] = '\0';
					DEBUG(1, "Received AT cmd from the Host PC:%s , length:%d\n",atcmd_rev, i);
					break;
				}
			}

			strncpy(usbd->tx_data, aterr_key, sizeof(aterr_key));
			usbd->send_data(usbd->tx_data, sizeof(aterr_key));

#ifdef CONFIG_S5P_USB_DMA
			msleep(30);
			/*BULK_IN/OUT_RESET_DATA_PID to 0*/
			usbd->prepare_dma(NULL, 0, 0);
#endif
			DEBUG(1, "Retry waiting key from the Host PC\n");
			goto setup_retry;
		} else {
			DEBUG(1, "Invalid request from the Host PC!! (len=%lu)\n", usbd->rx_len);
			usbd->cancel(END_RETRY);
			return 0;
		}
	} else if (ret < 0) {
		usbd->cancel(END_RETRY);
		return 0;
	} else {
		usbd->cancel(END_BOOT);
		return 0;
	}

	usbd->start();
	file_offset = 0;

	if (down_mode == MODE_UPLOAD) {
		process_data = process_upload_data;
		usbd->rx_len = sizeof(rdx_probe);
	} else {
		process_data = process_download_data;
	}

	/* receive the data from Host PC */
	while (1) {
		usbd->recv_setup(usbd->rx_data, usbd->rx_len);

		ret = usbd->recv_data();
		if (ret > 0) {
			ret = process_data(usbd);
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

	return 0;
}

U_BOOT_CMD(usbdown, CONFIG_SYS_MAXARGS, 1, do_usbd_down,
	"USB Downloader for SLP",
	"usbdown <force> - download binary images (force - don't check target)\n"
	"usbdown upload - upload debug info"
);
