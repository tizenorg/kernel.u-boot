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
#include <command.h>
#include <asm/errno.h>
#include <mmc.h>
#include <pit.h>

#include "Tizen_GPT_Ver08.h"

enum {
	NAND_ERASE = 0,
	NAND_WRITE,
	NAND_WRITE_YAFFS,
	NAND_LOCK,
	NAND_READ,
};

enum {
	MMC_READ = 0,
	MMC_WRITE
};

#define isdigit(c)  ('0' <= (c) && (c) <= '9')

static u8 pit[PIT_PKT_SIZE];
static char *noname = "NULL";

struct pitpart_data pitparts[PIT_PART_MAX];

static struct mmc *mmc;

static int partition_index;
static int partition_count;
static int fileparts_count;

static int pit_is_08_forced;

static int check_pit_integrity(unsigned char *addr)
{
	pit_header_t *hd = (pit_header_t *)addr;

	if (hd->magic != PIT_MAGIC)
		return 1;

	return 0;
}

int get_pitpart_count(void)
{
	pit_header_t *hd = (pit_header_t *)pit;

	return hd->count;
}

static int mmc_cmd(int ops, lbaint_t start, lbaint_t cnt, void *addr)
{
	int ret;
	int dev_num = CONFIG_SYS_MMC_PIT_DEV;

	mmc = find_mmc_device(dev_num);
	if (mmc == NULL) {
		printf("error: mmc isn't exist\n");
		return 1;
	}

	start /= mmc->read_bl_len;
	cnt /= mmc->read_bl_len;

	if (ops)
		ret =
		    mmc->block_dev.block_write(dev_num, start, cnt, addr);
	else
		ret =
		    mmc->block_dev.block_read(dev_num, start, cnt, addr);

	if (ret > 0)
		ret = 0;
	else
		ret = 1;

	return ret;
}

int pit_check_ver07(u8 *buf)
{
	pit_header_t *hd = (pit_header_t *)buf;
	char *gangfw = (char *)hd->gang_format;

	if (strncmp(gangfw, "COM_TAR2", 8))
		return 1;
	else
		return 0;
}

/* get index of first mountable partition [dev:1] in mmc */
int pit_get_partition_base(void)
{
	return partition_index;
}

/* return GPT partition id from pit id */
int pit_adjust_id_on_mmc(int index)
{
	int boot_id;
	int part_base;

	part_base = pit_get_partition_base();
	boot_id = (index - part_base) + 1;
	printf("Pit id is adjusted by GPT (%d -> %d)\n",
	       index, boot_id);

	return boot_id;
}

static int set_valid_pit(unsigned char *addr)
{
	pit_header_t *hd;

	hd = (pit_header_t *)addr;
	if (hd->magic == PIT_MAGIC) {
		/* copy to local buffer */
		memcpy(pit, addr, PIT_PKT_SIZE);
		return 0;
	}

	printf("Error: cann't found pit from 0x%x\n", CONFIG_PIT_DEFAULT_ADDR);
	return 1;
}

static void show_pit_info(void)
{
	int i;
	pit_header_t *hd;
	partition_info_t *pi;

	hd = (pit_header_t *)pit;
	pi = (partition_info_t *)(pit + sizeof(pit_header_t));
	printf("Partition Info Table\n");
	printf(" #: T  dev  id   offset     size  file\n");
	for (i = 0; i < hd->count; i++, pi++) {
		printf("%2d: %s %s %2d %8d %8d  %s\n", i,
		       pi->bin_type ? "CP" : "AP",
		       pi->dev_type ?
		       ((pi->dev_type == 2) ? "MMC " : "File") : "OND ",
		       pi->id, pi->blk_start, pi->blk_num, pi->file_name);
	}
}

static int do_pit_info(cmd_tbl_t *cmdtp, int flag, int argc,
		       char *const argv[])
{
	/* 'pit update' is run once before this */
	if (check_pit_integrity(pit))
		return 1;

	show_pit_info();

	return 0;
}

static int do_pit_read(cmd_tbl_t *cmdtp, int flag, int argc,
		       char *const argv[])
{
	int ret = 1;
	unsigned char *addr;

	if (argc < 2) {
		cmd_usage(cmdtp);
		return 1;
	} else {
		addr = (unsigned char *)simple_strtoul(argv[1], NULL, 16);
	}

	/* read only once from FLASH */
	if (check_pit_integrity(pit) == 0) {
		memcpy(addr, pit, PIT_PKT_SIZE);
		return 0;
	}

	ret = mmc_cmd(MMC_READ,
		      CONFIG_PIT_DEFAULT_ADDR, CONFIG_PIT_DEFAULT_SIZE,
		      (void *)addr);

	if (check_pit_integrity(addr))
		return ret;

	/* force update to PIT 08 when target used PIT 07 */
	if (pit_check_ver07(addr)) {
		puts("Force update pit from 07 to 08\n");
		memcpy(addr, &default_pit08, PIT_PKT_SIZE);
		pit_is_08_forced = 1;
		mmc_cmd(MMC_WRITE, CONFIG_PIT_DEFAULT_ADDR,
			PIT_PKT_SIZE, (void *)addr);
	}

	return ret;
}

int get_pitpart_id(char *name)
{
	int i;

	for (i = 0; pitparts[i].valid; i++) {
		if (!strcmp(pitparts[i].name, name))
			return i;
	}

	printf("Can't found partition (%s)\n", name);
	return -1;
}

int get_pitpart_id_by_filename(char *file_name)
{
	int i;

	for (i = 0; pitparts[i].valid; i++) {
		if (!strcmp(pitparts[i].file_name, file_name))
			return i;
		/* workaround for patch install files */
		if (!strncmp(file_name, "install-", 8)) {
			if (!strncmp(pitparts[i].file_name, file_name, 8))
				return i;
		}
	}

	printf("Can't found partition (%s)\n", file_name);
	return -1;
}

int get_pitpart_id_for_pit(char *file_name)
{
	int i;

	for (i = 0; pitparts[i].valid; i++) {
		if (strstr(pitparts[i].file_name, "pit"))
			if (pitparts[i].dev_type == PIT_DEVTYPE_MMC)
				return i;
	}

	printf("Can't found partition (%s)\n", file_name);
	return -1;
}

char *get_pitpart_name(int id)
{
	return pitparts[id].name;
}

u32 get_pitpart_size(int id)
{
	return pitparts[id].size;
}

int pit_no_partition_support(void)
{
	if (partition_count <= 0)
		return 1;

	if (getenv("mbrparts") == NULL)
		return 1;

	return 0;
}

static int name_to_version(char *name)
{
	int i = 0;
	char ver[8] = {0, };

	while (*name) {
		if (isdigit(*name))
			ver[i++] = *name;
		name++;
	}

	return simple_strtol(ver, NULL, 10);
}

int pit_get_version(void)
{
	int id;

	id = get_pitpart_id("pit");
	if (id < 0)
		return 0;

	return name_to_version(pitparts[id].file_name);
}

/*
 * return: positive means up version, negavite means down version.
 */
int pit_check_version(char *file_name)
{
	int old_ver, new_ver;

	old_ver = pit_get_version();
	if (old_ver == 0)
		return 0;

	new_ver = name_to_version(file_name);

	return new_ver - old_ver;
}

#define MBR_RESERVED_SIZE	0x8000

static int do_pit_write(cmd_tbl_t *cmdtp, int flag, int argc,
			char *const argv[])
{
	int ret;
	unsigned char *addr;
	int id;
	u32 offset;

	if (argc < 2) {
		cmd_usage(cmdtp);
		return 1;
	} else {
		addr = (unsigned char *)simple_strtoul(argv[1], NULL, 16);
	}

	id = get_pitpart_id("pit");
	if (id == -1)
		return 1;

	offset = pitparts[id].offset;

	switch (pitparts[id].dev_type) {
	case PIT_DEVTYPE_MMC:
		if (offset == 0)
			offset = MBR_RESERVED_SIZE;
		ret = mmc_cmd(MMC_WRITE, offset, PIT_PKT_SIZE, (void *)addr);
		break;
	default:
		printf("Error: not supported device (%d)\n",
		       pitparts[id].dev_type);
		return 1;
	}

	return ret;
}

static int get_pitpart_info(void)
{
	int i;
	pit_header_t *hd;
	partition_info_t *pi;

	if (check_pit_integrity(pit))
		return 1;

	hd = (pit_header_t *)pit;
	pi = (partition_info_t *)(pit + sizeof(pit_header_t));

	if (hd->count > PIT_PART_MAX) {
		printf("Error: too many pit info (%d > %d)\n",
		       hd->count, PIT_PART_MAX);
		return 1;
	}

	memset(pitparts, 0, sizeof(pitparts));
	fileparts_count = 0;
	partition_count = 0;

	for (i = 0; i < hd->count; i++, pi++) {
		pitparts[i].dev_type = pi->dev_type;
		pitparts[i].id = pi->id;
		pitparts[i].part_type = pi->part_type;
		pitparts[i].filesys = pi->filesys;

		if (pi->name)
			pitparts[i].name = pi->name;
		else
			pitparts[i].name = noname;

		pitparts[i].file_name = pi->file_name;
		pitparts[i].size = pi->blk_num * 512;
		pitparts[i].valid = 1;

		if (pi->dev_type == PIT_DEVTYPE_MMC) {
			pitparts[i].offset = pi->blk_start * 512;

			/*
			 * file system determine partition usage
			 * between moutable part and hidden part.
			 * If file system is exist in partitions,
			 * the partition regards as mountable partition.
			 */
			if ((pi->filesys == PART_FS_TYPE_ENHANCED) ||
			    (pi->filesys == PART_FS_TYPE_EXT4)) {
				partition_count++;
			} else {
				partition_index = i + 1;
			}
		} else if (pi->dev_type == PIT_DEVTYPE_FILE) {
			pitparts[i].offset = 0;
			fileparts_count++;
		}
	}

	return 0;
}

static int count_mbr;
static void set_mmcparts(char *buf, unsigned long long size, char *name, int id)
{
	if (!strcmp(name, "ums")) {
		/* UMS is the last partition */
		count_mbr += sprintf(buf + count_mbr, "-(%s)", name);
	} else {
		if (size % 1024 == 0) {
			size /= 1024;
			if (size % 1024 == 0)
				count_mbr += sprintf(buf + count_mbr, "%llum",
						size / 1024);
			else
				count_mbr +=
					sprintf(buf + count_mbr, "%lluk", size);
		} else {
			count_mbr += sprintf(buf + count_mbr, "%llu", size);
		}
		count_mbr += sprintf(buf + count_mbr, "(%s),", name);
	}
}

static void update_parts(void)
{
	int i;
	unsigned long long size;
	char mbr_parts[256] = { 0, };
	pit_header_t *hd;
	partition_info_t *pi;
	int partition_base = pit_get_partition_base();

	hd = (pit_header_t *)pit;
	pi = (partition_info_t *)(pit + sizeof(pit_header_t));

	count_mbr = 0;
	for (i = 0; i < hd->count; i++, pi++) {
		size = pi->blk_num * 512;

		if (pi->dev_type == PIT_DEVTYPE_MMC) {
			if (i < partition_base)
				continue;

			set_mmcparts(mbr_parts, size, pi->name, pitparts[i].id);
		}
	}

	setenv("mbrparts", mbr_parts);
}

static int do_pit_update(cmd_tbl_t *cmdtp, int flag, int argc,
			 char *const argv[])
{
	int ret;
	unsigned char *addr;

	addr = (unsigned char *)simple_strtoul(argv[1], NULL, 16);

	if (set_valid_pit(addr))
		return 1;

	ret = get_pitpart_info();

	update_parts();

	return ret;
}

static cmd_tbl_t cmd_pit_sub[] = {
	U_BOOT_CMD_MKENT(info, 1, 0, do_pit_info, "", ""),
	U_BOOT_CMD_MKENT(read, 2, 0, do_pit_read, "", ""),
	U_BOOT_CMD_MKENT(write, 2, 0, do_pit_write, "", ""),
	U_BOOT_CMD_MKENT(update, 1, 0, do_pit_update, "", ""),
};

int do_pit(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	/* Strip off leading 'onenand' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_pit_sub[0], ARRAY_SIZE(cmd_pit_sub));

	if (c)
		return c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	pit, 4, 1, do_pit,
	"PIT sub-system",
	"info - show PIT(Partition Info Table) data\n"
	"pit read addr - read PIT to addr\n"
	"pit write addr - write PIT from addr\n"
	"pit update <addr> - update mtd partition info (using addr as buffer)"
);

void check_pit(void)
{
	char buf[64];

	sprintf(buf, "pit read %x", CONFIG_PIT_DOWN_ADDR);
	run_command(buf, 0);

	sprintf(buf, "pit update %x", CONFIG_PIT_DOWN_ADDR);
	run_command(buf, 0);
}

/*
 * set dfu_alt_info_# using pit information
 *
 * The result of the conversion of PIT to dfu_alt_info is too long
 * So, Split the dfu_alt_info environment to dfu_alt_info_#
 *
 * dfu_alt_group: a number of dfu_alt_info_#
 * dfu_alt_num: a number of total dfu_alt_info entity
 */
#define SPLIT_NUM 10

void pit_to_dfu_alt_info(void)
{
	int i;
	char dfu_alt_info[256] = { 0, };
	char dfu_alt_group[4] = { 0, };
	char dfu_alt_num[4] = { 0, };

	int count_buf = 0;
	int part_num = 0;

	int env_count = 0;

	pit_header_t *hd;
	partition_info_t *pi;

	hd = (pit_header_t *)pit;
	pi = (partition_info_t *)(pit + sizeof(pit_header_t));

	for (i = 0; i < hd->count; i++, pi++) {
		char dev_name[8] = { 0, };

		unsigned int blk_num = 0;
		unsigned int blk_start = 0;

		if (pi->dev_type == PIT_DEVTYPE_MMC) {
			strcpy(dev_name, "mmc");
			blk_num = pi->blk_num;
			blk_start = pi->blk_start;
		} else {
			strcpy(dev_name, "ext4");
			blk_num = ++part_num;
			blk_start = 0;
		}

		/* ums */
		if (!strcmp(pi->name, PARTS_UMS)) {
			int tmp_id = get_pitpart_id(PARTS_UMS);
			strcpy(dev_name, "part");
			blk_num = pit_adjust_id_on_mmc(tmp_id);
			blk_start = 0;
		}

		/* set dfu_alt_info env value */
		count_buf += sprintf(dfu_alt_info + count_buf,
				      "%s %s %x %x",
				       pi->file_name, dev_name,
				       blk_start, blk_num);

		/* split into SPLIT_NUM */
		if ((i+1) % SPLIT_NUM) {
			count_buf += sprintf(dfu_alt_info + count_buf, ";");
		} else {
			char env_name[32] = { 0, };

			count_buf += sprintf(dfu_alt_info + count_buf, "\n");
			sprintf(env_name, "dfu_alt_info_%d", env_count);
			setenv(env_name, dfu_alt_info);

			env_count++;
			count_buf = 0;
			memset(dfu_alt_info, 0, sizeof(dfu_alt_info));
		}
	}

	/* set remained environment */
	if (count_buf > 0) {
		char env_name[32] = { 0, };

		sprintf(dfu_alt_info + count_buf-1, "\n");
		sprintf(env_name, "dfu_alt_info_%d", env_count);

		setenv(env_name, dfu_alt_info);
		env_count++;
	}

	/* a number of dfu_alt_info_# */
	sprintf(dfu_alt_group, "%d", env_count);
	setenv("dfu_alt_group", dfu_alt_group);

	/* a number of total dfu_alt_entity */
	sprintf(dfu_alt_num, "%d", hd->count);
	setenv("dfu_alt_num", dfu_alt_num);
}

/*
 * access: selects partitions to access
 *	0x0 - No access to boot partition (default)
 *	0x1 - R/W boot partition 1
 *	0x2 - R/W boot partition 2
 *	0x3 - R/W Replay Protected Memory Block (RPMB)
 */
int pit_mmc_boot_part_access(char *file_name, u8 access)
{
	int pit_idx;
	int part_id;

	pit_idx = get_pitpart_id_by_filename(file_name);
	if (pit_idx < 0)
		return -1;

	/* Unnecessary except s-boot */
	if (strcmp(pitparts[pit_idx].name, "s-boot-mmc"))
		return -1;

	part_id = pitparts[pit_idx].id;

	/* part_id > 80 */
	if (part_id >= PIT_BOOTPART0_ID) {
		struct mmc *mmc = find_mmc_device(0);
		int part_num = PIT_BOOTPARTN_GET(part_id);

		if (mmc_boot_part_access(mmc, 1, part_num, access) == 0)
			return 0;
		else
			return -1;
	} else {
		return -1;
	}

	return 0;
}
