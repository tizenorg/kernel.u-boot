/*
 * ext2 filesystem for onenand device.
 *
 * Copyright(C) 2009 Samsung Electronics
 * InKi Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <version.h>
#include <stdarg.h>
#include <malloc.h>
#include <lcd.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/global_data.h>

#include <onenand_ext2.h>

#define IMAGE_BASE		(0x380000)
#define IMAGE_SIZE		(8 * 1024 * 1024)
#define ONENAND_READ_SIZE	(4 * 1024)

#define BOOT_BLOCK_SIZE		(1024)
#define DATA_BLOCK_UNIT		(512)
#define INODE_TABLE_ENTRY_SIZE	(128)

extern int onenand_read(ulong off, char *buf, unsigned int *out_size);

/* it indicates block size of ext2 filesystem formatted. */
unsigned int block_size_of_fs = 0;

/* it indicates inode block number for root directory and current directory. */
unsigned int root_inode, current_inode;

/* it indicates inode block size for finding the end of directory entry. */
unsigned int inode_block_size = 0;

/* it indicates the location of inode table in group structure. */
unsigned int inode_table_location;

/* it is used to store contents of block data and table entry. */
char data_buf[ONENAND_READ_SIZE], table_buf[ONENAND_READ_SIZE];

/*
 * read a block from onenand.
 *
 * @offset : onenand offset for reading a block.
 * @blk_count : block count to be read.
 *
 * return value is the size read from onenand.
 */
static unsigned int read_blk_from_onenand(unsigned int offset, unsigned int blk_count, char *buf)
{
	unsigned int size, i, out_size = 0;

	for (i = 0; i < blk_count; i++) {
		offset += (i * ONENAND_READ_SIZE);
		onenand_read(IMAGE_BASE + offset, buf, &size);
		out_size += size;
		buf += size;
	}

	return out_size;
}

static int get_sblock(struct ext2_sblock *sblock)
{
	char *pbuf = NULL;

	pbuf = (char *) table_buf;

	/* first, read 4K block from onenand. */
	read_blk_from_onenand(0, 1, pbuf);

	/* sblock places in next to 1k, boot sector so read from there. */
	memcpy(sblock, pbuf + BOOT_BLOCK_SIZE, sizeof(struct ext2_sblock));

	dprint("total_inodes = %d, block_size = %d\n", sblock->total_inodes,
		sblock->log2_block_size);

	return 0;
}

static int get_group_dec(struct ext2_block_group *group)
{
	char *pbuf = NULL;
	unsigned int offset = 0;

	pbuf = (char *) table_buf;

	/* get group descriptor. */
	if (block_size_of_fs == 1024)
		/* boot block : 1k, super block : 1k. */
		offset = block_size_of_fs * 2;
	else if (block_size_of_fs == 4096)
		/* boot block : 1k, super block : 3k. */
		offset = block_size_of_fs;

	read_blk_from_onenand(offset, 1, pbuf);

	memcpy(group, pbuf, sizeof(struct ext2_block_group));

	dprint("block_id = %d, inode_id = %d, inode_table_id = %d\n",
		group->block_id, group->inode_id, group->inode_table_id);

	return 0;
}

static int get_root_inode_entry(struct ext2_inode *inode,
	struct ext2_block_group *group)
{
	char *pbuf = NULL;

	pbuf = (char *) table_buf;

	if (group == NULL) {
		dprint("group is NULL.\n");
		return -1;
	}

	/* get location of inode table. */
	inode_table_location = group->inode_table_id * block_size_of_fs;

	dprint("inode table location = %d\n", inode_table_location);

	read_blk_from_onenand(inode_table_location, 1, pbuf);

	/* 
	 * inode table has 128byte per entry and
	 * root inode places in second entry.
	 */
	memcpy(inode, pbuf + INODE_TABLE_ENTRY_SIZE,
		sizeof(struct ext2_inode));

	/* get inode block size and used to find the end of directory entry. */
	inode_block_size = inode->blockcnt * DATA_BLOCK_UNIT;

	dprint("dir_blocks = 0x%8x, size = %d, blockcnt = %d\n", inode->b.blocks.dir_blocks[0],
		inode->size, inode->blockcnt);

	return 0;
}

/* 
 * get root directory entry.
 *
 * @dirent : directory entry structure for storing directory entry value.
 * @inode : instance pointer of inode entry.
 *
 * the offset of root directory entry is returned.
 */

static unsigned int get_root_dir_entry(struct ext2_dirent *dirent, struct ext2_inode *inode)
{
	char *pbuf = NULL;
	unsigned int offset = 0;

	pbuf = (char *) table_buf;

	if (inode == NULL) {
		dprint("inode is NULL.\n");
		return 0;
	}

	/* get block number stored with data of inode. */
	offset = inode->b.blocks.dir_blocks[0] * block_size_of_fs;

	read_blk_from_onenand(offset, 1, pbuf);

	memcpy(dirent, pbuf, sizeof(struct ext2_dirent));

	dprint("first entry name of root directory is name = %s\n",
		dirent->name);

	return offset;
}

/* initialize ext2 filesystem and return pointer of root directory entry. */
unsigned int mount_ext2fs(void)
{
	struct ext2_sblock sblock;
	struct ext2_block_group group;
	struct ext2_inode inode;
	struct ext2_dirent dirent;

	unsigned int need_block = 0, ret = 0, offset = 0;

	ret = get_sblock(&sblock);
	if (ret < 0) {
		dprint("sblock is NULL.\n");
		return -1;
	}

	/* 
	 * in case that log2_block_size is 0, block_size is 1024,
	 * 2048 for 1 and 4096 for 2. 
	 */
	switch (sblock.log2_block_size + 1) {
	case 1:
		block_size_of_fs = 1024;
		break;
	case 2:
		block_size_of_fs = 2048;
		break;
	case 3:
		block_size_of_fs = 4096;
		break;
	default:
		block_size_of_fs = 4096;
		break;
	}

	dprint("block size of filesystem = %d\n", block_size_of_fs);

	/* get block size for inode table. */
	need_block = sblock.total_inodes * INODE_TABLE_ENTRY_SIZE /
		block_size_of_fs;

	dprint("need_block for inode table = %d\n", need_block);

	ret = get_group_dec(&group);
	if (ret < 0) {
		dprint("group_dec is NULL.\n");
		return -1;
	}

	ret = get_root_inode_entry(&inode, &group);
	if (ret < 0) {
		dprint("root_inode_entry is NULL.\n");
		return -1;
	}

	offset = get_root_dir_entry(&dirent, &inode);
	if (offset == 0) {
		dprint("root_dir_entry is NULL.\n");
		return -1;
	}

	return offset;
}

/* 
 * get inode to file.
 *
 * @inode_table : the offset of inode table to file.
 * @inode : inode structure for storing inode entry value.
 */
static int get_inode(unsigned int inode_table, struct ext2_inode *inode)
{
	unsigned int offset, extra_block_count, need_block = 0;
	char *pbuf = NULL;

	pbuf = (char *) table_buf;

	/* get block count. */
	need_block = (inode_table / block_size_of_fs);

	/* 
	 * get extra block count for loading from onenand device
	 * considering 1k and 2k block size for filesystem.
	 */
	extra_block_count = (block_size_of_fs * need_block) / ONENAND_READ_SIZE;

	offset = inode_table_location + (ONENAND_READ_SIZE * extra_block_count);

	read_blk_from_onenand(offset, 1, pbuf);

	/* also subtract as increased offset. */
	inode_table -= (ONENAND_READ_SIZE * extra_block_count);

	/* get inode entry to file. */
	memcpy(inode, (char *) pbuf + inode_table, sizeof(struct ext2_inode));

	return 0;
}

/*
 * get directory entry to file.
 *
 * @in_buf : pointer of system memory to directory entry.
 * @dirent : directory entry structure for storing directory entry value.
 */
static int get_dir_entry(char *in_buf, struct ext2_dirent *dirent)
{
	memcpy(dirent, (char *) in_buf, sizeof(struct ext2_dirent));

	dirent->name[dirent->namelen] = '\0';

	return 0;
}
/*
 * get inode offset.
 * 
 * @dirent : instance pointer to directory entry structure.
 *
 * return value is the offset to inode table.
 */
static int get_inode_offset(struct ext2_dirent *dirent)
{
	int inode_table;

	if (dirent == NULL) {
		printf("dirent is NULL.\n");
		return -1;
	}

	inode_table = (dirent->inode - 1) * INODE_TABLE_ENTRY_SIZE;

	return inode_table;
}

/* 
 * move in_inode which is directory entry pointer to next directory entry.
 *
 * @dirent : pointer of ext2_dirent structure.
 */
static unsigned int next_dir_entry(struct ext2_dirent *dirent)
{
	static unsigned int sum_point = 0;

	/* 
	 * it finds the end of directory entry.
	 * 
	 * directroy entries are stored in data block.
	 * (inode_block_size = data block count * data block size.)
	 */
	sum_point += dirent->direntlen;
	if ( sum_point >= inode_block_size) {
		sum_point = 0;

		return 0;
	}

	return dirent->direntlen;
}

/*
 * find file matched with filename.
 *
 * @in_inode : the offset of current directory entry.
 * @filename : the file name for finding.
 * @dirent : the pointer of directory entry structure found.
 *
 * return value : inode number of directory entry.
 */
int find_file_ext2(unsigned int in_inode, const char *filename,
	struct ext2_dirent *dirent)
{
	unsigned int next_point = 0;
	char *pbuf = NULL;

	pbuf = (char *) table_buf;

	read_blk_from_onenand(in_inode, 1, pbuf);

	get_dir_entry(pbuf, dirent);

	if ((strcmp(dirent->name, filename)) == 0) {
		return dirent->inode;
	}

	next_point = next_dir_entry(dirent);

	do {
		get_dir_entry(pbuf, dirent);

		dprint("soure file = %s, dst file = %s, len = %d\n", filename,
			dirent->name, dirent->namelen);

		if ((strcmp(dirent->name, filename)) == 0) {
			return dirent->inode;
		}

		next_point = next_dir_entry(dirent);
		pbuf += next_point;

	} while (next_point > 0);

	printf("failed to find file.\n");

	return -1;
}

/*
 * get the location of inode to file.
 *
 * @in_inode : the offset to current directory entry.
 * @name : file name.
 *
 * return value is the offset of inode table to file.
 */
int open_file_ext2(unsigned int in_inode, const char *name)
{
	unsigned int directory_inode, inode_table;
	struct ext2_dirent dirent;

	directory_inode = find_file_ext2(in_inode, name, &dirent);

	/* 
	 * the location of file block =
	 * inode table base + (directory inode number  - 1) * inode table entry size.
	 */
	inode_table = (directory_inode - 1) * INODE_TABLE_ENTRY_SIZE;

	dprint("directory_inode = %d, inode_table = %d\n", directory_inode, inode_table);

	return inode_table;
}

/*
 * get file size.
 *
 * @inode_table : offset of inode table to file.
 * @dirent : pointer of ext2_dirent structure.
 */
int get_filesize_ext2(unsigned int inode_table)
{
	struct ext2_inode inode;

	get_inode(inode_table, &inode);

	return inode.size;
}

/* read data block to file.
 *
 * @inode_table : offset of inode table to file.
 * @in_buf : memory buffer for storing contents of data block.
 * @size : file size.
 */
int read_file_ext2(unsigned int inode_table, char *in_buf, unsigned int size)
{
	struct ext2_datablock d_block;
	struct ext2_inode inode;
	unsigned int d_block_addr, id_block_addr, read_size = 0, id_block_num;
	char *ptable = NULL, *pdata = NULL;
	int i;

	ptable = (char *) table_buf;
	pdata = (char *) data_buf;

	get_inode(inode_table, &inode);

	/* get data block information for file. */
	memcpy(&d_block, (char *) &inode.b.blocks, sizeof(struct ext2_datablock));

	/* get real data from direct blocks. */
	for (i = 0; i < INDIRECT_BLOCKS; i++) {
		if (d_block.dir_blocks[i] > 0) {

			dprint("direct block[%d] num = %d\n", i, d_block.dir_blocks[i]);

			/* calculate real data address. */
			d_block_addr = (unsigned int) d_block.dir_blocks[i] *
				block_size_of_fs;
			if (size > block_size_of_fs) {
				read_blk_from_onenand(d_block_addr, 1, pdata);
				memcpy(in_buf, (char *) pdata, block_size_of_fs);
				size -= block_size_of_fs;
				in_buf += block_size_of_fs;
				read_size += block_size_of_fs;

				if (size <= 0)
					return read_size;
			} else {
				read_blk_from_onenand(d_block_addr, 1, pdata);
				memcpy(in_buf, (char *) pdata, size);
				read_size += size;

				return read_size;
			}
		}
	}

	/* 
	 * get real data from 1-dim indirect blocks only in case that
	 * 2-dim indirect block has no block num. 
	 */
	if (d_block.indir_block > 0 && d_block.double_indir_block <= 0) {
		d_block_addr = (unsigned int) d_block.indir_block *
			block_size_of_fs;
		dprint("1-dim indirect block address = 0x%x\n", d_block_addr);
		read_blk_from_onenand(d_block_addr, 1, ptable);

		/* 1-dim indirect block has 1k block and the size per entry is 4byte. */
		for (i = 0; i < block_size_of_fs; i+=4) {
			/* get indirect block number. */
			memcpy(&id_block_num, (char *) ptable + i, 4);

			dprint("1-dim indirect block[%d] num = %d\n", i, id_block_num);

			/* get real data from indirect block in case of hole. */
			if (id_block_num > 0) {
				/* calculate block number having real data. */
				id_block_addr = (unsigned int) id_block_num *
					block_size_of_fs;

				if (size > block_size_of_fs) {
					read_blk_from_onenand(id_block_addr, 1, pdata);
					memcpy(in_buf, (char *) pdata, block_size_of_fs);
					size -= block_size_of_fs;
					in_buf += block_size_of_fs;
					read_size += block_size_of_fs;

					if (size <= 0)
						return read_size;
				} else {
					read_blk_from_onenand(id_block_addr, 1, pdata);
					memcpy(in_buf, pdata, size);
					read_size += size;

					return read_size;
				}
			}
		}
	}

	 /* read operation for 2-dim and 3-dim indirect blocks should be added. */

	return read_size;
}

/* 
 * list files in directory.
 *
 * @in_inode : the offset of current directory entry.
 * @cmd : command indicating file attributes for listing.
 */
void ls_ext2(unsigned int in_inode, const int cmd)
{
	struct ext2_dirent dirent;
	struct ext2_inode inode;
	unsigned int next_point = 0;
	int ret, inode_table = 0;
	char *buf = NULL;

	buf = malloc(ONENAND_READ_SIZE);
	if (buf == NULL) {
		dprint("failed to allocate memory.\n");
		return;
	}

	read_blk_from_onenand(in_inode, 1, buf);

	switch (cmd) {
	case EXT2_LS_ONLY_FILE:
	case EXT2_LS_ALL_ENTRY:
	case EXT2_LS_ALL:
		do {
			ret = get_dir_entry(buf, &dirent);
			if (ret < 0) {
				dprint("failed to get directory entry.\n");
				return;
			}

			inode_table = get_inode_offset(&dirent);
			if (inode_table < 0) {
				dprint("failed to get inode table offset.\n");
				return;
			}

			ret = get_inode(inode_table, &inode);
			if (ret < 0) {
				dprint("failed to get inode entry.\n");
				return;
			}

			if (cmd == EXT2_LS_ONLY_FILE) {
				if (dirent.filetype == EXT2_FT_REG_FILE ||
					dirent.filetype == EXT2_FT_DIR) {
					if (dirent.filetype == EXT2_FT_DIR)
						printf("[ %s ]\n", dirent.name);
					else
						printf("%s\n", dirent.name);
				}
			} else if (cmd == EXT2_LS_ALL_ENTRY) {
				if (dirent.filetype == EXT2_FT_DIR)
					printf("[ %s ]\n", dirent.name);
				else
					printf("%s\n", dirent.name);
			} else if (cmd == EXT2_LS_ALL) {
				if (dirent.filetype == EXT2_FT_DIR)
					printf("%x	%d	[ %s ]\n",
						inode.mode, inode.size, dirent.name);
				else
					printf("%x	%d	%s\n",
						inode.mode, inode.size, dirent.name);
			}

			next_point = next_dir_entry(&dirent);
			buf += next_point;
		} while (next_point != 0);

		return;
	default:
		printf("it's wrong command.\n");
		return;
	};

	free(buf);
}

/*
 * change current directory.
 *
 * @in_inode : the offset of current directory entry.
 * @name : directory name.
 */
void cd_ext2(unsigned int in_inode, const char *name)
{
	unsigned int d_inode, out_inode, need_block = 0;
	struct ext2_inode inode;
	struct ext2_dirent dirent;
	char *buf = NULL;

	/* return inode of directory entry. */
	out_inode = find_file_ext2(in_inode, name, &dirent);
	if (out_inode < 0) {
		printf("failed to find file.\n");
		return;
	}

	/* check whether entry is director or not. */
	if (dirent.filetype != EXT2_FT_DIR) {
		printf("%s is not directory.\n", name);
		return;
	}

	d_inode = (dirent.inode - 1) * INODE_TABLE_ENTRY_SIZE;

	/* 
	 * get block size for loading from onenand.
	 * if d_inode is more then 4k then need_block should be more then 2.
	 */
	need_block = (d_inode / block_size_of_fs) + 1;

	buf = malloc(ONENAND_READ_SIZE * need_block);
	if (buf == NULL) {
		printf("failed to allocate memory.\n");
		return;
	}

	read_blk_from_onenand(inode_table_location, need_block, buf);

	/* get inode entry to file. */
	memcpy(&inode, (char *) buf + d_inode, sizeof(struct ext2_inode));

	current_inode = (inode.b.blocks.dir_blocks[0] * block_size_of_fs);

	free(buf);
}
/*
 * dump file.
 *
 * @in_inode : the offset of current directory entry.
 * @name : file name.
 */
void fd_ext2(unsigned int in_inode, const char *name)
{
	unsigned int in_size, inode_table, out_size;
	char *buf = NULL;

	inode_table = open_file_ext2(in_inode, name);
	in_size = get_filesize_ext2(inode_table);
	if (in_size < 0) {
		printf("failed to get file size.\n");
		return;
	}

	dprint("name = %s, in size = %d\n", name, in_size);

	buf = malloc(in_size);

	out_size = read_file_ext2(inode_table, buf, in_size);
	dprint("out size = %d, data address = 0x%8x\n", out_size, (unsigned int) buf);
}

void init_onenand_ext2(void)
{
	root_inode = (unsigned int) mount_ext2fs();
	if (root_inode < 0) {
		printf("failed to mount.\n");
		return;
	}

	dprint("ext2fs has been mounted.\n");

	current_inode = root_inode;
}

int do_ls_ext2(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int cmd = -1;

	if (argc == 1 && argv[1] == NULL) {
		cmd = EXT2_LS_ONLY_FILE;
	} else if (argc == 2 && (strcmp(argv[1], "-a") == 0)) {
		cmd = EXT2_LS_ALL_ENTRY;
	} else if (argc == 2 && (strcmp(argv[1], "-al") == 0)) {
		cmd = EXT2_LS_ALL;
	}

	ls_ext2(current_inode, cmd);

	return 0;
}

int do_cd_ext2(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc == 2 && argv[1] != NULL)
	    cd_ext2(current_inode, argv[1]);

	return 0;
}

int do_fd_ext2(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc == 2 && argv[1] != NULL)
	    fd_ext2(current_inode, argv[1]);

	return 0;
}

U_BOOT_CMD(ls_ext2, 3, 1, do_ls_ext2,
	"list files from ext2 filesystem.\n",
	"ls_ext2 [-al] [direct_name]\n"
);

U_BOOT_CMD(cd_ext2, 3, 1, do_cd_ext2,
	"change directory.\n",
	"cd_ext2 [direct_name]\n"
);

U_BOOT_CMD(fd_ext2, 3, 1, do_fd_ext2,
	"dump file.\n",
	"fd_ext2 [file_name]\n"
);
