/*
 * (C) Copyright 2011 Samsung Electronics
 *EXT4 filesystem implementation in Uboot by
 *Uma Shankar <uma.shankar@samsung.com>
 *Manjunatha C Achar <a.manjunatha@samsung.com>
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
#include "../ext4/crc16.h"
#include <div64.h>
//#define DEBUG_LOG 1

#define YES 1
#define NO 0
/* Maximum entries in 1 journal transaction */
#define MAX_JOURNAL_ENTRIES 80
struct journal_log {
		char *buf;
		int blknr;
	};

struct dirty_blocks {
		char *buf;
		int blknr;
	};

int gindex = 0;
int gd_index = 0;
int j_blkindex = 1;
struct journal_log *journal_ptr[MAX_JOURNAL_ENTRIES];
struct dirty_blocks *dirty_block_ptr[MAX_JOURNAL_ENTRIES];

void log_journal(char *journal_buffer,unsigned int blknr);
void put_metadata(char *metadata_buffer,unsigned int blknr);
void ext4fs_update(block_dev_desc_t *dev_desc);

#define EXT2_JOURNAL_INO	 		8	/* Journal inode */
#define EXT2_JOURNAL_SUPERBLOCK			0	/* Journal  Superblock number */
#define S_IFLNK					0120000	/* symbolic link */

#define JBD2_FEATURE_COMPAT_CHECKSUM		0x00000001
#define EXT3_JOURNAL_MAGIC_NUMBER		0xc03b3998U
#define TRANSACTION_RUNNING 			1
#define TRANSACTION_COMPLETE 			0
#define EXT3_FEATURE_INCOMPAT_RECOVER		0x0004 /* Needs recovery */
#define EXT3_JOURNAL_DESCRIPTOR_BLOCK		1
#define EXT3_JOURNAL_COMMIT_BLOCK		2
#define EXT3_JOURNAL_SUPERBLOCK_V1		3
#define EXT3_JOURNAL_SUPERBLOCK_V2		4
#define EXT3_JOURNAL_REVOKE_BLOCK		5
#define EXT3_JOURNAL_FLAG_ESCAPE		1
#define EXT3_JOURNAL_FLAG_SAME_UUID		2
#define EXT3_JOURNAL_FLAG_DELETED		4
#define EXT3_JOURNAL_FLAG_LAST_TAG		8
	
/*
 * Standard header for all descriptor blocks:
 */
typedef struct journal_header_s
{
	__u32 	h_magic;
	__u32 	h_blocktype;
	__u32 	h_sequence;
} journal_header_t;

/*
 * The journal superblock.  All fields are in big-endian byte order.
 */
typedef struct journal_superblock_s
{
	/* 0x0000 */
	journal_header_t s_header;

	/* 0x000C */
	/* Static information describing the journal */
	__u32 s_blocksize;	/* journal device blocksize */
	__u32 s_maxlen;		/* total blocks in journal file */
	__u32 s_first; 		/* first block of log information */

	/* 0x0018 */
	/* Dynamic information describing the current state of the log */
	__u32 s_sequence; 	/* first commit ID expected in log */
	__u32 s_start; 	/* blocknr of start of log */

	/* 0x0020 */
	/* Error value, as set by journal_abort(). */
	__s32 s_errno;

	/* 0x0024 */
	/* Remaining fields are only valid in a version-2 superblock */
	__u32 s_feature_compat; 	/* compatible feature set */
	__u32 s_feature_incompat;	/* incompatible feature set */
	__u32 s_feature_ro_compat; 	/* readonly-compatible feature set */
	/* 0x0030 */
	__u8	s_uuid[16]; 	/* 128-bit uuid for journal */

	/* 0x0040 */
	__u32 s_nr_users; 	/* Nr of filesystems sharing log */

	__u32 s_dynsuper; 	/* Blocknr of dynamic superblock copy*/

	/* 0x0048 */
	__u32 s_max_transaction;	/* Limit of journal blocks per trans.*/
	__u32 s_max_trans_data; /* Limit of data blocks per trans. */

	/* 0x0050 */
	__u32 s_padding[44];

	/* 0x0100 */
	__u8	s_users[16*48];		/* ids of all fs'es sharing the log */
	/* 0x0400 */
} journal_superblock_t;

/* To convert big endian journal superblock entries to little endian */
unsigned int be_le(unsigned int num)
{
	unsigned int swapped;
	swapped = (((num>>24)&0xff) | // move byte 3 to byte 0
		((num<<8)&0xff0000) | // move byte 1 to byte 2
		((num>>8)&0xff00) | // move byte 2 to byte 1
		((num<<24)&0xff000000)); // byte 0 to byte 3
		return swapped;
}

// 4-byte number
unsigned int le_be(unsigned int num)
{
	 return((num&0xff)<<24)+((num&0xff00)<<8)+((num&0xff0000)>>8)+((num>>24)&0xff);
}

struct ext3_journal_block_tag
{
	uint32_t block;
	uint32_t flags;
};

extern int ext2fs_devread(int sector, int byte_offset, int byte_len, char *buf);

/* Magic value used to identify an ext2 filesystem.  */
#define	EXT2_MAGIC				0xEF53
/* Amount of indirect blocks in an inode.  */
#define INDIRECT_BLOCKS				12
/* Maximum lenght of a pathname.  */
#define EXT2_PATH_MAX				4096
/* Maximum nesting of symlinks, used to prevent a loop.  */
#define	EXT2_MAX_SYMLINKCNT			8

/* Filetype used in directory entry.  */
#define	FILETYPE_UNKNOWN			0
#define	FILETYPE_REG				1
#define	FILETYPE_DIRECTORY			2
#define	FILETYPE_SYMLINK			7

/* Filetype information as used in inodes.  */
#define FILETYPE_INO_MASK			0170000
#define FILETYPE_INO_REG			0100000
#define FILETYPE_INO_DIRECTORY			0040000
#define FILETYPE_INO_SYMLINK			0120000
#define EXT2_ROOT_INO				2 /* Root inode */

/* Bits used as offset in sector */
#define DISK_SECTOR_BITS        		9

/* Log2 size of ext2 block in 512 blocks.  */
#define LOG2_EXT2_BLOCK_SIZE(data) (__le32_to_cpu (data->sblock.log2_block_size) + 1)

/* Log2 size of ext2 block in bytes.  */
#define LOG2_BLOCK_SIZE(data)	   (__le32_to_cpu (data->sblock.log2_block_size) + 10)

/* The size of an ext2 block in bytes.  */
#define EXT2_BLOCK_SIZE(data)	   (1 << LOG2_BLOCK_SIZE(data))

#define INODE_SIZE_FILESYSTEM(data)	(__le32_to_cpu (data->sblock.inode_size))
#define EXT4_EXTENTS_FLAG 			0x80000
#define EXT4_EXT_MAGIC				0xf30a

#define EXT2_FT_DIR 				2
#define SUCCESS 				1

// All fields are little-endian
struct ext4_dir {
	uint32_t inode1;
	uint16_t rec_len1;
	uint8_t	name_len1;
	uint8_t	file_type1;
	char	name1[4];
	uint32_t inode2;
	uint16_t rec_len2;
	uint8_t	name_len2;
	uint8_t	file_type2;
	char	name2[4];
	uint32_t inode3;
	uint16_t rec_len3;
	uint8_t	name_len3;
	uint8_t	file_type3;
	char	name3[12];
};

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

struct ext2_block_group
{
	__u32 block_id;	/* Blocks bitmap block */
	__u32 inode_id;	/* Inodes bitmap block */
	__u32 inode_table_id;	/* Inodes table block */
	__u16 free_blocks;	/* Free blocks count */
	__u16 free_inodes;	/* Free inodes count */
	__u16 used_dir_cnt;	/* Directories count */
	__u16 bg_flags;
	__u32 bg_reserved[2];
	__u16 bg_itable_unused; /* Unused inodes count */
	__u16 bg_checksum;	/* crc16(s_uuid+grouo_num+group_desc)*/
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

struct ext2_data *ext4fs_root = NULL;
ext2fs_node_t ext4fs_file = NULL;
int ext4fs_symlinknest = 0;
uint32_t *ext4fs_indir1_block = NULL;
int ext4fs_indir1_size = 0;
int ext4fs_indir1_blkno = -1;
uint32_t *ext4fs_indir2_block = NULL;
int ext4fs_indir2_size = 0;
int ext4fs_indir2_blkno = -1;

uint32_t *ext4fs_indir3_block = NULL;
int ext4fs_indir3_size = 0;
int ext4fs_indir3_blkno = -1;
static unsigned int inode_size;

#define BLOCK_NO_ONE			1
#define SUPERBLOCK_SECTOR		2
#define SUPERBLOCK_SIZE			1024

static int blocksize;
static int inodesize;
static int sector_per_block;
static unsigned long blockno = 0;
static unsigned long inode_no = 0;
static int first_execution = 0;
static int first_execution_inode = 0;
static int  no_blockgroup;
static unsigned char *gd_table = NULL;
static unsigned int   gd_table_block_no = 0;

/*superblock*/
static struct ext2_sblock *sb = NULL;
/*blockgroup bitmaps*/
static unsigned char **bg = NULL;
/*inode bitmaps*/
static unsigned char **inode_bmaps = NULL;
/*block group descritpor table*/
static struct ext2_block_group *gd = NULL;
/* number of blocks required for gdtable */
int no_blk_req_per_gdt;
static struct ext2_inode *g_parent_inode = NULL;
/*
 * Macro-instructions used to manage several block sizes
 */
#define EXT2_MIN_BLOCK_LOG_SIZE			10 /* 1024 */
#define EXT2_MAX_BLOCK_LOG_SIZE			16 /* 65536 */
#define EXT2_MIN_BLOCK_SIZE			(1 << EXT2_MIN_BLOCK_LOG_SIZE)
#define EXT2_MAX_BLOCK_SIZE			(1 << EXT2_MAX_BLOCK_LOG_SIZE)

#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001

#define EXT4_BG_INODE_UNINIT			0x0001 /* Inode table/bitmap not in use */
#define EXT4_BG_BLOCK_UNINIT			0x0002 /* Block bitmap not in use */
#define EXT4_BG_INODE_ZEROED			0x0004 /* On-disk itable initialized to zero */

#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM		0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS		0x0040 /* extents support */

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

int get_parent_inode_num(block_dev_desc_t *dev_desc,char *dirname, char *dname);
int  iget(unsigned int inode_no,struct ext2_inode* inode);

// taken from mkfs_minix.c. libbb candidate?
// "uint32_t size", since we never use it for anything >32 bits
static uint32_t ext4fs_div_roundup(uint32_t size, uint32_t n)
{
	// Overflow-resistant
	uint32_t res = size / n;
	if (res * n != size)
		res++;
	return res;
}

#ifdef DEBUG_LOG
void ext4fs_dump_the_block_group_before_writing()
{
	int i,j;
	int blocksize;
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);
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

void ext4fs_dump_the_block_group_after_writing()
{
	int i,j;
	int blocksize;
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);
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

void ext4fs_dump_the_inode_group_before_writing()
{
	int i,j;
	int blocksize;
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);
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

void ext4fs_dump_the_inode_group_after_writing()
{
	int i,j;
	int blocksize;
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);
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

static int ext4fs_blockgroup
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
	printf ("ext4fs read %d group descriptor (blkno %d blkoff %d)\n",
		group, blkno, blkoff);
#endif
	return (ext2fs_devread (blkno << LOG2_EXT2_BLOCK_SIZE(data),
		blkoff, sizeof(struct ext2_block_group), (char *)blkgrp));
}

static struct ext4_extent_header * ext4fs_find_leaf (struct ext2_data *data,
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

static int ext4fs_read_inode
	(struct ext2_data *data, int ino, struct ext2_inode *inode)
{
	struct ext2_block_group blkgrp;
	struct ext2_sblock *sblock = &data->sblock;
	int inodes_per_block;
	int status;

	unsigned int blkno;
	unsigned int blkoff;

#ifdef DEBUG
	printf ("ext4fs read inode %d, inode_size %d\n", ino, inode_size);
#endif
	/* It is easier to calculate if the first inode is 0.  */
	ino--;
	status = ext4fs_blockgroup (data, ino / __le32_to_cpu
	    (sblock->inodes_per_group), &blkgrp);

	if (status == 0)
		return (0);

	inodes_per_block = EXT2_BLOCK_SIZE(data) / inode_size;

	blkno = __le32_to_cpu (blkgrp.inode_table_id) +
		(ino % __le32_to_cpu (sblock->inodes_per_group))
		/ inodes_per_block;
	blkoff = (ino % inodes_per_block) * inode_size;
#ifdef DEBUG
	printf ("ext4fs read inode blkno %d blkoff %d\n", blkno, blkoff);
#endif
	/* Read the inode.  */
	status = ext2fs_devread (blkno << LOG2_EXT2_BLOCK_SIZE (data), blkoff,
		sizeof (struct ext2_inode), (char *) inode);
	if (status == 0)
		return (0);

	return (1);
}

void ext4fs_free_node (ext2fs_node_t node, ext2fs_node_t currroot)
{
	if ((node != &ext4fs_root->diropen) && (node != currroot))
		free (node);
}

int ext4fs_read_allocated_block(struct ext2_inode *inode, int fileblock)
{
	int blknr;
	int blksz ;
	int log2_blksz;
	int status;
	unsigned int rblock;
	unsigned int perblock;
	unsigned int perblock_parent;
	unsigned int perblock_child;

	/*get the blocksize of the filesystem*/
	blksz=EXT2_BLOCK_SIZE(ext4fs_root);
	log2_blksz = LOG2_EXT2_BLOCK_SIZE (ext4fs_root);

	if (le32_to_cpu(inode->flags) & EXT4_EXTENTS_FLAG) {
		char buf[EXT2_BLOCK_SIZE(ext4fs_root)];
		struct ext4_extent_header *leaf;
		struct ext4_extent *ext;
		int i;

		leaf = ext4fs_find_leaf (ext4fs_root, buf,
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
		if (ext4fs_indir1_block == NULL) {
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** SI ext2fs read block (indir 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
			ext4fs_indir1_blkno = -1;
		}
		if (blksz != ext4fs_indir1_size) {
			free (ext4fs_indir1_block);
			ext4fs_indir1_block = NULL;
			ext4fs_indir1_size = 0;
			ext4fs_indir1_blkno = -1;
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** SI ext2fs read block (indir 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.indir_block) <<
		     log2_blksz) != ext4fs_indir1_blkno) {
			status = ext2fs_devread (__le32_to_cpu(inode->b.blocks.indir_block) << log2_blksz,
						 0, blksz,
						 (char *) ext4fs_indir1_block);
			if (status == 0) {
				printf ("** SI ext2fs read block (indir 1) failed. **\n");
				return (0);
			}
			ext4fs_indir1_blkno =
				__le32_to_cpu (inode->b.blocks.
					       indir_block) << log2_blksz;
		}
		blknr = __le32_to_cpu (ext4fs_indir1_block
				       [fileblock - INDIRECT_BLOCKS]);
	}
	/* Double indirect.  */
	else if (fileblock <
		 (INDIRECT_BLOCKS + (blksz / 4 * (blksz / 4 + 1)))) {

		unsigned int perblock = blksz / 4;
		unsigned int rblock = fileblock - (INDIRECT_BLOCKS
						   + blksz / 4);

		if (ext4fs_indir1_block == NULL) {
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** DI ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
			ext4fs_indir1_blkno = -1;
		}
		if (blksz != ext4fs_indir1_size) {
			free (ext4fs_indir1_block);
			ext4fs_indir1_block = NULL;
			ext4fs_indir1_size = 0;
			ext4fs_indir1_blkno = -1;
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** DI ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.double_indir_block) <<
		     log2_blksz) != ext4fs_indir1_blkno) {
			status = ext2fs_devread (__le32_to_cpu(inode->b.blocks.double_indir_block) << log2_blksz,
						0, blksz,
						(char *) ext4fs_indir1_block);
			if (status == 0) {
				printf ("** DI ext2fs read block (indir 2 1) failed. **\n");
				return (-1);
			}
			ext4fs_indir1_blkno =
				__le32_to_cpu (inode->b.blocks.double_indir_block) << log2_blksz;
		}

		if (ext4fs_indir2_block == NULL) {
			ext4fs_indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir2_block == NULL) {
				printf ("** DI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir2_size = blksz;
			ext4fs_indir2_blkno = -1;
		}
		if (blksz != ext4fs_indir2_size) {
			free (ext4fs_indir2_block);
			ext4fs_indir2_block = NULL;
			ext4fs_indir2_size = 0;
			ext4fs_indir2_blkno = -1;
			ext4fs_indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir2_block == NULL) {
				printf ("** DI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir2_size = blksz;
		}
		if ((__le32_to_cpu (ext4fs_indir1_block[rblock / perblock]) <<
		     log2_blksz) != ext4fs_indir2_blkno) {
		     	status = ext2fs_devread (__le32_to_cpu(ext4fs_indir1_block[rblock / perblock]) << log2_blksz,
						 0, blksz,
						 (char *) ext4fs_indir2_block);
			if (status == 0) {
				printf ("** DI ext2fs read block (indir 2 2) failed. **\n");
				return (-1);
			}
			ext4fs_indir2_blkno =
				__le32_to_cpu (ext4fs_indir1_block[rblock / perblock]) << log2_blksz;
		}
		blknr = __le32_to_cpu (ext4fs_indir2_block[rblock % perblock]);
	}
	/* Tripple indirect.  */
	else {
		rblock = fileblock - (INDIRECT_BLOCKS + blksz / 4 + (blksz / 4 * blksz / 4));
		perblock_child = blksz / 4;
		perblock_parent = ((blksz / 4) * (blksz / 4));
		
		if (ext4fs_indir1_block == NULL) {
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
			ext4fs_indir1_blkno = -1;
		}
		if (blksz != ext4fs_indir1_size) {
			free (ext4fs_indir1_block);
			ext4fs_indir1_block = NULL;
			ext4fs_indir1_size = 0;
			ext4fs_indir1_blkno = -1;
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.tripple_indir_block) <<
			  log2_blksz) != ext4fs_indir1_blkno) {
			status = ext2fs_devread
				(__le32_to_cpu(inode->b.blocks.tripple_indir_block) << log2_blksz,
				0, blksz,
				(char *) ext4fs_indir1_block);
			if (status == 0) {
				printf ("** TI ext2fs read block (indir 2 1) failed. **\n");
				return (-1);
			}
			ext4fs_indir1_blkno =
				__le32_to_cpu (inode->b.blocks.tripple_indir_block) << log2_blksz;
		}
		
		if (ext4fs_indir2_block == NULL) {
			ext4fs_indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir2_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir2_size = blksz;
			ext4fs_indir2_blkno = -1;
		}
		if (blksz != ext4fs_indir2_size) {
			free (ext4fs_indir2_block);
			ext4fs_indir2_block = NULL;
			ext4fs_indir2_size = 0;
			ext4fs_indir2_blkno = -1;
			ext4fs_indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir2_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir2_size = blksz;
		}
		if ((__le32_to_cpu (ext4fs_indir1_block[rblock / perblock_parent]) <<
			  log2_blksz) != ext4fs_indir2_blkno) {
			status = ext2fs_devread (__le32_to_cpu(ext4fs_indir1_block[rblock / perblock_parent]) << log2_blksz,
						 0, blksz,
						 (char *) ext4fs_indir2_block);
			if (status == 0) {
				printf ("** TI ext2fs read block (indir 2 2) failed. **\n");
				return (-1);
			}
			ext4fs_indir2_blkno =
				__le32_to_cpu (ext4fs_indir1_block[rblock / perblock_parent]) << log2_blksz;
		}
		
		if (ext4fs_indir3_block == NULL) {
			ext4fs_indir3_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir3_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir3_size = blksz;
			ext4fs_indir3_blkno = -1;
		}
		if (blksz != ext4fs_indir3_size) {
			free (ext4fs_indir3_block);
			ext4fs_indir3_block = NULL;
			ext4fs_indir3_size = 0;
			ext4fs_indir3_blkno = -1;
			ext4fs_indir3_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir3_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir3_size = blksz;
		}
		if ((__le32_to_cpu (ext4fs_indir2_block[rblock / perblock_child]) <<
			  log2_blksz) != ext4fs_indir3_blkno) {
			status = ext2fs_devread (__le32_to_cpu(ext4fs_indir2_block[(rblock / perblock_child)
									%(blksz/4)]) << log2_blksz,0, blksz,
						 			(char *) ext4fs_indir3_block);
			if (status == 0) {
				printf ("** TI ext2fs read block (indir 2 2) failed. **\n");
				return (-1);
			}
			ext4fs_indir3_blkno =
				__le32_to_cpu (ext4fs_indir2_block[(rblock / perblock_child)%(blksz/4)]) << log2_blksz;
		}
		
		blknr = __le32_to_cpu (ext4fs_indir3_block[rblock % perblock_child]);
	}
#ifdef DEBUG
	printf ("ext4fs_read_block %08x\n", blknr);
#endif
	return (blknr);
}

static int ext4fs_read_block (ext2fs_node_t node, int fileblock)
{
	struct ext2_data *data = node->data;
	struct ext2_inode *inode = &node->inode;
	int blknr;
	int blksz = EXT2_BLOCK_SIZE (data);
	int log2_blksz = LOG2_EXT2_BLOCK_SIZE (data);
	int status;
	unsigned int rblock;
	unsigned int perblock;
	unsigned int perblock_parent;
	unsigned int perblock_child;

	if (le32_to_cpu(inode->flags) & EXT4_EXTENTS_FLAG) {
		char buf[EXT2_BLOCK_SIZE(data)];
		struct ext4_extent_header *leaf;
		struct ext4_extent *ext;
		int i;

		leaf = ext4fs_find_leaf (data, buf,
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
		if (ext4fs_indir1_block == NULL) {
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** SI ext2fs read block (indir 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
			ext4fs_indir1_blkno = -1;
		}
		if (blksz != ext4fs_indir1_size) {
			free (ext4fs_indir1_block);
			ext4fs_indir1_block = NULL;
			ext4fs_indir1_size = 0;
			ext4fs_indir1_blkno = -1;
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** SI ext2fs read block (indir 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.indir_block) <<
		     log2_blksz) != ext4fs_indir1_blkno) {
			status = ext2fs_devread (__le32_to_cpu(inode->b.blocks.indir_block) << log2_blksz,
				 0, blksz, (char *) ext4fs_indir1_block);
			if (status == 0) {
				printf ("** SI ext2fs read block (indir 1) failed. **\n");
				return (0);
			}
			ext4fs_indir1_blkno = __le32_to_cpu (inode->b.blocks.
			       indir_block) << log2_blksz;
		}
		blknr = __le32_to_cpu (ext4fs_indir1_block[fileblock - INDIRECT_BLOCKS]);
	}
	/* Double indirect.  */

	else if (fileblock <
		(INDIRECT_BLOCKS + (blksz / 4 * (blksz / 4 + 1)))) {
		perblock = blksz / 4;
		rblock = fileblock - (INDIRECT_BLOCKS + blksz / 4);

		if (ext4fs_indir1_block == NULL) {
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** DI ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
			ext4fs_indir1_blkno = -1;
		}
		if (blksz != ext4fs_indir1_size) {
			free (ext4fs_indir1_block);
			ext4fs_indir1_block = NULL;
			ext4fs_indir1_size = 0;
			ext4fs_indir1_blkno = -1;
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** DI ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.double_indir_block) <<
		     log2_blksz) != ext4fs_indir1_blkno) {
			status = ext2fs_devread
				(__le32_to_cpu(inode->b.blocks.double_indir_block) << log2_blksz,
				0, blksz,
				(char *) ext4fs_indir1_block);
			if (status == 0) {
				printf ("** DI ext2fs read block (indir 2 1) failed. **\n");
				return (-1);
			}
			ext4fs_indir1_blkno =
				__le32_to_cpu (inode->b.blocks.double_indir_block) << log2_blksz;
		}

		if (ext4fs_indir2_block == NULL) {
			ext4fs_indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir2_block == NULL) {
				printf ("** DI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir2_size = blksz;
			ext4fs_indir2_blkno = -1;
		}
		if (blksz != ext4fs_indir2_size) {
			free (ext4fs_indir2_block);
			ext4fs_indir2_block = NULL;
			ext4fs_indir2_size = 0;
			ext4fs_indir2_blkno = -1;
			ext4fs_indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir2_block == NULL) {
				printf ("** DI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir2_size = blksz;
		}
		if ((__le32_to_cpu (ext4fs_indir1_block[rblock / perblock]) <<
		     log2_blksz) != ext4fs_indir2_blkno) {
			status = ext2fs_devread (__le32_to_cpu(ext4fs_indir1_block[rblock / perblock]) << log2_blksz,
						 0, blksz,
						 (char *) ext4fs_indir2_block);
			if (status == 0) {
				printf ("** DI ext2fs read block (indir 2 2) failed. **\n");
				return (-1);
			}
			ext4fs_indir2_blkno =
				__le32_to_cpu (ext4fs_indir1_block[rblock / perblock]) << log2_blksz;
		}
		blknr = __le32_to_cpu (ext4fs_indir2_block[rblock % perblock]);
	}
	/* Tripple indirect.  */
	else {
		rblock = fileblock - (INDIRECT_BLOCKS + blksz / 4 + (blksz / 4 * blksz / 4));
		perblock_child = blksz / 4;
		perblock_parent = ((blksz / 4) * (blksz / 4));
		
		if (ext4fs_indir1_block == NULL) {
					ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
			ext4fs_indir1_blkno = -1;
		}
		if (blksz != ext4fs_indir1_size) {
				free (ext4fs_indir1_block);
			ext4fs_indir1_block = NULL;
			ext4fs_indir1_size = 0;
			ext4fs_indir1_blkno = -1;
			ext4fs_indir1_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir1_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 1) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir1_size = blksz;
		}
		if ((__le32_to_cpu (inode->b.blocks.tripple_indir_block) <<
			  log2_blksz) != ext4fs_indir1_blkno) {
			status = ext2fs_devread
				(__le32_to_cpu(inode->b.blocks.tripple_indir_block) << log2_blksz,
				0, blksz,
				(char *) ext4fs_indir1_block);
			if (status == 0) {
				printf ("** TI ext2fs read block (indir 2 1) failed. **\n");
				return (-1);
			}
			ext4fs_indir1_blkno =
				__le32_to_cpu (inode->b.blocks.tripple_indir_block) << log2_blksz;
		}
		
		if (ext4fs_indir2_block == NULL) {
			ext4fs_indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir2_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir2_size = blksz;
			ext4fs_indir2_blkno = -1;
		}
		if (blksz != ext4fs_indir2_size) {
			free (ext4fs_indir2_block);
			ext4fs_indir2_block = NULL;
			ext4fs_indir2_size = 0;
			ext4fs_indir2_blkno = -1;
			ext4fs_indir2_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir2_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir2_size = blksz;
		}
		if ((__le32_to_cpu (ext4fs_indir1_block[rblock / perblock_parent]) <<
			  log2_blksz) != ext4fs_indir2_blkno) {
			status = ext2fs_devread (__le32_to_cpu(ext4fs_indir1_block[rblock / perblock_parent])
								<< log2_blksz, 0, blksz,(char *) ext4fs_indir2_block);
			if (status == 0) {
				printf ("** TI ext2fs read block (indir 2 2) failed. **\n");
				return (-1);
			}
			ext4fs_indir2_blkno =
				__le32_to_cpu (ext4fs_indir1_block[rblock / perblock_parent]) << log2_blksz;
		}
		
		if (ext4fs_indir3_block == NULL) {
			ext4fs_indir3_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir3_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir3_size = blksz;
			ext4fs_indir3_blkno = -1;
		}
		if (blksz != ext4fs_indir3_size) {
			free (ext4fs_indir3_block);
			ext4fs_indir3_block = NULL;
			ext4fs_indir3_size = 0;
			ext4fs_indir3_blkno = -1;
			ext4fs_indir3_block = (uint32_t *) memalign(ARCH_DMA_MINALIGN, blksz);
			if (ext4fs_indir3_block == NULL) {
				printf ("** TI ext2fs read block (indir 2 2) malloc failed. **\n");
				return (-1);
			}
			ext4fs_indir3_size = blksz;
		}
		if ((__le32_to_cpu (ext4fs_indir2_block[rblock / perblock_child]) <<
			  log2_blksz) != ext4fs_indir3_blkno) {
			status = ext2fs_devread (__le32_to_cpu(ext4fs_indir2_block[(rblock / perblock_child)%(blksz/4)])
									<< log2_blksz, 0, blksz,(char *) ext4fs_indir3_block);
			if (status == 0) {
				printf ("** TI ext2fs read block (indir 2 2) failed. **\n");
				return (-1);
			}
			ext4fs_indir3_blkno =
				__le32_to_cpu (ext4fs_indir2_block[(rblock / perblock_child)%(blksz/4)]) << log2_blksz;
		}
		
		blknr = __le32_to_cpu (ext4fs_indir3_block[rblock % perblock_child]);
	}
#ifdef DEBUG
	printf ("ext4fs_read_block %08x\n", blknr);
#endif
	return (blknr);
}

int ext4fs_read_file
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
		blknr = ext4fs_read_block (node, i);
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

int ext4fs_write_file
	(block_dev_desc_t *dev_desc, struct ext2_inode *file_inode, int pos, unsigned int len, char *buf) {

	int i;
	int blockcnt;
	int log2blocksize = LOG2_EXT2_BLOCK_SIZE (ext4fs_root);
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
		blknr = ext4fs_read_allocated_block (file_inode, i);
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
					put_ext4(dev_desc,(uint64_t)(delayed_start*SECTOR_SIZE), delayed_buf,
									(uint32_t)delayed_extent);
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
				put_ext4(dev_desc,(uint64_t)(delayed_start*SECTOR_SIZE), delayed_buf,
									(uint32_t)delayed_extent);

			 	previous_block_number = -1;
			}
			memset(buf, 0, blocksize - skipfirst);
		}
		buf += blocksize - skipfirst;
	}
	if (previous_block_number != -1) {
		/* spill */
		put_ext4(dev_desc,(uint64_t)(delayed_start*SECTOR_SIZE), delayed_buf, (uint32_t)delayed_extent);

		previous_block_number = -1;
	}
	return (len);
}

static int ext4fs_iterate_dir (ext2fs_node_t dir, char *name, ext2fs_node_t * fnode, int *ftype)
{
	unsigned int fpos = 0;
	int status;
	struct ext2fs_node *diro = (struct ext2fs_node *) dir;

#ifdef DEBUG
	if (name != NULL)
		printf ("Iterate dir %s\n", name);
#endif /* of DEBUG */
	if (!diro->inode_read) {
		status = ext4fs_read_inode (diro->data, diro->ino,
					    &diro->inode);
		if (status == 0)
			return (0);
	}
	/* Search the file.  */
	while (fpos < __le32_to_cpu (diro->inode.size)) {
		struct ext2_dirent dirent;
		status = ext4fs_read_file (diro, fpos,
			sizeof (struct ext2_dirent),
			(char *) &dirent);
		if (status < 1)
			return (0);

		if (dirent.namelen != 0) {
			char filename[dirent.namelen + 1];
			ext2fs_node_t fdiro;
			int type = FILETYPE_UNKNOWN;

			status = ext4fs_read_file (diro,
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

				status = ext4fs_read_inode (diro->data,
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
					status = ext4fs_read_inode (diro->data,
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

static char *ext4fs_read_symlink (ext2fs_node_t node)
{
	char *symlink;
	struct ext2fs_node *diro = node;
	int status;

	if (!diro->inode_read) {
		status = ext4fs_read_inode (diro->data, diro->ino,
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
		status = ext4fs_read_file (diro, 0,
		   __le32_to_cpu (diro->inode.size), symlink);
		if (status == 0) {
			free (symlink);
			return (0);
		}
	}
	symlink[__le32_to_cpu (diro->inode.size)] = '\0';
	return (symlink);
}

int ext4fs_find_file1
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
			ext4fs_free_node (currnode, currroot);
			return (0);
		}

		oldnode = currnode;

		/* Iterate over the directory.  */
		found = ext4fs_iterate_dir (currnode, name, &currnode, &type);
		if (found == 0)
			return (0);
		if (found == -1)
			break;

		/* Read in the symlink and follow it.  */
		if (type == FILETYPE_SYMLINK) {
			char *symlink;

			/* Test if the symlink does not loop.  */
			if (++ext4fs_symlinknest == 8) {
				ext4fs_free_node (currnode, currroot);
				ext4fs_free_node (oldnode, currroot);
				return (0);
			}

			symlink = ext4fs_read_symlink (currnode);
			ext4fs_free_node (currnode, currroot);

			if (!symlink) {
				ext4fs_free_node (oldnode, currroot);
				return (0);
			}
#ifdef DEBUG
			printf ("Got symlink >%s<\n", symlink);
#endif /* of DEBUG */
			/* The symlink is an absolute path, go back to the root inode.  */
			if (symlink[0] == '/') {
				ext4fs_free_node (oldnode, currroot);
				oldnode = &ext4fs_root->diropen;
			}

			/* Lookup the node the symlink points to.  */
			status = ext4fs_find_file1 (symlink, oldnode,
				&currnode, &type);

			free (symlink);

			if (status == 0) {
				ext4fs_free_node (oldnode, currroot);
				return (0);
			}
		}

		ext4fs_free_node (oldnode, currroot);

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

int ext4fs_find_file
	(const char *path,
	 ext2fs_node_t rootnode, ext2fs_node_t * foundnode, int expecttype)
{
	int status;
	int foundtype = FILETYPE_DIRECTORY;

	ext4fs_symlinknest = 0;
	if (!path)
		return (0);

	status = ext4fs_find_file1 (path, rootnode, foundnode, &foundtype);
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

int  ext4fs_get_inode_no_from_inodebitmap(unsigned char *buffer)
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
		if(count>(uint32_t)ext4fs_root->sblock.inodes_per_group)
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

int  ext4fs_get_block_no_from_blockbitmap(unsigned char *buffer)
{
	int j=0;
	unsigned char input;
	int operand;
	int status;
	int count =0;
	int blocksize;

	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);
	unsigned char *ptr=buffer;
	if(blocksize!=1024)
		count=0;
	else
		count=1;
	
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

int ext4fs_update_bmap_with_one(unsigned long blockno, unsigned char * buffer, int index)
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
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);
	i=i-(index * blocksize);
	if(blocksize!=1024)
	{
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
	else
	{
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
}

int ext4fs_update_bmap_with_zero(unsigned long blockno, unsigned char * buffer, int index)
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
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);
	i=i-(index * blocksize);
	if(blocksize!=1024)
	{
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
	else
	{
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
}

int ext4fs_update_inode_bmap_with_one(unsigned long inode_no, unsigned char * buffer, int index)
{
	unsigned int i;
	unsigned int remainder;
	unsigned char  *ptr=buffer;
	unsigned char operand;
	int status;
	
	inode_no -= (index * (uint32_t)ext4fs_root->sblock.inodes_per_group);
	
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

int ext4fs_update_inode_bmap_with_zero(unsigned long inode_no, unsigned char * buffer,int index)
{
	unsigned int i;
	unsigned int remainder;
	unsigned char  *ptr=buffer;
	unsigned char operand;
	int status;

	inode_no -= (index * (uint32_t)ext4fs_root->sblock.inodes_per_group);
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

unsigned long ext4fs_get_next_available_block_no(block_dev_desc_t *dev_desc,int part_no)
{
	int i;
	int status;
	int remainder;
	int sector_per_block;
	int blocksize;
	unsigned int bg_bitmap_index;
	static int prev_bg_bitmap_index = -1;
	unsigned char *zero_buffer=NULL;
	unsigned char *journal_buffer=NULL;
	
	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);

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
				if(gd[i].bg_flags & EXT4_BG_BLOCK_UNINIT)
				{
					printf("block uninit set for group %d\n",i);
					zero_buffer = (unsigned char *)xzalloc(blocksize);
					put_ext4(dev_desc,((uint64_t)(gd[i].block_id * blocksize)),
									zero_buffer, blocksize);
					gd[i].bg_flags = gd[i].bg_flags & ~EXT4_BG_BLOCK_UNINIT;
					memcpy(bg[i],zero_buffer,blocksize);
					if(zero_buffer)
					{
						free(zero_buffer);
						zero_buffer = NULL;
					}
				}
				blockno=ext4fs_get_block_no_from_blockbitmap(bg[i]);
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
				//journal backup
				journal_buffer = (unsigned char *)xzalloc(blocksize);
				status = ext2fs_devread (gd[i].block_id*sector_per_block, 0, blocksize,journal_buffer);
				if(status==0){
					free(journal_buffer);
					return -1;
				}
				log_journal(journal_buffer,gd[i].block_id);
				free(journal_buffer);
				return  blockno;
			}
			else
			{
#ifdef DEBUG_LOG
				printf("no space left on the block group %d \n",i);
#endif
			}
		}
		return -1;
	}
	else
	{
		restart:
		blockno++;
		/*get the blockbitmap index respective to blockno */
		if(blocksize!=1024)
		{
			bg_bitmap_index=blockno/(uint32_t)ext4fs_root->sblock.blocks_per_group;
		}
		else
		{
			bg_bitmap_index=blockno/(uint32_t)ext4fs_root->sblock.blocks_per_group;	
			remainder = blockno%(uint32_t)ext4fs_root->sblock.blocks_per_group;
			if(!remainder)
				bg_bitmap_index--;
		}
		
		/*To skip completely filled block group bitmaps : Optimize the block allocation*/
		if(bg_bitmap_index >= no_blockgroup)
			return -1;
		
		if(gd[bg_bitmap_index].free_blocks==0)
		{
#ifdef DEBUG_LOG
			printf("block group %d is full. Skipping\n",bg_bitmap_index);
#endif
			blockno= blockno + ext4fs_root->sblock.blocks_per_group;
			blockno--;
			goto restart;
		}
	
		if(gd[bg_bitmap_index].bg_flags & EXT4_BG_BLOCK_UNINIT)
		{
			zero_buffer = (unsigned char *)xzalloc(blocksize);
			put_ext4(dev_desc,((uint64_t)(gd[bg_bitmap_index].block_id * blocksize)),
							zero_buffer, blocksize);
			memcpy(bg[bg_bitmap_index],zero_buffer,blocksize);
			gd[bg_bitmap_index].bg_flags = gd[bg_bitmap_index].bg_flags & ~EXT4_BG_BLOCK_UNINIT;
			if(zero_buffer)
			{
				free(zero_buffer);
				zero_buffer = NULL;
			}
		}
		
		if(ext4fs_update_bmap_with_one(blockno,bg[bg_bitmap_index],bg_bitmap_index)!=0)
		{
#ifdef DEBUG_LOG
			printf("going for restart for the block no %u %d\n",blockno,bg_bitmap_index);
#endif
			goto restart;
		}

		//journal backup
		if(prev_bg_bitmap_index != bg_bitmap_index){
			journal_buffer = (unsigned char *)xzalloc(blocksize);
			status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
									0, blocksize,journal_buffer);
			if(status==0){
				free(journal_buffer);
				return -1;
			}
			log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
			prev_bg_bitmap_index = bg_bitmap_index;
			free(journal_buffer);
		}
		gd[bg_bitmap_index].free_blocks--;
		sb->free_blocks--;
		return blockno;
	}
}

unsigned long ext4fs_get_next_available_inode_no(block_dev_desc_t *dev_desc,int part_no)
{
	int i;
	int status;
	unsigned int inode_bitmap_index;
	static int prev_inode_bitmap_index = -1;
	unsigned char *zero_buffer=NULL;
	unsigned char *journal_buffer=NULL;
	
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
				if (gd[i].bg_itable_unused !=
						 gd[i].free_inodes)
					gd[i].bg_itable_unused =
						 gd[i].free_inodes;
				if(gd[i].bg_flags & EXT4_BG_INODE_UNINIT)
				{
					zero_buffer = (unsigned char *)xzalloc(blocksize);
					put_ext4(dev_desc,((uint64_t)(gd[i].inode_id * blocksize)),
								zero_buffer, blocksize);
					gd[i].bg_flags = gd[i].bg_flags & ~EXT4_BG_INODE_UNINIT;
					memcpy(inode_bmaps[i],zero_buffer,blocksize);
					if(zero_buffer)
					{
						free(zero_buffer);
						zero_buffer = NULL;
					}
				}
				inode_no=ext4fs_get_inode_no_from_inodebitmap(inode_bmaps[i]);
				if(inode_no==-1) /*if block bitmap is completely fill*/
				{
					continue;
				}
				inode_no=inode_no +(i*ext4fs_root->sblock.inodes_per_group);
#ifdef DEBUG_LOG
				printf("every first inode no getting is %d\n",inode_no);
#endif
				first_execution_inode++;
				gd[i].free_inodes--;
				gd[i].bg_itable_unused--;
				sb->free_inodes--;
				//journal backup
				journal_buffer = (unsigned char *)xzalloc(blocksize);
				status = ext2fs_devread (gd[i].inode_id*sector_per_block, 0, blocksize,journal_buffer);
				if(status==0)
				{
					free(journal_buffer);
					return -1;
				}
				log_journal(journal_buffer,gd[i].inode_id);
				free(journal_buffer);
				return  inode_no;
			}
			else
			{
#ifdef DEBUG_LOG
				printf("no inode left on the block group %d \n",i);
#endif
			}
		}
		return -1;
	}
	else
	{
		restart:
		inode_no++;
		/*get the blockbitmap index respective to blockno */
		inode_bitmap_index=inode_no/(uint32_t)ext4fs_root->sblock.inodes_per_group;
		if(gd[inode_bitmap_index].bg_flags & EXT4_BG_INODE_UNINIT)
		{
			zero_buffer = (unsigned char *)xzalloc(blocksize);
			put_ext4(dev_desc,((uint64_t)(gd[inode_bitmap_index].inode_id * blocksize)),
							zero_buffer, blocksize);
			gd[inode_bitmap_index].bg_flags = gd[inode_bitmap_index].bg_flags & ~EXT4_BG_INODE_UNINIT;
			memcpy(inode_bmaps[inode_bitmap_index],zero_buffer,blocksize);
			if(zero_buffer)
			{
				free(zero_buffer);
				zero_buffer = NULL;
			}
		}
		
		if(ext4fs_update_inode_bmap_with_one(inode_no,inode_bmaps[inode_bitmap_index],inode_bitmap_index)!=0)
		{
#ifdef DEBUG_LOG
			printf("going for restart for the block no %u %d\n",inode_no,inode_bitmap_index);
#endif
			goto restart;
		}

		//journal backup
		if(prev_inode_bitmap_index != inode_bitmap_index){
			journal_buffer = (unsigned char *)xzalloc(blocksize);
			status = ext2fs_devread (gd[inode_bitmap_index].inode_id*sector_per_block,
										0,blocksize,journal_buffer);
			if(status==0){
				free(journal_buffer);
				return -1;
			}
			log_journal(journal_buffer,gd[inode_bitmap_index].inode_id);
			prev_inode_bitmap_index = inode_bitmap_index;
			free(journal_buffer);
		}
		if (gd[inode_bitmap_index].bg_itable_unused !=
				gd[inode_bitmap_index].free_inodes)
			gd[inode_bitmap_index].bg_itable_unused =
				gd[inode_bitmap_index].free_inodes;
		gd[inode_bitmap_index].free_inodes--;
		gd[inode_bitmap_index].bg_itable_unused--;
		sb->free_inodes--;
		return inode_no;
	}
}

int ext4fs_existing_filename_check(block_dev_desc_t *dev_desc,char *filename)
{
	unsigned int direct_blk_idx=0;
	unsigned int blknr=-1;
	int inodeno=-1;

	/*read the block no allocated to a file*/
	for(direct_blk_idx=0;direct_blk_idx<INDIRECT_BLOCKS;direct_blk_idx++)
	{
		blknr=ext4fs_read_allocated_block(g_parent_inode, direct_blk_idx);
		if(blknr==0)
		{
			break;
		}
		inodeno= ext4fs_if_filename_exists_in_root(dev_desc,filename,blknr);
		if(inodeno!=-1)
		{
			return inodeno;
		}
	}
	return -1;
}

int ext4fs_if_filename_exists_in_root(block_dev_desc_t *dev_desc,char *filename, unsigned int blknr)
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
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);

	/*get the first block of root*/
	first_block_no_of_root=blknr;
	
	/*calucalate sector per block*/
	sector_per_block=blocksize/SECTOR_SIZE;
	
   	/*read the first block of root directory inode*/
	root_first_block_buffer=xzalloc (blocksize);
	root_first_block_addr=root_first_block_buffer;
	status = ext2fs_devread (first_block_no_of_root*sector_per_block, 0,
							blocksize,(char *)root_first_block_buffer);
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

			if(strlen(filename)==dir->namelen)
			{
				if(strncmp(filename,ptr+sizeof(struct ext2_dirent),dir->namelen)==0)
				{
				printf("file found deleting\n");
				previous_dir->direntlen+=dir->direntlen;
				inodeno=dir->inode;
				memset(ptr+sizeof(struct ext2_dirent),'\0',dir->namelen);
				dir->inode=0;
				found=1;
				break;
				}
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
		put_metadata(root_first_block_addr,first_block_no_of_root);
		if(root_first_block_addr)
			free(root_first_block_addr);
		return inodeno;
	}
	else
	{
		goto fail;
	}

fail:
	if(root_first_block_addr)
	    free(root_first_block_addr);
	 return -1;
}

int ext4fs_delete_file(block_dev_desc_t *dev_desc, unsigned int inodeno)
{
	struct ext2_data *diro=ext4fs_root;
	struct ext2_inode inode;
	int status;
	unsigned int i,j;
	int remainder;
	int sector_per_block;
	unsigned int blknr;
	unsigned int inode_no;
	int bg_bitmap_index;
	int inode_bitmap_index;
	unsigned char* read_buffer;
	unsigned char* start_block_address=NULL;
	unsigned int* double_indirect_buffer;
	unsigned int* DIB_start_addr=NULL;
	unsigned int* triple_indirect_grand_parent_buffer=NULL;
	unsigned int* TIB_start_addr=NULL;
	unsigned int* triple_indirect_parent_buffer=NULL;
	unsigned int* TIPB_start_addr=NULL;
	struct ext2_block_group *gd;
	unsigned int no_blocks;

	static int prev_bg_bitmap_index = -1;
	unsigned char *journal_buffer=NULL;
	unsigned int inodes_per_block;
	unsigned int blkno;
	unsigned int blkoff;
	struct ext2_inode *inode_buffer;

	/*get the block group descriptor table*/
	gd=(struct ext2_block_group*)gd_table;
	status = ext4fs_read_inode (diro,inodeno,&inode);
	if (status == 0)
	{
		goto fail;
	}
	
	/*calucalate sector per block*/
	sector_per_block=blocksize/SECTOR_SIZE;

	/*read the block no allocated to a file*/
	no_blocks= inode.size/blocksize;
	if(inode.size%blocksize)
	no_blocks++;

	if (le32_to_cpu(inode.flags) & EXT4_EXTENTS_FLAG)
	{
		ext2fs_node_t node_inode=(ext2fs_node_t)xzalloc(sizeof (struct ext2fs_node));
		node_inode->data=ext4fs_root;
		node_inode->ino=inodeno;
		node_inode->inode_read=0;
		memcpy(&(node_inode->inode),&inode,sizeof(struct ext2_inode));

		for(i=0;i<no_blocks;i++)
		{
			blknr=ext4fs_read_block(node_inode, i);
			if(blocksize!=1024)
			{
				bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
			}
			else
			{
				bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
				remainder = blknr%(uint32_t)ext4fs_root->sblock.blocks_per_group;
				if(!remainder)
					bg_bitmap_index--;
			}
			ext4fs_update_bmap_with_zero(blknr,bg[bg_bitmap_index],bg_bitmap_index);
#ifdef DEBUG_LOG
			printf("EXT4_EXTENTS Block releasing %u: %d\n",blknr,bg_bitmap_index);
#endif
			gd[bg_bitmap_index].free_blocks++;
			sb->free_blocks++;

			//journal backup
			if(prev_bg_bitmap_index != bg_bitmap_index){
				journal_buffer = (unsigned char *)xzalloc(blocksize);
				status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
										0, blocksize,journal_buffer);
				if(status==0){
					free(journal_buffer);
					return -1;
				}
				log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
				prev_bg_bitmap_index = bg_bitmap_index;
				free(journal_buffer);
			}
		}
		if(node_inode)
		{
			free(node_inode);
			node_inode=NULL;
		}
	}
	else
	{
		/*deleting the single indirect block associated with inode*/
		if(inode.b.blocks.indir_block!=0)
		{
#ifdef DEBUG_LOG
			printf("SIPB releasing %d\n",inode.b.blocks.indir_block);
#endif
			blknr=inode.b.blocks.indir_block;
			if(blocksize!=1024)
			{
				bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
			}
			else
			{
				bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;	
				remainder = blknr%(uint32_t)ext4fs_root->sblock.blocks_per_group;
				if(!remainder)
					bg_bitmap_index--;
			}
			ext4fs_update_bmap_with_zero(blknr,bg[bg_bitmap_index],bg_bitmap_index);
			gd[bg_bitmap_index].free_blocks++;
			sb->free_blocks++;
			//journal backup
			if(prev_bg_bitmap_index != bg_bitmap_index){
				journal_buffer = (unsigned char *)xzalloc(blocksize);
				status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
										0, blocksize,journal_buffer);
				if(status==0){
					free(journal_buffer);
					return -1;
				}
				log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
				prev_bg_bitmap_index = bg_bitmap_index;
				free(journal_buffer);
			}
		}

		/*deleting the double indirect blocks */
		if(inode.b.blocks.double_indir_block!=0)
		{
			double_indirect_buffer=(unsigned int*)xzalloc(blocksize);
			DIB_start_addr=(unsigned int*)double_indirect_buffer;
			blknr=inode.b.blocks.double_indir_block;
			status = ext2fs_devread (blknr*sector_per_block, 0, blocksize,
							(unsigned int *)double_indirect_buffer);
			for(i=0;i<blocksize/sizeof(unsigned int);i++)
			{
				if(*double_indirect_buffer==0)
					break;
#ifdef DEBUG_LOG
				printf("DICB releasing %d\n",*double_indirect_buffer);
#endif
				if(blocksize!=1024)
				{
					bg_bitmap_index=(*double_indirect_buffer)/(uint32_t)ext4fs_root->sblock.blocks_per_group;
				}
				else
				{
					bg_bitmap_index=(*double_indirect_buffer)/(uint32_t)ext4fs_root->sblock.blocks_per_group;
					remainder = (*double_indirect_buffer)%(uint32_t)ext4fs_root->sblock.blocks_per_group;
					if(!remainder)
						bg_bitmap_index--;
				}
				ext4fs_update_bmap_with_zero(*double_indirect_buffer,bg[bg_bitmap_index],bg_bitmap_index);
				double_indirect_buffer++;
				gd[bg_bitmap_index].free_blocks++;
				sb->free_blocks++;
				//journal backup
				if(prev_bg_bitmap_index != bg_bitmap_index){
					journal_buffer = (unsigned char *)xzalloc(blocksize);
					status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
										0, blocksize,journal_buffer);
					if(status==0){
						free(journal_buffer);
						return -1;
					}
					log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
					prev_bg_bitmap_index = bg_bitmap_index;
					free(journal_buffer);
				}
			}

			/*removing the parent double indirect block*/
			/*find the bitmap index*/
			blknr=inode.b.blocks.double_indir_block;
			if(blocksize!=1024)
			{
				bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
			}
			else
			{
				bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
				remainder = blknr%(uint32_t)ext4fs_root->sblock.blocks_per_group;
				if(!remainder)
					bg_bitmap_index--;
			}
			ext4fs_update_bmap_with_zero(blknr,bg[bg_bitmap_index],bg_bitmap_index);
			gd[bg_bitmap_index].free_blocks++;
			sb->free_blocks++;
			//journal backup
			if(prev_bg_bitmap_index != bg_bitmap_index){
				journal_buffer = (unsigned char *)xzalloc(blocksize);
				status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
										0, blocksize,journal_buffer);
				if(status==0){
					free(journal_buffer);
					return -1;
				}
				log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
				prev_bg_bitmap_index = bg_bitmap_index;
				free(journal_buffer);
			}
#ifdef DEBUG_LOG
			printf("DIPB releasing %d\n",blknr);
#endif
			if(DIB_start_addr)
				free(DIB_start_addr);
		}

		/*deleting the triple indirect blocks */
		if(inode.b.blocks.tripple_indir_block!=0)
		{
			triple_indirect_grand_parent_buffer=(unsigned int*)xzalloc(blocksize);
			TIB_start_addr=(unsigned int*)triple_indirect_grand_parent_buffer;
			blknr=inode.b.blocks.tripple_indir_block;
			status = ext2fs_devread (blknr*sector_per_block, 0, blocksize,
							(unsigned int *)triple_indirect_grand_parent_buffer);
			for(i=0;i<blocksize/sizeof(unsigned int);i++)
			{
				if(*triple_indirect_grand_parent_buffer==0)
					break;
#ifdef DEBUG_LOG
				printf("TIGPB releasing %d\n",*triple_indirect_grand_parent_buffer);
#endif
				triple_indirect_parent_buffer=(unsigned int*)xzalloc(blocksize);
				TIPB_start_addr = (unsigned int*)triple_indirect_parent_buffer;
				status = ext2fs_devread ((*triple_indirect_grand_parent_buffer)*sector_per_block, 0,
										blocksize,(unsigned int *)triple_indirect_parent_buffer);
				for(j=0;j<blocksize/sizeof(unsigned int);j++)
				{
					if(*triple_indirect_parent_buffer==0)
					break;
					
					if(blocksize!=1024)
					{
						bg_bitmap_index=(*triple_indirect_parent_buffer)/
											(uint32_t)ext4fs_root->sblock.blocks_per_group;
					}
					else
					{
						bg_bitmap_index=(*triple_indirect_parent_buffer)/
										(uint32_t)ext4fs_root->sblock.blocks_per_group;
						remainder = (*triple_indirect_parent_buffer)%
										(uint32_t)ext4fs_root->sblock.blocks_per_group;
						if(!remainder)
							bg_bitmap_index--;
					}
					
					ext4fs_update_bmap_with_zero(*triple_indirect_parent_buffer,
									bg[bg_bitmap_index],bg_bitmap_index);
					triple_indirect_parent_buffer++;
					gd[bg_bitmap_index].free_blocks++;
					sb->free_blocks++;
					//journal backup
					if(prev_bg_bitmap_index != bg_bitmap_index){
						journal_buffer = (unsigned char *)xzalloc(blocksize);
						status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
												0, blocksize,journal_buffer);
						if(status==0){
							free(journal_buffer);
							return -1;
						}
						log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
						prev_bg_bitmap_index = bg_bitmap_index;
						free(journal_buffer);
					}
				}
				if(TIPB_start_addr)
					free(TIPB_start_addr);

				/*removing the grand parent blocks which is connected to inode*/
				if(blocksize!=1024)
				{
					bg_bitmap_index=(*triple_indirect_grand_parent_buffer)/
								(uint32_t)ext4fs_root->sblock.blocks_per_group;
				}
				else
				{
					bg_bitmap_index=(*triple_indirect_grand_parent_buffer)/
											(uint32_t)ext4fs_root->sblock.blocks_per_group;
					remainder = (*triple_indirect_grand_parent_buffer)%
											(uint32_t)ext4fs_root->sblock.blocks_per_group;
					if(!remainder)
						bg_bitmap_index--;
				}
				ext4fs_update_bmap_with_zero(*triple_indirect_grand_parent_buffer,
									bg[bg_bitmap_index],bg_bitmap_index);
				triple_indirect_grand_parent_buffer++;
				gd[bg_bitmap_index].free_blocks++;
				sb->free_blocks++;
				//journal backup
				if(prev_bg_bitmap_index != bg_bitmap_index){
					journal_buffer = (unsigned char *)xzalloc(blocksize);
					status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
											0, blocksize,journal_buffer);
					if(status==0){
						free(journal_buffer);
						return -1;
					}
					log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
					prev_bg_bitmap_index = bg_bitmap_index;
					free(journal_buffer);
				}
		}

		/*removing the grand parent triple indirect block*/
		/*find the bitmap index*/
		blknr=inode.b.blocks.tripple_indir_block;
		if(blocksize!=1024)
		{
			bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
		}
		else
		{
			bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
			remainder = blknr%(uint32_t)ext4fs_root->sblock.blocks_per_group;
			if(!remainder)
				bg_bitmap_index--;
		}
		ext4fs_update_bmap_with_zero(blknr,bg[bg_bitmap_index],bg_bitmap_index);		
		gd[bg_bitmap_index].free_blocks++;
		sb->free_blocks++;
		//journal backup
		if(prev_bg_bitmap_index != bg_bitmap_index){
			journal_buffer = (unsigned char *)xzalloc(blocksize);
			status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
									0, blocksize,journal_buffer);
			if(status==0){
				free(journal_buffer);
				return -1;
			}
			log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
			prev_bg_bitmap_index = bg_bitmap_index;
			free(journal_buffer);
		}
#ifdef DEBUG_LOG
			printf("TIGPB iteself releasing %d\n",blknr);
#endif
			if(TIB_start_addr)
				free(TIB_start_addr);
		}

		/*read the block no allocated to a file*/
		no_blocks= inode.size/blocksize;
		if(inode.size%blocksize)
		no_blocks++;
		for(i=0;i<no_blocks;i++)
		{
			blknr = ext4fs_read_allocated_block(&inode, i);
			if(blocksize!=1024)
			{
				bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
			}
			else
			{
				bg_bitmap_index=blknr/(uint32_t)ext4fs_root->sblock.blocks_per_group;
				remainder = blknr%(uint32_t)ext4fs_root->sblock.blocks_per_group;
				if(!remainder)
				bg_bitmap_index--;
			}
			ext4fs_update_bmap_with_zero(blknr,bg[bg_bitmap_index],bg_bitmap_index);	
#ifdef DEBUG_LOG
			printf("ActualB releasing %d: %d\n",blknr,bg_bitmap_index);
#endif
			gd[bg_bitmap_index].free_blocks++;
			sb->free_blocks++;
			//journal backup
			if(prev_bg_bitmap_index != bg_bitmap_index){
				journal_buffer = (unsigned char *)xzalloc(blocksize);
				status = ext2fs_devread (gd[bg_bitmap_index].block_id*sector_per_block,
										0, blocksize,journal_buffer);
				if(status==0){
					free(journal_buffer);
					return -1;
				}
				log_journal(journal_buffer,gd[bg_bitmap_index].block_id);
				prev_bg_bitmap_index = bg_bitmap_index;
				free(journal_buffer);
			}
		}
	}

	/*from the inode no to blockno*/
	inodes_per_block = EXT2_BLOCK_SIZE(ext4fs_root) / inode_size;
	inode_bitmap_index = inodeno / (uint32_t)ext4fs_root->sblock.inodes_per_group;

	/*get the block no*/
	inodeno--;
	blkno = __le32_to_cpu (gd[inode_bitmap_index].inode_table_id) +
	(inodeno % __le32_to_cpu (ext4fs_root->sblock.inodes_per_group))/ inodes_per_block;

	/*get the offset of the inode*/
	blkoff = ((inodeno) % inodes_per_block) * inode_size;

	/*read the block no containing the inode*/
	read_buffer=xzalloc(blocksize);
	start_block_address=read_buffer;
	status = ext2fs_devread (blkno* sector_per_block, 0, blocksize,(unsigned  char *) read_buffer);
	if(status == 0) {
		goto fail;
	}
	log_journal(read_buffer,blkno);

	read_buffer = read_buffer+blkoff;
	inode_buffer = (struct ext2_inode*)read_buffer;
	memset(inode_buffer,'\0',sizeof(struct ext2_inode));

	/*write the inode to original position in inode table*/
	put_metadata(start_block_address,blkno);

	/*update the respective inode bitmaps*/
	inode_no = ++inodeno;
	ext4fs_update_inode_bmap_with_zero(inode_no,inode_bmaps[inode_bitmap_index],inode_bitmap_index);
	gd[inode_bitmap_index].free_inodes++;
	sb->free_inodes++;
	//journal backup
	journal_buffer = (unsigned char *)xzalloc(blocksize);
	status = ext2fs_devread (gd[inode_bitmap_index].inode_id*sector_per_block, 0, blocksize,journal_buffer);
	if(status == 0) {
		free(journal_buffer);
		return -1;
	}
	log_journal(journal_buffer,gd[inode_bitmap_index].inode_id);
	free(journal_buffer);

	ext4fs_update(dev_desc);
	ext4fs_deinit(dev_desc);

	if(ext4fs_init(dev_desc) != 0) {
		printf("error in File System init\n");
		return -1;
	}

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

int ext4fs_get_bgdtable(void )
{
	int blocksize, i;
	int sector_per_block;
	int status;
	int grp_desc_size;
	unsigned char *temp;
	unsigned int var = gd_table_block_no;
	
	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);

	/*calucalate sector per block*/
	sector_per_block=blocksize/SECTOR_SIZE;
	grp_desc_size = sizeof(struct ext2_block_group);
	no_blk_req_per_gdt = (no_blockgroup * grp_desc_size)/ blocksize;
	if((no_blockgroup * grp_desc_size)% blocksize)
		no_blk_req_per_gdt++;

#ifdef DEBUG_LOG
	printf("size of grp desc =%d\n",grp_desc_size);
	printf("number of block groups =%d\n",no_blockgroup);
	printf("Number of blocks reqd for grp desc table =%d\n",no_blk_req_per_gdt);
	printf("gdtable block no is %d\n",gd_table_block_no);
#endif

	/*allocate mem for gdtable*/
	gd_table= (char *)xzalloc(blocksize * no_blk_req_per_gdt);
	temp = gd_table;
	/*Read the first group descriptor table*/
	status = ext2fs_devread (gd_table_block_no * sector_per_block, 0,
						blocksize * no_blk_req_per_gdt,gd_table);
	if (status == 0)
	{
		goto fail;
	}

	for(i =0;i < no_blk_req_per_gdt ;i++) {
		journal_ptr[gindex]->buf = (char *)xzalloc(blocksize);
		memcpy(journal_ptr[gindex]->buf,temp,blocksize);
		temp += blocksize;
		journal_ptr[gindex++]->blknr = var++;
	}
	return 0;

fail:
	if(gd_table) {
		free(gd_table);
		gd_table = NULL;
	}
	return -1;
}

void ext4fs_deinit(block_dev_desc_t *dev_desc)
{
	int i;
	struct ext2_inode inode_journal;
	journal_superblock_t *jsb;
	unsigned int blknr;
	char *temp_buff = (char *)xzalloc(blocksize);
	ext4fs_read_inode(ext4fs_root, EXT2_JOURNAL_INO,&inode_journal);
	blknr = ext4fs_read_allocated_block (&inode_journal, EXT2_JOURNAL_SUPERBLOCK);
	ext2fs_devread(blknr * sector_per_block, 0, blocksize, temp_buff);
	jsb=(journal_superblock_t*)temp_buff;

	jsb->s_start= le_be(0);
	put_ext4(dev_desc,(uint64_t)(blknr * blocksize),
			(struct journal_superblock_t*)temp_buff,(uint32_t)blocksize);

	/*get the superblock*/
	ext2fs_devread (SUPERBLOCK_SECTOR, 0, SUPERBLOCK_SIZE, sb);
	sb->feature_incompat &= ~EXT3_FEATURE_INCOMPAT_RECOVER;
	put_ext4(dev_desc,(uint64_t)(SUPERBLOCK_SIZE),(struct ext2_sblock*)sb,(uint32_t)SUPERBLOCK_SIZE);
	if(temp_buff) {
		free(temp_buff);
		temp_buff=NULL;
	}

	if(sb) {
		free(sb);
		sb=NULL;
	}
	
	if(bg) {
		for(i=0;i<no_blockgroup;i++) {
			if(bg[i]) {
				free(bg[i]);
				bg[i]=NULL;
			}
		}
		free(bg);
		bg=NULL;
	}

	if(inode_bmaps) {
		for(i=0;i<no_blockgroup;i++) {
			if(inode_bmaps[i]) {
				free(inode_bmaps[i]);
				inode_bmaps[i]=NULL;
			}
		}
		free(inode_bmaps);
		inode_bmaps=NULL;
	}

	if(gd_table) {
		free(gd_table);
		gd_table=NULL;
	}
	gd=NULL;

	/* Reinitiliazed the global inode and block bitmap first execution check variables */
	first_execution_inode = 0;
	first_execution = 0;
	inode_no=0;
	j_blkindex = 1;
	return;
}

/* This function stores the backup copy of meta data in RAM
*  journal_buffer -- Buffer containing meta data
*  blknr -- Block number on disk of the meta data buffer
*/
void log_journal(char *journal_buffer,unsigned int blknr)
{
	int i = 0;
	for(i =0;i < MAX_JOURNAL_ENTRIES; i++) {
		if(journal_ptr[i]->blknr == -1)
			break;
		if(journal_ptr[i]->blknr == blknr)
			return;
	}
	if(!journal_buffer)
		return ;
	journal_ptr[gindex]->buf = (char *)xzalloc(blocksize);
	if(!journal_ptr[gindex]->buf)
		return ;
	memcpy(journal_ptr[gindex]->buf,journal_buffer,blocksize);
	journal_ptr[gindex++]->blknr = blknr;
}

/* This function stores the modified meta data in RAM
*  metadata_buffer -- Buffer containing meta data
*  blknr -- Block number on disk of the meta data buffer
*/
void put_metadata(char *metadata_buffer,unsigned int blknr)
{
	if(!metadata_buffer)
		return ;
	dirty_block_ptr[gd_index]->buf = (char *)xzalloc(blocksize);
	if(!dirty_block_ptr[gd_index]->buf)
		return ;
	memcpy(dirty_block_ptr[gd_index]->buf,metadata_buffer,blocksize);
	dirty_block_ptr[gd_index++]->blknr = blknr;
}

/*------------------------------------*/
/*Revoke block handling functions*/

typedef struct journal_revoke_header_s
{
	journal_header_t r_header;
	int		 r_count;	/* Count of bytes used in the block */
} journal_revoke_header_t;

typedef struct  _revoke_blk_list
{
	char *content;/*revoke block itself*/
	struct _revoke_blk_list  *next;
}revoke_blk_list;

#define TRUE 1
#define FALSE 0
revoke_blk_list *revk_blk_list=NULL;
revoke_blk_list *prev_node=NULL;
static int first_node=TRUE;
#define RECOVER 1
#define SCAN 0


void print_revoke_blks(char *revk_blk)
{
	journal_revoke_header_t *header;
	int offset, max;
	unsigned int blocknr;
	
	if(revk_blk==NULL)
		return;

	/*decode revoke block*/
	header = (journal_revoke_header_t *) revk_blk;
	offset = sizeof(journal_revoke_header_t);
	max = be_le(header->r_count);
	printf("total bytes %d\n",max);

	while (offset < max) {
		blocknr = be_le(*((unsigned int*)(revk_blk+offset)));				
		printf("revoke blknr is %u\n",blocknr);
		offset += 4;
	}
	return;
}
static revoke_blk_list *
_get_node(void)
{
	revoke_blk_list *tmp_node;
	tmp_node=(revoke_blk_list * )malloc(sizeof(revoke_blk_list));
	if(tmp_node==NULL) {
		return NULL;
	}
	tmp_node->content=NULL;
	tmp_node->next=NULL;
	return tmp_node;
}

	
void 
push_revoke_blk (char * buffer)
{
	revoke_blk_list *node;
	
	if(buffer==NULL) {
		printf("buffer ptr is NULL\n");
		return;
	}
	
	/*get the node*/
	if(!(node=_get_node())){
		printf("_get_node: malloc failed\n");
		return;
	}
	
	/*fill the node*/
	node->content=(char*) malloc(blocksize);
	memcpy(node->content,buffer,blocksize);
	
	if(first_node==TRUE) {
		revk_blk_list=node;
		prev_node=node;
		first_node=FALSE; /*not enter again*/
	}
	else	 {
		prev_node->next=node;
		prev_node=node;
	}
	return;
}

void
free_revoke_blks()
{
	revoke_blk_list *tmp_node=revk_blk_list;
	revoke_blk_list *next_node=NULL;
		
	while(tmp_node !=NULL) {
		if (tmp_node->content)
		free(tmp_node->content);
		tmp_node=tmp_node->next;
	}
	
	tmp_node=revk_blk_list;
	while(tmp_node !=NULL) {
		next_node=tmp_node->next;
		free(tmp_node);
		tmp_node=next_node;
	}
	
	revk_blk_list=NULL;
	prev_node=NULL;
	first_node=TRUE;
	return;
}

int check_blknr_for_revoke(unsigned int blknr,int sequence_no)
{
	journal_revoke_header_t *header;
	int offset, max;
	unsigned int blocknr;
	char *revk_blk;
	revoke_blk_list *tmp_revk_node=revk_blk_list;
	while (tmp_revk_node!=NULL) {
		revk_blk=tmp_revk_node->content;
		
		/*decode revoke block*/
		header = (journal_revoke_header_t *) revk_blk;
		if (sequence_no < be_le(header->r_header.h_sequence)) {
			offset = sizeof(journal_revoke_header_t);
			max = be_le(header->r_count);
			
			while (offset < max) {
				blocknr = be_le(*((unsigned int*)(revk_blk+offset)));				
				if(blocknr==blknr)
				{ 
			      		goto found;
				}
				offset += 4;
			}
		}
		tmp_revk_node=tmp_revk_node->next;
	}
	return -1;
	
	found:
		return 0;
}
/*------------------------------------*/
/* This function parses the journal blocks and replays the suceessful transactions
*  A transaction is successful if commit block is found for a descriptor block
*  The tags in descriptor block contain the disk block numbers of the metadata
*  to be replayed
*/
void recover_transaction(block_dev_desc_t *dev_desc,int prev_desc_logical_no)
{
		struct ext2_inode inode_journal;
		journal_header_t *jdb;
		unsigned int blknr;
		char * p_jdb;
		int ofs, flags;
		int i;
		struct ext3_journal_block_tag *tag;
		unsigned char *temp_buff = (char *)xzalloc(blocksize);
		unsigned char *metadata_buff = (char *)xzalloc(blocksize);
		i = prev_desc_logical_no;
		ext4fs_read_inode(ext4fs_root, EXT2_JOURNAL_INO,&inode_journal);
		blknr = ext4fs_read_allocated_block (&inode_journal, i);
		ext2fs_devread(blknr * sector_per_block, 0, blocksize, temp_buff);
		p_jdb=(char *)temp_buff;
		jdb=(journal_header_t *)temp_buff;
		ofs = sizeof(journal_header_t);

		do {
		 tag = (struct ext3_journal_block_tag *)&p_jdb[ofs];
		 ofs += sizeof (struct ext3_journal_block_tag);

		 if (ofs > blocksize)
		   break;

		 flags = be_le (tag->flags);
		 if (! (flags & EXT3_JOURNAL_FLAG_SAME_UUID))
		   ofs += 16;

		 i++;
#ifdef DEBUG_LOG
		 printf("\t\ttag %u\n",be_le(tag->block));
#endif
		 if(revk_blk_list!=NULL)  {
		 	if(check_blknr_for_revoke(be_le(tag->block),be_le(jdb->h_sequence))==0)  {
		 		//printf("blknr %u  this should be ignored\n",be_le(tag->block));
				continue;
		 	}
		 } 

		 blknr = ext4fs_read_allocated_block (&inode_journal, i);
		 ext2fs_devread(blknr * sector_per_block, 0, blocksize, metadata_buff);
		 put_ext4(dev_desc,(uint64_t)(be_le(tag->block) * blocksize),
										metadata_buff,(uint32_t)blocksize);
		}
		while (! (flags & EXT3_JOURNAL_FLAG_LAST_TAG));
		if(temp_buff)
			free(temp_buff);
		if(metadata_buff)
			free(metadata_buff);
}


int print_jrnl_status(int recovery_flag)
{
	/*
	*if this function is called  scanning/recovery 
	*of journal must be  stoped.
	*/
	if(recovery_flag==RECOVER) {
		printf("Journal Recovery Completed\n");
	}
	else {
		printf("Journal Scan Completed\n");
	}
	return 0;
}


int check_journal_state(block_dev_desc_t *dev_desc,int recovery_flag)
{
		struct ext2_inode inode_journal;
		journal_superblock_t *jsb;
		journal_header_t *jdb;
		char * p_jdb;
		int i;
		int DB_FOUND = NO;
		unsigned int blknr;
		char *temp_buff = (char *)xzalloc(blocksize);
		char *temp_buff1 = (char *)xzalloc(blocksize);

		int transaction_state=TRANSACTION_COMPLETE;
		int prev_desc_logical_no =0;
		int curr_desc_logical_no =0;

		int ofs, flags;
		struct ext3_journal_block_tag *tag;

		ext4fs_read_inode(ext4fs_root, EXT2_JOURNAL_INO,&inode_journal);
		blknr = ext4fs_read_allocated_block (&inode_journal, EXT2_JOURNAL_SUPERBLOCK);
		ext2fs_devread(blknr * sector_per_block, 0, blocksize, temp_buff);
		jsb=(journal_superblock_t*)temp_buff;

		if(sb->feature_incompat & EXT3_FEATURE_INCOMPAT_RECOVER) {
			if(recovery_flag==RECOVER)
			printf("umount didn't happened :recovery required \n");
		}
		else {
			if(recovery_flag==RECOVER)
			printf("File System is consistent \n");
			goto END;
		}

		if(be_le(jsb->s_start)==0){
			goto END;
		}

		if(jsb->s_feature_compat & le_be(JBD2_FEATURE_COMPAT_CHECKSUM)){
			//printf("checksum set journal \n");
		}
		else {
			jsb->s_feature_compat |= le_be(JBD2_FEATURE_COMPAT_CHECKSUM);
		}

#ifdef DEBUG_LOG
		if(be_le(jsb->s_header.h_blocktype)==EXT3_JOURNAL_SUPERBLOCK_V1 ||
							be_le(jsb->s_header.h_blocktype)==EXT3_JOURNAL_SUPERBLOCK_V2 )
		{
			printf("journal superblock info:\n");
			printf("errno is %u\n",be_le(jsb->s_errno));
			printf("start is %u\n",be_le(jsb->s_start));
			printf("header sequence is %u\n",be_le(jsb->s_header.h_sequence));
			printf("header magic is %u\n",be_le(jsb->s_header.h_magic));
			printf("sequence is %u\n",be_le(jsb->s_sequence));
			printf("first block of log is %u\n",be_le(jsb->s_first));
			printf("total block of journal is %u\n",be_le(jsb->s_maxlen));
			printf("s_feature_compat is %u\n",be_le(jsb->s_feature_compat));
			printf("s_feature_incompat is %u\n",be_le(jsb->s_feature_incompat));
			printf("s_feature_ro_compat is %u\n",be_le(jsb->s_feature_ro_compat));
			printf("s_nr_users is %u\n",be_le(jsb->s_nr_users));
			printf("total block of journal is %u\n",be_le(jsb->s_dynsuper));
			printf("----------------------\n");
		}
#endif
			i = be_le(jsb->s_first);
			while(1){
				blknr = ext4fs_read_allocated_block (&inode_journal, i);
				memset(temp_buff1,'\0',blocksize);
				ext2fs_devread(blknr * sector_per_block, 0, blocksize, temp_buff1);
				jdb=(journal_header_t*)temp_buff1;

				if(be_le(jdb->h_blocktype)==EXT3_JOURNAL_DESCRIPTOR_BLOCK)/*descriptor block*/
				{
#ifdef DEBUG_LOG
					printf("Descriptor block \n");
#endif
					if(be_le(jdb->h_sequence)!=be_le(jsb->s_sequence)){
						print_jrnl_status(recovery_flag);
						break;
					}

					curr_desc_logical_no = i;
					if(transaction_state==TRANSACTION_COMPLETE)
						transaction_state=TRANSACTION_RUNNING;
					else{
						//printf("recovery required\n");
						return -1;
					}
					p_jdb=(char *)temp_buff1;
#ifdef DEBUG_LOG
					printf("\tIts Descriptor block: %u\n",blknr);
					printf("\t\tsequence no is %u\n",be_le(jdb->h_sequence));
					printf("\t\tmagic   no is %u\n",be_le(jdb->h_magic));
#endif
					ofs = sizeof(journal_header_t);
		           		do {
		                		tag = (struct ext3_journal_block_tag *)&p_jdb[ofs];
		                		ofs += sizeof (struct ext3_journal_block_tag);

		                		if (ofs > blocksize)
		                  		break;

		                		flags = be_le (tag->flags);
		                		if (! (flags & EXT3_JOURNAL_FLAG_SAME_UUID))
		                  			ofs += 16;

		                			i++;
#ifdef DEBUG_LOG
		               			 printf("\t\ttag %u\n",be_le(tag->block));
#endif
					}
		             		while (! (flags & EXT3_JOURNAL_FLAG_LAST_TAG));
					i++;
					DB_FOUND = YES;
				}
				else if(be_le(jdb->h_blocktype)==EXT3_JOURNAL_COMMIT_BLOCK){
#ifdef DEBUG_LOG
					printf("Commit block\n");
#endif
					if ((be_le(jdb->h_sequence) !=
						 be_le(jsb->s_sequence))) {
						print_jrnl_status(recovery_flag);
						break;
					}

					if ((transaction_state ==
						TRANSACTION_RUNNING) ||
							(DB_FOUND == NO)) {
						transaction_state=TRANSACTION_COMPLETE;
						i++;
						jsb->s_sequence = le_be(be_le(jsb->s_sequence)+1);
					}
#ifdef DEBUG_LOG
					printf("\tIts Commit block: %u\n",blknr);
					printf("\t\tsequence no is %u\n",be_le(jdb->h_sequence));
					printf("\t\tmagic   no is %u\n",be_le(jdb->h_magic));
					printf("---------------------------\n");
#endif
					prev_desc_logical_no = curr_desc_logical_no;
					if ((recovery_flag == RECOVER) &&
							 (DB_FOUND == YES))
						recover_transaction(dev_desc,prev_desc_logical_no);
					DB_FOUND = NO;
				}
				else if (be_le(jdb->h_blocktype)==EXT3_JOURNAL_REVOKE_BLOCK){
					if(be_le(jdb->h_sequence)!=be_le(jsb->s_sequence)) {
						print_jrnl_status(recovery_flag);
						break;
					}
#ifdef DEBUG_LOG
					printf("\tIts Revoke block: %u\n",blknr);
					printf("\t\tsequence no is %u\n",be_le(jdb->h_sequence));
					printf("\t\tmagic   no is %u\n",be_le(jdb->h_magic));
					printf("---------------------------\n");
#endif
					if(recovery_flag==SCAN) {
						push_revoke_blk(jdb);
					}
					i++;
				}
				else {
#ifdef DEBUG_LOG
					printf("Else Case \n");
#endif
					if(be_le(jdb->h_sequence)!=be_le(jsb->s_sequence)) {
						print_jrnl_status(recovery_flag);
					break;
					}
				}
		}

END:
			if(recovery_flag==RECOVER)  {
				jsb->s_start= le_be(1);
				jsb->s_sequence = le_be(be_le(jsb->s_sequence)+1);
				/*get the superblock*/
				ext2fs_devread (SUPERBLOCK_SECTOR, 0, SUPERBLOCK_SIZE, sb);
				sb->feature_incompat |= EXT3_FEATURE_INCOMPAT_RECOVER;

				/*Update the super block*/
				put_ext4(dev_desc,(uint64_t)(SUPERBLOCK_SIZE),(struct ext2_sblock*)sb,(uint32_t)SUPERBLOCK_SIZE);
				ext2fs_devread (SUPERBLOCK_SECTOR, 0, SUPERBLOCK_SIZE, sb);

				blknr = ext4fs_read_allocated_block (&inode_journal, EXT2_JOURNAL_SUPERBLOCK);
				put_ext4(dev_desc,(uint64_t)(blknr * blocksize),
						(struct journal_superblock_t*)temp_buff,(uint32_t)blocksize);
				free_revoke_blks();
			}

				if(temp_buff){
					free(temp_buff);
					temp_buff =NULL;
				}

				if(temp_buff1){
					free(temp_buff1);
					temp_buff1 =NULL;
				}
				return 0;
}

int ext4fs_init(block_dev_desc_t *dev_desc )
{
	int status;
	int i;
	char *temp = NULL;
	unsigned int real_free_blocks=0;
	
	/*get the blocksize inode sector per block of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);
	inodesize=INODE_SIZE_FILESYSTEM(ext4fs_root);
	sector_per_block=blocksize/SECTOR_SIZE;
	
	/*get the superblock*/
	sb=(struct ext2_sblock *)xzalloc(SUPERBLOCK_SIZE);
	status = ext2fs_devread (SUPERBLOCK_SECTOR, 0, SUPERBLOCK_SIZE, sb);
	if(status==0)
	{
		goto fail;
	}

	for(i=0; i< MAX_JOURNAL_ENTRIES; i++){
		journal_ptr[i] = (struct journal_log *)xzalloc(sizeof(struct journal_log));
		if(!journal_ptr[i])
			goto fail;
		dirty_block_ptr[i] = (struct journal_log *)xzalloc(sizeof(struct dirty_blocks));
		if(!dirty_block_ptr[i])
			goto fail;
		journal_ptr[i]->buf = NULL;
		journal_ptr[i]->blknr = -1;

		dirty_block_ptr[i]->buf = NULL;
		dirty_block_ptr[i]->blknr = -1;
	}

	if(blocksize == 4096) {
		temp = (char *)xzalloc(blocksize);
		if(!temp)
			goto fail;
		journal_ptr[gindex]->buf = (char *)xzalloc(blocksize);
		if(!journal_ptr[gindex]->buf)
			goto fail;
		ext2fs_devread (0, 0, blocksize, temp);
		memcpy(temp + SUPERBLOCK_SIZE, sb,SUPERBLOCK_SIZE);
		memcpy(journal_ptr[gindex]->buf,temp,blocksize);
		journal_ptr[gindex++]->blknr = 0;
		free(temp);
	}
	else {
		journal_ptr[gindex]->buf = (char *)xzalloc(blocksize);
		if(!journal_ptr[gindex]->buf)
			goto fail;
		memcpy(journal_ptr[gindex]->buf,sb,SUPERBLOCK_SIZE);
		journal_ptr[gindex++]->blknr = 1;
	}

	if(check_journal_state(dev_desc,SCAN))
		goto fail;
	/* Check the file system state using journal super block */
	if(check_journal_state(dev_desc,RECOVER))
		goto fail;

	/*get the no of blockgroups*/
	no_blockgroup = (uint32_t)ext4fs_div_roundup((uint32_t)(ext4fs_root->sblock.total_blocks
				- ext4fs_root->sblock.first_data_block),(uint32_t)ext4fs_root->sblock.blocks_per_group);

	/*get the block group descriptor table*/
	gd_table_block_no=((EXT2_MIN_BLOCK_SIZE == blocksize)+1);
	if(ext4fs_get_bgdtable()== -1)
	{
		printf("***Eror in getting the block group descriptor table\n");
		goto fail;
	}
	else
	{
		gd=(struct ext2_block_group*)gd_table;
	}

	/*load all the available bitmap block of the partition*/
	bg=(unsigned char **)malloc(no_blockgroup * sizeof(unsigned char *));
	for(i=0;i<no_blockgroup;i++)
	{
		bg[i]=(unsigned char*)xzalloc(blocksize);
		if(!bg[i])
			goto fail;
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
		if(!inode_bmaps[i])
			goto fail;
	}

	for(i=0;i<no_blockgroup;i++)
	{
		status = ext2fs_devread (gd[i].inode_id*sector_per_block, 0, blocksize,
									(unsigned char*)inode_bmaps[i]);
		if(status==0)
		{
			goto fail;
		}
	}

	/*
	*check filesystem consistency with free blocks of file system
	*some time we observed that superblock freeblocks does not match
	*with the  blockgroups freeblocks when improper reboot of a linux kernel
	*/
	for(i=0;i<no_blockgroup;i++)
	{
		real_free_blocks=real_free_blocks + gd[i].free_blocks;
	}
	if(real_free_blocks!=sb->free_blocks)
	{
		sb->free_blocks=real_free_blocks;
	}

#ifdef DEBUG_LOG
	printf("-----------------------------\n");
	printf("inode size: %d\n",inodesize);
	printf("block size: %d\n",blocksize);
	printf("blocks per group: %d\n",ext4fs_root->sblock.blocks_per_group);
	printf("no of groups in this partition: %d\n",no_blockgroup);
	printf("block group descriptor blockno: %d\n",gd_table_block_no);
	printf("sector size is %d\n",SECTOR_SIZE);
	printf("size of inode structure is %d\n",sizeof(struct ext2_inode));
	printf("-----------------------------\n");
#endif

#ifdef DEBUG_LOG
	//ext4fs_dump_the_inode_group_before_writing();
	ext4fs_dump_the_block_group_before_writing();

#endif

	return 0;
fail:
	ext4fs_deinit(dev_desc);
	return -1;
}

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

int ext4fs_checksum_update(unsigned int i)
{
	struct ext2_block_group *desc;
	__u16 crc = 0;

	desc = (struct ext2_block_group *)&gd[i];

	if (sb->feature_ro_compat & EXT4_FEATURE_RO_COMPAT_GDT_CSUM) {
		int offset = offsetof(struct ext2_block_group, bg_checksum);

		crc = ext2fs_crc16(~0, sb->unique_id,
				   sizeof(sb->unique_id));
		crc = ext2fs_crc16(crc, &i, sizeof(i));
		crc = ext2fs_crc16(crc, desc, offset);
		offset += sizeof(desc->bg_checksum); /* skip checksum */
		assert(offset == sizeof(*desc));
	}
	return crc;
}

int update_descriptor_block(block_dev_desc_t *dev_desc, unsigned int blknr)
{
	journal_header_t jdb;
	struct ext3_journal_block_tag tag;
	struct ext2_inode inode_journal;
	journal_superblock_t *jsb;
	unsigned int jsb_blknr;
	char *buf =NULL, *temp = NULL;
	int  i;
	char *temp_buff = (char *)xzalloc(blocksize);

	ext4fs_read_inode(ext4fs_root, EXT2_JOURNAL_INO,&inode_journal);
	jsb_blknr = ext4fs_read_allocated_block (&inode_journal, EXT2_JOURNAL_SUPERBLOCK);
	ext2fs_devread(jsb_blknr * sector_per_block, 0, blocksize, temp_buff);
	jsb=(journal_superblock_t*)temp_buff;

	jdb.h_blocktype = le_be(EXT3_JOURNAL_DESCRIPTOR_BLOCK);
	jdb.h_magic = le_be(EXT3_JOURNAL_MAGIC_NUMBER);	
	jdb.h_sequence = jsb->s_sequence;
	buf = (char *)xzalloc(blocksize);
	temp = buf;
	memcpy(buf,&jdb, sizeof(journal_header_t));
	temp += sizeof(journal_header_t);

	for(i =0;i < MAX_JOURNAL_ENTRIES; i++) {
		if(journal_ptr[i]->blknr == -1)
			break;

		tag.block = le_be(journal_ptr[i]->blknr);
		tag.flags = le_be(EXT3_JOURNAL_FLAG_SAME_UUID);
		memcpy(temp,&tag, sizeof (struct ext3_journal_block_tag));
		temp = temp + sizeof (struct ext3_journal_block_tag);
	}

	tag.block = le_be(journal_ptr[--i]->blknr);
	tag.flags = le_be(EXT3_JOURNAL_FLAG_LAST_TAG);
	memcpy(temp - sizeof (struct ext3_journal_block_tag) ,&tag, sizeof (struct ext3_journal_block_tag));
	put_ext4(dev_desc,(uint64_t)(blknr*blocksize),buf,(uint32_t)blocksize);

	if(temp_buff){
		free(temp_buff);
		temp_buff =NULL;
	}
	if(buf){
		free(buf);
		buf =NULL;
	}
	return 0;
}

int update_commit_block(block_dev_desc_t *dev_desc ,unsigned int blknr)
{
	journal_header_t jdb;
	char *buf = NULL;
	struct ext2_inode inode_journal;
	journal_superblock_t *jsb;
	unsigned int jsb_blknr;
	char *temp_buff = (char *)xzalloc(blocksize);

	ext4fs_read_inode(ext4fs_root, EXT2_JOURNAL_INO,&inode_journal);
	jsb_blknr = ext4fs_read_allocated_block (&inode_journal, EXT2_JOURNAL_SUPERBLOCK);
	ext2fs_devread(jsb_blknr * sector_per_block, 0, blocksize, temp_buff);
	jsb=(journal_superblock_t*)temp_buff;

	jdb.h_blocktype = le_be(EXT3_JOURNAL_COMMIT_BLOCK);
	jdb.h_magic = le_be(EXT3_JOURNAL_MAGIC_NUMBER);
	jdb.h_sequence = jsb->s_sequence;
	buf = (char *)xzalloc(blocksize);
	memcpy(buf,&jdb, sizeof(journal_header_t));
	put_ext4(dev_desc,(uint64_t)(blknr*blocksize),buf,(uint32_t)blocksize);

	if(temp_buff){
		free(temp_buff);
		temp_buff =NULL;
	}
	if(buf){
		free(buf);
		buf =NULL;
	}
	return 0;
}

int update_journal(block_dev_desc_t *dev_desc)
{
	struct ext2_inode inode_journal;
	unsigned int blknr;
	int i;
	unsigned char *metadata_buff = (unsigned char *)xzalloc(blocksize);
	ext4fs_read_inode(ext4fs_root, EXT2_JOURNAL_INO,&inode_journal);
	blknr = ext4fs_read_allocated_block (&inode_journal, j_blkindex++);
	update_descriptor_block(dev_desc ,blknr);
	for(i=0 ;i < MAX_JOURNAL_ENTRIES; i++ ) {
		if(journal_ptr[i]->blknr == -1)
		break;
		blknr = ext4fs_read_allocated_block (&inode_journal, j_blkindex++);
		put_ext4(dev_desc,(uint64_t)(blknr*blocksize),journal_ptr[i]->buf,(uint32_t)blocksize);
	}
	blknr = ext4fs_read_allocated_block (&inode_journal, j_blkindex++);
	update_commit_block(dev_desc ,blknr);
	printf("update journal finished \n");
	if(metadata_buff){
		free(metadata_buff);
		metadata_buff = NULL;
	}
	return 0;
}

void ext4fs_update(block_dev_desc_t *dev_desc)
{
	unsigned int i;
	update_journal(dev_desc);
	/*Update the super block*/
	put_ext4(dev_desc,(uint64_t)(SUPERBLOCK_SIZE),(struct ext2_sblock*)sb,(uint32_t)SUPERBLOCK_SIZE);

	/*update the block group*/
	for(i=0;i<no_blockgroup;i++)
	{
		gd[i].bg_checksum = ext4fs_checksum_update(i);
		put_ext4(dev_desc,(uint64_t)(gd[i].block_id*blocksize),bg[i],(uint32_t)blocksize);
	}

	/*update the inode table group*/
	for(i=0;i<no_blockgroup;i++)
	{
		put_ext4(dev_desc,(uint64_t)(gd[i].inode_id*blocksize),inode_bmaps[i],(uint32_t)blocksize);
	}

	/*update the block group descriptor table*/
	put_ext4(dev_desc,(uint64_t)(gd_table_block_no*blocksize),(struct ext2_block_group*)gd_table,
					(uint32_t)(blocksize * no_blk_req_per_gdt));

	for(i =0;i < MAX_JOURNAL_ENTRIES; i++) {
		if(dirty_block_ptr[i]->blknr == -1)
			break;
		put_ext4(dev_desc,(uint64_t)(dirty_block_ptr[i]->blknr * blocksize),dirty_block_ptr[i]->buf,
							(uint32_t)blocksize);
		free(dirty_block_ptr[i]->buf);
	}

	for(i =0;i < MAX_JOURNAL_ENTRIES; i++) {
		if(journal_ptr[i]->blknr == -1)
			break;
		free(journal_ptr[i]->buf);
	}

	for(i=0; i< MAX_JOURNAL_ENTRIES; i++){
		free(journal_ptr[i]);
		free(dirty_block_ptr[i]);
	}

	gindex = 0;
	gd_index = 0;
}

static int check_void_in_dentry(struct ext2_dirent *dir,char *filename)
{
	int dentry_length = 0;
	short int padding_factor = 0;
	int sizeof_void_space = 0;
	int new_entry_byte_reqd = 0;
	
	if(dir->namelen % 4 != 0) {
		padding_factor = 4 - (dir->namelen % 4);/*for null storage*/
	}
	dentry_length = sizeof(struct ext2_dirent) + dir->namelen + padding_factor;
	sizeof_void_space = dir->direntlen - dentry_length;
	if(sizeof_void_space == 0) {
		return 0;
	}
	else {
		padding_factor = 0;
		if(strlen(filename) % 4 != 0) {
			padding_factor = 4 - (strlen(filename) % 4);
		}
		new_entry_byte_reqd = strlen(filename) + sizeof(struct ext2_dirent) + padding_factor;
		if(sizeof_void_space >= new_entry_byte_reqd) {
			dir->direntlen = dentry_length;
			return sizeof_void_space;
		}
		else	{
			return 0;
		}
	}
}
int ext4fs_write(block_dev_desc_t *dev_desc, int part_no,char *fname,
				unsigned char* buffer,unsigned long sizebytes)
{
	unsigned char *root_first_block_buffer = NULL;
	unsigned int i,j,k;
	int status,ret = 0;
	
	/*inode*/
	struct ext2_inode *file_inode = NULL;
	unsigned char *inode_buffer = NULL;
	unsigned int parent_inodeno;
	unsigned int inodeno;
	time_t timestamp = NULL;
	unsigned int first_block_no_of_root;

	/*directory entry*/
	struct ext2_dirent *dir = NULL;
	int templength = 0;
	int totalbytes = 0;
	short int padding_factor = 0;

	/*filesize*/
	uint64_t total_no_of_bytes_for_file;
	unsigned int total_no_of_block_for_file;
	unsigned int total_remaining_blocks;

	/*final actual data block*/
	unsigned int 	actual_block_no;

	/*direct*/
	unsigned int 	direct_blockno;

	/*single indirect*/
	unsigned int 	*single_indirect_buffer = NULL;
	unsigned int 	single_indirect_blockno;
	unsigned int 	*single_indirect_start_address = NULL;

	/*double indirect*/
	unsigned int 	double_indirect_blockno_parent;
	unsigned int 	double_indirect_blockno_child;
	unsigned int 	*double_indirect_buffer_parent = NULL;
	unsigned int 	*double_indirect_buffer_child = NULL;
	unsigned int 	*double_indirect_block_startaddress = NULL;
	unsigned int 	*double_indirect_buffer_child_start = NULL;

	/*Triple Indirect*/
	unsigned int triple_indirect_blockno_grand_parent;
	unsigned int triple_indirect_blockno_parent;
	unsigned int triple_indirect_blockno_child;
	unsigned int *triple_indirect_buffer_grand_parent = NULL;
	unsigned int *triple_indirect_buffer_parent = NULL;
	unsigned int *triple_indirect_buffer_child = NULL;
	unsigned int *triple_indirect_buffer_grand_parent_startaddress = NULL;
	unsigned int *triple_indirect_buffer_parent_startaddress = NULL;
	unsigned int *triple_indirect_buffer_child_startaddress = NULL;

	unsigned int 	existing_file_inodeno;
	unsigned int new_entry_byte_reqd;
	unsigned int last_entry_dirlen;
	int direct_blk_idx;
	unsigned int root_blknr;

	unsigned char *ptr = NULL;
	struct ext2_dirent *temp_dir = NULL;
	ALLOC_CACHE_ALIGN_BUFFER(char, filename, 256);
	memset(filename, 0x00, sizeof(filename));

	unsigned int previous_blknr = -1;
	unsigned int *zero_buffer = NULL;


	char * temp_ptr = NULL;
	unsigned int itable_blkno, parent_itable_blkno,blkoff;
	struct ext2_sblock *sblock = &(ext4fs_root->sblock);
	unsigned int inodes_per_block;
	unsigned int inode_bitmap_index;
	int sizeof_void_space = 0;
	memset(filename, 0x00, sizeof(filename));
	g_parent_inode = (struct ext2_inode *)xzalloc(sizeof(struct ext2_inode));
	if(!g_parent_inode)
		goto fail;

	if(ext4fs_init(dev_desc)!=0)
	{
		printf("error in File System init\n");
		return -1;
	}
	inodes_per_block = blocksize / inodesize;
	parent_inodeno = get_parent_inode_num_write(dev_desc,fname,filename);
	if(parent_inodeno==-1) {
		goto fail;
	}

	if(iget(parent_inodeno,g_parent_inode))
		goto fail;

	/*check if the filename is already present in root*/
	existing_file_inodeno= ext4fs_existing_filename_check(dev_desc,filename);
	if(existing_file_inodeno!=-1)/*file is exist having the same name as filename*/
	{
		inodeno = existing_file_inodeno;
		ret = ext4fs_delete_file(dev_desc,existing_file_inodeno);
		first_execution=0;
		blockno=0;

		first_execution_inode=0;
		inode_no=0;
		if(ret)
			goto fail;
	}

	/*calucalate how many blocks required*/
	total_no_of_bytes_for_file = sizebytes;
	total_no_of_block_for_file = lldiv(total_no_of_bytes_for_file, blocksize);
	
	if(do_div(total_no_of_bytes_for_file, blocksize) != 0)
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

	/*############START: Update the root directory entry block of root inode ##############*/
	/*since we are populating this file under  root
	*directory take the root inode and its first block
	*currently we are looking only in first block of root
	*inode*/

	zero_buffer = xzalloc(blocksize);
	if(!zero_buffer)
		goto fail;
	root_first_block_buffer=memalign(ARCH_DMA_MINALIGN, blocksize);
RESTART:

	/*read the block no allocated to a file*/
	for(direct_blk_idx=0;direct_blk_idx<INDIRECT_BLOCKS;direct_blk_idx++)
	{
			root_blknr=ext4fs_read_allocated_block(g_parent_inode, direct_blk_idx);
			if(root_blknr==0)
			{
				first_block_no_of_root=previous_blknr;
				break;
			}
			previous_blknr=root_blknr;
	}
#ifdef DEBUG_LOG
	printf("previous blk no =%u \n",previous_blknr);
	printf("first_block_no_of_root =%u \n",first_block_no_of_root);
#endif

	status = ext2fs_devread (first_block_no_of_root*sector_per_block, 0, blocksize,
				(char *)root_first_block_buffer);
	if (status == 0)
	{
		goto fail;
	}
	else
	{
		log_journal(root_first_block_buffer,first_block_no_of_root);
		dir=(struct ext2_dirent *) root_first_block_buffer;
		ptr=(unsigned char*)dir;
		totalbytes=0;
		while(dir->direntlen>0)
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
			*it is a last entry of directory entry
			*/

			/*traversing the each directory entry*/
			if(blocksize-totalbytes==dir->direntlen)
			{
				if(strlen(filename)%4!=0)
				{
					padding_factor=4-(strlen(filename)%4);
				}
				new_entry_byte_reqd = strlen(filename) + sizeof(struct ext2_dirent)+padding_factor;
				padding_factor = 0;
				/*update last directory entry length to its length because
				*we are creating new directory entry*/
				if(dir->namelen%4!=0)
				{
					padding_factor=4-(dir->namelen%4);/*for null storage*/
				}
				last_entry_dirlen= dir->namelen+sizeof(struct ext2_dirent)+padding_factor;
				if((blocksize-totalbytes -last_entry_dirlen) < new_entry_byte_reqd)
				{
#ifdef DEBUG_LOG
					printf("Storage capcity of 1st Block over allocating new block\n");
#endif
					if (direct_blk_idx==INDIRECT_BLOCKS-1)
					{
						printf("Directory capacity exceeds the limit\n");
						goto fail;
					}
					g_parent_inode->b.blocks.dir_blocks[direct_blk_idx]= 
									ext4fs_get_next_available_block_no(dev_desc,part_no);
					if(g_parent_inode->b.blocks.dir_blocks[direct_blk_idx] == -1)
					{
						printf("no block left to assign\n");
						goto fail;
					}
					put_ext4(dev_desc,((uint64_t)(g_parent_inode->b.blocks.dir_blocks[direct_blk_idx] * blocksize)),
									zero_buffer, blocksize);
					g_parent_inode->size = g_parent_inode->size + blocksize;
					g_parent_inode->blockcnt= g_parent_inode->blockcnt + sector_per_block;
					put_metadata(root_first_block_buffer,first_block_no_of_root);
					goto RESTART;
				}
				dir->direntlen=last_entry_dirlen;
				break;
			}

			templength=dir->direntlen;
			totalbytes=totalbytes+templength;
			if(sizeof_void_space = check_void_in_dentry(dir,filename)) {
				break;
			}
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
		printf("current inode no is %u\n",dir->inode);
		printf("current directory entry length is %d\n",dir->direntlen);
		printf("current name length is %d\n",dir->namelen);
		printf("current fileltype is %d\n",dir->filetype);
		printf("total bytes is %d\n",totalbytes);
		printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
		printf("-------------------------------------\n");
#endif
	/*get the next available inode number*/
	inodeno=ext4fs_get_next_available_inode_no(dev_desc,part_no);
	if(inodeno==-1)
	{
		printf("no inode left to assign\n");
		goto fail;
	}
#ifdef DEBUG_LOG
		printf("New Inode Assignment :%d\n",inodeno);
#endif
		dir->inode=inodeno;
		if(sizeof_void_space)
			dir->direntlen = sizeof_void_space;
		else
			dir->direntlen = blocksize - totalbytes;
		dir->namelen=strlen(filename);
		dir->filetype=FILETYPE_REG; /*regular file*/
		temp_dir=dir;
		dir=(unsigned char*)dir;
		dir=(unsigned char*)dir+sizeof(struct ext2_dirent);
		memcpy(dir,filename,strlen(filename));
#if DEBUG_LOG
		dir=(struct ext2_dirent*)temp_dir;

		printf("------------------------------------\n");
		printf("JUST AFTER ADDING NAME\n");
		printf("------------------------------------\n");
		printf("current inode no is %u\n",dir->inode);
		printf("current directory entry length is %d\n",dir->direntlen);
		printf("current name length is %d\n",dir->namelen);
		printf("current fileltype is %d\n",dir->filetype);
		printf("total bytes is %d\n",totalbytes);
		printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
		printf("-------------------------------------\n");
#endif
	}

	/*update or write  the 1st block of root inode*/
	put_metadata(root_first_block_buffer,first_block_no_of_root);
	
	if(root_first_block_buffer)
		free(root_first_block_buffer);

	/*############END: Update the root directory entry block of root inode ##############*/

	/*prepare file inode*/
	inode_buffer = xmalloc(inodesize);
	memset(inode_buffer, '\0', inodesize);
	file_inode = (struct ext2_inode *)inode_buffer;
	file_inode->mode= S_IFREG | S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH;
	/* ToDo: Update correct time*/
	file_inode->mtime= timestamp;
	file_inode->atime= timestamp;
	file_inode->ctime= timestamp;
	file_inode->nlinks= 1;
	file_inode->size = sizebytes;

	/*----------------------------START:Allocation of Blocks to Inode---------------------------- */
	/*allocation of direct blocks*/
	for(i=0;i<INDIRECT_BLOCKS;i++)
	{
		direct_blockno=ext4fs_get_next_available_block_no(dev_desc,part_no);
		if(direct_blockno==-1)
		{
			printf("no block left to assign\n");
			goto fail;
		}
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
		single_indirect_blockno=ext4fs_get_next_available_block_no(dev_desc,part_no);
		if(single_indirect_blockno==-1)
		{
			printf("no block left to assign\n");
			goto fail;
		}
		total_no_of_block_for_file++;
#ifdef DEBUG_LOG
		printf("SIPB %d: %d\n",single_indirect_blockno,total_remaining_blocks);
#endif
		status = ext2fs_devread (single_indirect_blockno*sector_per_block, 0, blocksize,
					(unsigned int *)single_indirect_buffer);
		memset(single_indirect_buffer,'\0',blocksize);
		if (status == 0)
		{
			goto fail;
		}

		for(i=0;i<(blocksize/sizeof(unsigned int));i++)
		{
			actual_block_no=ext4fs_get_next_available_block_no(dev_desc,part_no);
			if(actual_block_no==-1)
			{
				printf("no block left to assign\n");
				goto fail;
			}
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
		put_ext4(dev_desc,((uint64_t)(single_indirect_blockno * blocksize)),
						single_indirect_start_address, blocksize);
		file_inode->b.blocks.indir_block=single_indirect_blockno;
		if(single_indirect_start_address)
			free(single_indirect_start_address);
	}

	/*allocation of double indirect blocks*/
	if(total_remaining_blocks!=0)
	{
		/*double indirect parent block connecting to inode*/
		double_indirect_blockno_parent=ext4fs_get_next_available_block_no(dev_desc,part_no);
		if(double_indirect_blockno_parent==-1)
		{
				printf("no block left to assign\n");
				goto fail;
		}
		double_indirect_buffer_parent=xzalloc (blocksize);
		if(!double_indirect_buffer_parent)
		goto fail;

		double_indirect_block_startaddress=double_indirect_buffer_parent;
		total_no_of_block_for_file++;
#ifdef DEBUG_LOG
		printf("DIPB %d: %d\n",double_indirect_blockno_parent,total_remaining_blocks);
#endif
		status = ext2fs_devread (double_indirect_blockno_parent*sector_per_block, 0,
					blocksize,(unsigned int *)double_indirect_buffer_parent);
		memset(double_indirect_buffer_parent,'\0',blocksize);

		/*START: for each double indirect parent block create one more block*/
		for(i=0;i<(blocksize/sizeof(unsigned int));i++)
		{
			double_indirect_blockno_child=ext4fs_get_next_available_block_no(dev_desc,part_no);
			if(double_indirect_blockno_child==-1)
			{
				printf("no block left to assign\n");
				goto fail;
			}
			double_indirect_buffer_child=xzalloc (blocksize);
			if(!double_indirect_buffer_child)
			goto fail;

			double_indirect_buffer_child_start=double_indirect_buffer_child;
			*double_indirect_buffer_parent=double_indirect_blockno_child;
			double_indirect_buffer_parent++;
			total_no_of_block_for_file++;
#ifdef DEBUG_LOG
			printf("DICB %d: %d\n",double_indirect_blockno_child,total_remaining_blocks);
#endif
			status = ext2fs_devread (double_indirect_blockno_child*sector_per_block, 0,
						blocksize,(unsigned int *)double_indirect_buffer_child);
			memset(double_indirect_buffer_child,'\0',blocksize);
			/*END: for each double indirect parent block create one more block*/

			/*filling of actual datablocks for each child*/
			for(j=0;j<(blocksize/sizeof(unsigned int));j++)
			{
				actual_block_no=ext4fs_get_next_available_block_no(dev_desc,part_no);
				if(actual_block_no==-1)
				{
					printf("no block left to assign\n");
					goto fail;
				}
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
			put_ext4(dev_desc,((uint64_t)(double_indirect_blockno_child * blocksize)),
					double_indirect_buffer_child_start, blocksize);
			if(double_indirect_buffer_child_start)
				free(double_indirect_buffer_child_start);

			if(total_remaining_blocks==0)
				break;
		}
		put_ext4(dev_desc,((uint64_t)(double_indirect_blockno_parent * blocksize)),
					double_indirect_block_startaddress, blocksize);
		if(double_indirect_block_startaddress)
			free(double_indirect_block_startaddress);
		file_inode->b.blocks.double_indir_block=double_indirect_blockno_parent;
	}

	/*Triple Indirection */

	/*allocation of triple indirect blocks*/
	if(total_remaining_blocks!=0)
	{
		/*triple indirect grand parent block connecting to inode*/
		triple_indirect_blockno_grand_parent=ext4fs_get_next_available_block_no(dev_desc,part_no);
		if(triple_indirect_blockno_grand_parent==-1)
		{
				printf("no block left to assign\n");
				goto fail;
		}
		triple_indirect_buffer_grand_parent=xzalloc (blocksize);
		if(!triple_indirect_buffer_grand_parent)
			goto fail;

		triple_indirect_buffer_grand_parent_startaddress=triple_indirect_buffer_grand_parent;
		total_no_of_block_for_file++;
#ifdef DEBUG_LOG
		printf("TIGPB %d: %d\n",triple_indirect_blockno_grand_parent,total_remaining_blocks);
#endif

		/* for each 4 byte grand parent entry create one more block*/
		for(i=0;i<(blocksize/sizeof(unsigned int));i++)
		{
			triple_indirect_blockno_parent=ext4fs_get_next_available_block_no(dev_desc,part_no);
			if(triple_indirect_blockno_parent==-1)
			{
				printf("no block left to assign\n");
				goto fail;
			}
			triple_indirect_buffer_parent=xzalloc (blocksize);
			if(!triple_indirect_buffer_parent)
				goto fail;

			triple_indirect_buffer_parent_startaddress=triple_indirect_buffer_parent;
			*triple_indirect_buffer_grand_parent=triple_indirect_blockno_parent;
			triple_indirect_buffer_grand_parent++;
			total_no_of_block_for_file++;
#ifdef DEBUG_LOG
			printf("TIPB %d: %d\n",triple_indirect_blockno_parent,total_remaining_blocks);
#endif
			/*for each 4 byte entry parent  create one more block*/
				for(j=0;j<(blocksize/sizeof(unsigned int));j++)
				{
					triple_indirect_blockno_child=ext4fs_get_next_available_block_no(dev_desc,part_no);
					if(triple_indirect_blockno_child==-1)
					{
						printf("no block left to assign\n");
						goto fail;
					}
					triple_indirect_buffer_child=xzalloc (blocksize);
					if(!triple_indirect_buffer_child)
						goto fail;

					triple_indirect_buffer_child_startaddress=triple_indirect_buffer_child;
					*triple_indirect_buffer_parent=triple_indirect_blockno_child;
					triple_indirect_buffer_parent++;
					total_no_of_block_for_file++;
#ifdef DEBUG_LOG
			printf("TICB %d: %d\n",triple_indirect_blockno_parent,total_remaining_blocks);
#endif
						/*filling of actual datablocks for each child*/
						for(k=0;k<(blocksize/sizeof(unsigned int));k++)
						{
							actual_block_no=ext4fs_get_next_available_block_no(dev_desc,part_no);
							if(actual_block_no==-1)
							{
								printf("no block left to assign\n");
								goto fail;
							}
							*triple_indirect_buffer_child=actual_block_no;
#ifdef DEBUG_LOG
				printf("TIAB %d: %d\n",actual_block_no,total_remaining_blocks);
#endif
							triple_indirect_buffer_child++;
							total_remaining_blocks--;
							if(total_remaining_blocks==0)
								break;
						}
						/*write the child block */
						put_ext4(dev_desc,((uint64_t)(triple_indirect_blockno_child * blocksize)),
								triple_indirect_buffer_child_startaddress, blocksize);
						if(triple_indirect_buffer_child_startaddress)
							free(triple_indirect_buffer_child_startaddress);

						if(total_remaining_blocks==0)
							break;
				}
				/*write the parent block */
				put_ext4(dev_desc,((uint64_t)(triple_indirect_blockno_parent * blocksize)),
				triple_indirect_buffer_parent_startaddress, blocksize);
				if(triple_indirect_buffer_parent_startaddress)
					free(triple_indirect_buffer_parent_startaddress);

			if(total_remaining_blocks==0)
				break;
		}
		/*write the grand parent block */
		put_ext4(dev_desc,((uint64_t)(triple_indirect_blockno_grand_parent * blocksize)),
					triple_indirect_buffer_grand_parent_startaddress, blocksize);
		if(triple_indirect_buffer_grand_parent_startaddress)
			free(triple_indirect_buffer_grand_parent_startaddress);
		file_inode->b.blocks.tripple_indir_block=triple_indirect_blockno_grand_parent;
	}
	/*----------------------------END:Allocation of Blocks to Inode---------------------------- */

	/*
	*write the inode to inode table after last filled inode in inode table
	*we are using hardcoded  gd[0].inode_table_id becuase 0th blockgroup
	*is suffcient to create more than 1000 file.TODO exapnd the logic to
	*all blockgroup
	*/
	file_inode->blockcnt= (total_no_of_block_for_file*blocksize)/SECTOR_SIZE;

	temp_ptr = (char *)xzalloc(blocksize);
	if(!temp_ptr)
		goto fail;
	inode_bitmap_index = inodeno/(uint32_t)ext4fs_root->sblock.inodes_per_group;
	inodeno--;
	itable_blkno = __le32_to_cpu (gd[inode_bitmap_index].inode_table_id) +
		(inodeno % __le32_to_cpu (sblock->inodes_per_group))
		/ inodes_per_block;
	blkoff = (inodeno % inodes_per_block) * inodesize;
	ext2fs_devread (itable_blkno*sector_per_block, 0, blocksize,temp_ptr);
	log_journal(temp_ptr,itable_blkno);

	memcpy(temp_ptr + blkoff,inode_buffer,inodesize);
	put_metadata(temp_ptr,itable_blkno);

	/*Copy the file content into data blocks*/
	ext4fs_write_file(dev_desc,file_inode,0,sizebytes,buffer);
#ifdef DEBUG_LOG
	//ext4fs_dump_the_inode_group_after_writing();
	ext4fs_dump_the_block_group_after_writing();
#endif
	inode_bitmap_index = parent_inodeno/(uint32_t)ext4fs_root->sblock.inodes_per_group;
	parent_inodeno--;
	parent_itable_blkno = __le32_to_cpu (gd[inode_bitmap_index].inode_table_id) +
		(parent_inodeno % __le32_to_cpu (sblock->inodes_per_group))/ inodes_per_block;
	blkoff = (parent_inodeno % inodes_per_block) * inodesize;
	if(parent_itable_blkno != itable_blkno) {
	  	memset(temp_ptr,'\0',blocksize);
		ext2fs_devread (parent_itable_blkno*sector_per_block, 0, blocksize,temp_ptr);
		log_journal(temp_ptr,parent_itable_blkno);

		memcpy(temp_ptr + blkoff,g_parent_inode,sizeof(struct ext2_inode));
		put_metadata(temp_ptr,parent_itable_blkno);
		free(temp_ptr);
	}
	else {
		/* If parent and child fall in same inode table block both should be kept in 1 buffer*/
		memcpy(temp_ptr + blkoff,g_parent_inode,sizeof(struct ext2_inode));
		gd_index--;
		put_metadata(temp_ptr,itable_blkno);
		free(temp_ptr);
	}
	ext4fs_update(dev_desc);

#ifdef DEBUG_LOG
	/*print the block no allocated to a file*/
	unsigned int blknr;
	total_no_of_block_for_file=lldiv(total_no_of_bytes_for_file, blocksize);
	if(do_div(total_no_of_bytes_for_file, blocksize) !=0)
	{
		total_no_of_block_for_file++;
	}
	printf("Total no of blocks for File: %u\n",total_no_of_block_for_file);
	for(i=0;i<total_no_of_block_for_file;i++)
	{
		blknr = ext4fs_read_allocated_block (file_inode, i);
		printf("Actual Data Blocks Allocated to File: %u\n",blknr);
	}
#endif
	ext4fs_deinit(dev_desc);
	first_execution=0;
	blockno=0;

	first_execution_inode=0;
	inode_no=0;
	if(inode_buffer)
		free(inode_buffer);
	if(g_parent_inode){
		free(g_parent_inode);
		g_parent_inode=NULL;
	}

return 0;

fail:
	ext4fs_deinit(dev_desc);
	if(inode_buffer)
		free(inode_buffer);
	if(zero_buffer)
		free(zero_buffer);
	if(g_parent_inode){
		free(g_parent_inode);
		g_parent_inode=NULL;
	}
	return -1;
}

static int search_dir(block_dev_desc_t *dev_desc,struct ext2_inode *parent_inode,unsigned char *dirname)
{
	unsigned char *block_buffer;
	int blocksize;
	int sector_per_block;
	struct ext2_dirent *dir;
	struct ext2_dirent *previous_dir = NULL;
	int totalbytes = 0;
	int templength = 0;
	int status;
	int found = 0;
	unsigned char *ptr;
	int inodeno;
	int direct_blk_idx;
	unsigned int blknr;
	unsigned int previous_blknr;

	/*get the blocksize of the filesystem*/
	blocksize=EXT2_BLOCK_SIZE(ext4fs_root);

	/*calucalate sector per block*/
	sector_per_block=blocksize/SECTOR_SIZE;

	/*get the first block of root*/
	/*read the block no allocated to a file*/
	for(direct_blk_idx=0;direct_blk_idx<INDIRECT_BLOCKS;direct_blk_idx++)
	{
		blknr=ext4fs_read_allocated_block(parent_inode, direct_blk_idx);
		if(blknr==0)
		{
			goto fail;
		}

		/*read the blocks of parenet inode*/
		block_buffer=xzalloc (blocksize);
		if(!block_buffer)
			goto fail;

		status = ext2fs_devread (blknr*sector_per_block, 0, blocksize,(char *)block_buffer);
		if (status == 0) 
		{
			goto fail;
		}
		else
		{
			dir=(struct ext2_dirent *) block_buffer;
			ptr=(unsigned char*)dir;
			totalbytes=0;
			while(dir->direntlen>=0)
			{
			/*blocksize-totalbytes because last directory length i.e.,
			*dir->direntlen is free availble space in the block that means
			*it is a last entry of directory entry
			*/
#if DEBUG_LOG
			printf("---------------------------\n");
			printf("dirname is %s\n",dirname);
			printf("current inode no is %d\n",dir->inode);
			printf("current directory entry length is %d\n",dir->direntlen);
			printf("current name length is %d\n",dir->namelen);
			printf("current fileltype is %d\n",dir->filetype);
			printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
			printf("-------------------------------------\n");
#endif

			if(strlen(dirname)==dir->namelen)
			{
				if(strncmp(dirname,ptr+sizeof(struct ext2_dirent),dir->namelen)==0)
				{
					previous_dir->direntlen+=dir->direntlen;
					inodeno=dir->inode;
					dir->inode=0;
					found=1;
					break;
				}
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

	if(found==1) {
		if(block_buffer) {
			free(block_buffer);
			block_buffer = NULL;
		}
		return inodeno;
	}

		if(block_buffer){
			free(block_buffer);
			block_buffer = NULL;
		}
	}

fail:
	if(block_buffer){
		free(block_buffer);
		block_buffer = NULL;
	}
	return -1;
}

static int find_dir_depth(char *dirname)
{
	char *token = strtok(dirname, "/");
	int count=0;
	while(token != NULL) {
		token = strtok(NULL,"/");
		count++;
	}
	return count+1 +1;
	/*
	*for example  for string /home/temp
	*depth=home(1)+temp(1)+1 extra for NULL;
	*so count is 4;
	*/
}

static int	parse_path(char **arr,char *dirname)
{
	char *token = strtok(dirname, "/");
	int i=0;

	/*add root*/
	arr[i] = xzalloc(strlen("/")+1);
	if(!arr[i])
		return -1;

	arr[i++] = "/";

	/*add each path entry after root*/
	while(token != NULL) {
		arr[i] = xzalloc(strlen(token)+1);
		if(!arr[i])
			return -1;
		memcpy(arr[i++],token,strlen(token));
		token = strtok(NULL,"/");
	}

	arr[i]=NULL;
	/*success*/
	return 0;
}

int  iget(unsigned int inode_no,struct ext2_inode* inode)
{
	if( ext4fs_read_inode (ext4fs_root,inode_no,inode)==0)
		return -1;
	else
		return 0;
}

/*
* Function:get_parent_inode_num
* Return Value: inode Number of the parent directory of  file/Directory to be created
* dev_desc : Input parameter, device descriptor
* dirname : Input parmater, input path name of the file/directory to be created
* dname : Output parameter, to be filled with the name of the directory extracted from dirname
*/
int get_parent_inode_num(block_dev_desc_t *dev_desc,char *dirname, char *dname)
{
	char **ptr = NULL;
	int i,depth = 0;
	char *depth_dirname = NULL;
	char *parse_dirname = NULL;
	struct ext2_inode *parent_inode = NULL;
	struct ext2_inode *first_inode = NULL;
	unsigned int matched_inode_no;
	unsigned int result_inode_no = -1;

	if(*dirname!='/')	{
		printf("Please supply Absolute path\n");
		return -1;
	}

	/*TODO: input validation make equivalent to linux*/
	/*get the memory size*/
	depth_dirname = (char *)xzalloc(strlen(dirname)+1);
	if(!depth_dirname)
		return -1;

	memcpy(depth_dirname,dirname,strlen(dirname));
	depth=find_dir_depth(depth_dirname);
	parse_dirname = (char *)xzalloc(strlen(dirname)+1);
	if(!parse_dirname)
		goto fail;
	memcpy(parse_dirname,dirname,strlen(dirname));

	/*allocate memory for each directory level*/
	ptr=(char**)xzalloc((depth)* sizeof(char *));
	if(!ptr)
		goto fail;
	parse_path(ptr,parse_dirname);
	parent_inode=(struct ext2_inode*)xzalloc(sizeof(struct ext2_inode));
	if(!parent_inode)
		goto fail;
	first_inode=(struct ext2_inode*)xzalloc(sizeof(struct ext2_inode));
	if(!first_inode)
		goto fail;
	memcpy(parent_inode,ext4fs_root->inode,sizeof(struct ext2_inode));
	memcpy(first_inode, parent_inode, sizeof(struct ext2_inode));

	for(i=1;i<depth;i++) {
		matched_inode_no=search_dir(dev_desc,parent_inode,ptr[i]);
		if(matched_inode_no == -1) {
			if(ptr[i+1] == NULL && i==1) {
				result_inode_no = EXT2_ROOT_INO;
				goto END;
			}
			else {
				if(ptr[i+1] == NULL)
					break;
				printf("Invalid path :(mkdir -p) Not Supported\n");
				result_inode_no = -1;
				goto fail;
			}
		}
		else {
			if(ptr[i+1] != NULL){
			memset(parent_inode,'\0',sizeof(struct ext2_inode));
			if(iget(matched_inode_no,parent_inode)) {
				result_inode_no = -1;
				goto fail;
			}
			result_inode_no=matched_inode_no;
			}
			else
				break;
		}
	}

END:
	if(i==1)
		matched_inode_no=search_dir(dev_desc,first_inode,ptr[i]);
	else
		matched_inode_no=search_dir(dev_desc,parent_inode,ptr[i]);

	if(matched_inode_no != -1) {
		printf("file/Directory already present\n");
		return -1;
	}

	if(strlen(ptr[i])> 256) {
		result_inode_no = -1;
		goto fail;
	}
	memcpy(dname, ptr[i],strlen(ptr[i]));

fail:
	if(depth_dirname)
		free(depth_dirname);
	if(parse_dirname)
		free(parse_dirname);
	if(ptr)
		free(ptr);
	if(parent_inode)
		free(parent_inode);
	if(first_inode)
		free(first_inode);

	return result_inode_no;
}

static int create_dir(block_dev_desc_t *dev_desc, unsigned int parent_inode_num,
					struct ext2_inode *parent_inode,char *dirname)
{
	unsigned int direct_blk_idx;
	unsigned int parent_blknr;
	unsigned int previous_blknr = -1;
	int partno = 1;
	unsigned int inodeno;
	unsigned int working_block_no_of_parent = 0;
	unsigned char *working_block_buffer_for_parent = NULL;
	unsigned int inode_bitmap_index;
	unsigned char *buf = NULL;
	short create_dir_status = 0;

	/*directory entry*/
	struct ext2_dirent *dir = NULL;
	struct ext4_dir *dir_entry = NULL;

	int templength = 0;
	int totalbytes = 0;
	short int padding_factor = 0;
	unsigned int new_entry_byte_reqd;
	unsigned int last_entry_dirlen;
	int status;

	unsigned char *ptr = NULL;
	struct ext2_dirent *temp_dir = NULL;

	char * temp_ptr = NULL;
	unsigned int parent_itable_blkno,itable_blkno,blkoff;
	struct ext2_sblock *sblock = &(ext4fs_root->sblock);
	unsigned int inodes_per_block = blocksize / inodesize;

	/*inode*/
	struct ext2_inode *file_inode = NULL;
	unsigned char *inode_buffer = NULL;
	unsigned char *zero_buffer = (unsigned char *)xzalloc(blocksize);
	if(!zero_buffer)
		return -1;
	working_block_buffer_for_parent = (unsigned char *)xzalloc(blocksize);

RESTART:
	/*read the block no allocated to a file*/
	for(direct_blk_idx=0;direct_blk_idx<INDIRECT_BLOCKS;direct_blk_idx++) {
		parent_blknr=ext4fs_read_allocated_block(parent_inode, direct_blk_idx);
		if(parent_blknr==0) {
			working_block_no_of_parent=previous_blknr;
			break;
		}
		previous_blknr=parent_blknr;
	}

#ifdef DEBUG_LOG
		printf("previous blk no =%u \n",previous_blknr);
		printf("first_block_no_of_root =%u \n",working_block_no_of_parent);
#endif

		status = ext2fs_devread (working_block_no_of_parent*sector_per_block, 0, blocksize,
					(char *)working_block_buffer_for_parent);
		if (status == 0) {
			goto fail;
		}
		else {
			log_journal(working_block_buffer_for_parent,working_block_no_of_parent);
			dir=(struct ext2_dirent *) working_block_buffer_for_parent;
			ptr=(unsigned char*)dir;
			totalbytes=0;
			while(dir->direntlen>0) {
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
				*it is a last entry of directory entry
				*/

				/*traversing the each directory entry*/
				if(blocksize-totalbytes==dir->direntlen)
				{
					if(strlen(dirname)%4!=0)
					{
						padding_factor=4-(strlen(dirname)%4);
					}
					new_entry_byte_reqd = strlen(dirname) + sizeof(struct ext2_dirent)+padding_factor;
					padding_factor = 0;
					/*update last directory entry length to its length because
					*we are creating new directory entry*/
					if(dir->namelen%4!=0)
					{
						padding_factor=4-(dir->namelen%4);/*for null storage*/
					}
					last_entry_dirlen= dir->namelen+sizeof(struct ext2_dirent)+padding_factor;
					if((blocksize-totalbytes -last_entry_dirlen) < new_entry_byte_reqd)
					{
#ifdef DEBUG_LOG
						printf("Storage capcity of 1st Block over allocating new block\n");
#endif
						if (direct_blk_idx==INDIRECT_BLOCKS-1)
						{
							printf("Directory capacity exceeds the limit\n");
							goto fail;
						}
						parent_inode->b.blocks.dir_blocks[direct_blk_idx]= 
										ext4fs_get_next_available_block_no(dev_desc,partno);
						if(parent_inode->b.blocks.dir_blocks[direct_blk_idx] == -1)
						{
							printf("no block left to assign\n");
							goto fail;
						}
						put_ext4(dev_desc,((uint64_t)(parent_inode->b.blocks.dir_blocks[direct_blk_idx] * blocksize)),
										zero_buffer, blocksize);
						parent_inode->size = parent_inode->size + blocksize;
						parent_inode->blockcnt= parent_inode->blockcnt + sector_per_block;
						put_metadata(working_block_buffer_for_parent,working_block_no_of_parent);
						goto RESTART;
					}
					dir->direntlen=last_entry_dirlen;
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
			printf("current inode no is %u\n",dir->inode);
			printf("current directory entry length is %d\n",dir->direntlen);
			printf("current name length is %d\n",dir->namelen);
			printf("current fileltype is %d\n",dir->filetype);
			printf("total bytes is %d\n",totalbytes);
			printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
			printf("-------------------------------------\n");
#endif
		/*get the next available inode number*/
		inodeno=ext4fs_get_next_available_inode_no(dev_desc,partno);
		if(inodeno==-1)
		{
			printf("no inode left to assign\n");
			goto fail;
		}
#ifdef DEBUG_LOG
			printf("New Inode Assignment :%d\n",inodeno);
#endif
			dir->inode=inodeno;
			dir->direntlen=blocksize-totalbytes;
			dir->namelen=strlen(dirname);
			dir->filetype=FILETYPE_DIRECTORY;
			temp_dir=dir;
			dir=(unsigned char*)dir;
			dir=(unsigned char*)dir+sizeof(struct ext2_dirent);
			memcpy(dir,dirname,strlen(dirname));
#if DEBUG_LOG
			dir=(struct ext2_dirent*)temp_dir;

			printf("------------------------------------\n");
			printf("JUST AFTER ADDING NAME\n");
			printf("------------------------------------\n");
			printf("current inode no is %u\n",dir->inode);
			printf("current directory entry length is %d\n",dir->direntlen);
			printf("current name length is %d\n",dir->namelen);
			printf("current fileltype is %d\n",dir->filetype);
			printf("total bytes is %d\n",totalbytes);
			printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
			printf("-------------------------------------\n");
#endif
		}
		put_metadata(working_block_buffer_for_parent,working_block_no_of_parent);

		/*Increase the Link count and write the parent inode
		 *journal backup of parent inode block */
		temp_ptr = (char *)xzalloc(blocksize);
		if(!temp_ptr)
			goto fail;
		inode_bitmap_index=parent_inode_num/(uint32_t)ext4fs_root->sblock.inodes_per_group;
		parent_inode_num--;
		parent_itable_blkno = __le32_to_cpu (gd[inode_bitmap_index].inode_table_id) +
			(parent_inode_num % __le32_to_cpu (sblock->inodes_per_group))/ inodes_per_block;
		blkoff = (parent_inode_num % inodes_per_block) * inodesize;
		ext2fs_devread (parent_itable_blkno*sector_per_block, 0, blocksize,temp_ptr);

		log_journal(temp_ptr,parent_itable_blkno);
		parent_inode_num++;
		parent_inode->nlinks++;
		memcpy(temp_ptr + blkoff,parent_inode,sizeof(struct ext2_inode));
		put_metadata(temp_ptr,parent_itable_blkno);

		/* Update the group descriptor directory count*/
		inode_bitmap_index=inodeno/(uint32_t)ext4fs_root->sblock.inodes_per_group;
		gd[inode_bitmap_index].used_dir_cnt++;

		/*prepare the inode for the directory to be created*/
		inode_buffer = xmalloc(inodesize);
		memset(inode_buffer, '\0', inodesize);
		file_inode = (struct ext2_inode *)inode_buffer;
		file_inode->mode= S_IFDIR | S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH;
		/* ToDo: Update correct time*/
		file_inode->mtime= NULL;
		file_inode->atime= NULL;
		file_inode->ctime= NULL;
		file_inode->nlinks= 2;
		file_inode->size = blocksize;
		file_inode->blockcnt= blocksize/SECTOR_SIZE;
		file_inode->b.blocks.dir_blocks[0] = ext4fs_get_next_available_block_no(dev_desc,partno);

		inodeno--;
		itable_blkno = __le32_to_cpu (gd[inode_bitmap_index].inode_table_id) +
		(inodeno % __le32_to_cpu (sblock->inodes_per_group))/ inodes_per_block;
		blkoff = (inodeno % inodes_per_block) * inodesize;
		if(parent_itable_blkno != itable_blkno) {
			memset(temp_ptr,'\0',blocksize);
			ext2fs_devread (itable_blkno*sector_per_block, 0, blocksize,temp_ptr);
			log_journal(temp_ptr,itable_blkno);
			memcpy(temp_ptr + blkoff,inode_buffer,inodesize);
			put_metadata(temp_ptr,itable_blkno);
			free(temp_ptr);
		}
		else {
			/* If parent and child fall in same inode table block both should be kept in 1 buffer*/
			memcpy(temp_ptr + blkoff,inode_buffer,inodesize);
			gd_index--;
			put_metadata(temp_ptr,itable_blkno);
			free(temp_ptr);
		}
		inodeno++;

		/*create . and ../ for  new directory*/
		buf = xzalloc(blocksize);
		memset(buf, 0, blocksize);
		dir_entry = (struct ext4_dir *)buf;
		dir_entry->inode1 = inodeno;
		dir_entry->rec_len1 = 12;
		dir_entry->name_len1 = 1;
		dir_entry->file_type1 = EXT2_FT_DIR;
		dir_entry->name1[0] = '.';
		dir_entry->inode2 = parent_inode_num;
		dir_entry->rec_len2 = (blocksize - 12);
		dir_entry->name_len2 = 2;
		dir_entry->file_type2 = EXT2_FT_DIR;
		dir_entry->name2[0] = '.'; dir_entry->name2[1] = '.';
		put_ext4(dev_desc,(uint64_t)(file_inode->b.blocks.dir_blocks[0] * blocksize), buf, blocksize);
		create_dir_status= SUCCESS;

fail:
	if(working_block_buffer_for_parent){
		free(working_block_buffer_for_parent);
		working_block_buffer_for_parent=NULL;
	}
	if(buf){
		free(buf);
		buf=NULL;
	}
	if(zero_buffer){
		free(zero_buffer);
		zero_buffer=NULL;
	}
	if(create_dir_status)
		return 0;
	else
		return -1;
}

int ext4fs_create_dir (block_dev_desc_t *dev_desc, char *dirname)
{
	unsigned int inodeno;
	int retval = 0;
	struct ext2_inode parent_inode;
	char dname[256] = {0,};

	/*get the blocksize inode sector per block of the filesystem*/
	if(ext4fs_init(dev_desc)!=0) {
		printf("error in File System init\n");
		return -1;
	}
	inodeno= get_parent_inode_num(dev_desc,dirname,dname);
	if(inodeno==-1) {
		retval = -1;
		goto fail;
	}
	if(iget(inodeno,&parent_inode)){
		retval = -1;
		goto fail;
	}
	retval = create_dir(dev_desc,inodeno,&parent_inode,dname);
	if(retval){
		printf("Failed to create directory\n");
		goto fail;
	}
	ext4fs_update(dev_desc);
	printf("Done !!!\n");
fail:
	ext4fs_deinit(dev_desc);
	return  retval;
}

static int create_symlink(block_dev_desc_t *dev_desc,int link_type,unsigned int src_inodeno,
		unsigned int parent_inode_num,struct ext2_inode *parent_inode,char *src_path, char *linkname)
{
	unsigned int direct_blk_idx;
	unsigned int parent_blknr;
	unsigned int previous_blknr = -1;
	int partno = 1;
	unsigned int inodeno = 0;
	unsigned int working_block_no_of_parent = 0;
	unsigned char *working_block_buffer_for_parent = NULL;
	unsigned int inode_bitmap_index;
	unsigned char *buf = NULL;
	short create_dir_status = 1;

	/*directory entry*/
	struct ext2_dirent *dir = NULL;
	struct ext4_dir *dir_entry = NULL;
	struct ext2_dirent *temp_dir = NULL;
	int templength = 0;
	int totalbytes = 0;
	short int padding_factor = 0;
	unsigned int new_entry_byte_reqd;
	unsigned int last_entry_dirlen;
	int status;
	unsigned char *ptr;
	char * temp_ptr = (char *)xzalloc(blocksize);
	unsigned int temp_blkno, blkoff;
	struct ext2_sblock *sblock = &(ext4fs_root->sblock);
	unsigned int inodes_per_block = blocksize / inodesize;

	/*inode*/
	struct ext2_inode *file_inode = NULL;
	unsigned char *inode_buffer = NULL;
	unsigned int *zero_buffer = (unsigned char *)xzalloc(blocksize);
	if(!zero_buffer)
		return -1;
	working_block_buffer_for_parent = (unsigned char *)xzalloc(blocksize);

RESTART:
	/*read the block no allocated to a file*/
	for(direct_blk_idx=0;direct_blk_idx<INDIRECT_BLOCKS;direct_blk_idx++){
		parent_blknr=ext4fs_read_allocated_block(parent_inode, direct_blk_idx);
		if(parent_blknr==0) {
			working_block_no_of_parent=previous_blknr;
			break;
		}
		previous_blknr=parent_blknr;
	}

#ifdef DEBUG_LOG
		printf("previous blk no =%u \n",previous_blknr);
		printf("first_block_no_of_root =%u \n",working_block_no_of_parent);
#endif
		status = ext2fs_devread (working_block_no_of_parent*sector_per_block, 0, blocksize,
					(char *)working_block_buffer_for_parent);
		if (status == 0){
			goto fail;
		}
		else {
			log_journal(working_block_buffer_for_parent,working_block_no_of_parent);
			dir=(struct ext2_dirent *) working_block_buffer_for_parent;
			ptr=(unsigned char*)dir;
			totalbytes=0;
			while(dir->direntlen>0){
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
				*it is a last entry of directory entry
				*/

				/*traversing the each directory entry*/

				if(blocksize-totalbytes==dir->direntlen)
				{
					if(strlen(linkname)%4!=0)
					{
						padding_factor=4-(strlen(linkname)%4);
					}
					new_entry_byte_reqd = strlen(linkname) + sizeof(struct ext2_dirent)+padding_factor;
					padding_factor = 0;
					/*update last directory entry length to its length because
					*we are creating new directory entry*/
					if(dir->namelen%4!=0)
					{
						padding_factor=4-(dir->namelen%4);/*for null storage*/
					}
					last_entry_dirlen= dir->namelen+sizeof(struct ext2_dirent)+padding_factor;
					if((blocksize-totalbytes -last_entry_dirlen) < new_entry_byte_reqd)
					{
#ifdef DEBUG_LOG
						printf("Storage capacity of 1st Block over allocating new block\n");
#endif
						if (direct_blk_idx==INDIRECT_BLOCKS-1)
						{
							printf("Directory capacity exceeds the limit\n");
							goto fail;
						}
						parent_inode->b.blocks.dir_blocks[direct_blk_idx]= 
										ext4fs_get_next_available_block_no(dev_desc,partno);
						if(parent_inode->b.blocks.dir_blocks[direct_blk_idx] == -1)
						{
							printf("no block left to assign\n");
							goto fail;
						}
						put_ext4(dev_desc,((uint64_t)(parent_inode->b.blocks.dir_blocks[direct_blk_idx] * blocksize)),
										zero_buffer, blocksize);
						parent_inode->size = parent_inode->size + blocksize;
						parent_inode->blockcnt= parent_inode->blockcnt + sector_per_block;
						put_metadata(working_block_buffer_for_parent,working_block_no_of_parent);
						goto RESTART;
					}
					dir->direntlen=last_entry_dirlen;
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
			printf("current inode no is %u\n",dir->inode);
			printf("current directory entry length is %d\n",dir->direntlen);
			printf("current name length is %d\n",dir->namelen);
			printf("current fileltype is %d\n",dir->filetype);
			printf("total bytes is %d\n",totalbytes);
			printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
			printf("-------------------------------------\n");
#endif
		if(link_type == SOFT_LINK){
		/*get the next available inode number*/
		inodeno=ext4fs_get_next_available_inode_no(dev_desc,partno);
		if(inodeno==-1)
		{
			printf("no inode left to assign\n");
			goto fail;
		}
#ifdef DEBUG_LOG
			printf("New Inode Assignment :%d\n",inodeno);
#endif
			dir->inode=inodeno;
			}
		else
			dir->inode=src_inodeno;

			dir->direntlen=blocksize-totalbytes;
			dir->namelen=strlen(linkname);
			if(link_type == SOFT_LINK)
			dir->filetype=FILETYPE_SYMLINK;
			else
				dir->filetype=FILETYPE_REG;
			temp_dir=dir;
			dir=(unsigned char*)dir;
			dir=(unsigned char*)dir+sizeof(struct ext2_dirent);
			memcpy(dir,linkname,strlen(linkname));
#if DEBUG_LOG
			dir=(struct ext2_dirent*)temp_dir;

			printf("------------------------------------\n");
			printf("JUST AFTER ADDING NAME\n");
			printf("------------------------------------\n");
			printf("current inode no is %u\n",dir->inode);
			printf("current directory entry length is %d\n",dir->direntlen);
			printf("current name length is %d\n",dir->namelen);
			printf("current fileltype is %d\n",dir->filetype);
			printf("total bytes is %d\n",totalbytes);
			printf("dir->name is %s\n",ptr+sizeof(struct ext2_dirent));
			printf("-------------------------------------\n");
#endif
		}
		/*update  the directory entry block of parent inode for new directory*/
		put_metadata(working_block_buffer_for_parent,working_block_no_of_parent);

		if(link_type == SOFT_LINK){
			/*prepare the inode for the symlink to be created*/
			inode_buffer = xmalloc(inodesize);
			memset(inode_buffer, '\0', inodesize);
			file_inode = (struct ext2_inode *)inode_buffer;
			file_inode->mode= S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;
			/* ToDo: Update correct time*/
			file_inode->mtime= NULL;
			file_inode->atime= NULL;
			file_inode->ctime= NULL;
			file_inode->nlinks= 1;
			file_inode->size = strlen(src_path);
			file_inode->blockcnt= 0;
			memcpy(file_inode->b.symlink,src_path, strlen(src_path));

			inode_bitmap_index=inodeno/(uint32_t)ext4fs_root->sblock.inodes_per_group;
			inodeno--;
			temp_blkno = __le32_to_cpu (gd[inode_bitmap_index].inode_table_id) +
				(inodeno % __le32_to_cpu (sblock->inodes_per_group))/ inodes_per_block;
			blkoff = (inodeno % inodes_per_block) * inodesize;
			ext2fs_devread (temp_blkno*sector_per_block, 0, blocksize,temp_ptr);
			log_journal(temp_ptr,temp_blkno);

			memcpy(temp_ptr + blkoff,inode_buffer,inodesize);
			put_metadata(temp_ptr,temp_blkno);
			free(temp_ptr);
		}

fail:
	if(working_block_buffer_for_parent){
		free(working_block_buffer_for_parent);
		working_block_buffer_for_parent=NULL;
	}
	if(buf){
		free(buf);
		buf=NULL;
	}
	if(zero_buffer){
		free(zero_buffer);
		zero_buffer=NULL;
	}
	if(create_dir_status)
		return 0;
	else
		return -1;
}

/*
* Function:get_parent_inode_num_write
* Return Value: inode Number of the parent directory of  file/Directory to be created
* dev_desc : Input parameter, device descriptor
* dirname : Input parmater, input path name of the file/directory to be created
* dname : Output parameter, to be filled with the name of the directory extracted from dirname
*/
int get_parent_inode_num_write(block_dev_desc_t *dev_desc,char *dirname, char *dname)
{
	char **ptr = NULL;
	int i,depth = 0;
	char *depth_dirname = NULL;
	char *parse_dirname = NULL;
	struct ext2_inode *parent_inode = NULL;
	struct ext2_inode *first_inode = NULL;
	unsigned int matched_inode_no;
	unsigned int result_inode_no;
	if(*dirname!='/') {
		printf("Please supply Absolute path\n");
		return -1;
	}

	/*TODO: input validation make equivalent to linux*/

	/*get the memory size*/
	depth_dirname = (char *)xzalloc(strlen(dirname)+1);
	memcpy(depth_dirname,dirname,strlen(dirname));
	depth=find_dir_depth(depth_dirname);
	parse_dirname = (char *)xzalloc(strlen(dirname)+1);
	memcpy(parse_dirname,dirname,strlen(dirname));

	/*allocate memory for each directory level*/
	ptr=(char**)xzalloc((depth)* sizeof(char *));
	parse_path(ptr,parse_dirname);

	parent_inode=(struct ext2_inode*)xzalloc(sizeof(struct ext2_inode));
	first_inode=(struct ext2_inode*)xzalloc(sizeof(struct ext2_inode));
	memcpy(parent_inode,ext4fs_root->inode,sizeof(struct ext2_inode));
	memcpy(first_inode, parent_inode, sizeof(struct ext2_inode));

	result_inode_no = EXT2_ROOT_INO;
	for(i=1;i<depth;i++) {
		matched_inode_no=search_dir(dev_desc,parent_inode,ptr[i]);
		if(matched_inode_no == -1) {
			if(ptr[i+1] == NULL && i==1) {
				result_inode_no = EXT2_ROOT_INO;
				goto END;
			}
			else {
				if(ptr[i+1] == NULL)
				break;
				printf("Invalid path \n");
				return -1;
			}
		}
		else {
			if(ptr[i+1] != NULL){
			memset(parent_inode,'\0',sizeof(struct ext2_inode));
			if(iget(matched_inode_no,parent_inode))
				return -1;

			result_inode_no=matched_inode_no;
			}
			else
				break;
		}
	}

END:
	memcpy(dname, ptr[i],strlen(ptr[i])); 

	if(depth_dirname)
		free(depth_dirname);
	if(parse_dirname)
		free(parse_dirname);
	if(ptr)
		free(ptr);
	if(parent_inode)
		free(parent_inode);
	if(first_inode)
		free(first_inode);

	return result_inode_no;
}

int namei(block_dev_desc_t *dev_desc,char *dirname,char *dname)
{
	char **ptr = NULL;
	int i,depth = 0;
	char *depth_dirname = NULL;
	char *parse_dirname = NULL;
	struct ext2_inode *parent_inode = NULL;
	struct ext2_inode *first_inode = NULL;
	unsigned int matched_inode_no;
	unsigned int result_inode_no;

	if(*dirname!='/') {
		printf("Please supply Absolute path\n");
		return -1;
	}

	/*TODO: input validation make equivalent to linux*/

	/*get the memory size*/
	depth_dirname = (char *)xzalloc(strlen(dirname)+1);
	memcpy(depth_dirname,dirname,strlen(dirname));
	depth=find_dir_depth(depth_dirname);
	parse_dirname = (char *)xzalloc(strlen(dirname)+1);
	memcpy(parse_dirname,dirname,strlen(dirname));

	/*allocate memory for each directory level*/
	ptr=(char**)xzalloc((depth)* sizeof(char *));
	parse_path(ptr,parse_dirname);

	parent_inode=(struct ext2_inode*)xzalloc(sizeof(struct ext2_inode));
	first_inode=(struct ext2_inode*)xzalloc(sizeof(struct ext2_inode));
	memcpy(parent_inode,ext4fs_root->inode,sizeof(struct ext2_inode));
	memcpy(first_inode, parent_inode, sizeof(struct ext2_inode));

	for(i=1;i<depth;i++) {
		matched_inode_no=search_dir(dev_desc,parent_inode,ptr[i]);
		if(matched_inode_no == -1) {
			if(ptr[i+1] == NULL && i==1) {
				result_inode_no = EXT2_ROOT_INO;
				goto END;
			}
			else {
				if(ptr[i+1] == NULL)
					break;
				printf("Invalid Source Path\n");
				return -1;
			}
		}
		else {
			if(ptr[i+1] != NULL){
			memset(parent_inode,'\0',sizeof(struct ext2_inode));
			if(iget(matched_inode_no,parent_inode))
				return -1;

			result_inode_no=matched_inode_no;
			}
			else
				break;
		}
	}

END:
	if(i==1)
		matched_inode_no=search_dir(dev_desc,first_inode,ptr[i]);
	else
		matched_inode_no=search_dir(dev_desc,parent_inode,ptr[i]);

	if(matched_inode_no != -1) {
		result_inode_no=matched_inode_no;
	}
	else
		result_inode_no = -1;
	memcpy(dname, ptr[i],strlen(ptr[i]));

	if(depth_dirname)
		free(depth_dirname);
	if(parse_dirname)
		free(parse_dirname);
	if(ptr)
		free(ptr);
	if(parent_inode)
		free(parent_inode);
	if(first_inode)
		free(first_inode);

	return result_inode_no;
}

int ext4fs_create_symlink (block_dev_desc_t *dev_desc, int link_type,char *src_path, char *target_path)
{
	unsigned int target_parent_inodeno;
	struct ext2_inode target_parent_inode;
	unsigned int src_inodeno = 0;
	struct ext2_inode src_inode;
	int inode_bitmap_index;
	char dname_src[256] = {0,};
	char dname_target[256] = {0,};
	int retval = 0;
	struct ext2_sblock *sblock = &(ext4fs_root->sblock);
	unsigned int inodes_per_block;
	unsigned int temp_blkno, blkoff;
	char *ptr = NULL;

	/*get the blocksize inode sector per block of the filesystem*/
	if(ext4fs_init(dev_desc)!=0)
	{
		printf("error in File System init\n");
		return -1;
	}
	inodes_per_block = blocksize / inodesize;
	target_parent_inodeno= get_parent_inode_num(dev_desc,target_path,dname_target);
	if(target_parent_inodeno==-1) {
		retval = -1;
		goto FAIL;
	}
	if(iget(target_parent_inodeno,&target_parent_inode)){
		retval = -1;
		goto FAIL;
	}
	if(link_type==HARD_LINK){
	src_inodeno = namei(dev_desc,src_path,dname_src);
	if(src_inodeno==-1) {
		printf("File Does Not exist\n");
		retval = -1;
		goto FAIL;
	}
	if(iget(src_inodeno,&src_inode)){
		retval = -1;
		goto FAIL;
	}
	ptr = (char *)xzalloc(blocksize);
	if(!ptr) {
		retval = -1;
		goto FAIL;
	}
	inode_bitmap_index=src_inodeno/(uint32_t)ext4fs_root->sblock.inodes_per_group;
	src_inodeno--;
	temp_blkno = __le32_to_cpu (gd[inode_bitmap_index].inode_table_id) +
		(src_inodeno % __le32_to_cpu (sblock->inodes_per_group))/ inodes_per_block;	
	ext2fs_devread (temp_blkno*sector_per_block, 0, blocksize,ptr);
	log_journal(ptr,temp_blkno);
	src_inodeno++;
	free(ptr);

	src_inode.nlinks++;
	if(src_inode.mode & S_IFDIR){
		printf("Hard Link not allowed for a directory \n");
		retval = -1;
		goto FAIL;
	}

	/*update the inode table entry for the new directory created*/
	/* Update the group descriptor directory count*/
	inode_bitmap_index=src_inodeno/(uint32_t)ext4fs_root->sblock.inodes_per_group;
	}

	retval = create_symlink(dev_desc,link_type,src_inodeno,target_parent_inodeno,&target_parent_inode,
								src_path,dname_target);
	if(retval){
		printf("Failed to create Link\n");
		retval = -1;
		goto FAIL;
	}
	else{
		if(link_type==HARD_LINK){
			ptr = (char *)xzalloc(blocksize);
			if(!ptr) {
				retval = -1;
				goto FAIL;
			}
			inode_bitmap_index=src_inodeno/(uint32_t)ext4fs_root->sblock.inodes_per_group;
			src_inodeno--;
			temp_blkno = __le32_to_cpu (gd[inode_bitmap_index].inode_table_id) +
			(src_inodeno % __le32_to_cpu (sblock->inodes_per_group))/ inodes_per_block;
			blkoff = (src_inodeno % inodes_per_block) * inodesize;
			ext2fs_devread (temp_blkno*sector_per_block, 0, blocksize,ptr);
			memcpy(ptr + blkoff,&src_inode,sizeof(struct ext2_inode));
			put_metadata(ptr,temp_blkno);
			free(ptr);
			}
	}
	ext4fs_update(dev_desc);
	printf("Done !!!\n");
FAIL:
	ext4fs_deinit(dev_desc);
	return  retval;
}

int ext4fs_ls (const char *dirname)
{
	ext2fs_node_t dirnode;
	int status;

	if (dirname== NULL)
		return (0);

	status = ext4fs_find_file (dirname, &ext4fs_root->diropen, &dirnode,
		FILETYPE_DIRECTORY);
	if (status != 1) {
		printf ("** Can not find directory. **\n");
		return (1);
	}

	ext4fs_iterate_dir (dirnode, NULL, NULL, NULL);
	ext4fs_free_node (dirnode, &ext4fs_root->diropen);
	return (0);
}

int ext4fs_open (const char *filename)
{
	ext2fs_node_t fdiro = NULL;
	int status;
	int len;

	if (ext4fs_root == NULL)
		return (-1);

	ext4fs_file = NULL;
	status = ext4fs_find_file (filename, &ext4fs_root->diropen, &fdiro,
		FILETYPE_REG);
	if (status == 0)
		goto fail;

	if (!fdiro->inode_read) {
		status = ext4fs_read_inode (fdiro->data, fdiro->ino,
			&fdiro->inode);
		if (status == 0)
			goto fail;
	}
	len = __le32_to_cpu (fdiro->inode.size);
	ext4fs_file = fdiro;
	return (len);

fail:
	ext4fs_free_node (fdiro, &ext4fs_root->diropen);
	return (-1);
}

int ext4fs_close (void)
{
	if ((ext4fs_file != NULL) && (ext4fs_root != NULL)) {
		ext4fs_free_node (ext4fs_file, &ext4fs_root->diropen);
		ext4fs_file = NULL;
	}
	if (ext4fs_root != NULL) {
		free (ext4fs_root);
		ext4fs_root = NULL;
	}
	if (ext4fs_indir1_block != NULL) {
		free (ext4fs_indir1_block);
		ext4fs_indir1_block = NULL;
		ext4fs_indir1_size = 0;
		ext4fs_indir1_blkno = -1;
	}
	if (ext4fs_indir2_block != NULL) {
		free (ext4fs_indir2_block);
		ext4fs_indir2_block = NULL;
		ext4fs_indir2_size = 0;
		ext4fs_indir2_blkno = -1;
	}
	return (0);
}

int ext4fs_read (char *buf, unsigned len)
{
	int status;

	if (ext4fs_root == NULL) {
		return (0);
	}

	if (ext4fs_file == NULL) {
		return (0);
	}

	status = ext4fs_read_file (ext4fs_file, 0, len, buf);
	return (status);
}

int ext4fs_mount (unsigned part_length)
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

	status = ext4fs_read_inode (data, 2, data->inode);
	if (status == 0)
		goto fail;

	ext4fs_root = data;

	return (1);

fail:
	printf("Failed to mount ext2 filesystem...\n");
	free (data);
	ext4fs_root = NULL;
	return (0);
}
