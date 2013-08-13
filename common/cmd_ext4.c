/*
 * (C) Copyright 2011 - 2012 Samsung Electronics
 * EXT4 filesystem implementation in Uboot by
 * Uma Shankar <uma.shankar@samsung.com>
 * Manjunatha C Achar <a.manjunatha@samsung.com>
 *
 * Ext4fs support
 * made from existing cmd_ext2.c file of Uboot
 *
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * made from cmd_reiserfs by
 *
 * (C) Copyright 2003 - 2004
 * Sysgo Real-Time Solutions, AG <www.elinos.com>
 * Pavel Bartusek <pba@sysgo.com>
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

/*
 * Changelog:
 *	0.1 - Newly created file for ext4fs support. Taken from cmd_ext2.c
 *	        file in uboot. Added ext4fs ls load and write support.
 */

#include <common.h>
#include <part.h>
#include <config.h>
#include <command.h>
#include <image.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <ext4fs.h>
#include <linux/stat.h>
#include <malloc.h>
#include <fs.h>

#if defined(CONFIG_CMD_USB) && defined(CONFIG_USB_STORAGE)
#include <usb.h>
#endif

int do_ext4_load(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	return do_load(cmdtp, flag, argc, argv, FS_TYPE_EXT, 16);
}

int do_ext4_ls(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	return do_ls(cmdtp, flag, argc, argv, FS_TYPE_EXT);
}

#if defined(CONFIG_CMD_EXT4_WRITE)
int do_ext4_write(cmd_tbl_t *cmdtp, int flag, int argc,
				char *const argv[])
{
	const char *filename = "/";
	int dev, part;
	unsigned long ram_address;
	unsigned long file_size;
	disk_partition_t info;
	block_dev_desc_t *dev_desc;

	if (argc < 6)
		return cmd_usage(cmdtp);

	part = get_device_and_partition(argv[1], argv[2], &dev_desc, &info, 1);
	if (part < 0)
		return 1;

	dev = dev_desc->dev;

	/* get the filename */
	filename = argv[4];

	/* get the address in hexadecimal format (string to int) */
	ram_address = simple_strtoul(argv[3], NULL, 16);

	/* get the filesize in base 10 format */
	file_size = simple_strtoul(argv[5], NULL, 10);

	/* set the device as block device */
	ext4fs_set_blk_dev(dev_desc, &info);

	/* mount the filesystem */
	if (!ext4fs_mount(info.size)) {
		printf("Bad ext4 partition %s %d:%d\n", argv[1], dev, part);
		goto fail;
	}

	/* start write */
	if (ext4fs_write(filename, (unsigned char *)ram_address, file_size)) {
		printf("** Error ext4fs_write() **\n");
		goto fail;
	}
	ext4fs_close();

	return 0;

fail:
	ext4fs_close();

	return 1;
}

U_BOOT_CMD(ext4write, 6, 1, do_ext4_write,
	"create a file in the root directory",
	"<interface> <dev[:part]> <addr> <absolute filename path> [sizebytes]\n"
	"    - create a file in / directory");

#endif

U_BOOT_CMD(ext4ls, 4, 1, do_ext4_ls,
	   "list files in a directory (default /)",
	   "<interface> <dev[:part]> [directory]\n"
	   "    - list files from 'dev' on 'interface' in a 'directory'");

U_BOOT_CMD(ext4load, 6, 0, do_ext4_load,
	   "load binary file from a Ext4 filesystem",
	   "<interface> <dev[:part]> [addr] [filename] [bytes]\n"
	   "    - load binary file 'filename' from 'dev' on 'interface'\n"
	   "      to address 'addr' from ext4 filesystem.\n"
	   "      All numeric parameters are assumed to be hex.");

static int total_sector;

int do_ext4_format(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int part;
	disk_partition_t info;
	block_dev_desc_t *dev_desc;

	if (argc < 3)
		return cmd_usage(cmdtp);

	part = get_device_and_partition(argv[1], argv[2], &dev_desc, &info, 1);

	if (part < 0)
		return 1;

	/* set the device as block device */
	ext4fs_set_blk_dev(dev_desc, &info);
	total_sector = (info.size * info.blksz) / SECTOR_SIZE;

	printf("formatting...\n\n");

	if (mkfs_ext4(dev_desc, part))
		return 1;

	return 0;
}


U_BOOT_CMD(ext4format, 3, 1, do_ext4_format,
	   "format device as EXT4 filesystem",
	   "<interface> <dev[:part]>\n"
	   "	  - format device as EXT4 filesystem on 'dev'");

void put_ext4fs(block_dev_desc_t *dev_desc,
			uint64_t off, void *buf, uint32_t size)
{
	uint64_t startblock, remainder;
	unsigned int sector_size = 512;
	unsigned char *temp_ptr = NULL;
	char sec_buf[SECTOR_SIZE];

	startblock = off / (uint64_t)sector_size;
	startblock += part_offset;
	remainder = off % (uint64_t)sector_size;
	remainder &= SECTOR_SIZE - 1;

	if (dev_desc == NULL)
		return;

	if ((startblock + (size/SECTOR_SIZE)) > (part_offset + total_sector)) {
		printf("part_offset is %lu\n", part_offset);
		printf("total_sector is %u\n", total_sector);
		printf("error: overflow occurs\n");
		return;
	}

	if (remainder) {
		if (dev_desc->block_read) {
			dev_desc->block_read(dev_desc->dev, startblock, 1,
				(unsigned char *)sec_buf);
			temp_ptr = (unsigned char *)sec_buf;
			memcpy((temp_ptr + remainder),
			       (unsigned char *)buf, size);
			dev_desc->block_write(dev_desc->dev, startblock, 1,
				(unsigned char *)sec_buf);
		}
	} else {
		if (size/SECTOR_SIZE != 0)	{
			dev_desc->block_write(dev_desc->dev, startblock,
			size/SECTOR_SIZE, (unsigned long *)buf);
		} else {
			dev_desc->block_read(dev_desc->dev, startblock, 1,
			(unsigned char *)sec_buf);
			temp_ptr = (unsigned char *)sec_buf;
			memcpy(temp_ptr, buf, size);
			dev_desc->block_write(dev_desc->dev, startblock, 1,
					(unsigned long *)sec_buf);
		}
	}

	return;
}

