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
#include <asm/errno.h>
#include <mobile/header.h>
#include <mobile/pit.h>
#include <mobile/fota.h>
#include <mobile/secure.h>
#include <image.h>
#include <fat.h>

#ifndef CONFIG_CMD_PIT
#error "mobile misc need to define the CONFIG_CMD_PIT"
#endif

#ifndef CONFIG_SECURE_BOOTING
#define SECURE_KEY_SIZE		0
#endif

enum {
	OPS_READ,
	OPS_WRITE,
	OPS_ERASE,
};

extern struct pitpart_data pitparts[];

#ifdef CONFIG_CMD_ONENAND
extern int do_onenand(cmd_tbl_t * cmdtp, int flag, int argc,
		      char *const argv[]);
static int nand_cmd(int ops, u32 addr, u32 ofs, u32 len)
{
	int ret;
	char cmd[8], arg1[12], arg2[12], arg3[12];
	char *const argv[] = { "onenand", cmd, arg1, arg2, arg3 };
	int argc = 0;

	sprintf(arg1, "%x", addr);
	sprintf(arg2, "%x", ofs);
	sprintf(arg3, "%x", len);

	switch (ops) {
	case OPS_ERASE:
		sprintf(cmd, "erase");
		argc = 4;
		break;
	case OPS_READ:
		sprintf(cmd, "read");
		argc = 5;
		break;
	case OPS_WRITE:
		sprintf(cmd, "write");
		argc = 5;
		break;
	default:
		printf("Error: wrong cmd on OneNAND\n");
		break;
	}

	ret = do_onenand(NULL, 0, argc, argv);
	if (ret)
		printf("Error: NAND Command\n");

	return ret;
}
#endif

#ifdef CONFIG_GENERIC_MMC
#include <mmc.h>

static struct mmc *mmc;

static int mmc_cmd(int ops, u64 start, u64 cnt, void *addr)
{
	int ret;
	int dev_num = CONFIG_MMC_DEFAULT_DEV;

	mmc = find_mmc_device(dev_num);
	if (mmc == NULL) {
		printf("error: mmc isn't exist\n");
		return 1;
	}

	start /= mmc->read_bl_len;
	cnt /= mmc->read_bl_len;

	/*printf("mmc %s 0x%llx 0x%llx\n", ops ? "write" : "read", start, cnt); */

	if (ops)
		ret =
		    mmc->block_dev.block_write(dev_num, (u32) start, (u32) cnt,
					       addr);
	else
		ret =
		    mmc->block_dev.block_read(dev_num, (u32) start, (u32) cnt,
					      addr);

	if (ret > 0)
		ret = 0;
	else
		ret = 1;

	return ret;
}
#endif

int set_valid_flag(int index, u32 addr, int valid)
{
	int ret = 0;
	struct boot_header *bh;
	image_header_t *hdr;

	printf("flag %s on %s\n", valid ? "set" : "clear",
	       pitparts[index].name);

	switch (pitparts[index].dev_type) {
	case PIT_DEVTYPE_ONENAND:
		bh = (struct boot_header *)addr;
		if (bh->magic == HDR_BOOT_MAGIC)
			bh->valid = valid;
		else if (bh->magic == HDR_KERNEL_MAGIC)
			bh->valid = valid;
		else
			ret = 1;
		break;
	case PIT_DEVTYPE_MMC:
	case PIT_DEVTYPE_FILE:
		if (!strncmp(pitparts[index].name, PARTS_BOOTLOADER, 6)) {
			bh = (struct boot_header *)
			    (addr + (u32) pitparts[index].size - HDR_SIZE);
		} else if (!strncmp(pitparts[index].name, PARTS_KERNEL, 6)) {
			hdr = (image_header_t *)addr;
			bh = (struct boot_header *)(addr +
				ALIGN(image_get_data_size(hdr) + image_get_header_size(), FS_BLOCK_SIZE) +
				SECURE_KEY_SIZE);
		}

		if (bh->magic == HDR_BOOT_MAGIC)
			bh->valid = valid;
		else if (bh->magic == HDR_KERNEL_MAGIC)
			bh->valid = valid;
		else
			ret = 1;
		break;
	default:
		break;
	}

	if (ret)
		printf("error: can't found magic code(0x%08x) at 0x%x for %s",
		       bh->magic, (u32) bh, pitparts[index].name);

	return ret;
}

static int check_valid_flag(int index, char *opt)
{
	int ret;
	u64 offset;
	u32 length;
	char cmd[64];
	u32 ramaddr = CONFIG_SYS_DOWN_ADDR;
	u8 buf[FS_BLOCK_SIZE];
	struct boot_header *bh = (struct boot_header *)buf;
	image_header_t *hdr = (image_header_t *)buf;

	/* caution! support only mmc */
	if (pitparts[index].dev_type == PIT_DEVTYPE_MMC) {
		offset = pitparts[index].offset;

		if (!strncmp(pitparts[index].name, PARTS_BOOTLOADER, 6))
			length = ((u32) pitparts[index].size) - FS_BLOCK_SIZE;
		else if (!strncmp(pitparts[index].name, PARTS_KERNEL, 6)) {
			mmc_cmd(OPS_READ, offset , sizeof(image_header_t),
				(void *)hdr);
			length = ALIGN(image_get_data_size(hdr) +
					image_get_header_size(), FS_BLOCK_SIZE);
			length = ALIGN(length + SECURE_KEY_SIZE, FS_BLOCK_SIZE);
		}

		mmc_cmd(OPS_READ, offset + length, FS_BLOCK_SIZE, (void *)buf);

		if ((bh->magic == HDR_BOOT_MAGIC) && bh->valid) {
			debug("found header: ");
			debug("board (%s) version (%s) date (%s)\n",
			      bh->boardname, bh->version, bh->date);
			return 0;
		}
	}

	if (pitparts[index].dev_type == PIT_DEVTYPE_FILE) {
		sprintf(cmd, "ext4load mmc %d:%s %x /%s%s",
			CONFIG_MMC_DEFAULT_DEV, getenv("mmcbootpart"),
			ramaddr, pitparts[index].file_name, opt);

		ret = run_command(cmd, 0);
		if (ret < 0) {
			printf("cmd: %s\n", cmd);
			return 1;
		}

		length = (u32)simple_strtoul(getenv("filesize"), NULL, 16);
		bh = (struct boot_header *)(ramaddr + length - FS_BLOCK_SIZE);

		if ((bh->magic == HDR_KERNEL_MAGIC) && bh->valid) {
			debug("found header: ");
			debug("board (%s) version (%s) date (%s)\n",
			      bh->boardname, bh->version, bh->date);
			return 0;
		}
	}

	printf("Error: can't found bootable partition for %s%s\n",
	       pitparts[index].file_name, opt);

	return 1;
}

int backup_bootable_part(void)
{
	int ret;
	int index, index_bak;
	u32 length;
	char cmd[64];
	u32 ramaddr = CONFIG_SYS_DOWN_ADDR;
	u8 buf[FS_BLOCK_SIZE];
	image_header_t *hdr = (image_header_t *)buf;

	printf("\nbackup start\n");

	/* bootloader */
	index = get_pitpart_id(PARTS_BOOTLOADER "-mmc");
	index_bak = get_pitpart_id(PARTS_BOOTLOADER "-mmc-bak");
	if (!check_valid_flag(index, "")) {
		printf("bootloader backup from 1st to 2nd\n");
		mmc_cmd(OPS_READ,
			pitparts[index].offset, pitparts[index].size,
			(void *)ramaddr);

		mmc_cmd(OPS_WRITE,
			pitparts[index_bak].offset, pitparts[index_bak].size,
			(void *)ramaddr);

		/* clear the valid flag for origin */
		set_valid_flag(index, ramaddr, 0);
		mmc_cmd(OPS_WRITE,
			pitparts[index].offset, pitparts[index].size,
			(void *)ramaddr);
	} else if (!check_valid_flag(index_bak, "-bak")) {
		printf("bootloader restore from 2nd to 1st\n");
		mmc_cmd(OPS_READ,
			pitparts[index_bak].offset, pitparts[index_bak].size,
			(void *)ramaddr);

		/* clear the valid flag for origin */
		set_valid_flag(index, ramaddr, 0);
		mmc_cmd(OPS_WRITE,
			pitparts[index].offset, pitparts[index].size,
			(void *)ramaddr);
	} else {
		printf("bootloader fail..\n");
	}

	/* kernel */
	index = get_pitpart_id(PARTS_KERNEL);
	if (!check_valid_flag(index, "")) {
		printf("kernel backup from 1st to 2nd\n");

		/* already loaded */

		length = (u32)simple_strtoul(getenv("filesize"), NULL, 16);

		sprintf(cmd, "ext4write mmc %d:%s /%s-bak 0x%x %d",
			CONFIG_MMC_DEFAULT_DEV, getenv("mmcbootpart"),
			pitparts[index].file_name, ramaddr, length);

		ret = run_command(cmd, 0);
		if (ret < 0) {
			printf("cmd: %s\n", cmd);
			return 1;
		}

		/* clear the valid flag for origin */
		set_valid_flag(index, ramaddr, 0);
		sprintf(cmd, "ext4write mmc %d:%s /%s 0x%x %d",
			CONFIG_MMC_DEFAULT_DEV, getenv("mmcbootpart"),
			pitparts[index].file_name, ramaddr, length);

		ret = run_command(cmd, 0);
		if (ret < 0) {
			printf("cmd: %s\n", cmd);
			return 1;
		}
	} else if (!check_valid_flag(index, "-bak")) {
		printf("kernel restore from 2nd to 1st\n");

		/* already loaded */

		length = (u32)simple_strtoul(getenv("filesize"), NULL, 16);

		/* clear the valid flag for origin */
		set_valid_flag(index, ramaddr, 0);
		sprintf(cmd, "fatwrite mmc %d:%s 0x%x %s 0x%x",
			CONFIG_MMC_DEFAULT_DEV, getenv("mmcbootpart"), ramaddr,
			pitparts[index].file_name, length);

		ret = run_command(cmd, 0);
		if (ret < 0) {
			printf("cmd: %s\n", cmd);
			return 1;
		}
	} else {
		printf("kernel fail..\n");
	}

	printf("\nbackup done\n");
	return 0;
}

int check_kernel_part(void)
{
	int index = get_pitpart_id(PARTS_KERNEL);
	char buf[16];

	if (!check_valid_flag(index, "")) {
		/* do nothing */
	} else if (!check_valid_flag(index, "-bak")) {
		sprintf(buf, "%s%s", pitparts[index].file_name, "-bak");
		setenv("kernelname", buf);
	} else {
		printf("Can't found bootable kernel\n");
		/* XXX: this will be enabled on May 2011 */
		/* setenv("loaduimage", NULL);
		   return 1; */
	}

	return 0;
}
