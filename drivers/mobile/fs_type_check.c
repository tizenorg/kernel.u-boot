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
#include <mbr.h>
#include <fat.h>
#include <mobile/fs_type_check.h>

struct fs_check {
	unsigned int type;
	char *name;
	unsigned int offset;
	unsigned int magic_sz;
	char magic[16];
};

static struct fs_check fs_types[2] = {
	{
		FS_TYPE_EXT4,
		"ext4",
		0x438,
		2,
		{0x53, 0xef}
	},
	{
		FS_TYPE_VFAT,
		"vfat",
		0x52,
		5,
		{'F', 'A', 'T', '3', '2'}
	},
};

static unsigned long part_offset = 0;

int fstype_register_device(block_dev_desc_t * dev_desc, int part_no)
{
	struct mbr mbr;
	disk_partition_t info;

	if (!dev_desc->block_read)
		return -1;

	if (dev_desc->block_read(dev_desc->dev, 0, 1, (void *)&mbr) != 1) {
		printf("** Can't read from device %d **\n", dev_desc->dev);
		return -1;
	}

	if (mbr.signature != 0xaa55) {
		printf("** signature error 0x%x **\n", mbr.signature);
		return -1;
	}

	if (!get_partition_info(dev_desc, part_no, &info)) {
		part_offset = info.start;
	} else {
		printf("** Partition %d not valid on device %d **\n",
		       part_no, dev_desc->dev);
		return -1;
	}

	return 0;
}

int fstype_check(block_dev_desc_t * dev_desc)
{
	int i;
	u32 ofs;
	u8 buf[FS_BLOCK_SIZE];

	if (!dev_desc->block_read)
		return -1;

	for (i = 0; i < ARRAY_SIZE(fs_types); i++) {
		ofs = fs_types[i].offset;

		dev_desc->block_read(dev_desc->dev,
				     part_offset + ofs / FS_BLOCK_SIZE, 1,
				     (void *)buf);

		if (!memcmp(&buf[ofs % FS_BLOCK_SIZE], fs_types[i].magic,
			    fs_types[i].magic_sz)) {
			printf("found %s\n", fs_types[i].name);
			return fs_types[i].type;
		}
	}

	return -1;
}

int do_fstype_check(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int dev = 0;
	int part = 1;
	char *ep;
	block_dev_desc_t *dev_desc = NULL;

	if (argc < 2) {
		printf("usage: fatformat <interface> <dev[:part]>\n");
		return 0;
	}

	dev = (int)simple_strtoul(argv[2], &ep, 16);

	dev_desc = get_dev(argv[1], dev);

	if (dev_desc == NULL) {
		puts("\n** invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts("\n** invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fstype_register_device(dev_desc, part)) {
		printf("\n** Unable to use %s %d:%d to check filesystem **\n",
		       argv[1], dev, part);
		return 1;
	}

	printf("checking..\n");
	if (fstype_check(dev_desc) < 0)
		return 1;

	return 0;
}

U_BOOT_CMD(
	fstype, 3, 1, do_fstype_check,
	"Check Filesystem Type",
	"<interface> <dev[:part]> - check filesystem\n"
);
