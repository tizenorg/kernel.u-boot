/*
 * (C) Copyright 2004
 *  esd gmbh <www.esd-electronics.com>
 *  Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 *  based on code from grub2 fs/ext2.c and fs/fshelp.c by
 *
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <ext2fs.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/stat.h>
#include <linux/time.h>
#include <div64.h>

//#define DEBUG_LOG 1

extern int ext2fs_devread(int sector, int byte_offset, int byte_len,
	char *buf);

/* Magic value used to identify an ext2 filesystem.  */
#define	EXT2_MAGIC		0xEF53
/* Amount of indirect blocks in an inode.  */
#define INDIRECT_BLOCKS		12
/* Maximum lenght of a pathname.  */
#define EXT2_PATH_MAX		4096
/* Maximum nesting of symlinks, used to prevent a loop.  */
#define	EXT2_MAX_SYMLINKCNT	8

/* Filetype used in directory entry.  */
#define	FILETYPE_UNKNOWN	0
#define	FILETYPE_REG		1
#define	FILETYPE_DIRECTORY	2
#define	FILETYPE_SYMLINK	7

/* Filetype information as used in inodes.  */
#define FILETYPE_INO_MASK	0170000
#define FILETYPE_INO_REG	0100000
#define FILETYPE_INO_DIRECTORY	0040000
#define FILETYPE_INO_SYMLINK	0120000

/* Bits used as offset in sector */
#define DISK_SECTOR_BITS        9

/* Log2 size of ext2 block in 512 blocks.  */
#define LOG2_EXT2_BLOCK_SIZE(data) (__le32_to_cpu (data->sblock.log2_block_size) + 1)

/* Log2 size of ext2 block in bytes.  */
#define LOG2_BLOCK_SIZE(data)	   (__le32_to_cpu (data->sblock.log2_block_size) + 10)

/* The size of an ext2 block in bytes.  */
#define EXT2_BLOCK_SIZE(data)	   (1 << LOG2_BLOCK_SIZE(data))

#define INODE_SIZE_FILESYSTEM(data)	(__le32_to_cpu (data->sblock.inode_size))
#define EXT4_EXTENTS_FLAG 	0x80000
#define EXT4_EXT_MAGIC		0xf30a

/* The ext2 superblock.  */
struct ext2_sblock {
	uint32_t total_inodes;
	uint32_t total_blocks;
	uint32_t reserved_blocks;
	uint32_t free_blocks;
	uint32_t free_inodes;
	uint32_t first_data_block;
	uint32_t log2_block_size;
	uint32_t log2_fragment_size;
	uint32_t blocks_per_group;
	uint32_t fragments_per_group;
	uint32_t inodes_per_group;
	uint32_t mtime;
	uint32_t utime;
	uint16_t mnt_count;
	uint16_t max_mnt_count;
	uint16_t magic;
	uint16_t fs_state;
	uint16_t error_handling;
	uint16_t minor_revision_level;
	uint32_t lastcheck;
	uint32_t checkinterval;
	uint32_t creator_os;
	uint32_t revision_level;
	uint16_t uid_reserved;
	uint16_t gid_reserved;
	uint32_t first_inode;
	uint16_t inode_size;
	uint16_t block_group_number;
	uint32_t feature_compatibility;
	uint32_t feature_incompat;
	uint32_t feature_ro_compat;
	uint32_t unique_id[4];
	char volume_name[16];
	char last_mounted_on[64];
	uint32_t compression_info;
};

/* The ext2 blockgroup.  */
struct ext2_block_group {
	uint32_t block_id;
	uint32_t inode_id;
	uint32_t inode_table_id;
	uint16_t free_blocks;
	uint16_t free_inodes;
	uint16_t used_dir_cnt;
	uint32_t reserved[3];
};

/* The ext2 inode.  */
struct ext2_inode {
	uint16_t mode;
	uint16_t uid;
	uint32_t size;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t dtime;
	uint16_t gid;
	uint16_t nlinks;
	uint32_t blockcnt;	/* Blocks of 512 bytes!! */
	uint32_t flags;
	uint32_t osd1;
	union {
		struct datablocks {
			uint32_t dir_blocks[INDIRECT_BLOCKS];
			uint32_t indir_block;
			uint32_t double_indir_block;
			uint32_t tripple_indir_block;
		} blocks;
		char symlink[60];
	} b;
	uint32_t version;
	uint32_t acl;
	uint32_t dir_acl;
	uint32_t fragment_addr;
	uint32_t osd2[3];
};

/* The header of an ext2 directory entry.  */
struct ext2_dirent {
	uint32_t inode;
	uint16_t direntlen;
	uint8_t namelen;
	uint8_t filetype;
};

struct ext2fs_node {
	struct ext2_data *data;
	struct ext2_inode inode;
	int ino;
	int inode_read;
};

struct ext4_extent_header {
	uint16_t magic;
	uint16_t entries;
	uint16_t max;
	uint16_t depth;
	uint32_t generation;
};

struct ext4_extent {
	uint32_t block;
	uint16_t len;
	uint16_t start_hi;
	uint32_t start;
};

struct ext4_extent_idx {
	uint32_t block;
	uint32_t leaf;
	uint16_t leaf_hi;
	uint16_t unused;
};

/* Information about a "mounted" ext2 filesystem.  */
struct ext2_data {
	struct ext2_sblock sblock;
	struct ext2_inode *inode;
	struct ext2fs_node diropen;
};


typedef struct ext2fs_node *ext2fs_node_t;

struct ext2_data *ext2fs_root = NULL;
ext2fs_node_t ext2fs_file = NULL;
int symlinknest = 0;
uint32_t *indir1_block = NULL;
int indir1_size = 0;
int indir1_blkno = -1;
uint32_t *indir2_block = NULL;
int indir2_size = 0;
int indir2_blkno = -1;
static unsigned int inode_size;

#define BLOCK_NO_ONE  	1
#define SUPERBLOCK_SECTOR 2
#define SUPERBLOCK_SIZE 1024

int blocksize;
int inodesize;
int sector_per_block;

unsigned char *block_bitmap_ptr=NULL;
static unsigned long blockno=0;
static unsigned long inode_no=0;

static int first_execution=0;
static int first_execution_inode=0;



int  no_blockgroup;
unsigned char *gd_table=NULL;
unsigned int   gd_table_block_no=0;

/*superblock*/
struct ext2_sblock *sb=NULL;

/*blockgroup bitmaps*/
unsigned char **bg=NULL;

/*inode bitmaps*/
unsigned char **inode_bmaps=NULL;


/*block group descritpor table*/
struct ext2_block_group *gd=NULL;


/*
 * Macro-instructions used to manage several block sizes
 */
#define EXT2_MIN_BLOCK_LOG_SIZE	10 /* 1024 */
#define EXT2_MAX_BLOCK_LOG_SIZE	16 /* 65536 */
#define EXT2_MIN_BLOCK_SIZE	(1 << EXT2_MIN_BLOCK_LOG_SIZE)
#define EXT2_MAX_BLOCK_SIZE	(1 << EXT2_MAX_BLOCK_LOG_SIZE)


static int ext2fs_blockgroup
	(struct ext2_data *data, int group, struct ext2_block_group *blkgrp)
{
	unsigned int blkno;
	unsigned int blkoff;
	unsigned int desc_per_blk;

	desc_per_blk = EXT2_BLOCK_SIZE(data) / sizeof(struct ext2_block_group);

	blkno = __le32_to_cpu(data->sblock.first_data_block) + 1
		+ group / desc_per_blk;
	blkoff = (group % desc_per_blk) * sizeof(struct ext2_block_group);
#ifdef DEBUG
	printf ("ext2fs read %d group descriptor (blkno %d blkoff %d)\n",
		group, blkno, blkoff);
#endif
	return (ext2fs_devread (blkno << LOG2_EXT2_BLOCK_SIZE(data),
		blkoff, sizeof(struct ext2_block_group), (char *)blkgrp));
}

static struct ext4_extent_header * ext4_find_leaf (struct ext2_data *data,
	char *buf, struct ext4_extent_header *ext_block, uint32_t fileblock)
{
	struct ext4_extent_idx *index;

	while (1)
	{
		int i;
		unsigned int block;

		index = (struct ext4_extent_idx *) (ext_block + 1);

		if (le32_to_cpu(ext_block->magic) != EXT4_EXT_MAGIC)
			return 0;

		if (ext_block->depth == 0)
			return ext_block;

		for (i = 0; i < le32_to_cpu (ext_block->entries); i++) {
			if (fileblock < le32_to_cpu(index[i].block))
				break;
		}

		if (--i < 0)
			return 0;

		block = le32_to_cpu (index[i].leaf_hi);
		block = (block << 32) + le32_to_cpu (index[i].leaf);

		if (ext2fs_devread (block << LOG2_EXT2_BLOCK_SIZE (data),
		  0, EXT2_BLOCK_SIZE(data), buf))
			{
			ext_block = (struct ext4_extent_header *) buf;
			return ext_block;
			}
		else
			return 0;
	}

}

static int ext2fs_read_inode
	(struct ext2_data *data, int ino, struct ext2_inode *inode)
{
	struct ext2_block_group blkgrp;
	struct ext2_sblock *sblock = &data->sblock;
	int inodes_per_block;
	int status;

	unsigned int blkno;
	unsigned int blkoff;

#ifdef DEBUG
	printf ("ext2fs read inode %d, inode_size %d\n", ino, inode_size);
#endif
	/* It is easier to calculate if the first inode is 0.  */
	ino--;
	status = ext2fs_blockgroup (data, ino / __le32_to_cpu
	    (sblock->inodes_per_group), &blkgrp);

	if (status == 0)
		return (0);

	inodes_per_block = EXT2_BLOCK_SIZE(data) / inode_size;

	blkno = __le32_to_cpu (blkgrp.inode_table_id) +
		(ino % __le32_to_cpu (sblock->inodes_per_group))
		/ inodes_per_block;
	blkoff = (ino % inodes_per_block) * inode_size;
#ifdef DEBUG
	printf ("ext2fs read inode blkno %d blkoff %d\n", blkno, blkoff);
#endif
	/* Read the inode.  */
	status = ext2fs_devread (blkno << LOG2_EXT2_BLOCK_SIZE (data), blkoff,
		sizeof (struct ext2_inode), (char *) inode);
	if (status == 0)
		return (0);

	return (1);
}


void ext2fs_free_node (ext2fs_node_t node, ext2fs_node_t currroot)
{
	if ((node != &ext2fs_root->diropen) && (node != currroot))
		free (node);
}

int readblock(struct ext2_inode *inode, int fileblock)
{
	int blknr;
	int blksz ;
	int log2_blksz;
	int status;


	/*get the blocksize of the filesystem*/
	blksz=EXT2_BLOCK_SIZE(ext2fs_root);
	log2_blksz = LOG2_EXT2_BLOCK_SIZE (ext2fs_root);



	/* Direct blocks.  */
	if (fileblock < INDIRECT_BLOCKS) {
		blknr = __le32_to_cpu (inode->b.blocks.dir_blocks[fileblock]);
	}
	/* Indirect.  */
	else if (fileblock < (INDIRECT_BLOCKS + (blksz / 4))) {
		if (indir1_block == NULL) {
			indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir1_block == NULL) {
				printf ("** ext2fs read block (indir 1) malloc failed. **\n");
				return (-1);
			}
			indir1_size = blksz;
			indir1_blkno = -1;
		}
		if (blksz != indir1_size) {
			free (indir1_block);
			indir1_block = NULL;
			indir1_size = 0;
			indir1_blkno = -1;
			indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir1_block == NULL) {
				printf ("** ext2fs read block (indir 1) malloc failed. **\n");
				return (-1);
			}
			indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.indir_block) <<
		     log2_blksz) != indir1_blkno) {
			status = ext2fs_devread (__le32_to_cpu(inode->b.blocks.indir_block) << log2_blksz,
						 0, blksz,
						 (char *) indir1_block);
			if (status == 0) {
				printf ("** ext2fs read block (indir 1) failed. **\n");
				return (0);
			}
			indir1_blkno =
				__le32_to_cpu (inode->b.blocks.
					       indir_block) << log2_blksz;
		}
		blknr = __le32_to_cpu (indir1_block
				       [fileblock - INDIRECT_BLOCKS]);
	}
	/* Double indirect.  */
	else if (fileblock <
		 (INDIRECT_BLOCKS + (blksz / 4 * (blksz / 4 + 1)))) {

		unsigned int perblock = blksz / 4;
		unsigned int rblock = fileblock - (INDIRECT_BLOCKS
						   + blksz / 4);

		if (indir1_block == NULL) {
			indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir1_block == NULL) {
				printf ("** ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			indir1_size = blksz;
			indir1_blkno = -1;
		}
		if (blksz != indir1_size) {
			free (indir1_block);
			indir1_block = NULL;
			indir1_size = 0;
			indir1_blkno = -1;
			indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir1_block == NULL) {
				printf ("** ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.double_indir_block) <<
		     log2_blksz) != indir1_blkno) {
			status = ext2fs_devread (__le32_to_cpu(inode->b.blocks.double_indir_block) << log2_blksz,
						0, blksz,
						(char *) indir1_block);
			if (status == 0) {
				printf ("** ext2fs read block (indir 2 1) failed. **\n");
				return (-1);
			}
			indir1_blkno =
				__le32_to_cpu (inode->b.blocks.double_indir_block) << log2_blksz;
		}

		if (indir2_block == NULL) {
			indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir2_block == NULL) {
				printf ("** ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			indir2_size = blksz;
			indir2_blkno = -1;
		}
		if (blksz != indir2_size) {
			free (indir2_block);
			indir2_block = NULL;
			indir2_size = 0;
			indir2_blkno = -1;
			indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir2_block == NULL) {
				printf ("** ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			indir2_size = blksz;
		}
		if ((__le32_to_cpu (indir1_block[rblock / perblock]) <<
		     log2_blksz) != indir2_blkno) {
		     	status = ext2fs_devread (__le32_to_cpu(indir1_block[rblock / perblock]) << log2_blksz,
						 0, blksz,
						 (char *) indir2_block);
			if (status == 0) {
				printf ("** ext2fs read block (indir 2 2) failed. **\n");
				return (-1);
			}
			indir2_blkno =
				__le32_to_cpu (indir1_block[rblock / perblock]) << log2_blksz;
		}
		blknr = __le32_to_cpu (indir2_block[rblock % perblock]);
	}
	/* Tripple indirect.  */
	else {
		printf ("** ext2fs doesn't support tripple indirect blocks. **\n");
		return (-1);
	}
#ifdef DEBUG
	printf ("ext2fs_read_block %08x\n", blknr);
#endif
	return (blknr);
}

static int ext2fs_read_block (ext2fs_node_t node, int fileblock)
{
	struct ext2_data *data = node->data;
	struct ext2_inode *inode = &node->inode;
	int blknr;
	int blksz = EXT2_BLOCK_SIZE (data);
	int log2_blksz = LOG2_EXT2_BLOCK_SIZE (data);
	int status;

	if (le32_to_cpu(inode->flags) & EXT4_EXTENTS_FLAG) {
		char buf[EXT2_BLOCK_SIZE(data)];
		struct ext4_extent_header *leaf;
		struct ext4_extent *ext;
		int i;

		leaf = ext4_find_leaf (data, buf,
			(struct ext4_extent_header *) inode->b.blocks.dir_blocks,
			fileblock);
		if (!leaf) {
			printf ("invalid extent \n");
			return -1;
		}

		ext = (struct ext4_extent *) (leaf + 1);

		for (i = 0; i < le32_to_cpu (leaf->entries); i++) {
			if (fileblock < le32_to_cpu (ext[i].block))
				break;
		}

		if (--i >= 0) {
			fileblock -= le32_to_cpu (ext[i].block);
			if (fileblock >= le32_to_cpu (ext[i].len)) {
				return 0;
			} else {
				unsigned int start;

				start = le32_to_cpu (ext[i].start_hi);
				start = (start << 32) + le32_to_cpu (ext[i].start);

				return fileblock + start;
			}
		} else {
			printf("something wrong with extent \n");
			return -1;
		}
	}

	/* Direct blocks.  */
	if (fileblock < INDIRECT_BLOCKS) {
		blknr = __le32_to_cpu (inode->b.blocks.dir_blocks[fileblock]);
	}
	/* Indirect.  */
	else if (fileblock < (INDIRECT_BLOCKS + (blksz / 4))) {
		if (indir1_block == NULL) {
			indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir1_block == NULL) {
				printf ("** ext2fs read block (indir 1) malloc failed. **\n");
				return (-1);
			}
			indir1_size = blksz;
			indir1_blkno = -1;
		}
		if (blksz != indir1_size) {
			free (indir1_block);
			indir1_block = NULL;
			indir1_size = 0;
			indir1_blkno = -1;
			indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir1_block == NULL) {
				printf ("** ext2fs read block (indir 1) malloc failed. **\n");
				return (-1);
			}
			indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.indir_block) <<
		     log2_blksz) != indir1_blkno) {
			status = ext2fs_devread (__le32_to_cpu(inode->b.blocks.indir_block) << log2_blksz,
				 0, blksz, (char *) indir1_block);
			if (status == 0) {
				printf ("** ext2fs read block (indir 1) failed. **\n");
				return (0);
			}
			indir1_blkno = __le32_to_cpu (inode->b.blocks.
			       indir_block) << log2_blksz;
		}
		blknr = __le32_to_cpu (indir1_block[fileblock - INDIRECT_BLOCKS]);
	}
	/* Double indirect.  */
	else if (fileblock <
		(INDIRECT_BLOCKS + (blksz / 4 * (blksz / 4 + 1)))) {
		unsigned int perblock = blksz / 4;
		unsigned int rblock = fileblock - (INDIRECT_BLOCKS + blksz / 4);

		if (indir1_block == NULL) {
			indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir1_block == NULL) {
				printf ("** ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			indir1_size = blksz;
			indir1_blkno = -1;
		}
		if (blksz != indir1_size) {
			free (indir1_block);
			indir1_block = NULL;
			indir1_size = 0;
			indir1_blkno = -1;
			indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir1_block == NULL) {
				printf ("** ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.double_indir_block) <<
		     log2_blksz) != indir1_blkno) {
			status = ext2fs_devread
				(__le32_to_cpu(inode->b.blocks.double_indir_block) << log2_blksz,
				0, blksz,
				(char *) indir1_block);
			if (status == 0) {
				printf ("** ext2fs read block (indir 2 1) failed. **\n");
				return (-1);
			}
			indir1_blkno =
				__le32_to_cpu (inode->b.blocks.double_indir_block) << log2_blksz;
		}

		if (indir2_block == NULL) {
			indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir2_block == NULL) {
				printf ("** ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			indir2_size = blksz;
			indir2_blkno = -1;
		}
		if (blksz != indir2_size) {
			free (indir2_block);
			indir2_block = NULL;
			indir2_size = 0;
			indir2_blkno = -1;
			indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (indir2_block == NULL) {
				printf ("** ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			indir2_size = blksz;
		}
		if ((__le32_to_cpu (indir1_block[rblock / perblock]) <<
		     log2_blksz) != indir2_blkno) {
			status = ext2fs_devread (__le32_to_cpu(indir1_block[rblock / perblock]) << log2_blksz,
						 0, blksz,
						 (char *) indir2_block);
			if (status == 0) {
				printf ("** ext2fs read block (indir 2 2) failed. **\n");
				return (-1);
			}
			indir2_blkno =
				__le32_to_cpu (indir1_block[rblock / perblock]) << log2_blksz;
		}
		blknr = __le32_to_cpu (indir2_block[rblock % perblock]);
	}
	/* Tripple indirect.  */
	else {
		printf ("** ext2fs doesn't support tripple indirect blocks. **\n");
		return (-1);
	}
#ifdef DEBUG
	printf ("ext2fs_read_block %08x\n", blknr);
#endif
	return (blknr);
}


int ext2fs_read_file
	(ext2fs_node_t node, int pos, unsigned int len, char *buf) {

	int i;
	int blockcnt;
	int log2blocksize = LOG2_EXT2_BLOCK_SIZE (node->data);
	int blocksize = 1 << (log2blocksize + DISK_SECTOR_BITS);
	unsigned int filesize = __le32_to_cpu(node->inode.size);
	int previous_block_number = -1;
	int delayed_start = 0;
	int delayed_extent = 0;
	int delayed_skipfirst = 0;
	int delayed_next = 0;
	char * delayed_buf = NULL;
	int status;

	/* Adjust len so it we can't read past the end of the file.  */
	if (len > filesize)
		len = filesize;

	blockcnt = ((len + pos) + blocksize - 1) / blocksize;

	for (i = pos / blocksize; i < blockcnt; i++) {
		int blknr;
		int blockoff = pos % blocksize;
		int blockend = blocksize;
		int skipfirst = 0;
		blknr = ext2fs_read_block (node, i);
		if (blknr < 0) {
			return (-1);
		}
		blknr = blknr << log2blocksize;

		/* Last block.  */
		if (i == blockcnt - 1) {
			blockend = (len + pos) % blocksize;

			/* The last portion is exactly blocksize.  */
			if (!blockend)
				blockend = blocksize;
		}

		/* First block.  */
		if (i == pos / blocksize) {
			skipfirst = blockoff;
			blockend -= skipfirst;
		}
		if (blknr) {
			int status;

			if (previous_block_number != -1) {
				if (delayed_next == blknr) {
				delayed_extent += blockend;
				delayed_next += blockend >> SECTOR_BITS;
				} else { /* spill */
					status = ext2fs_devread(delayed_start,
						delayed_skipfirst, delayed_extent,
							delayed_buf);
					if (status == 0)
						return -1;
					previous_block_number = blknr;
					delayed_start = blknr;
					delayed_extent = blockend;
					delayed_skipfirst = skipfirst;
					delayed_buf = buf;
					delayed_next = blknr +
						(blockend >> SECTOR_BITS);
				}
			} else {
				previous_block_number = blknr;
				delayed_start = blknr;
				delayed_extent = blockend;
				delayed_skipfirst = skipfirst;
				delayed_buf = buf;
				delayed_next = blknr + (blockend >> SECTOR_BITS);
			}
		} else {
			if (previous_block_number != -1) {
				/* spill */
				status = ext2fs_devread(delayed_start,
					delayed_skipfirst, delayed_extent,
						 delayed_buf);
			  	if (status == 0)
					return -1;
			 	previous_block_number = -1;
			}
			memset(buf, 0, blocksize - skipfirst);
		}
		buf += blocksize - skipfirst;
	}
	if (previous_block_number != -1) {
		/* spill */
		status = ext2fs_devread(delayed_start,
			 delayed_skipfirst, delayed_extent, delayed_buf);
		if (status == 0)
			return -1;
		previous_block_number = -1;
	}
	return (len);
}

int ext2fs_write_file
	(block_dev_desc_t *dev_desc, struct ext2_inode *file_inode, int pos, unsigned int len, char *buf) {

	int i;
	int blockcnt;
	int log2blocksize = LOG2_EXT2_BLOCK_SIZE (ext2fs_root);
	unsigned int filesize = __le32_to_cpu(file_inode->size);
	int previous_block_number = -1;
	int delayed_start = 0;
	int delayed_extent = 0;
	int delayed_next = 0;
	char * delayed_buf = NULL;

	/* Adjust len so it we can't read past the end of the file.  */
	if (len > filesize)
		len = filesize;

	blockcnt = ((len + pos) + blocksize - 1) / blocksize;

	for (i = pos / blocksize; i < blockcnt; i++) {
		int blknr;
		int blockend = blocksize;
		int skipfirst = 0;
		blknr = readblock (file_inode, i);
		if (blknr < 0) {
			return (-1);
		}
		blknr = blknr << log2blocksize;

		if (blknr) {
			if (previous_block_number != -1) {
				if (delayed_next == blknr) {
				delayed_extent += blockend;
				delayed_next += blockend >> SECTOR_BITS;
				} else { /* spill */
					PUT(dev_desc,(uint64_t)(delayed_start*SECTOR_SIZE), delayed_buf, (uint32_t)delayed_extent);
					previous_block_number = blknr;
					delayed_start = blknr;
					delayed_extent = blockend;
					delayed_buf = buf;
					delayed_next = blknr +
						(blockend >> SECTOR_BITS);
				}
			} else {
				previous_block_number = blknr;
				delayed_start = blknr;
				delayed_extent = blockend;
				delayed_buf = buf;
				delayed_next = blknr + (blockend >> SECTOR_BITS);
			}
		} else {
			if (previous_block_number != -1) {
				/* spill */
				PUT(dev_desc,(uint64_t)(delayed_start*SECTOR_SIZE), delayed_buf, (uint32_t)delayed_extent);

			 	previous_block_number = -1;
			}
			memset(buf, 0, blocksize - skipfirst);
		}
		buf += blocksize - skipfirst;
	}
	if (previous_block_number != -1) {
		/* spill */
		PUT(dev_desc,(uint64_t)(delayed_start*SECTOR_SIZE), delayed_buf, (uint32_t)delayed_extent);

		previous_block_number = -1;
	}
	return (len);
}

static int ext2fs_iterate_dir (ext2fs_node_t dir, char *name, ext2fs_node_t * fnode, int *ftype)
{
	unsigned int fpos = 0;
	int status;
	struct ext2fs_node *diro = (struct ext2fs_node *) dir;

#ifdef DEBUG
	if (name != NULL)
		printf ("Iterate dir %s\n", name);
#endif /* of DEBUG */
	if (!diro->inode_read) {
		status = ext2fs_read_inode (diro->data, diro->ino,
					    &diro->inode);
		if (status == 0)
			return (0);
	}
	/* Search the file.  */
	while (fpos < __le32_to_cpu (diro->inode.size)) {
		struct ext2_dirent dirent;

		status = ext2fs_read_file (diro, fpos,
			sizeof (struct ext2_dirent),
			(char *) &dirent);
		if (status < 1)
			return (0);

		if (dirent.namelen != 0) {
			char filename[dirent.namelen + 1];
			ext2fs_node_t fdiro;
			int type = FILETYPE_UNKNOWN;

			status = ext2fs_read_file (diro,
						   fpos + sizeof (struct ext2_dirent),
						   dirent.namelen, filename);
			if (status < 1)
				return (0);

			fdiro = memalign(ARCH_DMA_MINALIGN, sizeof (struct ext2fs_node));
			if (!fdiro) {
				return (0);
			}

			fdiro->data = diro->data;
			fdiro->ino = __le32_to_cpu (dirent.inode);

			filename[dirent.namelen] = '\0';

			if (dirent.filetype != FILETYPE_UNKNOWN) {
				fdiro->inode_read = 0;

				if (dirent.filetype == FILETYPE_DIRECTORY) {
					type = FILETYPE_DIRECTORY;
				} else if (dirent.filetype ==
					   FILETYPE_SYMLINK) {
					type = FILETYPE_SYMLINK;
				} else if (dirent.filetype == FILETYPE_REG) {
					type = FILETYPE_REG;
				}
			} else {
				/* The filetype can not be read from the dirent, get it from inode */

				status = ext2fs_read_inode (diro->data,
					__le32_to_cpu(dirent.inode),
					&fdiro->inode);
				if (status == 0) {
					free (fdiro);
					return (0);
				}
				fdiro->inode_read = 1;

				if ((__le16_to_cpu (fdiro->inode.mode) &
					FILETYPE_INO_MASK) == FILETYPE_INO_DIRECTORY) {
					type = FILETYPE_DIRECTORY;
				} else if ((__le16_to_cpu (fdiro->inode.mode)
					& FILETYPE_INO_MASK) ==
					FILETYPE_INO_SYMLINK) {
					type = FILETYPE_SYMLINK;
				} else if ((__le16_to_cpu (fdiro->inode.mode)
					& FILETYPE_INO_MASK) ==
					FILETYPE_INO_REG) {
					type = FILETYPE_REG;
				}
			}
#ifdef DEBUG
			printf ("iterate >%s<\n", filename);
#endif /* of DEBUG */
			if ((name != NULL) && (fnode != NULL)
				&& (ftype != NULL)) {
				if (strcmp (filename, name) == 0) {
					*ftype = type;
					*fnode = fdiro;
					return (1);
				}
			} else {
				if (fdiro->inode_read == 0) {
					status = ext2fs_read_inode (diro->data,
					__le32_to_cpu (dirent.inode),
					&fdiro->inode);
					if (status == 0) {
						free (fdiro);
						return (0);
					}
					fdiro->inode_read = 1;
				}
				switch (type) {
				case FILETYPE_DIRECTORY:
					printf ("<DIR> ");
					break;
				case FILETYPE_SYMLINK:
					printf ("<SYM> ");
					break;
				case FILETYPE_REG:
					printf ("      ");
					break;
				default:
					printf ("< ? > ");
					break;
				}
				printf ("%10d %s\n",
					__le32_to_cpu (fdiro->inode.size),
					filename);
			}
			free (fdiro);
		}
		fpos += __le16_to_cpu (dirent.direntlen);
	}
	return (0);
}


static char *ext2fs_read_symlink (ext2fs_node_t node)
{
	char *symlink;
	struct ext2fs_node *diro = node;
	int status;

	if (!diro->inode_read) {
		status = ext2fs_read_inode (diro->data, diro->ino,
					    &diro->inode);
		if (status == 0)
			return (0);
	}
	symlink = memalign(ARCH_DMA_MINALIGN, __le32_to_cpu (diro->inode.size) + 1);
	if (!symlink)
		return (0);

	/* If the filesize of the symlink is bigger than
	   60 the symlink is stored in a separate block,
	   otherwise it is stored in the inode.  */
	if (__le32_to_cpu (diro->inode.size) <= 60) {
		strncpy (symlink, diro->inode.b.symlink,
			 __le32_to_cpu (diro->inode.size));
	} else {
		status = ext2fs_read_file (diro, 0,
		   __le32_to_cpu (diro->inode.size), symlink);
		if (status == 0) {
			free (symlink);
			return (0);
		}
	}
	symlink[__le32_to_cpu (diro->inode.size)] = '\0';
	return (symlink);
}


int ext2fs_find_file1
	(const char *currpath,
	 ext2fs_node_t currroot, ext2fs_node_t * currfound, int *foundtype)
{
	char fpath[strlen (currpath) + 1];
	char *name = fpath;
	char *next;
	int status;
	int type = FILETYPE_DIRECTORY;
	ext2fs_node_t currnode = currroot;
	ext2fs_node_t oldnode = currroot;

	strncpy (fpath, currpath, strlen (currpath) + 1);

	/* Remove all leading slashes.  */
	while (*name == '/') {
		name++;
	}
	if (!*name) {
		*currfound = currnode;
		return (1);
	}

	for (;;) {
		int found;

		/* Extract the actual part from the pathname.  */
		next = strchr (name, '/');
		if (next) {
			/* Remove all leading slashes.  */
			while (*next == '/') {
				*(next++) = '\0';
			}
		}

		/* At this point it is expected that the current node
			is a directory, check if this is true.  */
		if (type != FILETYPE_DIRECTORY) {
			ext2fs_free_node (currnode, currroot);
			return (0);
		}

		oldnode = currnode;

		/* Iterate over the directory.  */
		found = ext2fs_iterate_dir (currnode, name, &currnode, &type);
		if (found == 0)
			return (0);
		if (found == -1)
			break;

		/* Read in the symlink and follow it.  */
		if (type == FILETYPE_SYMLINK) {
			char *symlink;

			/* Test if the symlink does not loop.  */
			if (++symlinknest == 8) {
				ext2fs_free_node (currnode, currroot);
				ext2fs_free_node (oldnode, currroot);
				return (0);
			}

			symlink = ext2fs_read_symlink (currnode);
			ext2fs_free_node (currnode, currroot);

			if (!symlink) {
				ext2fs_free_node (oldnode, currroot);
				return (0);
			}
#ifdef DEBUG
			printf ("Got symlink >%s<\n", symlink);
#endif /* of DEBUG */
			/* The symlink is an absolute path, go back to the root inode.  */
			if (symlink[0] == '/') {
				ext2fs_free_node (oldnode, currroot);
				oldnode = &ext2fs_root->diropen;
			}

			/* Lookup the node the symlink points to.  */
			status = ext2fs_find_file1 (symlink, oldnode,
				&currnode, &type);

			free (symlink);

			if (status == 0) {
				ext2fs_free_node (oldnode, currroot);
				return (0);
			}
		}

		ext2fs_free_node (oldnode, currroot);

		/* Found the node!  */
		if (!next || *next == '\0') {
			*currfound = currnode;
			*foundtype = type;
			return (1);
		}
		name = next;
	}
	return (-1);
}


int ext2fs_find_file
	(const char *path,
	 ext2fs_node_t rootnode, ext2fs_node_t * foundnode, int expecttype)
{
	int status;
	int foundtype = FILETYPE_DIRECTORY;

	symlinknest = 0;
	if (!path)
		return (0);

	status = ext2fs_find_file1 (path, rootnode, foundnode, &foundtype);
	if (status == 0)
		return (0);

	/* Check if the node that was found was of the expected type.  */
	if ((expecttype == FILETYPE_REG) && (foundtype != expecttype)) {
		return (0);
	} else if ((expecttype == FILETYPE_DIRECTORY)
		   && (foundtype != expecttype)) {
		return (0);
	}
	return (1);
}



int  get_inode_no_from_inodebitmap(unsigned char *buffer)
{
	int j=0;
	unsigned char input;
	int operand;
	int status;
	int count =1;


	/*get the blocksize of the filesystem*/
	unsigned char *ptr=buffer;
	while(*ptr==255)
	{
		ptr++;
		count+=8;
		if(count>(uint32_t)ext2fs_root->sblock.inodes_per_group)
			return -1;
	}


	for(j=0;j<blocksize;j++)
	{
		input=*ptr;
		int i=0;
		while(i<=7)
		{
			operand=1<<i;
			status = input & operand;
			if(status)
			{
				i++;
				count++;
			}
			else
			{
				*ptr|= operand;
				return count;
			}
		}
		 ptr=ptr+1;
	}
	return -1;
}



int  get_block_no_from_blockbitmap(unsigned char *buffer)
{
	int j=0;
	unsigned char input;
	int operand;
	int status;
	int count =0;
	int blocksize;

	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);
	unsigned char *ptr=buffer;

	while(*ptr==255)
	{
		ptr++;
		count+=8;
		if(count==(blocksize *8))
			return -1;
	}


	for(j=0;j<blocksize;j++)
	{
		input=*ptr;
		int i=0;
		while(i<=7)
		{
			operand=1<<i;
			status = input & operand;
			if(status)
			{
				i++;
				count++;
			}
			else
			{
				*ptr|= operand;
				return count;
			}
		}
		 ptr=ptr+1;
	}
	return -1;
}

int update_bmap_with_one(unsigned long blockno, unsigned char * buffer, int index)
{
	unsigned int i;
	unsigned int remainder;
	unsigned char  *ptr=buffer;
	unsigned char operand;
	int status;
	int blocksize;
	i=blockno/8;
	remainder=blockno%8;

	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);
	i=i-(index * blocksize);

	ptr=ptr+i;
	operand=1<<remainder;
	status=*ptr & operand;
	if(status)
	{
		return -1;
	}
	else
	{
		*ptr=*ptr|operand;
		return 0;
	}


}
int update_bmap_with_zero(unsigned long blockno, unsigned char * buffer, int index)
{
	unsigned int i;
	unsigned int remainder;
	unsigned char  *ptr=buffer;
	unsigned char operand;
	int status;
	i=blockno/8;
	remainder=blockno%8;
	int blocksize;

	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);
	i=i-(index * blocksize);

	ptr=ptr+i;
	operand=(1<<remainder);
	status=*ptr & operand;
	if(status)
	{
		*ptr=*ptr & ~(operand);
		return 0;

	}
	else
	{
		return -1;
	}

}


int update_inode_bmap_with_one(unsigned long inode_no, unsigned char * buffer, int index)
{
	unsigned int i;
	unsigned int remainder;
	unsigned char  *ptr=buffer;
	unsigned char operand;
	int status;

	inode_no -= (index * (uint32_t)ext2fs_root->sblock.inodes_per_group);

	i=inode_no/8;
	remainder=inode_no%8;
	if(remainder==0)
	{
		ptr=ptr+i-1;
		operand=(1<<7);
	}
	else
	{
		ptr=ptr+i;
		operand=(1<<(remainder-1));
	}
	status=*ptr & operand;
	if(status)
	{
		return -1;
	}
	else
	{
		*ptr=*ptr|operand;
		return 0;
	}

}

int update_inode_bmap_with_zero(unsigned long inode_no, unsigned char * buffer,int index)
{
	unsigned int i;
	unsigned int remainder;
	unsigned char  *ptr=buffer;
	unsigned char operand;
	int status;

	inode_no -= (index * (uint32_t)ext2fs_root->sblock.inodes_per_group);
	i=inode_no/8;
	remainder=inode_no%8;
	if(remainder==0)
	{
		ptr=ptr+i-1;
		operand=(1<<7);
	}
	else
	{
		ptr=ptr+i;
		operand=(1<<(remainder-1));
	}
	status=*ptr & operand;
	if(status)
	{
		*ptr=*ptr & ~(operand);
		return 0;

	}
	else
	{
		return -1;
	}

}

unsigned long get_next_availbale_block_no(block_dev_desc_t *dev_desc,int part_no)
{
	int i;
	int blocksize;
	unsigned int bg_bitmap_index;



	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);

	/*calucalate sector per block*/
	sector_per_block=blocksize/SECTOR_SIZE;
	struct ext2_block_group *gd=(struct ext2_block_group*)gd_table;


	if(first_execution==0)
	{
		for(i=0;i<no_blockgroup;i++)
		{
			if(gd[i].free_blocks)
			{
#ifdef DEBUG_LOG
				printf("Block group reading is %d\n",i);
#endif
				blockno=get_block_no_from_blockbitmap(bg[i]);
				if(blockno==-1) /*if block bitmap is completely fill*/
				{
					continue;
				}
				blockno=blockno +(i*blocksize*8);
#ifdef DEBUG_LOG
				printf("every first block no getting is %d\n",blockno);
#endif

				first_execution++;
				gd[i].free_blocks--;
				sb->free_blocks--;
				return  blockno;
			}
			else
			{
#ifdef DEBUG_LOG
				printf("no space left on the block group %d \n",i);
#endif
			}
		}
	}
	else
	{
		restart:
		blockno++;
		/*get the blockbitmap index respective to blockno */
		bg_bitmap_index=blockno/(uint32_t)ext2fs_root->sblock.blocks_per_group;	
		if(update_bmap_with_one(blockno,bg[bg_bitmap_index],bg_bitmap_index)!=0)
		{
#ifdef DEBUG_LOG
			printf("going for restart for the block no %u %d\n",blockno,bg_bitmap_index);
#endif
			goto restart;
		}

		gd[bg_bitmap_index].free_blocks--;
		sb->free_blocks--;
		return blockno;
	}
	return 0;
}

unsigned long get_next_availbale_inode_no(block_dev_desc_t *dev_desc,int part_no)
{
	int i;
	unsigned int inode_bitmap_index;

	struct ext2_block_group *gd=(struct ext2_block_group*)gd_table;

	if(first_execution_inode==0)
	{
		for(i=0;i<no_blockgroup;i++)
		{
			if(gd[i].free_inodes)
			{
#ifdef DEBUG_LOG
				printf("Block group reading is %d\n",i);
#endif
				inode_no=get_inode_no_from_inodebitmap(inode_bmaps[i]);
				if(inode_no==-1) /*if block bitmap is completely fill*/
				{
					continue;
				}
				inode_no=inode_no +(i*ext2fs_root->sblock.inodes_per_group);
#ifdef DEBUG_LOG
				printf("every first inode no getting is %d\n",inode_no);
#endif

				first_execution_inode++;
				gd[i].free_inodes--;
				sb->free_inodes--;
				return  inode_no;
			}
			else
			{
#ifdef DEBUG_LOG
				printf("no inode left on the block group %d \n",i);
#endif
			}
		}
	}
	else
	{
		restart:
		inode_no++;
		/*get the blockbitmap index respective to blockno */
		inode_bitmap_index=inode_no/(uint32_t)ext2fs_root->sblock.inodes_per_group;
		if(update_inode_bmap_with_one(inode_no,inode_bmaps[inode_bitmap_index],inode_bitmap_index)!=0)
		{
#ifdef DEBUG_LOG
			printf("going for restart for the block no %u %d\n",inode_no,inode_bitmap_index);
#endif
			goto restart;
		}

		gd[inode_bitmap_index].free_inodes--;
		sb->free_inodes--;
		return inode_no;
	}
	return 0;
}

int check_filename_exists_in_root(block_dev_desc_t *dev_desc,char *filename)
{

	unsigned char *root_first_block_buffer;
	unsigned char *root_first_block_addr;
	unsigned int first_block_no_of_root;
	int blocksize;
	int sector_per_block;
	struct ext2_dirent *dir = NULL;
	struct ext2_dirent *previous_dir = NULL;
	int totalbytes=0;
	int templength=0;
	int status;
	int found=0;
	unsigned char *ptr;
	int inodeno;

	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);

	/*get the first block of root*/
	first_block_no_of_root=ext2fs_root->inode->b.blocks.dir_blocks[0];


	/*calucalate sector per block*/
	sector_per_block=blocksize/SECTOR_SIZE;

	/*read the first block of root directory inode*/
	root_first_block_buffer=xzalloc (blocksize);//uma malloc changed to xzalloc
	root_first_block_addr=root_first_block_buffer;
	status = ext2fs_devread (first_block_no_of_root*sector_per_block, 0, blocksize,(char *)root_first_block_buffer);
	if (status == 0)
	{
		goto fail;
	}
	else
	{
		dir=(struct ext2_dirent *) root_first_block_buffer;
		ptr=(unsigned char*)dir;
		totalbytes=0;
		while(dir->direntlen>=0)
		{
			/*blocksize-totalbytes because last directory length i.e.,
			*dir->direntlen is free availble space in the block that means
			*it is a last entry of directory entry
			*/
#ifdef DEBUG_LOG
			printf("in deleting the entry---------------------------\n");
			printf("current inode no is %d\n",dir->inode);
			printf("current directory entry length is %d\n",dir->direntlen);
			printf("current name length is %d\n",dir->namelen);
			printf("current fileltype is %d\n",dir->filetype);
			printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
			printf("-------------------------------------\n");
#endif


			if(strcmp(filename,(ptr+sizeof(struct ext2_dirent)))==0)
			{
				printf("file found deleting\n");
				previous_dir->direntlen+=dir->direntlen;
				inodeno=dir->inode;
				dir->inode=0;
				found=1;
				break;
			}

			if(blocksize-totalbytes==dir->direntlen)
			{
				break;
			}
			/*traversing the each directory entry*/
			templength=dir->direntlen;
			totalbytes=totalbytes+templength;
			previous_dir=dir;
			dir=(unsigned char*)dir;
			dir=(unsigned char*)dir+templength;
			dir=(struct ext2_dirent*)dir;
			ptr=dir;
		}

	}


	if(found==1)
	{
		PUT(dev_desc,((uint64_t)(first_block_no_of_root * blocksize)),root_first_block_addr, blocksize);
		return inodeno;
	}
	else
	{
		return -1;
	}

fail:
    free(root_first_block_buffer);

    return -1;
}



int delete_file(block_dev_desc_t *dev_desc, unsigned int inodeno)
{
	struct ext2_data *diro=ext2fs_root;
	struct ext2_inode inode;
	int status;
	unsigned int i;
	int sector_per_block;
	unsigned int blknr;
	unsigned int inode_no;
	int bg_bitmap_index;
	int inode_bitmap_index;
	unsigned char* read_buffer = NULL;
	unsigned char* start_block_address = NULL;
	unsigned int* double_indirect_buffer = NULL;
	unsigned int* DIB_start_addr = NULL;
	struct ext2_block_group *gd;


	/*get the block group descriptor table*/
	gd=(struct ext2_block_group*)gd_table;

	status = ext2fs_read_inode (diro,inodeno,&inode);
	if (status == 0)
	{
		goto fail;
	}

	/*calucalate sector per block*/
	sector_per_block=blocksize/SECTOR_SIZE;


	/*deleting the single indirect block associated with inode*/
	if(inode.b.blocks.indir_block!=0)
	{
#ifdef DEBUG_LOG
		printf("SIPB releasing %d\n",inode.b.blocks.indir_block);
#endif
		blknr=inode.b.blocks.indir_block;
		bg_bitmap_index=blknr/(uint32_t)ext2fs_root->sblock.blocks_per_group;
		update_bmap_with_zero(blknr,bg[bg_bitmap_index],bg_bitmap_index);
		gd[bg_bitmap_index].free_blocks++;
		sb->free_blocks++;
	}


	/*deleting the double indirect blocks */

	if(inode.b.blocks.double_indir_block!=0)
	{
		double_indirect_buffer=(unsigned int*)xzalloc(blocksize);
		DIB_start_addr=(unsigned int*)double_indirect_buffer;
		blknr=inode.b.blocks.double_indir_block;
		status = ext2fs_devread (blknr*sector_per_block, 0, blocksize,(unsigned int *)double_indirect_buffer);
		for(i=0;i<blocksize/sizeof(unsigned int);i++)
		{
			if(*double_indirect_buffer==0)
			break;
#ifdef DEBUG_LOG
			printf("DICB releasing %d\n",*double_indirect_buffer);
#endif
			bg_bitmap_index=(*double_indirect_buffer)/(uint32_t)ext2fs_root->sblock.blocks_per_group;
			update_bmap_with_zero(*double_indirect_buffer,bg[bg_bitmap_index],bg_bitmap_index);
			double_indirect_buffer++;
			gd[bg_bitmap_index].free_blocks++;
			sb->free_blocks++;
		}

		/*removing the parent double indirect block*/
		/*find the bitmap index*/
		blknr=inode.b.blocks.double_indir_block;
		bg_bitmap_index=blknr/(uint32_t)ext2fs_root->sblock.blocks_per_group;
		update_bmap_with_zero(blknr,bg[bg_bitmap_index],bg_bitmap_index);
		gd[bg_bitmap_index].free_blocks++;
		sb->free_blocks++;
#ifdef DEBUG_LOG
		printf("DIPB releasing %d\n",blknr);
#endif
		if(DIB_start_addr)
			free(DIB_start_addr);
	}



	/*read the block no allocated to a file*/
	for(i=0;i<inode.size/blocksize;i++)
	{
		blknr = readblock (&inode, i);
		bg_bitmap_index=blknr/(uint32_t)ext2fs_root->sblock.blocks_per_group;
		update_bmap_with_zero(blknr,bg[bg_bitmap_index],bg_bitmap_index);
#ifdef DEBUG_LOG
		printf("ActualB releasing %d: %d\n",blknr,bg_bitmap_index);
#endif
		gd[bg_bitmap_index].free_blocks++;
		sb->free_blocks++;

	}

	/*from the inode no to blockno*/
	unsigned int inodes_per_block;
	unsigned int blkno;
	unsigned int blkoff;
	struct ext2_inode *inode_buffer;
	inodes_per_block = EXT2_BLOCK_SIZE(ext2fs_root) / inode_size;


	/*get the block no*/
	inodeno--;
	blkno = __le32_to_cpu (gd[0].inode_table_id) +
	(inodeno % __le32_to_cpu (ext2fs_root->sblock.inodes_per_group))/ inodes_per_block;


	/*get the offset of the inode*/
	blkoff = ((inodeno) % inodes_per_block) * inode_size;

	/*read the block no containing the inode*/
	read_buffer=xzalloc(blocksize);
	start_block_address=read_buffer;
	status = ext2fs_devread (blkno* sector_per_block, 0, blocksize,(unsigned  char *) read_buffer);
	if(status==0)
	{
		goto fail;
	}
	read_buffer=read_buffer+blkoff;
	inode_buffer=(struct ext2_inode*)read_buffer;
	memset(inode_buffer,'\0',sizeof(struct ext2_inode));

	/*write the inode to original position in inode table*/
	PUT(dev_desc,((uint64_t)(blkno * blocksize)),start_block_address, blocksize);

	/*update the respective inode bitmaps*/
	inode_no=++inodeno;
	inode_bitmap_index=inode_no/(uint32_t)ext2fs_root->sblock.inodes_per_group;
	update_inode_bmap_with_zero(inode_no,inode_bmaps[inode_bitmap_index],inode_bitmap_index);
	gd[inode_bitmap_index].free_inodes++;
	sb->free_inodes++;

	ext2_fs_update(dev_desc);


	/*free*/
	if(start_block_address)
		free(start_block_address);
	return 0;

	fail:
	if(DIB_start_addr)
		free(DIB_start_addr);
	if(start_block_address)
		free(start_block_address);
	return -1;
}

#ifdef DEBUG_LOG
void dump_the_block_group_before_writing()
{
	int i,j;
	int blocksize;
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);
	for(i=0;i<no_blockgroup;i++)
	{
		printf("\n-----------------\n");
		printf("[BEFORE WRITING ]block group  %d\n",i);
		printf("-----------------\n");
		for(j=0;j<blocksize;j++)
		{
			printf("%u",*(bg[i]+j));
		}
	}
}

void dump_the_block_group_after_writing()
{
	int i,j;
	int blocksize;
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);
	for(i=0;i<no_blockgroup;i++)
	{
		printf("\n-----------------\n");
		printf("[AFTER WRITING]block group %d\n",i);
		printf("-----------------\n");
		for(j=0;j<blocksize;j++)
		{
			printf("%u",*(bg[i]+j));
		}
	}
}

void dump_the_inode_group_before_writing()
{
	int i,j;
	int blocksize;
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);
	for(i=0;i<no_blockgroup;i++)
	{
		printf("\n-----------------\n");
		printf("[BEFORE WRITING ]block group  %d\n",i);
		printf("-----------------\n");
		for(j=0;j<blocksize;j++)
		{
			printf("%u",*(inode_bmaps[i]+j));
		}
	}
}


void dump_the_inode_group_after_writing()
{
	int i,j;
	int blocksize;
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);
	for(i=0;i<no_blockgroup;i++)
	{
		printf("\n-----------------\n");
		printf("[AFTER WRITING]block group %d\n",i);
		printf("-----------------\n");
		for(j=0;j<blocksize;j++)
		{
			printf("%u",*(inode_bmaps[i]+j));
		}
	}
}

#endif

int get_bgdtable()
{
	int blocksize;
	int sector_per_block;
	int status;

	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);

	/*calucalate sector per block*/
	sector_per_block=blocksize/SECTOR_SIZE;

	/*allocate mem for gdtable*/
	gd_table=xzalloc(blocksize);

	//printf("gdtable block no is %d\n",gd_table_block_no);

	/*Read the first group descriptor table*/
	status = ext2fs_devread (gd_table_block_no * sector_per_block, 0,blocksize,gd_table);
	if (status == 0)
	{
		goto fail;
	}
	return 0;

	fail:
	return -1;

}

void ext2_fs_deinit()
{
	int i;
	if(sb)
	{
		free(sb);
		sb=NULL;
	}
	if(bg)
	{
		for(i=0;i<no_blockgroup;i++)
		{
			if(bg[i])
			{
				free(bg[i]);
				bg[i]=NULL;
			}
		}
		free(bg);
		bg=NULL;
	}


	if(inode_bmaps)
	{
		for(i=0;i<no_blockgroup;i++)
		{
			if(inode_bmaps[i])
			{
				free(inode_bmaps[i]);
				inode_bmaps[i]=NULL;
			}
		}
		free(inode_bmaps);
		inode_bmaps=NULL;
	}


	if(gd_table)
	{
		free(gd_table);
		gd_table=NULL;
	}

	if(gd)/*it just a pointer to gd_table*/
	{
		gd=NULL;
	}

	return;
}


int ext2_fs_init()
{
	int status;
	int i;

	/*get the blocksize inode sector per block of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext2fs_root);
	inodesize=INODE_SIZE_FILESYSTEM(ext2fs_root);
	sector_per_block=blocksize/SECTOR_SIZE;


	/*get the superblock*/
	sb=(struct ext2_sblock *)xzalloc(SUPERBLOCK_SIZE);
	status = ext2fs_devread (SUPERBLOCK_SECTOR, 0, SUPERBLOCK_SIZE, sb);
	if(status==0)
	{
		goto fail;
	}

	/*get the block group descriptor table*/
	gd_table_block_no=((EXT2_MIN_BLOCK_SIZE == blocksize)+1);
	if(get_bgdtable()== -1)
	{
		printf("***Eror in getting the block group descriptor table\n");
		goto fail;
	}
	else
	{
		gd=(struct ext2_block_group*)gd_table;
	}

	/*get the no of blockgroups*/
	no_blockgroup = (uint32_t)div_roundup((uint32_t)(ext2fs_root->sblock.total_blocks - ext2fs_root->sblock.first_data_block),(uint32_t)ext2fs_root->sblock.blocks_per_group);


	/*load all the available bitmap block of the partition*/
	bg=(unsigned char **)malloc(no_blockgroup * sizeof(unsigned char *));
	for(i=0;i<no_blockgroup;i++)
	{
		bg[i]=(unsigned char*)xzalloc(blocksize);
	}

	for(i=0;i<no_blockgroup;i++)
	{
		status = ext2fs_devread (gd[i].block_id*sector_per_block, 0, blocksize, (unsigned char*)bg[i]);
		if(status==0)
		{
			goto fail;
		}
	}


	/*load all the available inode bitmap of the partition*/
	inode_bmaps=(unsigned char **)malloc(no_blockgroup * sizeof(unsigned char *));
	for(i=0;i<no_blockgroup;i++)
	{
		inode_bmaps[i]=(unsigned char*)xzalloc(blocksize);
	}

	for(i=0;i<no_blockgroup;i++)
	{
		status = ext2fs_devread (gd[i].inode_id*sector_per_block, 0, blocksize, (unsigned char*)inode_bmaps[i]);
		if(status==0)
		{
			goto fail;
		}
	}


#ifdef DEBUG_LOG
	printf("-----------------------------\n");
	printf("inode size: %d\n",inodesize);
	printf("block size: %d\n",blocksize);
	printf("blocks per group: %d\n",ext2fs_root->sblock.blocks_per_group);
	printf("no of groups in this partition: %d\n",no_blockgroup);
	printf("block group descriptor blockno: %d\n",gd_table_block_no);
	printf("sector size is %d\n",SECTOR_SIZE);
	printf("size of inode structure is %d\n",sizeof(struct ext2_inode));
	printf("-----------------------------\n");
#endif

#ifdef DEBUG_LOG
	dump_the_inode_group_before_writing();
#endif

	return 0;
	fail:
	ext2_fs_deinit();
	return -1;
}


int ext2_fs_update(block_dev_desc_t *dev_desc)
{
	int i;
	/*Update the super block*/
	PUT(dev_desc,(uint64_t)(SUPERBLOCK_SIZE),(struct ext2_sblock*)sb,(uint32_t)SUPERBLOCK_SIZE);

	for(i=0;i<no_blockgroup;i++)
	{
		PUT(dev_desc,(uint64_t)(gd[i].block_id*blocksize),bg[i],(uint32_t)blocksize);
	}

	for(i=0;i<no_blockgroup;i++)
	{
		PUT(dev_desc,(uint64_t)(gd[i].inode_id*blocksize),inode_bmaps[i],(uint32_t)blocksize);
	}

	/*update the block group descriptor table*/
	PUT(dev_desc,(uint64_t)(gd_table_block_no*blocksize),(struct ext2_block_group*)gd_table,(uint32_t)blocksize);
	return 0;
}




int ext2fs_write(block_dev_desc_t *dev_desc, int part_no,char *filename, unsigned char* buffer,unsigned long sizebytes)
{
	unsigned char *root_first_block_buffer=NULL;
	unsigned int i,j;
	int status;

	/*inode*/
	struct ext2_inode *file_inode=NULL;
	unsigned char *inode_buffer=NULL;
	unsigned int inodeno;
	time_t timestamp = 0;
	unsigned int first_block_no_of_root;

	/*directory entry*/
	struct ext2_dirent *dir=NULL;
	int templength=0;
	int totalbytes=0;
	short int padding_factor=0;

	/*filesize*/
	uint64_t total_no_of_bytes_for_file;
	unsigned int total_no_of_block_for_file;
	unsigned int total_remaining_blocks;

	/*final actual data block*/
	unsigned int 	actual_block_no;

	/*direct*/
	unsigned int 	direct_blockno;

	/*single indirect*/
	unsigned int 	*single_indirect_buffer=NULL;
	unsigned int 	single_indirect_blockno;
	unsigned int 	*single_indirect_start_address=NULL;

	/*double indirect*/
	unsigned int 	double_indirect_blockno_parent;
	unsigned int 	double_indirect_blockno_child;
	unsigned int 	*double_indirect_buffer_parent=NULL;
	unsigned int 	*double_indirect_buffer_child=NULL;
	unsigned int 	*double_indirect_block_startaddress=NULL;
	unsigned int 	*double_indirect_buffer_child_start=NULL;
	unsigned int 	existing_file_inodeno;


	if(ext2_fs_init()!=0)
	{
		printf("error in File System init\n");
		return -1;
	}

	/*check if the filename is already present in root*/
	existing_file_inodeno=check_filename_exists_in_root(dev_desc,filename);
	if(existing_file_inodeno!=-1)/*file is exist having the same name as filename*/
	{
		delete_file(dev_desc,existing_file_inodeno);
		first_execution=0;
		blockno=0;

		first_execution_inode=0;
		inode_no=0;
	}


	/*calucalate how many blocks required*/
	total_no_of_bytes_for_file=sizebytes;
	total_no_of_block_for_file=lldiv(total_no_of_bytes_for_file, blocksize);
	if(do_div(total_no_of_bytes_for_file, blocksize) !=0)
	{
		total_no_of_block_for_file++;
	}
	total_remaining_blocks=total_no_of_block_for_file;


#ifdef DEBUG_LOG
	printf("total_no_of_block_for_file %d\n",total_no_of_block_for_file);
	printf("superblock freebloks:%u\n",sb->free_blocks);
#endif


	/*Test for available space in partition*/
	if(sb->free_blocks < total_no_of_block_for_file)
	{
		printf("Not enough space on partition !!!\n");
		goto fail;
	}


	/*prepare file inode*/
	inode_buffer = xmalloc(inodesize);
	memset(inode_buffer, '\0', inodesize);
	file_inode = (struct ext2_inode *)inode_buffer;
	file_inode->mode= S_IFREG | S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH;
	file_inode->mtime= timestamp;
	file_inode->atime= timestamp;
	file_inode->ctime= timestamp;
	file_inode->nlinks= 1;
	file_inode->size = sizebytes;
	file_inode->blockcnt= (total_no_of_block_for_file*blocksize)/SECTOR_SIZE;



	/*----------------------------START:Allocation of Blocks to Inode---------------------------- */
	/*allocation of direct blocks*/
	for(i=0;i<INDIRECT_BLOCKS;i++)
	{
		direct_blockno=get_next_availbale_block_no(dev_desc,part_no);
		file_inode->b.blocks.dir_blocks[i]=direct_blockno;
#ifdef DEBUG_LOG
		printf("DB %d: %d\n",direct_blockno,total_remaining_blocks);
#endif
		total_remaining_blocks--;
		if(total_remaining_blocks==0)
		break;
	}


	/*allocation of single indirect blocks*/
	if(total_remaining_blocks!=0)
	{
		single_indirect_buffer=malloc(blocksize);
		single_indirect_start_address=single_indirect_buffer;
		single_indirect_blockno=get_next_availbale_block_no(dev_desc,part_no);
#ifdef DEBUG_LOG
		printf("SIPB %d: %d\n",single_indirect_blockno,total_remaining_blocks);
#endif
		status = ext2fs_devread (single_indirect_blockno*sector_per_block, 0, blocksize,(unsigned int *)single_indirect_buffer);
		memset(single_indirect_buffer,'\0',blocksize);
		if (status == 0)
		{
			goto fail;
		}

		for(i=0;i<(blocksize/sizeof(unsigned int));i++)
		{
			actual_block_no=get_next_availbale_block_no(dev_desc,part_no);
			*single_indirect_buffer=actual_block_no;
#ifdef DEBUG_LOG
			printf("SIAB %d: %d\n",*single_indirect_buffer,total_remaining_blocks);
#endif
			single_indirect_buffer++;
			total_remaining_blocks--;
			if(total_remaining_blocks==0)
			break;
		}

		/*write the block to disk*/
		PUT(dev_desc,((uint64_t)(single_indirect_blockno * blocksize)),single_indirect_start_address, blocksize);
		file_inode->b.blocks.indir_block=single_indirect_blockno;
		if(single_indirect_start_address)
		free(single_indirect_start_address);
	}


	/*allocation of double indirect blocks*/
	if(total_remaining_blocks!=0)
	{
		/*double indirect parent block connecting to inode*/
		double_indirect_blockno_parent=get_next_availbale_block_no(dev_desc,part_no);
		double_indirect_buffer_parent=xzalloc (blocksize);
		double_indirect_block_startaddress=double_indirect_buffer_parent;
#ifdef DEBUG_LOG
		printf("DIPB %d: %d\n",double_indirect_blockno_parent,total_remaining_blocks);
#endif
		status = ext2fs_devread (double_indirect_blockno_parent*sector_per_block, 0, blocksize,(unsigned int *)double_indirect_buffer_parent);
		memset(double_indirect_buffer_parent,'\0',blocksize);

		/*START: for each double indirect parent block create one more block*/
		for(i=0;i<(blocksize/sizeof(unsigned int));i++)
		{
			double_indirect_blockno_child=get_next_availbale_block_no(dev_desc,part_no);
			double_indirect_buffer_child=xzalloc (blocksize);
			double_indirect_buffer_child_start=double_indirect_buffer_child;
			*double_indirect_buffer_parent=double_indirect_blockno_child;
			double_indirect_buffer_parent++;
#ifdef DEBUG_LOG
			printf("DICB %d: %d\n",double_indirect_blockno_child,total_remaining_blocks);
#endif
			status = ext2fs_devread (double_indirect_blockno_child*sector_per_block, 0, blocksize,(unsigned int *)double_indirect_buffer_child);
			memset(double_indirect_buffer_child,'\0',blocksize);
			/*END: for each double indirect parent block create one more block*/


			/*filling of actual datablocks for each child*/
			for(j=0;j<(blocksize/sizeof(unsigned int));j++)
			{
				actual_block_no=get_next_availbale_block_no(dev_desc,part_no);
				*double_indirect_buffer_child=actual_block_no;
#ifdef DEBUG_LOG
				printf("DIAB %d: %d\n",actual_block_no,total_remaining_blocks);
#endif
				double_indirect_buffer_child++;
				total_remaining_blocks--;
				if(total_remaining_blocks==0)
				break;
			}
			/*write the block  table*/
			PUT(dev_desc,((uint64_t)(double_indirect_blockno_child * blocksize)),double_indirect_buffer_child_start, blocksize);
			if(double_indirect_buffer_child_start)
			free(double_indirect_buffer_child_start);


			if(total_remaining_blocks==0)
				break;

		}
		PUT(dev_desc,((uint64_t)(double_indirect_blockno_parent * blocksize)),double_indirect_block_startaddress, blocksize);
		if(double_indirect_block_startaddress)
		free(double_indirect_block_startaddress);
		file_inode->b.blocks.double_indir_block=double_indirect_blockno_parent;
	}
	/*----------------------------END:Allocation of Blocks to Inode---------------------------- */

	/*get the next available inode number*/
	inodeno=get_next_availbale_inode_no(dev_desc,part_no);
#ifdef DEBUG_LOG
	printf("New Inode Assignment :%d\n",inodeno);
#endif

	/*
	*write the inode to inode table after last filled inode in inode table
	*we are using hardcoded  gd[0].inode_table_id becuase 0th blockgroup
	*is suffcient to create more than 1000 file.TODO exapnd the logic to
	*all blockgroup
	*/
	PUT(dev_desc,((uint64_t)(gd[0].inode_table_id * blocksize) + ((inodeno-1) * inodesize)),	inode_buffer, inodesize);

	/*############START: Update the root directory entry block of root inode ##############*/
    /*since we are populating this file under  root
	*directory take the root inode and its first block
	*currently we are looking only in first block of root
	*inode*/
	first_block_no_of_root=ext2fs_root->inode->b.blocks.dir_blocks[0];


    /*read the first block of root directory inode*/
	root_first_block_buffer=memalign(ARCH_DMA_MINALIGN, blocksize);
	status = ext2fs_devread (first_block_no_of_root*sector_per_block, 0, blocksize,(char *)root_first_block_buffer);
	if (status == 0)
	{
		goto fail;
	}
	else
	{
		dir=(struct ext2_dirent *) root_first_block_buffer;
		unsigned char *ptr=(unsigned char*)dir;
		totalbytes=0;
		while(dir->direntlen>=0)
		{
#ifdef DEBUG_LOG
			printf("------------------------------------\n");
			printf("current inode no is %d\n",dir->inode);
			printf("current directory entry length is %d\n",dir->direntlen);
			printf("current name length is %d\n",dir->namelen);
			printf("current fileltype is %d\n",dir->filetype);
			printf("total bytes is %d\n",totalbytes);
			printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
			printf("-------------------------------------\n");
#endif

			/*blocksize-totalbytes because last directory length i.e.,
			*dir->direntlen is free availble space in the block that means
			*it is a last entry of directory entry for more info  a.manjunatha@samsung.com
			*/

			/*traversing the each directory entry*/
			if(blocksize-totalbytes==dir->direntlen)
			{
				/*update last directory entry length to its length because
				*we are creating new directory entry*/
				if(dir->namelen%4!=0)
				{
					padding_factor=4-(dir->namelen%4);
				}
				dir->direntlen=dir->namelen+sizeof(struct ext2_dirent)+padding_factor;/*for null storage*/
				break;
			}

			templength=dir->direntlen;
			totalbytes=totalbytes+templength;
			dir=(unsigned char*)dir;
			dir=(unsigned char*)dir+templength;
			dir=(struct ext2_dirent*)dir;
			ptr=dir;
		}

		/*make a pointer ready for creating next directory entry*/
		templength=dir->direntlen;
		totalbytes=totalbytes+templength;
		dir=(unsigned char*)dir;
		dir=(unsigned char*)dir+templength;
		dir=(struct ext2_dirent*)dir;
		ptr=dir;

#if DEBUG_LOG
		printf("------------------------------------\n");
		printf("JUST BEFORE ADDING NAME\n");
		printf("------------------------------------\n");
		printf("current inode no is %d\n",dir->inode);
		printf("current directory entry length is %d\n",dir->direntlen);
		printf("current name length is %d\n",dir->namelen);
		printf("current fileltype is %d\n",dir->filetype);
		printf("total bytes is %d\n",totalbytes);
		printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
		printf("-------------------------------------\n");
#endif

		dir->inode=inodeno;
		dir->direntlen=blocksize-totalbytes;
		dir->namelen=strlen(filename);
		dir->filetype=FILETYPE_REG; /*regular file*/

		dir=(unsigned char*)dir;
		dir=(unsigned char*)dir+sizeof(struct ext2_dirent);
		memcpy(dir,filename,strlen(filename));
#if DEBUG_LOG
		printf("------------------------------------\n");
		printf("JUST AFTER ADDING NAME\n");
		printf("------------------------------------\n");
		printf("current inode no is %d\n",dir->inode);
		printf("current directory entry length is %d\n",dir->direntlen);
		printf("current name length is %d\n",dir->namelen);
		printf("current fileltype is %d\n",dir->filetype);
		printf("total bytes is %d\n",totalbytes);
		printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
		printf("-------------------------------------\n");
#endif

	}

	/*update or write  the 1st block of root inode*/
	PUT(dev_desc,((uint64_t)(first_block_no_of_root * blocksize)),	root_first_block_buffer, blocksize);
	if(root_first_block_buffer)
		free(root_first_block_buffer);
	/*############END: Update the root directory entry block of root inode ##############*/

	/*Copy the file content into data blocks*/
	ext2fs_write_file(dev_desc,file_inode,0,sizebytes,buffer);

#ifdef DEBUG_LOG
	dump_the_inode_group_after_writing();
#endif

	ext2_fs_update(dev_desc);


#ifdef DEBUG_LOG
	/*print the block no allocated to a file*/
	unsigned int blknr;
	for(i=0;i<total_no_of_block_for_file;i++)
	{
		blknr = readblock (file_inode, i);
		printf("Actual Data Blocks Allocated to File: %u\n",blknr);
	}
#endif
	ext2_fs_deinit();
	first_execution=0;
	blockno=0;

	first_execution_inode=0;
	inode_no=0;
	if(inode_buffer)
		free(inode_buffer);

return 0;

fail:
	ext2_fs_deinit();
	if(inode_buffer)
		free(inode_buffer);
	return -1;
}




int ext2fs_ls (const char *dirname)
{
	ext2fs_node_t dirnode;
	int status;

	if (ext2fs_root == NULL)
		return (0);

	status = ext2fs_find_file (dirname, &ext2fs_root->diropen, &dirnode,
		FILETYPE_DIRECTORY);
	if (status != 1) {
		printf ("** Can not find directory. **\n");
		return (1);
	}

	ext2fs_iterate_dir (dirnode, NULL, NULL, NULL);
	ext2fs_free_node (dirnode, &ext2fs_root->diropen);
	return (0);
}


int ext2fs_open (const char *filename)
{
	ext2fs_node_t fdiro = NULL;
	int status;
	int len;

	if (ext2fs_root == NULL)
		return (-1);

	ext2fs_file = NULL;
	status = ext2fs_find_file (filename, &ext2fs_root->diropen, &fdiro,
		FILETYPE_REG);
	if (status == 0)
		goto fail;

	if (!fdiro->inode_read) {
		status = ext2fs_read_inode (fdiro->data, fdiro->ino,
			&fdiro->inode);
		if (status == 0)
			goto fail;
	}
	len = __le32_to_cpu (fdiro->inode.size);
	ext2fs_file = fdiro;
	return (len);

fail:
	ext2fs_free_node (fdiro, &ext2fs_root->diropen);
	return (-1);
}


int ext2fs_close (void)
{
	if ((ext2fs_file != NULL) && (ext2fs_root != NULL)) {
		ext2fs_free_node (ext2fs_file, &ext2fs_root->diropen);
		ext2fs_file = NULL;
	}
	if (ext2fs_root != NULL) {
		free (ext2fs_root);
		ext2fs_root = NULL;
	}
	if (indir1_block != NULL) {
		free (indir1_block);
		indir1_block = NULL;
		indir1_size = 0;
		indir1_blkno = -1;
	}
	if (indir2_block != NULL) {
		free (indir2_block);
		indir2_block = NULL;
		indir2_size = 0;
		indir2_blkno = -1;
	}
	return (0);
}


int ext2fs_read (char *buf, unsigned len)
{
	int status;

	if (ext2fs_root == NULL) {
		return (0);
	}

	if (ext2fs_file == NULL) {
		return (0);
	}

	status = ext2fs_read_file (ext2fs_file, 0, len, buf);
	return (status);
}

int ext2fs_mount (unsigned part_length)
{
	struct ext2_data *data;
	int status;

	data = memalign(ARCH_DMA_MINALIGN, sizeof (struct ext2_data));
	if (!data) {
		return (0);
	}
	/* Read the superblock.  */
	status = ext2fs_devread (1 * 2, 0, sizeof (struct ext2_sblock),
		(char *) &data->sblock);

	if (status == 0)
		goto fail;

	/* Make sure this is an ext2 filesystem.  */
	if (__le16_to_cpu (data->sblock.magic) != EXT2_MAGIC) {
		goto fail;
	}
	if (__le32_to_cpu(data->sblock.revision_level == 0)) {
		inode_size = 128;
	} else {
		inode_size = __le16_to_cpu(data->sblock.inode_size);
	}
#ifdef DEBUG
	printf("EXT2 rev %d, inode_size %d\n",
		__le32_to_cpu(data->sblock.revision_level), inode_size);
#endif
	data->diropen.data = data;
	data->diropen.ino = 2;
	data->diropen.inode_read = 1;
	data->inode = &data->diropen.inode;

	status = ext2fs_read_inode (data, 2, data->inode);
	if (status == 0)
		goto fail;

	ext2fs_root = data;

	return (1);

fail:
	printf("Failed to mount ext2 filesystem...\n");
	free (data);
	ext2fs_root = NULL;
	return (0);
}
