/*
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
 * Ext4fs support
 */
#include <common.h>
#include <part.h>
#include <config.h>
#include <command.h>
#include <image.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <ext2fs.h>
#include <ext4fs.h>
#include <linux/stat.h>
#include "../disk/part_dos.h"
#include <malloc.h>

#if defined(CONFIG_CMD_USB) && defined(CONFIG_USB_STORAGE)
#include <usb.h>
#endif

#if !defined(CONFIG_DOS_PARTITION) && !defined(CONFIG_EFI_PARTITION)
#error DOS or EFI partition support must be selected
#endif

static int total_sector;
static unsigned long part_offset = 0;
static unsigned long part_size = 0;

static int cur_part = 1;

#define DOS_PART_MAGIC_OFFSET 0x1fe
#define DOS_FS_TYPE_OFFSET 0x36
#define DOS_FS32_TYPE_OFFSET	0x52


/*main entrance of mkfs.ext4 or ext4formating..*/
extern int mkfs_ext4(block_dev_desc_t *, int);

void put_ext4(block_dev_desc_t *dev_desc,uint64_t off, void *buf, uint32_t size)
{
	uint64_t startblock,remainder;
	unsigned int sector_size = 512;	
	unsigned char *temp_ptr=NULL;
	char sec_buf[SECTOR_SIZE];

	startblock = off / (uint64_t)sector_size;
	startblock += part_offset;
	remainder = off % (uint64_t)sector_size;
	remainder &= SECTOR_SIZE - 1;
	
	if (dev_desc == NULL)
		return ;

	if ((startblock + (size/SECTOR_SIZE)) > (part_offset +total_sector)) 
	{
		printf("part_offset is %lu\n", part_offset);
		printf("total_sector is %u\n", total_sector);
		printf("error: overflow occurs\n");
		return ;
	}

	if(remainder)
	{
		if (dev_desc->block_read) 
		{
			dev_desc->block_read(dev_desc->dev, startblock, 1,
				(unsigned char *) sec_buf);
			temp_ptr=(unsigned char*)sec_buf;
			memcpy((temp_ptr + remainder),(unsigned char*)buf,size);
			dev_desc->block_write(dev_desc->dev, startblock, 1,
				(unsigned char *)sec_buf);
		}
	}
	else
	{
		if(size/SECTOR_SIZE!=0)
		{
			dev_desc->block_write(dev_desc->dev, startblock,
			size/SECTOR_SIZE,(unsigned long *) buf);
		}
		else
		{
			dev_desc->block_read(dev_desc->dev, startblock, 1,
			(unsigned char *) sec_buf);
			temp_ptr=(unsigned char*)sec_buf;
			memcpy(temp_ptr,buf,size);
			dev_desc->block_write(dev_desc->dev, startblock,
			1,(unsigned long *) sec_buf);
		}
	}
	return;
}



int ext4_register_device (block_dev_desc_t * dev_desc, int part_no)
{
	unsigned char buffer[SECTOR_SIZE];

	disk_partition_t info;

	if (!dev_desc->block_read)
		return -1;

	/* check if we have a MBR (on floppies we have only a PBR) */
	if (dev_desc->block_read(dev_desc->dev, 0, 1, (ulong *)buffer) != 1) 
	{
		printf("** Can't read from device %d **\n",
			dev_desc->dev);
		return -1;
	}
	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		 buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) 
	{
		/* no signature found */
		return -1;
	}

	/* First we assume there is a MBR */
	if (!get_partition_info(dev_desc, part_no, &info)) 
	{
		part_offset = info.start;
		cur_part = part_no;
		part_size = info.size;
	}
	else if ((strncmp((char *)&buffer[DOS_FS_TYPE_OFFSET], "FAT", 3) == 0) ||
			(strncmp((char *)&buffer[DOS_FS32_TYPE_OFFSET], "FAT32", 5) == 0)) 
	{
		/* ok, we assume we are on a PBR only */
		cur_part = 1;
		part_offset = 0;
	}
	else 
	{
		printf("** Partition %d not valid on device %d **\n",
			part_no, dev_desc->dev);
		return -1;
	}

	return 0;
}



int do_ext4_load (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *filename = NULL;
	char *ep;
	int dev, part = 1;
	ulong addr = 0, part_length;
	int filelen;
	disk_partition_t info;
	block_dev_desc_t *dev_desc = NULL;
	char buf [12];
	unsigned long count;
	char *addr_str;

	switch (argc) {
	case 3:
		addr_str = getenv("loadaddr");
		if (addr_str != NULL)
			addr = simple_strtoul (addr_str, NULL, 16);
		else
			addr = CONFIG_SYS_LOAD_ADDR;

		filename = getenv ("bootfile");
		count = 0;
		break;
	case 4:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = getenv ("bootfile");
		count = 0;
		break;
	case 5:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = argv[4];
		count = 0;
		break;
	case 6:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = argv[4];
		count = simple_strtoul (argv[5], NULL, 16);
		break;

	default:
		return cmd_usage(cmdtp);
	}

	if (!filename) {
		puts ("** No boot file defined **\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		printf ("** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts ("** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	//PRINTF("Using device %s%d, partition %d\n", argv[1], dev, part);

	if (part != 0) {
		if (get_partition_info (dev_desc, part, &info)) {
			printf ("** Bad partition %d **\n", part);
			return 1;
		}

		if (strncmp((char *)info.type, BOOT_PART_TYPE, sizeof(info.type)) != 0) {
			printf ("** Invalid partition type \"%.32s\""
				" (expect \"" BOOT_PART_TYPE "\")\n",
				info.type);
			return 1;
		}
		printf ("Loading file \"%s\" "
			"from %s device %d:%d (%.32s)\n",
			filename,
			argv[1], dev, part, info.name);
	} else {
		printf ("Loading file \"%s\" from %s device %d\n",
			filename, argv[1], dev);
	}


	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",	argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	if (!ext4fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %s %d:%d **\n",
			argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	filelen = ext4fs_open(filename);
	if (filelen < 0) {
		printf("** File not found %s\n", filename);
		ext4fs_close();
		return 1;
	}
	if ((count < filelen) && (count != 0)) {
		 filelen = count;
	}

	if (ext4fs_read((char *)addr, filelen) != filelen) {
		printf("** Unable to read \"%s\" from %s %d:%d **\n",
			filename, argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	ext4fs_close();

	/* Loading ok, update default load address */
	load_addr = addr;

	printf ("%d bytes read\n", filelen);
	sprintf(buf, "%X", filelen);
	setenv("filesize", buf);

	return 0;
}




int do_ext4_ls (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *filename = "/";
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;
	int part_length;

	if (argc < 3)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (argc == 4)
		filename = argv[3];

	//PRINTF("Using device %s %d:%d, directory: %s\n", argv[1], dev, part, filename);

	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",	argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	if (!ext4fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	if (ext4fs_ls (filename)) {
		printf ("** Error ext2fs_ls() **\n");
		ext4fs_close();
		return 1;
	};

	ext4fs_close();

	return 0;
}

int do_ext4_format (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;
	int part_length;
	disk_partition_t info;

	if (argc < 3)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	if (*ep) 
	{
		if (*ep != ':') 
		{
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) 
	{
		printf ("** Bad partition - %s %d:%d **\n",  argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	if (ext4_register_device(dev_desc,part)!=0) 
	{
		printf("\n** Unable to use %s %d:%d for fattable **\n",
			argv[1], dev, part);
		return 1;
	}

	if (!get_partition_info(dev_desc, part, &info)) 
	{
		total_sector = (info.size * info.blksz) / SECTOR_SIZE;
	} else 
	{
		printf("error : get partition info\n");
		return 1;
	}

	printf("formatting...\n\n");
	if (mkfs_ext4(dev_desc, part))
		return 1;

	return 0;
}



int do_ext4_write(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	block_dev_desc_t *dev_desc=NULL;
	char *filename = "/";
	int part_length;
	int part=1;
	int dev=0;
	char *ep;
	unsigned long ram_address;
	unsigned long file_size;
	disk_partition_t info;

	if (argc < 6)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	/*get the filename*/
	filename = argv[3];

	/*get the address in hexadecimal format (string to int)*/
	ram_address = simple_strtoul (argv[4], NULL, 16);

	/*get the filesize in base 10 format*/
	file_size=simple_strtoul (argv[5], NULL, 10);

	/*set the device as block device*/
	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) 
	{
		printf ("** Bad partition - %s %d:%d **\n",	argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	/*register the device and partition*/
	if (ext4_register_device(dev_desc,part)!=0)
	{
		printf("\n** Unable to use %s %d:%d for fattable **\n",
			argv[1], dev, part);
		return 1;
	}	


	/*get the partition information*/
	if (!get_partition_info(dev_desc, part, &info))
	{		
		total_sector = (info.size * info.blksz) / SECTOR_SIZE;
	} 
	else 
	{
		printf("error : get partition info\n");
		return 1;
	}
	

	/*mount the filesystem*/
	if (!ext4fs_mount(part_length)) 
	{
		printf ("** Bad ext4 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	/*start write*/
	if (ext4fs_write(dev_desc,part,filename,(unsigned char*)ram_address,file_size))
	{
		printf ("** Error ext4fs_write() **\n");
		ext4fs_close();
		return 1;
	}
	return 0;
}

int do_ext4_create_dir(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *dirname = "/";
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;
	int part_length;
	disk_partition_t info;

	if (argc < 4)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (argc == 4)
		dirname = argv[3];

	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",	argv[1], dev, part);
		ext4fs_close();
		return 1;
	}
	/*register the device and partition*/
	if (ext4_register_device(dev_desc,part)!=0)
	{
		printf("\n** Unable to use %s %d:%d for fattable **\n",
			argv[1], dev, part);
		return 1;
	}	

	if (!get_partition_info(dev_desc, part, &info)) 
	{
		total_sector = (info.size * info.blksz) / SECTOR_SIZE;
	} else 
	{
		printf("error : get partition info\n");
		return 1;
	}
	
	if (!ext4fs_mount(part_length)) {
		printf ("** Bad ext4 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	if (ext4fs_create_dir (dev_desc,dirname)) {
		printf ("** Error ext4fs_create_dir() **\n");
		ext4fs_close();
		return 1;
	};

	ext4fs_close();

	return 0;
}
int do_ext4_create_symlink(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *src_path = "/";
	char *target_path = "/";
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;
	int part_length;
	int link_type;
	disk_partition_t info;

	if (argc < 6)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if(!strcmp(argv[3],"H") || !strcmp(argv[3],"h"))
		link_type = HARD_LINK;
	else
	if(!strcmp(argv[3],"S") || !strcmp(argv[3],"s"))
		link_type = SOFT_LINK;
	else
		return cmd_usage(cmdtp);
	
	src_path 	= argv[4];
	target_path = argv[5];

	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",	argv[1], dev, part);
		ext4fs_close();
		return 1;
	}
	/*register the device and partition*/
	if (ext4_register_device(dev_desc,part)!=0)
	{
		printf("\n** Unable to use %s %d:%d for fattable **\n",
			argv[1], dev, part);
		return 1;
	}	

	if (!get_partition_info(dev_desc, part, &info)) 
	{
		total_sector = (info.size * info.blksz) / SECTOR_SIZE;
	} else 
	{
		printf("error : get partition info\n");
		return 1;
	}
	
	if (!ext4fs_mount(part_length)) {
		printf ("** Bad ext4 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext4fs_close();
		return 1;
	}

	if (ext4fs_create_symlink (dev_desc,link_type,src_path,target_path)) {
		printf ("** Error ext4fs_create_dir() **\n");
		ext4fs_close();
		return 1;
	};

	ext4fs_close();

	return 0;
}

U_BOOT_CMD
(
	ext4symlink,	6, 1, do_ext4_create_symlink,
	"create a file in the root directory",
	"<interface> <dev[:part]> [H:Hardlink/S:Softlink][Absolute Source Path] [Absolute Target Path]\n"
	"	  - create a Symbolic Link"
);

U_BOOT_CMD
(
	ext4mkdir,	4, 1, do_ext4_create_dir,
	"create a file in the root directory",
	"<interface> <dev[:part]> [Absolute Directory Path]\n"
	"	  - create a directory"
);
U_BOOT_CMD
(
	ext4write,	6, 1, do_ext4_write,
	"create a file in the root directory",
	"<interface> <dev[:part]> [Absolute filename path] [Address] [sizebytes]\n"
	"	  - create a file in / directory"
);

U_BOOT_CMD
(
	ext4format, 3, 1, do_ext4_format,
	"format device as EXT4 filesystem",
	"<interface> <dev[:part]>\n"
	"	  - format device as EXT4 filesystem on 'dev'"
);

U_BOOT_CMD(
	ext4ls,	4, 1, do_ext4_ls,
	"list files in a directory (default /)",
	"<interface> <dev[:part]> [directory]\n"
	"	  - list files from 'dev' on 'interface' in a 'directory'"
);


U_BOOT_CMD(
	ext4load,	6, 0, do_ext4_load,
	"load binary file from a Ext2 filesystem",
	"<interface> <dev[:part]> [addr] [filename] [bytes]\n"
	"	  - load binary file 'filename' from 'dev' on 'interface'\n"
	"		 to address 'addr' from ext2 filesystem"
);


