#include <common.h>
#include <ext_common.h>
#include <ext4fs.h>
#include <malloc.h>
#include <div64.h>
#include "ext2fs.h"
#include "ext4_common.h"
#include "ext4_journal.h"
#include "ext2fs_gd.h"
#include "iterate.h"

/*
 * Note we override the kernel include file's idea of what the default
 * check interval (never) should be.  It's a good idea to check at
 * least *occasionally*, specially since servers will never rarely get
 * to reboot, since Linux is so robust these days.  :-)
 *
 * 180 days (six months) seems like a good value.
 */
#ifdef EXT2_DFL_CHECKINTERVAL
#undef EXT2_DFL_CHECKINTERVAL
#endif
#define EXT2_DFL_CHECKINTERVAL (86400L * 180L)

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL && size != 0)
		printf("bb_msg_memory_exhausted\n");
	return ptr;
}

void *xzalloc(size_t size)
{
	void *ptr = xmalloc(size);
	memset(ptr, 0, size);
	return ptr;
}

static errcode_t check_magic(ext2fs_generic_bitmap bitmap)
{
	if (!bitmap || !((bitmap->magic == EXT2_ET_MAGIC_GENERIC_BITMAP) ||
			 (bitmap->magic == EXT2_ET_MAGIC_INODE_BITMAP) ||
			 (bitmap->magic == EXT2_ET_MAGIC_BLOCK_BITMAP)))
		return EXT2_ET_MAGIC_GENERIC_BITMAP;
	return 0;
}

/*
 * Mark the inode bitmap as dirty
 */
void ext2fs_mark_ib_dirty(ext2_filsys fs)
{
	fs->flags |= EXT2_FLAG_IB_DIRTY | EXT2_FLAG_CHANGED;
}

/*
 * Mark the block bitmap as dirty
 */
void ext2fs_mark_bb_dirty(ext2_filsys fs)
{
	fs->flags |= EXT2_FLAG_BB_DIRTY | EXT2_FLAG_CHANGED;
}

/*
 * This is an efficient, overflow safe way of calculating ceil((1.0 * a) / b)
 */
unsigned int ext2fs_div_ceil(unsigned int a, unsigned int b)
{
	if (!a)
		return 0;
	return ((a - 1) / b) + 1;
}

/*
 *  Allocate memory
 */
errcode_t ext2fs_get_mem(unsigned long size, void *ptr)
{
	void *pp;

	pp = memalign(ARCH_DMA_MINALIGN, size);
	if (!pp)
		return EXT2_ET_NO_MEMORY;
	memcpy(ptr, &pp, sizeof(pp));
	return 0;
}

errcode_t ext2fs_get_array(unsigned long count, unsigned long size, void *ptr)
{
	if (count && (-1UL)/count < size)
		return EXT2_ET_NO_MEMORY;
	return ext2fs_get_mem(count*size, ptr);
}

/*
 * Free memory
 */
errcode_t ext2fs_free_mem(void *ptr)
{
	void *p;

	memcpy(&p, ptr, sizeof(p));
	free(p);
	p = 0;
	memcpy(ptr, &p, sizeof(p));
	return 0;
}

static int test_root(int a, int b)
{
	if (a == 0)
		return 1;
	while (1) {
		if (a == 1)
			return 1;
		if (a % b)
			return 0;
		a = a / b;
	}
}

int ext2fs_bg_has_super(ext2_filsys fs, int group_block)
{
	if (!(fs->super->s_feature_ro_compat &
	      EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER))
		return 1;

	if (test_root(group_block, 3) || (test_root(group_block, 5)) ||
	    test_root(group_block, 7))
		return 1;

	return 0;
}

errcode_t ext2fs_make_generic_bitmap(errcode_t magic, ext2_filsys fs,
				     __u32 start, __u32 end, __u32 real_end,
				     const char *descr, char *init_map,
				     ext2fs_generic_bitmap *ret)
{
	ext2fs_generic_bitmap	bitmap;
	errcode_t		retval;
	size_t			size;

	retval = ext2fs_get_mem(sizeof(struct ext2fs_struct_generic_bitmap),
				&bitmap);
	if (retval)
		return retval;

	bitmap->magic = magic;
	bitmap->fs = fs;
	bitmap->start = start;
	bitmap->end = end;
	bitmap->real_end = real_end;
	switch (magic) {
	case EXT2_ET_MAGIC_INODE_BITMAP:
		bitmap->base_error_code = EXT2_ET_BAD_INODE_MARK;
		break;
	case EXT2_ET_MAGIC_BLOCK_BITMAP:
		bitmap->base_error_code = EXT2_ET_BAD_BLOCK_MARK;
		break;
	default:
		bitmap->base_error_code = EXT2_ET_BAD_GENERIC_MARK;
	}
	if (descr) {
		retval = ext2fs_get_mem(strlen(descr)+1, &bitmap->description);
		if (retval) {
			ext2fs_free_mem(&bitmap);
			return retval;
		}
		strcpy(bitmap->description, descr);
	} else {
		bitmap->description = 0;
	}

	size = (size_t) (((bitmap->real_end - bitmap->start) / 8) + 1);
	/* Round up to allow for the BT x86 instruction */
	size = (size + 7) & ~3;
	retval = ext2fs_get_mem(size, &bitmap->bitmap);
	if (retval) {
		ext2fs_free_mem(&bitmap->description);
		ext2fs_free_mem(&bitmap);
		return retval;
	}

	if (init_map)
		memcpy(bitmap->bitmap, init_map, size);
	else
		memset(bitmap->bitmap, 0, size);
	*ret = bitmap;
	return 0;
}

errcode_t ext2fs_get_memalign(unsigned long size,
				       unsigned long align, char **ptr)
{
	if (align == 0)
		align = 8;

	*ptr = (char *)memalign(ARCH_DMA_MINALIGN, size);
	if (ptr == NULL)
		return EXT2_ET_NO_MEMORY;
	else
		return 0;
}

/*
 * Mark a filesystem superblock as dirty
 */
void ext2fs_mark_super_dirty(ext2_filsys fs)
{
	fs->flags |= EXT2_FLAG_DIRTY | EXT2_FLAG_CHANGED;
}

/*
 * Calculate the number of GDT blocks to reserve for online filesystem growth.
 * The absolute maximum number of GDT blocks we can reserve is determined by
 * the number of block pointers that can fit into a single block.
 */
static unsigned int calc_reserved_gdt_blocks(ext2_filsys fs)
{
	struct ext2_super_block *sb = fs->super;
	unsigned long bpg = sb->s_blocks_per_group;
	unsigned int gdpb = EXT2_DESC_PER_BLOCK(sb);
	unsigned long max_blocks = 0xffffffff;
	unsigned long rsv_groups;
	unsigned int rsv_gdb;

	/* We set it at 1024x the current filesystem size, or
	 * the upper block count limit (2^32), whichever is lower.
	 */
	if (sb->s_blocks_count < max_blocks / 1024)
		max_blocks = sb->s_blocks_count * 1024;
	rsv_groups = ext2fs_div_ceil(max_blocks - sb->s_first_data_block, bpg);
	rsv_gdb = ext2fs_div_ceil(rsv_groups, gdpb) - fs->desc_blocks;
	if (rsv_gdb > EXT2_ADDR_PER_BLOCK(sb))
		rsv_gdb = EXT2_ADDR_PER_BLOCK(sb);

	return rsv_gdb;
}

/*
 * Return the first block (inclusive) in a group
 */
blk_t ext2fs_group_first_block(ext2_filsys fs, dgrp_t group)
{
	return fs->super->s_first_data_block +
		(group * fs->super->s_blocks_per_group);
}

/*
 * Return the last block (inclusive) in a group
 */
blk_t ext2fs_group_last_block(ext2_filsys fs, dgrp_t group)
{
	return (group == fs->group_desc_count - 1 ?
		fs->super->s_blocks_count - 1 :
		ext2fs_group_first_block(fs, group) +
			(fs->super->s_blocks_per_group - 1));
}


/*
 * This function returns the location of the superblock, block group
 * descriptors for a given block group.  It currently returns the
 * number of free blocks assuming that inode table and allocation
 * bitmaps will be in the group.  This is not necessarily the case
 * when the flex_bg feature is enabled, so callers should take care!
 * It was only really intended for use by mke2fs, and even there it's
 * not that useful.  In the future, when we redo this function for
 * 64-bit block numbers, we should probably return the number of
 * blocks used by the super block and group descriptors instead.
 *
 * See also the comment for ext2fs_reserve_super_and_bgd()
 */
int ext2fs_super_and_bgd_loc(ext2_filsys fs,
			     dgrp_t group,
			     blk_t *ret_super_blk,
			     blk_t *ret_old_desc_blk,
			     blk_t *ret_new_desc_blk,
			     int *ret_meta_bg)
{
	blk_t	group_block, super_blk = 0, old_desc_blk = 0, new_desc_blk = 0;
	unsigned int meta_bg, meta_bg_size;
	blk_t	numblocks, old_desc_blocks;
	int	has_super;

	group_block = ext2fs_group_first_block(fs, group);

	if (fs->super->s_feature_incompat & EXT2_FEATURE_INCOMPAT_META_BG)
		old_desc_blocks = fs->super->s_first_meta_bg;
	else
		old_desc_blocks =
			fs->desc_blocks + fs->super->s_reserved_gdt_blocks;

	if (group == fs->group_desc_count-1) {
		numblocks = (fs->super->s_blocks_count -
			     fs->super->s_first_data_block) %
			fs->super->s_blocks_per_group;
		if (!numblocks)
			numblocks = fs->super->s_blocks_per_group;
	} else {
		numblocks = fs->super->s_blocks_per_group;
	}

	has_super = ext2fs_bg_has_super(fs, group);

	if (has_super) {
		super_blk = group_block;
		numblocks--;
	}
	meta_bg_size = EXT2_DESC_PER_BLOCK(fs->super);
	meta_bg = group / meta_bg_size;

	if (!(fs->super->s_feature_incompat & EXT2_FEATURE_INCOMPAT_META_BG) ||
	    (meta_bg < fs->super->s_first_meta_bg)) {
		if (has_super) {
			old_desc_blk = group_block + 1;
			numblocks -= old_desc_blocks;
		}
	} else {
		if (((group % meta_bg_size) == 0) ||
		    ((group % meta_bg_size) == 1) ||
		    ((group % meta_bg_size) == (meta_bg_size-1))) {
			if (has_super)
				has_super = 1;
			new_desc_blk = group_block + has_super;
			numblocks--;
		}
	}

	numblocks -= 2 + fs->inode_blocks_per_group;

	if (ret_super_blk)
		*ret_super_blk = super_blk;
	if (ret_old_desc_blk)
		*ret_old_desc_blk = old_desc_blk;
	if (ret_new_desc_blk)
		*ret_new_desc_blk = new_desc_blk;
	if (ret_meta_bg)
		*ret_meta_bg = meta_bg;

	return numblocks;
}

/*
 * For the benefit of those who are trying to port Linux to another
 * architecture, here are some C-language equivalents.  You should
 * recode these in the native assmebly language, if at all possible.
 *
 * C language equivalents written by Theodore Ts'o, 9/26/92.
 * Modified by Pete A. Zaitcev 7/14/95 to be portable to big endian
 * systems, as well as non-32 bit systems.
 */

/*
 * Return the group # of a block
 */
int ext2fs_group_of_blk(ext2_filsys fs, blk_t blk)
{
	return (blk - fs->super->s_first_data_block) /
		fs->super->s_blocks_per_group;
}

/*
 * Return the group # of an inode number
 */
int ext2fs_group_of_ino(ext2_filsys fs, ext2_ino_t ino)
{
	return (ino - 1) / fs->super->s_inodes_per_group;
}

int ext2fs_set_bit(unsigned int nr, void *addr)
{
	int		mask, retval;
	unsigned char	*ADDR = (unsigned char *)addr;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	retval = mask & *ADDR;
	*ADDR |= mask;
	return retval;
}

int ext2fs_clear_bit(unsigned int nr, void *addr)
{
	int		mask, retval;
	unsigned char	*ADDR = (unsigned char *)addr;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	retval = mask & *ADDR;
	*ADDR &= ~mask;
	return retval;
}

int ext2fs_test_bit(unsigned int nr, const void *addr)
{
	int			mask;
	const unsigned char	*ADDR = (const unsigned char *)addr;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	return mask & *ADDR;
}

int ext2fs_mark_generic_bitmap(ext2fs_generic_bitmap bitmap,
					 __u32 bitno)
{
	if ((bitno < bitmap->start) || (bitno > bitmap->end))
		return 0;

	return ext2fs_set_bit(bitno - bitmap->start, bitmap->bitmap);
}

int ext2fs_mark_block_bitmap(ext2fs_block_bitmap bitmap,
				       blk_t block)
{
	return ext2fs_mark_generic_bitmap((ext2fs_generic_bitmap) bitmap,
					  block);
}

int ext2fs_unmark_generic_bitmap(ext2fs_generic_bitmap bitmap,
					   blk_t bitno)
{
	if ((bitno < bitmap->start) || (bitno > bitmap->end))
		return 0;

	return ext2fs_clear_bit(bitno - bitmap->start, bitmap->bitmap);
}

void ext2fs_fast_unmark_block_bitmap(ext2fs_block_bitmap bitmap,
					      blk_t block)
{
	ext2fs_unmark_generic_bitmap((ext2fs_generic_bitmap) bitmap, block);
}

int ext2fs_unmark_block_bitmap(ext2fs_block_bitmap bitmap,
					 blk_t block)
{
	return ext2fs_unmark_generic_bitmap((ext2fs_generic_bitmap) bitmap,
					    block);
}

int ext2fs_mark_inode_bitmap(ext2fs_inode_bitmap bitmap,
				       ext2_ino_t inode)
{
	return ext2fs_mark_generic_bitmap((ext2fs_generic_bitmap) bitmap,
					  inode);
}

int ext2fs_unmark_inode_bitmap(ext2fs_inode_bitmap bitmap,
					 ext2_ino_t inode)
{
	return ext2fs_unmark_generic_bitmap((ext2fs_generic_bitmap) bitmap,
				     inode);
}

int ext2fs_test_generic_bitmap(ext2fs_generic_bitmap bitmap,
					blk_t bitno)
{
	if ((bitno < bitmap->start) || (bitno > bitmap->end))
		return 0;

	return ext2fs_test_bit(bitno - bitmap->start, bitmap->bitmap);
}

int ext2fs_fast_test_inode_bitmap(ext2fs_inode_bitmap bitmap,
					   ext2_ino_t inode)
{
	return ext2fs_test_generic_bitmap((ext2fs_generic_bitmap) bitmap,
					  inode);
}


__u16 ext2fs_group_desc_csum(ext2_filsys fs, dgrp_t group)
{
	__u16 crc = 0;
	struct ext2_group_desc *desc;

	desc = &fs->group_desc[group];

	if (fs->super->s_feature_ro_compat & EXT4_FEATURE_RO_COMPAT_GDT_CSUM) {
		int offset = offsetof(struct ext2_group_desc, bg_checksum);

		crc = ext2fs_crc16(~0, fs->super->s_uuid,
				   sizeof(fs->super->s_uuid));
		crc = ext2fs_crc16(crc, &group, sizeof(group));
		crc = ext2fs_crc16(crc, desc, offset);
		offset += sizeof(desc->bg_checksum); /* skip checksum */
		assert(offset == sizeof(*desc));

		/* for checksum of struct ext4_group_desc do the rest...*/
		if (offset < fs->super->s_desc_size) {
			crc = ext2fs_crc16(crc, (char *)desc + offset,
				    fs->super->s_desc_size - offset);
		}
	}
	return crc;
}

int ext2fs_group_desc_csum_verify(ext2_filsys fs, dgrp_t group)
{
	if (EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
				       EXT4_FEATURE_RO_COMPAT_GDT_CSUM) &&
	    (fs->group_desc[group].bg_checksum !=
	     ext2fs_group_desc_csum(fs, group)))
		return 0;

	return 1;
}

void ext2fs_group_desc_csum_set(ext2_filsys fs, dgrp_t group)
{
	if (EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
				       EXT4_FEATURE_RO_COMPAT_GDT_CSUM))
		fs->group_desc[group].bg_checksum =
			ext2fs_group_desc_csum(fs, group);
}

/*
 * This function reserves the superblock and block group descriptors
 * for a given block group.  It currently returns the number of free
 * blocks assuming that inode table and allocation bitmaps will be in
 * the group.  This is not necessarily the case when the flex_bg
 * feature is enabled, so callers should take care!  It was only
 * really intended for use by mke2fs, and even there it's not that
 * useful.  In the future, when we redo this function for 64-bit block
 * numbers, we should probably return the number of blocks used by the
 * super block and group descriptors instead.
 *
 * See also the comment for ext2fs_super_and_bgd_loc()
 */
int ext2fs_reserve_super_and_bgd(ext2_filsys fs,
				 dgrp_t group,
				 ext2fs_block_bitmap bmap)
{
	blk_t	super_blk, old_desc_blk, new_desc_blk;
	int	j, old_desc_blocks, num_blocks;

	num_blocks = ext2fs_super_and_bgd_loc(fs, group, &super_blk,
					      &old_desc_blk, &new_desc_blk, 0);

	if (fs->super->s_feature_incompat & EXT2_FEATURE_INCOMPAT_META_BG)
		old_desc_blocks = fs->super->s_first_meta_bg;
	else
		old_desc_blocks =
			fs->desc_blocks + fs->super->s_reserved_gdt_blocks;

	if (super_blk || (group == 0))
		ext2fs_mark_block_bitmap(bmap, super_blk);

	if (old_desc_blk) {
		if (fs->super->s_reserved_gdt_blocks && fs->block_map == bmap)
			fs->group_desc[group].bg_flags &= ~EXT2_BG_BLOCK_UNINIT;
		for (j = 0; j < old_desc_blocks; j++)
			if (old_desc_blk + j < fs->super->s_blocks_count)
				ext2fs_mark_block_bitmap(bmap,
							 old_desc_blk + j);
	}
	if (new_desc_blk)
		ext2fs_mark_block_bitmap(bmap, new_desc_blk);

	return num_blocks;
}

void ext2fs_free_generic_bitmap(ext2fs_inode_bitmap bitmap)
{
	if (check_magic(bitmap))
		return;

	bitmap->magic = 0;
	if (bitmap->description) {
		ext2fs_free_mem(&bitmap->description);
		bitmap->description = 0;
	}
	if (bitmap->bitmap) {
		ext2fs_free_mem(&bitmap->bitmap);
		bitmap->bitmap = 0;
	}
	ext2fs_free_mem(&bitmap);
}

void ext2fs_free_block_bitmap(ext2fs_block_bitmap bitmap)
{
	ext2fs_free_generic_bitmap(bitmap);
}

void ext2fs_free_inode_bitmap(ext2fs_inode_bitmap bitmap)
{
	ext2fs_free_generic_bitmap(bitmap);
}

/*
 * This procedure frees a badblocks list.
 */
void ext2fs_u32_list_free(ext2_u32_list bb)
{
	if (bb->magic != EXT2_ET_MAGIC_BADBLOCKS_LIST)
		return;

	if (bb->list)
		ext2fs_free_mem(&bb->list);
	bb->list = 0;
	ext2fs_free_mem(&bb);
}

void ext2fs_badblocks_list_free(ext2_badblocks_list bb)
{
	ext2fs_u32_list_free((ext2_u32_list) bb);
}

/*
 * Free a directory block list
 */
void ext2fs_free_dblist(ext2_dblist dblist)
{
	if (!dblist || (dblist->magic != EXT2_ET_MAGIC_DBLIST))
		return;

	if (dblist->list)
		ext2fs_free_mem(&dblist->list);
	dblist->list = 0;
	if (dblist->fs && dblist->fs->dblist == dblist)
		dblist->fs->dblist = 0;
	dblist->magic = 0;
	ext2fs_free_mem(&dblist);
}

/*
 * Free the inode cache structure
 */
static void ext2fs_free_inode_cache(struct ext2_inode_cache *icache)
{
	if (--icache->refcount)
		return;
	if (icache->buffer)
		ext2fs_free_mem(&icache->buffer);
	if (icache->cache)
		ext2fs_free_mem(&icache->cache);
	icache->buffer_blk = 0;
	ext2fs_free_mem(&icache);
}

void ext2fs_free(ext2_filsys fs)
{
	if (!fs || (fs->magic != EXT2_ET_MAGIC_EXT2FS_FILSYS))
		return;

	if (fs->device_name)
		ext2fs_free_mem(&fs->device_name);
	if (fs->super)
		ext2fs_free_mem(&fs->super);
	if (fs->orig_super)
		ext2fs_free_mem(&fs->orig_super);
	if (fs->group_desc)
		ext2fs_free_mem(&fs->group_desc);
	if (fs->block_map)
		ext2fs_free_block_bitmap(fs->block_map);
	if (fs->inode_map)
		ext2fs_free_inode_bitmap(fs->inode_map);

	if (fs->badblocks)
		ext2fs_badblocks_list_free(fs->badblocks);
	fs->badblocks = 0;

	if (fs->dblist)
		ext2fs_free_dblist(fs->dblist);

	if (fs->icache)
		ext2fs_free_inode_cache(fs->icache);

	fs->magic = 0;

	ext2fs_free_mem(&fs);
}

/*
 * Compare @mem to zero buffer by 256 bytes.
 * Return 1 if @mem is zeroed memory, otherwise return 0.
 */
static int mem_is_zero(const char *mem, size_t len)
{
	static const char zero_buf[256];

	while (len >= sizeof(zero_buf)) {
		if (memcmp(mem, zero_buf, sizeof(zero_buf)))
			return 0;
		len -= sizeof(zero_buf);
		mem += sizeof(zero_buf);
	}
	/* Deal with leftover bytes. */
	if (len)
		return !memcmp(mem, zero_buf, len);
	return 1;
}

/*
 * Return true if all of the bits in a specified range are clear
 */
static int ext2fs_test_clear_generic_bitmap_range(ext2fs_generic_bitmap bitmap,
						  unsigned int start,
						  unsigned int len)
{
	size_t start_byte, len_byte = len >> 3;
	unsigned int start_bit, len_bit = len % 8;
	int first_bit = 0;
	int last_bit  = 0;
	int mark_count = 0;
	int mark_bit = 0;
	int i;
	const char *ADDR = bitmap->bitmap;

	start -= bitmap->start;
	start_byte = start >> 3;
	start_bit = start % 8;

	if (start_bit != 0) {
		/*
		 * The compared start block number or start inode number
		 * is not the first bit in a byte.
		 */
		mark_count = 8 - start_bit;
		if (len < 8 - start_bit) {
			mark_count = (int)len;
			mark_bit = len + start_bit - 1;
		} else {
			mark_bit = 7;
		}

		for (i = mark_count; i > 0; i--, mark_bit--)
			first_bit |= 1 << mark_bit;

		/*
		 * Compare blocks or inodes in the first byte.
		 * If there is any marked bit, this function returns 0.
		 */
		if (first_bit & ADDR[start_byte])
			return 0;
		else if (len <= 8 - start_bit)
			return 1;

		start_byte++;
		len_bit = (len - mark_count) % 8;
		len_byte = (len - mark_count) >> 3;
	}

	/*
	 * The compared start block number or start inode number is
	 * the first bit in a byte.
	 */
	if (len_bit != 0) {
		/*
		 * The compared end block number or end inode number is
		 * not the last bit in a byte.
		 */
		for (mark_bit = len_bit - 1; mark_bit >= 0; mark_bit--)
			last_bit |= 1 << mark_bit;

		/*
		 * Compare blocks or inodes in the last byte.
		 * If there is any marked bit, this function returns 0.
		 */
		if (last_bit & ADDR[start_byte + len_byte])
			return 0;
		else if (len_byte == 0)
			return 1;
	}

	/* Check whether all bytes are 0 */
	return mem_is_zero(ADDR + start_byte, len_byte);
}

int ext2fs_test_block_bitmap_range(ext2fs_block_bitmap bitmap,
				   blk_t block, int num)
{
	EXT2_CHECK_MAGIC(bitmap, EXT2_ET_MAGIC_BLOCK_BITMAP);
	if ((block < bitmap->start) || (block+num-1 > bitmap->real_end))
		return 0;

	return ext2fs_test_clear_generic_bitmap_range((ext2fs_generic_bitmap)
						      bitmap, block, num);
}

int ext2fs_test_inode_bitmap_range(ext2fs_inode_bitmap bitmap,
				   ino_t inode, int num)
{
	EXT2_CHECK_MAGIC(bitmap, EXT2_ET_MAGIC_INODE_BITMAP);
	if ((inode < bitmap->start) || (inode+num-1 > bitmap->real_end))
		return 0;

	return ext2fs_test_clear_generic_bitmap_range((ext2fs_generic_bitmap)
						      bitmap, inode, num);
}


int ext2fs_fast_test_block_bitmap(ext2fs_block_bitmap bitmap,
					    blk_t block)
{
	return ext2fs_test_generic_bitmap((ext2fs_generic_bitmap) bitmap,
					  block);
}

errcode_t ext2fs_get_generic_bitmap_range(ext2fs_generic_bitmap bmap,
					  errcode_t magic,
					  __u32 start, __u32 num,
					  void *out)
{
	if (!bmap || (bmap->magic != magic))
		return magic;

	if ((start < bmap->start) || (start+num-1 > bmap->real_end))
		return EXT2_ET_INVALID_ARGUMENT;

	memcpy(out, bmap->bitmap + (start >> 3), (num+7) >> 3);
	return 0;
}

errcode_t ext2fs_get_inode_bitmap_range(ext2fs_inode_bitmap bmap,
					ext2_ino_t start, unsigned int num,
					void *out)
{
	return ext2fs_get_generic_bitmap_range(bmap,
						EXT2_ET_MAGIC_INODE_BITMAP,
						start, num, out);
}

errcode_t ext2fs_get_block_bitmap_range(ext2fs_block_bitmap bmap,
					blk_t start, unsigned int num,
					void *out)
{
	return ext2fs_get_generic_bitmap_range(bmap,
						EXT2_ET_MAGIC_BLOCK_BITMAP,
						start, num, out);
}


static errcode_t write_bitmaps(ext2_filsys fs, int do_inode, int do_block)
{
	dgrp_t		i;
	unsigned int	j;
	int		block_nbytes, inode_nbytes;
	unsigned int	nbits;
	errcode_t	retval;
	char		*block_buf, *inode_buf;
	int		csum_flag = 0;
	blk_t		blk;
	blk_t		blk_itr = fs->super->s_first_data_block;
	ext2_ino_t	ino_itr = 1;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!(fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
				       EXT4_FEATURE_RO_COMPAT_GDT_CSUM))
		csum_flag = 1;

	inode_nbytes = 0;
	block_nbytes = 0;

	if (do_block) {
		block_nbytes = EXT2_BLOCKS_PER_GROUP(fs->super) / 8;
		retval = ext2fs_get_memalign(fs->blocksize, fs->blocksize,
					     (char **)&block_buf);

		if (retval)
			return retval;
		memset(block_buf, 0xff, fs->blocksize);
	}

	if (do_inode) {
		inode_nbytes = (size_t)
			((EXT2_INODES_PER_GROUP(fs->super)+7) / 8);
		retval = ext2fs_get_memalign(fs->blocksize, fs->blocksize,
					     (char **)&inode_buf);
		if (retval)
			return retval;
		memset(inode_buf, 0xff, fs->blocksize);
	}

	for (i = 0; i < fs->group_desc_count; i++) {
		if (!do_block)
			goto skip_block_bitmap;

		if (csum_flag && fs->group_desc[i].bg_flags &
		    EXT2_BG_BLOCK_UNINIT)
			goto skip_this_block_bitmap;

		retval = ext2fs_get_block_bitmap_range(fs->block_map,
				blk_itr, block_nbytes << 3, block_buf);

		if (retval)
			return retval;

		if (i == fs->group_desc_count - 1) {
			/* Force bitmap padding for the last group */
			nbits = ((fs->super->s_blocks_count
				  - fs->super->s_first_data_block)
				 % EXT2_BLOCKS_PER_GROUP(fs->super));
			if (nbits)
				for (j = nbits; j < fs->blocksize * 8; j++)
					ext2fs_set_bit(j, block_buf);
		}
		blk = fs->group_desc[i].bg_block_bitmap;
		if (blk) {
			put_ext4fs(dev_desc, (uint64_t)(blk * fs->blocksize),
				   block_buf, (uint32_t) fs->blocksize);
			retval = 0;
			if (retval)
				return EXT2_ET_BLOCK_BITMAP_WRITE;
		}
skip_this_block_bitmap:
		blk_itr += block_nbytes << 3;
skip_block_bitmap:

		if (!do_inode)
			continue;

		if (csum_flag && fs->group_desc[i].bg_flags &
		    EXT2_BG_INODE_UNINIT)
			goto skip_this_inode_bitmap;

		retval = ext2fs_get_inode_bitmap_range(fs->inode_map,
				ino_itr, inode_nbytes << 3, inode_buf);
		if (retval)
			return retval;

		blk = fs->group_desc[i].bg_inode_bitmap;
		if (blk) {
			put_ext4fs(dev_desc, (uint64_t)(blk * fs->blocksize),
				   inode_buf, (uint32_t) fs->blocksize);
		retval = 0;

			if (retval)
				return EXT2_ET_INODE_BITMAP_WRITE;
		}
skip_this_inode_bitmap:
		ino_itr += inode_nbytes << 3;
	}
	if (do_block) {
		fs->flags &= ~EXT2_FLAG_BB_DIRTY;
		ext2fs_free_mem(&block_buf);
	}
	if (do_inode) {
		fs->flags &= ~EXT2_FLAG_IB_DIRTY;
		ext2fs_free_mem(&inode_buf);
	}
	return 0;
}

/*
 * Check to see if a filesystem's inode bitmap is dirty
 */
int ext2fs_test_ib_dirty(ext2_filsys fs)
{
	return fs->flags & EXT2_FLAG_IB_DIRTY;
}

/*
 * Check to see if a filesystem's block bitmap is dirty
 */
int ext2fs_test_bb_dirty(ext2_filsys fs)
{
	return fs->flags & EXT2_FLAG_BB_DIRTY;
}

errcode_t ext2fs_write_bitmaps(ext2_filsys fs)
{
	int do_inode = fs->inode_map && ext2fs_test_ib_dirty(fs);
	int do_block = fs->block_map && ext2fs_test_bb_dirty(fs);

	if (!do_inode && !do_block)
		return 0;

	return write_bitmaps(fs, do_inode, do_block);
}

errcode_t ext2fs_allocate_inode_bitmap(ext2_filsys fs,
				       const char *descr,
				       ext2fs_inode_bitmap *ret)
{
	__u32		start, end, real_end;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	fs->write_bitmaps = ext2fs_write_bitmaps;

	start = 1;
	end = fs->super->s_inodes_count;
	real_end = (EXT2_INODES_PER_GROUP(fs->super) * fs->group_desc_count);

	return ext2fs_make_generic_bitmap(EXT2_ET_MAGIC_INODE_BITMAP, fs,
					   start, end, real_end,
					   descr, 0, ret);
}

errcode_t ext2fs_allocate_block_bitmap(ext2_filsys fs,
				       const char *descr,
				       ext2fs_block_bitmap *ret)
{
	__u32		start, end, real_end;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	fs->write_bitmaps = ext2fs_write_bitmaps;

	start = fs->super->s_first_data_block;
	end = fs->super->s_blocks_count-1;
	real_end = (EXT2_BLOCKS_PER_GROUP(fs->super)
		    * fs->group_desc_count)-1 + start;

	return ext2fs_make_generic_bitmap(EXT2_ET_MAGIC_BLOCK_BITMAP, fs,
					   start, end, real_end,
					   descr, 0, ret);
}
errcode_t ext2fs_initialize(const char *name, int flags,
			    struct ext2_super_block *param,
			    ext2_filsys *ret_fs)
{
	ext2_filsys	fs;
	errcode_t	retval;
	struct ext2_super_block *super;
	int		frags_per_block;
	unsigned int	rem;
	unsigned int	overhead = 0;
	unsigned int	ipg;
	dgrp_t		i;
	blk_t		numblocks;
	int		rsv_gdt;
	int		csum_flag;
	int		io_flags;
	char		*buf = 0;
	char		c;

	if (!param || !param->s_blocks_count)
		return EXT2_ET_INVALID_ARGUMENT;

	retval = ext2fs_get_mem(sizeof(struct struct_ext2_filsys), &fs);
	if (retval)
		return retval;

	memset(fs, 0, sizeof(struct struct_ext2_filsys));
	fs->magic = EXT2_ET_MAGIC_EXT2FS_FILSYS;
	fs->flags = flags | EXT2_FLAG_RW;
	fs->umask = 022;
	io_flags = IO_FLAG_RW;

	if (flags & EXT2_FLAG_EXCLUSIVE)
		io_flags |= IO_FLAG_EXCLUSIVE;

	retval = ext2fs_get_mem(strlen(name)+1, &fs->device_name);

	if (retval)
		goto cleanup;
	strcpy(fs->device_name, name);

	retval = ext2fs_get_mem(SUPERBLOCK_SIZE, &super);
	if (retval)
		goto cleanup;
	fs->super = super;

	memset(super, 0, SUPERBLOCK_SIZE);

#define set_field(field, default) (super->field = param->field ? \
				   param->field : (default))

	super->s_magic = EXT2_SUPER_MAGIC;
	super->s_state = EXT2_VALID_FS;

	set_field(s_log_block_size, 0);	/* default blocksize: 1024 bytes */
	set_field(s_log_frag_size, 0); /* default fragsize: 1024 bytes */
	set_field(s_first_data_block, super->s_log_block_size ? 0 : 1);
	set_field(s_max_mnt_count, EXT2_DFL_MAX_MNT_COUNT);
	set_field(s_errors, EXT2_ERRORS_DEFAULT);
	set_field(s_feature_compat, 0);
	set_field(s_feature_incompat, 0);
	set_field(s_feature_ro_compat, 0);
	set_field(s_first_meta_bg, 0);
	set_field(s_raid_stride, 0);		/* default stride size: 0 */
	set_field(s_raid_stripe_width, 0);	/* default stripe width: 0 */
	set_field(s_log_groups_per_flex, 0);
	set_field(s_flags, 0);
	if (super->s_feature_incompat & ~EXT2_LIB_FEATURE_INCOMPAT_SUPP) {
		retval = EXT2_ET_UNSUPP_FEATURE;
		goto cleanup;
	}
	if (super->s_feature_ro_compat & ~EXT2_LIB_FEATURE_RO_COMPAT_SUPP) {
		retval = EXT2_ET_RO_UNSUPP_FEATURE;
		goto cleanup;
	}

	set_field(s_rev_level, EXT2_GOOD_OLD_REV);
	if (super->s_rev_level >= EXT2_DYNAMIC_REV) {
		set_field(s_first_ino, EXT2_GOOD_OLD_FIRST_INO);
		set_field(s_inode_size, EXT2_GOOD_OLD_INODE_SIZE);
		if (super->s_inode_size >= sizeof(struct ext2_inode_large)) {
			int extra_isize = sizeof(struct ext2_inode_large) -
				EXT2_GOOD_OLD_INODE_SIZE;
			set_field(s_min_extra_isize, extra_isize);
			set_field(s_want_extra_isize, extra_isize);
		}
	} else {
		super->s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
		super->s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
	}

	set_field(s_checkinterval, EXT2_DFL_CHECKINTERVAL);
	super->s_mkfs_time = fs->now ? fs->now : (__u32)NULL;
	super->s_lastcheck = fs->now ? fs->now : (__u32)NULL;

	super->s_creator_os = CREATOR_OS;

	fs->blocksize = EXT2_SB_BLOCK_SIZE(super);
	fs->fragsize = EXT2_FRAG_SIZE(super);
	frags_per_block = fs->blocksize / fs->fragsize;

	/* default: (fs->blocksize*8) blocks/group, up to 2^16 (GDT limit) */
	set_field(s_blocks_per_group, fs->blocksize * 8);
	if (super->s_blocks_per_group > EXT2_MAX_BLOCKS_PER_GROUP(super))
		super->s_blocks_per_group = EXT2_MAX_BLOCKS_PER_GROUP(super);
	super->s_frags_per_group = super->s_blocks_per_group * frags_per_block;

	super->s_blocks_count = param->s_blocks_count;
	super->s_r_blocks_count = param->s_r_blocks_count;
	if (super->s_r_blocks_count >= param->s_blocks_count) {
		retval = EXT2_ET_INVALID_ARGUMENT;
		goto cleanup;
	}
	/*
	 * If we're creating an external journal device, we don't need
	 * to bother with the rest.
	 */
	if (super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) {
		fs->group_desc_count = 0;
		ext2fs_mark_super_dirty(fs);
		*ret_fs = fs;
		return 0;
	}

retry:
	fs->group_desc_count = ext2fs_div_ceil(super->s_blocks_count -
					       super->s_first_data_block,
					       EXT2_BLOCKS_PER_GROUP(super));
	if (fs->group_desc_count == 0) {
		retval = EXT2_ET_TOOSMALL;
		goto cleanup;
	}
	fs->desc_blocks = ext2fs_div_ceil(fs->group_desc_count,
					  EXT2_DESC_PER_BLOCK(super));

	i = fs->blocksize >= 4096 ? 1 : 4096 / fs->blocksize;
	set_field(s_inodes_count, super->s_blocks_count / i);

	/*
	 * Make sure we have at least EXT2_FIRST_INO + 1 inodes, so
	 * that we have enough inodes for the filesystem(!)
	 */
	if (super->s_inodes_count < EXT2_FIRST_INODE(super)+1)
		super->s_inodes_count = EXT2_FIRST_INODE(super)+1;

	/*
	 * There should be at least as many inodes as the user
	 * requested.  Figure out how many inodes per group that
	 * should be.  But make sure that we don't allocate more than
	 * one bitmap's worth of inodes each group.
	 */
	ipg = ext2fs_div_ceil(super->s_inodes_count, fs->group_desc_count);
	if (ipg > fs->blocksize * 8) {
		if (super->s_blocks_per_group >= 256) {
			/* Try again with slightly different parameters */
			super->s_blocks_per_group -= 8;
			super->s_blocks_count = param->s_blocks_count;
			super->s_frags_per_group = super->s_blocks_per_group *
				frags_per_block;
			goto retry;
		} else {
			retval = EXT2_ET_TOO_MANY_INODES;
			goto cleanup;
		}
	}

	if (ipg > (unsigned) EXT2_MAX_INODES_PER_GROUP(super))
		ipg = EXT2_MAX_INODES_PER_GROUP(super);

ipg_retry:
	super->s_inodes_per_group = ipg;

	/*
	 * Make sure the number of inodes per group completely fills
	 * the inode table blocks in the descriptor.  If not, add some
	 * additional inodes/group.  Waste not, want not...
	 */
	fs->inode_blocks_per_group = (((super->s_inodes_per_group *
					EXT2_INODE_SIZE(super)) +
				       EXT2_SB_BLOCK_SIZE(super) - 1) /
				      EXT2_SB_BLOCK_SIZE(super));
	super->s_inodes_per_group = ((fs->inode_blocks_per_group *
				      EXT2_SB_BLOCK_SIZE(super)) /
				     EXT2_INODE_SIZE(super));
	/*
	 * Finally, make sure the number of inodes per group is a
	 * multiple of 8.  This is needed to simplify the bitmap
	 * splicing code.
	 */
	super->s_inodes_per_group &= ~7;
	fs->inode_blocks_per_group = (((super->s_inodes_per_group *
					EXT2_INODE_SIZE(super)) +
				       EXT2_SB_BLOCK_SIZE(super) - 1) /
				      EXT2_SB_BLOCK_SIZE(super));

	/*
	 * adjust inode count to reflect the adjusted inodes_per_group
	 */
	if ((__u64)super->s_inodes_per_group * fs->group_desc_count > ~0U) {
		ipg--;
		goto ipg_retry;
	}
	super->s_inodes_count = super->s_inodes_per_group *
		fs->group_desc_count;
	super->s_free_inodes_count = super->s_inodes_count;

	/*
	 * check the number of reserved group descriptor table blocks
	 */
	if (super->s_feature_compat & EXT2_FEATURE_COMPAT_RESIZE_INODE)
		rsv_gdt = calc_reserved_gdt_blocks(fs);
	else
		rsv_gdt = 0;
	set_field(s_reserved_gdt_blocks, rsv_gdt);
	if (super->s_reserved_gdt_blocks > EXT2_ADDR_PER_BLOCK(super)) {
		retval = EXT2_ET_RES_GDT_BLOCKS;
		goto cleanup;
	}

	/*
	 * Calculate the maximum number of bookkeeping blocks per
	 * group.  It includes the superblock, the block group
	 * descriptors, the block bitmap, the inode bitmap, the inode
	 * table, and the reserved gdt blocks.
	 */
	overhead = (int) (3 + fs->inode_blocks_per_group +
			  fs->desc_blocks + super->s_reserved_gdt_blocks);

	/* This can only happen if the user requested too many inodes */
	if (overhead > super->s_blocks_per_group) {
		retval = EXT2_ET_TOO_MANY_INODES;
		goto cleanup;
	}

	/*
	 * See if the last group is big enough to support the
	 * necessary data structures.  If not, we need to get rid of
	 * it.  We need to recalculate the overhead for the last block
	 * group, since it might or might not have a superblock
	 * backup.
	 */
	overhead = (int) (2 + fs->inode_blocks_per_group);
	if (ext2fs_bg_has_super(fs, fs->group_desc_count - 1))
		overhead += 1 + fs->desc_blocks + super->s_reserved_gdt_blocks;
	rem = ((super->s_blocks_count - super->s_first_data_block) %
	       super->s_blocks_per_group);
	if ((fs->group_desc_count == 1) && rem && (rem < overhead)) {
		retval = EXT2_ET_TOOSMALL;
		goto cleanup;
	}
	if (rem && (rem < overhead+50)) {
		super->s_blocks_count -= rem;
		goto retry;
	}

	/*
	 * At this point we know how big the filesystem will be.  So
	 * we can do any and all allocations that depend on the block
	 * count.
	 */

	retval = ext2fs_get_mem(strlen(fs->device_name) + 80, &buf);
	if (retval)
		goto cleanup;

	strcpy(buf, "block bitmap for ");
	strcat(buf, fs->device_name);
	retval = ext2fs_allocate_block_bitmap(fs, buf, &fs->block_map);
	if (retval)
		goto cleanup;

	strcpy(buf, "inode bitmap for ");
	strcat(buf, fs->device_name);
	retval = ext2fs_allocate_inode_bitmap(fs, buf, &fs->inode_map);
	if (retval)
		goto cleanup;

	ext2fs_free_mem(&buf);

	retval = ext2fs_get_array(fs->desc_blocks, fs->blocksize,
				&fs->group_desc);
	if (retval)
		goto cleanup;

	memset(fs->group_desc, 0, (size_t) fs->desc_blocks * fs->blocksize);

	/*
	 * Reserve the superblock and group descriptors for each
	 * group, and fill in the correct group statistics for group.
	 * Note that although the block bitmap, inode bitmap, and
	 * inode table have not been allocated (and in fact won't be
	 * by this routine), they are accounted for nevertheless.
	 *
	 * If FLEX_BG meta-data grouping is used, only account for the
	 * superblock and group descriptors (the inode tables and
	 * bitmaps will be accounted for when allocated).
	 */
	super->s_free_blocks_count = 0;
	csum_flag = EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
					       EXT4_FEATURE_RO_COMPAT_GDT_CSUM);
	for (i = 0; i < fs->group_desc_count; i++) {
		/*
		 * Don't set the BLOCK_UNINIT group for the last group
		 * because the block bitmap needs to be padded.
		 */
		if (csum_flag) {
			if (i != fs->group_desc_count - 1)
				fs->group_desc[i].bg_flags |=
					EXT2_BG_BLOCK_UNINIT;
			fs->group_desc[i].bg_flags |= EXT2_BG_INODE_UNINIT;
			numblocks = super->s_inodes_per_group;
			if (i == 0)
				numblocks -= super->s_first_ino;
			fs->group_desc[i].bg_itable_unused = numblocks;
		}
		numblocks = ext2fs_reserve_super_and_bgd(fs, i, fs->block_map);
		if (fs->super->s_log_groups_per_flex)
			numblocks += 2 + fs->inode_blocks_per_group;

		super->s_free_blocks_count += numblocks;
		fs->group_desc[i].bg_free_blocks_count = numblocks;
		fs->group_desc[i].bg_free_inodes_count =
			fs->super->s_inodes_per_group;
		fs->group_desc[i].bg_used_dirs_count = 0;
		ext2fs_group_desc_csum_set(fs, i);
	}

	c = (char) 255;
	if (((int) c) == -1)
		super->s_flags |= EXT2_FLAGS_SIGNED_HASH;
	else
		super->s_flags |= EXT2_FLAGS_UNSIGNED_HASH;

	ext2fs_mark_super_dirty(fs);
	ext2fs_mark_bb_dirty(fs);
	ext2fs_mark_ib_dirty(fs);

	*ret_fs = fs;
	return 0;

cleanup:
	free(buf);
	ext2fs_free(fs);
	return retval;
}

errcode_t ext2fs_get_free_blocks(ext2_filsys fs, blk_t start, blk_t finish,
				 int num, ext2fs_block_bitmap map, blk_t *ret)
{
	blk_t	b = start;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!map)
		map = fs->block_map;
	if (!map)
		return EXT2_ET_NO_BLOCK_BITMAP;
	if (!b)
		b = fs->super->s_first_data_block;
	if (!finish)
		finish = start;
	if (!num)
		num = 1;
	do {
		if (b+num-1 > fs->super->s_blocks_count)
			b = fs->super->s_first_data_block;
		if (ext2fs_test_block_bitmap_range(map, b, num)) {
			*ret = b;
			return 0;
		}
		b++;
	} while (b != finish);
	return EXT2_ET_BLOCK_ALLOC_FAIL;
}

/* alloc_tables */

/*
 * This routine searches for free blocks that can allocate a full
 * group of bitmaps or inode tables for a flexbg group.  Returns the
 * block number with a correct offset were the bitmaps and inode
 * tables can be allocated continously and in order.
 */
static blk_t flexbg_offset(ext2_filsys fs, dgrp_t group, blk_t start_blk,
			   ext2fs_block_bitmap bmap, int offset, int size,
			   int elem_size)
{
	int		flexbg, flexbg_size;
	blk_t		last_blk, first_free = 0;
	dgrp_t		last_grp;

	flexbg_size = 1 << fs->super->s_log_groups_per_flex;
	flexbg = group / flexbg_size;

	if (size > (int) (fs->super->s_blocks_per_group / 8))
		size = (int) fs->super->s_blocks_per_group / 8;

	if (offset)
		offset -= 1;

	/*
	 * Don't do a long search if the previous block
	 * search is still valid.
	 */
	if (start_blk && group % flexbg_size) {
		if (ext2fs_test_block_bitmap_range(bmap, start_blk + elem_size,
						   size))
			return start_blk + elem_size;
	}

	start_blk = ext2fs_group_first_block(fs, flexbg_size * flexbg);
	last_grp = group | (flexbg_size - 1);
	if (last_grp > fs->group_desc_count)
		last_grp = fs->group_desc_count;
	last_blk = ext2fs_group_last_block(fs, last_grp);

	/* Find the first available block */
	if (ext2fs_get_free_blocks(fs, start_blk, last_blk, 1, bmap,
				   &first_free))
		return first_free;

	if (ext2fs_get_free_blocks(fs, first_free + offset, last_blk, size,
				   bmap, &first_free))
		return first_free;

	return first_free;
}

errcode_t ext2fs_allocate_group_table(ext2_filsys fs, dgrp_t group,
				      ext2fs_block_bitmap bmap)
{
	errcode_t	retval;
	blk_t		group_blk, start_blk, last_blk, new_blk, blk;
	dgrp_t		last_grp = 0;
	int		j, rem_grps = 0, flexbg_size = 0;

	group_blk = ext2fs_group_first_block(fs, group);
	last_blk = ext2fs_group_last_block(fs, group);

	if (!bmap)
		bmap = fs->block_map;

	if (EXT2_HAS_INCOMPAT_FEATURE(fs->super,
				      EXT4_FEATURE_INCOMPAT_FLEX_BG) &&
	    fs->super->s_log_groups_per_flex) {
		flexbg_size = 1 << fs->super->s_log_groups_per_flex;
		last_grp = group | (flexbg_size - 1);
		rem_grps = last_grp - group;
		if (last_grp > fs->group_desc_count)
			last_grp = fs->group_desc_count;
	}

	/*
	 * Allocate the block and inode bitmaps, if necessary
	 */
	if (fs->stride) {
		retval = ext2fs_get_free_blocks(fs, group_blk, last_blk,
						1, bmap, &start_blk);
		if (retval)
			return retval;
		start_blk += fs->inode_blocks_per_group;
		start_blk += ((fs->stride * group) %
			      (last_blk - start_blk + 1));
		if (start_blk >= last_blk)
			start_blk = group_blk;
	} else {
		start_blk = group_blk;
	}

	if (flexbg_size) {
		blk_t prev_block = 0;
		if (group && fs->group_desc[group-1].bg_block_bitmap)
			prev_block = fs->group_desc[group-1].bg_block_bitmap;

		start_blk = flexbg_offset(fs, group, prev_block, bmap,
						 0, rem_grps, 1);
		last_blk = ext2fs_group_last_block(fs, last_grp);
	}

	if (!fs->group_desc[group].bg_block_bitmap) {
		retval = ext2fs_get_free_blocks(fs, start_blk, last_blk,
						1, bmap, &new_blk);
		if (retval == EXT2_ET_BLOCK_ALLOC_FAIL)
			retval = ext2fs_get_free_blocks(fs, group_blk,
					last_blk, 1, bmap, &new_blk);
		if (retval)
			return retval;
		ext2fs_mark_block_bitmap(bmap, new_blk);
		fs->group_desc[group].bg_block_bitmap = new_blk;
		if (flexbg_size) {
			dgrp_t gr = ext2fs_group_of_blk(fs, new_blk);
			fs->group_desc[gr].bg_free_blocks_count--;
			fs->super->s_free_blocks_count--;
			fs->group_desc[gr].bg_flags &= ~EXT2_BG_BLOCK_UNINIT;
			ext2fs_group_desc_csum_set(fs, gr);
		}
	}

	if (flexbg_size) {
		blk_t prev_block = 0;
		if (group && fs->group_desc[group-1].bg_inode_bitmap)
			prev_block = fs->group_desc[group-1].bg_inode_bitmap;
		start_blk = flexbg_offset(fs, group, prev_block, bmap,
						 flexbg_size, rem_grps, 1);
		last_blk = ext2fs_group_last_block(fs, last_grp);
	}

	if (!fs->group_desc[group].bg_inode_bitmap) {
		retval = ext2fs_get_free_blocks(fs, start_blk, last_blk,
						1, bmap, &new_blk);
		if (retval == EXT2_ET_BLOCK_ALLOC_FAIL)
			retval = ext2fs_get_free_blocks(fs, group_blk,
					last_blk, 1, bmap, &new_blk);
		if (retval)
			return retval;
		ext2fs_mark_block_bitmap(bmap, new_blk);
		fs->group_desc[group].bg_inode_bitmap = new_blk;
		if (flexbg_size) {
			dgrp_t gr = ext2fs_group_of_blk(fs, new_blk);
			fs->group_desc[gr].bg_free_blocks_count--;
			fs->super->s_free_blocks_count--;
			fs->group_desc[gr].bg_flags &= ~EXT2_BG_BLOCK_UNINIT;
			ext2fs_group_desc_csum_set(fs, gr);
		}
	}

	/*
	 * Allocate the inode table
	 */
	if (flexbg_size) {
		blk_t prev_block = 0;
		if (group && fs->group_desc[group-1].bg_inode_table)
			prev_block = fs->group_desc[group-1].bg_inode_table;
		if (last_grp == fs->group_desc_count)
			rem_grps = last_grp - group;
		group_blk = flexbg_offset(fs, group, prev_block, bmap,
						 flexbg_size * 2,
						 fs->inode_blocks_per_group *
						 rem_grps,
						 fs->inode_blocks_per_group);
		last_blk = ext2fs_group_last_block(fs, last_grp);
	}

	if (!fs->group_desc[group].bg_inode_table) {
		retval = ext2fs_get_free_blocks(fs, group_blk, last_blk,
						fs->inode_blocks_per_group,
						bmap, &new_blk);
		if (retval)
			return retval;
		for (j = 0, blk = new_blk;
		     j < fs->inode_blocks_per_group;
		     j++, blk++) {
			ext2fs_mark_block_bitmap(bmap, blk);
			if (flexbg_size) {
				dgrp_t gr = ext2fs_group_of_blk(fs, blk);
				fs->group_desc[gr].bg_free_blocks_count--;
				fs->super->s_free_blocks_count--;
				fs->group_desc[gr].bg_flags &=
							~EXT2_BG_BLOCK_UNINIT;
				ext2fs_group_desc_csum_set(fs, gr);
			}
		}
		fs->group_desc[group].bg_inode_table = new_blk;
	}

	ext2fs_group_desc_csum_set(fs, group);
	return 0;
}

errcode_t ext2fs_allocate_tables(ext2_filsys fs)
{
	errcode_t	retval;
	dgrp_t		i;

	for (i = 0; i < fs->group_desc_count; i++) {
		retval = ext2fs_allocate_group_table(fs, i, fs->block_map);
		if (retval)
			return retval;
	}
	return 0;
}

/*
 * Convenience function which zeros out _num_ blocks starting at
 * _blk_.  In case of an error, the details of the error is returned
 * via _ret_blk_ and _ret_count_ if they are non-NULL pointers.
 * Returns 0 on success, and an error code on an error.
 *
 * As a special case, if the first argument is NULL, then it will
 * attempt to free the static zeroizing buffer.  (This is to keep
 * programs that check for memory leaks happy.)
 */
#define STRIDE_LENGTH 128

errcode_t ext2fs_zero_blocks(ext2_filsys fs, blk_t blk, int num,
			      blk_t *ret_blk, int *ret_count,
			      block_dev_desc_t *dev_desc)
{
	int		j, count;
	static char	*buf;

	/* If fs is null, clean up the static buffer and return */
	if (!fs) {
		if (buf) {
			free(buf);
			buf = 0;
		}
		return 0;
	}
	/* Allocate the zeroizing buffer if necessary */
	if (!buf) {
		buf = malloc(fs->blocksize * STRIDE_LENGTH);
		if (!buf) {
			printf("no memory\n");
			return -1;
		}
		memset(buf, 0, fs->blocksize * STRIDE_LENGTH);
	}

	/* OK, do the write loop */
	j = 0;
	while (j < num) {
		if (blk % STRIDE_LENGTH) {
			count = STRIDE_LENGTH - (blk % STRIDE_LENGTH);
			if (count > (num - j))
				count = num - j;
		} else {
			count = num - j;
			if (count > STRIDE_LENGTH)
				count = STRIDE_LENGTH;
		}
		put_ext4fs(dev_desc, (uint64_t)(blk * fs->blocksize),
			   buf, (uint32_t)(count * fs->blocksize));
		j += count; blk += count;
	}
	if (buf) {
		free(buf);
		buf = NULL;
	}
	return 0;
}

/* mkdir */

/*
 * Check for uninit block bitmaps and deal with them appropriately
 */
static void check_block_uninit(ext2_filsys fs, ext2fs_block_bitmap map,
			  dgrp_t group)
{
	blk_t		i;
	blk_t		blk, super_blk, old_desc_blk, new_desc_blk;
	int		old_desc_blocks;

	if (!(EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
					 EXT4_FEATURE_RO_COMPAT_GDT_CSUM)) ||
	    !(fs->group_desc[group].bg_flags & EXT2_BG_BLOCK_UNINIT))
		return;

	blk = (group * fs->super->s_blocks_per_group) +
		fs->super->s_first_data_block;

	ext2fs_super_and_bgd_loc(fs, group, &super_blk,
				 &old_desc_blk, &new_desc_blk, 0);

	if (fs->super->s_feature_incompat &
	    EXT2_FEATURE_INCOMPAT_META_BG)
		old_desc_blocks = fs->super->s_first_meta_bg;
	else
		old_desc_blocks = fs->desc_blocks +
					fs->super->s_reserved_gdt_blocks;

	for (i = 0; i < fs->super->s_blocks_per_group; i++, blk++) {
		if ((blk == super_blk) ||
		    (old_desc_blk && old_desc_blocks &&
		     (blk >= old_desc_blk) &&
		     (blk < old_desc_blk + old_desc_blocks)) ||
		    (new_desc_blk && (blk == new_desc_blk)) ||
		    (blk == fs->group_desc[group].bg_block_bitmap) ||
		    (blk == fs->group_desc[group].bg_inode_bitmap) ||
		    (blk >= fs->group_desc[group].bg_inode_table &&
		     (blk < fs->group_desc[group].bg_inode_table
		      + fs->inode_blocks_per_group)))
			ext2fs_mark_block_bitmap(map, blk);
		else
			ext2fs_fast_unmark_block_bitmap(map, blk);
	}
	fs->group_desc[group].bg_flags &= ~EXT2_BG_BLOCK_UNINIT;
	ext2fs_group_desc_csum_set(fs, group);
}

/*
 * Check for uninit inode bitmaps and deal with them appropriately
 */
static void check_inode_uninit(ext2_filsys fs, ext2fs_inode_bitmap map,
			  dgrp_t group)
{
	ext2_ino_t	i, ino;

	if (!(EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
					 EXT4_FEATURE_RO_COMPAT_GDT_CSUM)) ||
	    !(fs->group_desc[group].bg_flags & EXT2_BG_INODE_UNINIT))
		return;

	ino = (group * fs->super->s_inodes_per_group) + 1;
	for (i = 0; i < fs->super->s_inodes_per_group; i++, ino++)
		ext2fs_unmark_inode_bitmap(map, ino);

	fs->group_desc[group].bg_flags &= ~EXT2_BG_INODE_UNINIT;
	check_block_uninit(fs, fs->block_map, group);
}

/*
 * Right now, just search forward from the parent directory's block
 * group to find the next free inode.
 *
 * Should have a special policy for directories.
 */
errcode_t ext2fs_new_inode(ext2_filsys fs, ext2_ino_t dir,
			   int mode, ext2fs_inode_bitmap map, ext2_ino_t *ret)
{
	ext2_ino_t	dir_group = 0;
	ext2_ino_t	i;
	ext2_ino_t	start_inode;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!map)
		map = fs->inode_map;
	if (!map)
		return EXT2_ET_NO_INODE_BITMAP;

	if (dir > 0)
		dir_group = (dir - 1) / EXT2_INODES_PER_GROUP(fs->super);

	start_inode = (dir_group * EXT2_INODES_PER_GROUP(fs->super)) + 1;
	if (start_inode < EXT2_FIRST_INODE(fs->super))
		start_inode = EXT2_FIRST_INODE(fs->super);
	if (start_inode > fs->super->s_inodes_count)
		return EXT2_ET_INODE_ALLOC_FAIL;
	i = start_inode;

	do {
		if (((i - 1) % EXT2_INODES_PER_GROUP(fs->super)) == 0)
			check_inode_uninit(fs, map, (i - 1) /
					   EXT2_INODES_PER_GROUP(fs->super));

		if (!ext2fs_fast_test_inode_bitmap(map, i))
			break;
		i++;
		if (i > fs->super->s_inodes_count)
			i = EXT2_FIRST_INODE(fs->super);
	} while (i != start_inode);

	if (ext2fs_fast_test_inode_bitmap(map, i))
		return EXT2_ET_INODE_ALLOC_FAIL;
	*ret = i;
	return 0;
}

/*
 * Stupid algorithm --- we now just search forward starting from the
 * goal.  Should put in a smarter one someday....
 */
errcode_t ext2fs_new_block(ext2_filsys fs, blk_t goal,
			   ext2fs_block_bitmap map, blk_t *ret)
{
	blk_t	i;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!map)
		map = fs->block_map;
	if (!map)
		return EXT2_ET_NO_BLOCK_BITMAP;
	if (!goal || (goal >= fs->super->s_blocks_count))
		goal = fs->super->s_first_data_block;
	i = goal;
	check_block_uninit(fs, map,
			   (i - fs->super->s_first_data_block) /
			   EXT2_BLOCKS_PER_GROUP(fs->super));
	do {
		if (((i - fs->super->s_first_data_block) %
		     EXT2_BLOCKS_PER_GROUP(fs->super)) == 0)
			check_block_uninit(fs, map,
					   (i - fs->super->s_first_data_block) /
					   EXT2_BLOCKS_PER_GROUP(fs->super));

		if (!ext2fs_fast_test_block_bitmap(map, i)) {
			*ret = i;
			return 0;
		}
		i++;
		if (i >= fs->super->s_blocks_count)
			i = fs->super->s_first_data_block;
	} while (i != goal);
	return EXT2_ET_BLOCK_ALLOC_FAIL;
}

/*
 * Create new directory block
 */
errcode_t ext2fs_new_dir_block(ext2_filsys fs, ext2_ino_t dir_ino,
			       ext2_ino_t parent_ino, char **block)
{
	struct ext2_dir_entry	*dir = NULL;
	errcode_t		retval;
	char			*buf;
	int			rec_len;
	int			filetype = 0;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	retval = ext2fs_get_mem(fs->blocksize, &buf);
	if (retval)
		return retval;
	memset(buf, 0, fs->blocksize);
	dir = (struct ext2_dir_entry *)buf;

	retval = ext2fs_set_rec_len(fs, fs->blocksize, dir);
	if (retval)
		return retval;

	if (dir_ino) {
		if (fs->super->s_feature_incompat &
		    EXT2_FEATURE_INCOMPAT_FILETYPE)
			filetype = EXT2_FT_DIR << 8;
		/*
		 * Set up entry for '.'
		 */
		dir->inode = dir_ino;
		dir->name_len = 1 | filetype;
		dir->name[0] = '.';
		rec_len = fs->blocksize - EXT2_DIR_REC_LEN(1);
		dir->rec_len = EXT2_DIR_REC_LEN(1);

		/*
		 * Set up entry for '..'
		 */
		dir = (struct ext2_dir_entry *)(buf + dir->rec_len);
		retval = ext2fs_set_rec_len(fs, rec_len, dir);
		if (retval)
			return retval;
		dir->inode = parent_ino;
		dir->name_len = 2 | filetype;
		dir->name[0] = '.';
		dir->name[1] = '.';
	}
	*block = buf;
	return 0;
}

/*
 * This routine flushes the icache, if it exists.
 */
errcode_t ext2fs_flush_icache(ext2_filsys fs)
{
	int	i;

	if (!fs->icache)
		return 0;

	for (i = 0; i < fs->icache->cache_size; i++)
		fs->icache->cache[i].ino = 0;

	fs->icache->buffer_blk = 0;
	return 0;
}

static errcode_t create_icache(ext2_filsys fs)
{
	errcode_t	retval;

	if (fs->icache)
		return 0;
	retval = ext2fs_get_mem(sizeof(struct ext2_inode_cache), &fs->icache);
	if (retval)
		return retval;

	memset(fs->icache, 0, sizeof(struct ext2_inode_cache));
	retval = ext2fs_get_mem(fs->blocksize, &fs->icache->buffer);
	if (retval) {
		ext2fs_free_mem(&fs->icache);
		return retval;
	}
	fs->icache->buffer_blk = 0;
	fs->icache->cache_last = -1;
	fs->icache->cache_size = 4;
	fs->icache->refcount = 1;
	retval = ext2fs_get_array(fs->icache->cache_size,
				  sizeof(struct ext2_inode_cache_ent),
				  &fs->icache->cache);
	if (retval) {
		ext2fs_free_mem(&fs->icache->buffer);
		ext2fs_free_mem(&fs->icache);
		return retval;
	}
	ext2fs_flush_icache(fs);
	return 0;
}

/*
 * Functions to read and write a single inode.
 */
errcode_t ext2fs_read_inode_full(ext2_filsys fs, ext2_ino_t ino,
				 struct ext2fs_inode *inode, int bufsize)
{
	unsigned long	group, block, block_nr, offset;
	char		*ptr;
	errcode_t	retval;
	int		clen, i, length;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if ((ino == 0) || (ino > fs->super->s_inodes_count))
		return EXT2_ET_BAD_INODE_NUM;

	/* Create inode cache if not present */
	if (!fs->icache) {
		retval = create_icache(fs);
		if (retval)
			return retval;
	}
	/* Check to see if it's in the inode cache */
	if (bufsize == sizeof(struct ext2fs_inode)) {
		/* only old good inode can be retrieved from the cache */
		for (i = 0; i < fs->icache->cache_size; i++) {
			if (fs->icache->cache[i].ino == ino) {
				*inode = fs->icache->cache[i].inode;
				return 0;
			}
		}
	}

	group = (ino - 1) / EXT2_INODES_PER_GROUP(fs->super);

	if (group > fs->group_desc_count)
		return EXT2_ET_BAD_INODE_NUM;
	offset = ((ino - 1) % EXT2_INODES_PER_GROUP(fs->super)) *
		EXT2_INODE_SIZE(fs->super);

	block = offset >> EXT2_BLOCK_SIZE_BITS(fs->super);
	if (!fs->group_desc[(unsigned)group].bg_inode_table)
		return EXT2_ET_MISSING_INODE_TABLE;
	block_nr = fs->group_desc[(unsigned)group].bg_inode_table + block;
	offset &= (EXT2_SB_BLOCK_SIZE(fs->super) - 1);

	length = EXT2_INODE_SIZE(fs->super);
	if (bufsize < length)
		length = bufsize;

	ptr = (char *)inode;
	while (length) {
		clen = length;
		if ((offset + length) > fs->blocksize)
			clen = fs->blocksize - offset;

		if (block_nr != fs->icache->buffer_blk) {
			int factor = fs->blocksize/512;
			retval = ext2fs_devread(block_nr * factor , 0,
					fs->blocksize, fs->icache->buffer);
			retval = 0;
			if (retval)
				return retval;
			fs->icache->buffer_blk = block_nr;
		}
		memcpy(ptr, ((char *)fs->icache->buffer) + (unsigned)offset,
		       clen);

		offset = 0;
		length -= clen;
		ptr += clen;
		block_nr++;
	}

	/* Update the inode cache */
	fs->icache->cache_last = (fs->icache->cache_last + 1) %
		fs->icache->cache_size;
	fs->icache->cache[fs->icache->cache_last].ino = ino;
	fs->icache->cache[fs->icache->cache_last].inode = *inode;
	return 0;
}

errcode_t ext2fs_read_inode(ext2_filsys fs, ext2_ino_t ino,
			    struct ext2fs_inode *inode)
{
	return ext2fs_read_inode_full(fs, ino, inode,
					sizeof(struct ext2fs_inode));
}

errcode_t ext2fs_write_inode_full(ext2_filsys fs, ext2_ino_t ino,
				  struct ext2fs_inode *inode, int bufsize)
{
	unsigned long group, block, block_nr, offset;
	errcode_t retval = 0;
	struct ext2_inode_large temp_inode, *w_inode;
	char *ptr;
	int clen, i, length;
	/*uma added */
	static int buffer_allocated;
	static unsigned int prev_bufsize;
	static int FLAG;
	if (prev_bufsize != fs->blocksize) {
		buffer_allocated = 0;
		FLAG = 0;
		if (my_buffer) {
			free(my_buffer);
			my_buffer = NULL;
		}
	}
	if (buffer_allocated == 0) {
		my_buffer = xzalloc(fs->blocksize);
		buffer_allocated++;
		prev_bufsize = fs->blocksize;
	}

	/*uma end */
	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	/* Check to see if user provided an override function */
	if (fs->write_inode) {
		retval = (fs->write_inode)(fs, ino, inode);
		if (retval != EXT2_ET_CALLBACK_NOTHANDLED)
			return retval;
	}

	/* Check to see if the inode cache needs to be updated */
	if (fs->icache) {
		for (i = 0; i < fs->icache->cache_size; i++) {
			if (fs->icache->cache[i].ino == ino) {
				fs->icache->cache[i].inode = *inode;
				break;
			}
		}
	} else {
		retval = create_icache(fs);
		if (retval)
			return retval;
	}

	if (!(fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if ((ino == 0) || (ino > fs->super->s_inodes_count))
		return EXT2_ET_BAD_INODE_NUM;

	length = bufsize;
	if (length < EXT2_INODE_SIZE(fs->super))
		length = EXT2_INODE_SIZE(fs->super);

	if (length > (int) sizeof(struct ext2_inode_large)) {
		w_inode = malloc(length);
		if (!w_inode)
			return 12;
	} else {
		w_inode = &temp_inode;
	}

	memset(w_inode, 0, length);

	memcpy(w_inode, inode, bufsize);

	group = (ino - 1) / EXT2_INODES_PER_GROUP(fs->super);
	offset = ((ino - 1) % EXT2_INODES_PER_GROUP(fs->super)) *
		EXT2_INODE_SIZE(fs->super);
	block = offset >> EXT2_BLOCK_SIZE_BITS(fs->super);
	if (!fs->group_desc[(unsigned) group].bg_inode_table) {
		retval = EXT2_ET_MISSING_INODE_TABLE;
		goto errout;
	}
	block_nr = fs->group_desc[(unsigned) group].bg_inode_table + block;
	offset &= (EXT2_SB_BLOCK_SIZE(fs->super) - 1);
	length = EXT2_INODE_SIZE(fs->super);

	if (length > bufsize)
		length = bufsize;

	ptr = (char *)w_inode;

	while (length) {
		clen = length;
		if ((offset + length) > fs->blocksize)
			clen = fs->blocksize - offset;

		if (fs->icache->buffer_blk != block_nr) {
			int factor = fs->blocksize/512;
			retval = ext2fs_devread(block_nr * factor , 0,
					fs->blocksize, fs->icache->buffer);
			retval = 0;
			if (retval)
				goto errout;
			fs->icache->buffer_blk = block_nr;
		}

		memcpy((char *)fs->icache->buffer + (unsigned)offset,
		       ptr, clen);

		if (ino == 8) {
			FLAG++;
			if (journal_broken_cache_exit &&
			    FLAG >= my_journal_blocks) {
				memcpy(my_buffer, fs->icache->buffer,
				       fs->blocksize);
				my_block_nr = block_nr;
				/*Fix for format of same blocksize twice */
				FLAG = 0;
				buffer_allocated = 0;
				journal_broken_cache_exit = 0;
			}
		} else {
			put_ext4fs(dev_desc,
				   (uint64_t)(block_nr * fs->blocksize),
				   fs->icache->buffer,
				   (uint32_t)(fs->blocksize));
		}

		retval = 0;
		offset = 0;
		ptr += clen;
		length -= clen;
		block_nr++;
	}

	fs->flags |= EXT2_FLAG_CHANGED;
errout:
	if (w_inode && w_inode != &temp_inode)
		free(w_inode);
	return retval;
}

errcode_t ext2fs_write_inode(ext2_filsys fs, ext2_ino_t ino,
			     struct ext2fs_inode *inode)
{
	return ext2fs_write_inode_full(fs, ino, inode,
				       sizeof(struct ext2fs_inode));
}

/*
 * This function should be called when writing a new inode.  It makes
 * sure that extra part of large inodes is initialized properly.
 */
errcode_t ext2fs_write_new_inode(ext2_filsys fs, ext2_ino_t ino,
				 struct ext2fs_inode *inode)
{
	struct ext2fs_inode	*buf;
	int			size = EXT2_INODE_SIZE(fs->super);
	struct ext2_inode_large	*large_inode;
	errcode_t		retval;
	__u32			t = fs->now ? fs->now : (__u32) NULL;

	if (!inode->i_ctime)
		inode->i_ctime = t;
	if (!inode->i_mtime)
		inode->i_mtime = t;
	if (!inode->i_atime)
		inode->i_atime = t;

	if (size == sizeof(struct ext2fs_inode))
		return ext2fs_write_inode_full(fs, ino, inode,
					       sizeof(struct ext2fs_inode));

	buf = malloc(size);
	if (!buf)
		return 12;

	memset(buf, 0, size);
	*buf = *inode;

	large_inode = (struct ext2_inode_large *)buf;
	large_inode->i_extra_isize = sizeof(struct ext2_inode_large) -
		EXT2_GOOD_OLD_INODE_SIZE;
	if (!large_inode->i_crtime)
		large_inode->i_crtime = t;

	retval = ext2fs_write_inode_full(fs, ino, buf, size);
	free(buf);
	return retval;
}

errcode_t ext2fs_iblk_set(ext2_filsys fs, struct ext2fs_inode *inode, blk64_t b)
{
	if (!(fs->super->s_feature_ro_compat &
	      EXT4_FEATURE_RO_COMPAT_HUGE_FILE) ||
	    !(inode->i_flags & EXT4_HUGE_FILE_FL))
		b *= fs->blocksize / 512;

	inode->i_blocks = b & 0xFFFFFFFF;
	if (fs->super->s_feature_ro_compat & EXT4_FEATURE_RO_COMPAT_HUGE_FILE)
		inode->osd2.linux2.l_i_blocks_hi = b >> 32;
	else if (b >> 32)
		return 75;
	return 0;
}


errcode_t ext2fs_write_dir_block2(ext2_filsys fs, blk_t block,
				  void *inbuf, int flags)
{
	put_ext4fs(dev_desc, (uint64_t)(block * fs->blocksize),
		   inbuf, (uint32_t)(fs->blocksize));
	return 0;
}


errcode_t ext2fs_write_dir_block(ext2_filsys fs, blk_t block,
				 void *inbuf)
{
	return ext2fs_write_dir_block2(fs, block, inbuf, 0);
}

errcode_t ext2fs_read_dir_block2(ext2_filsys fs, blk_t block,
				 void *buf, int flags)
{
	errcode_t	retval;
	char		*p, *end;
	struct ext2_dir_entry *dirent;
	unsigned int	name_len, rec_len;

	int factor = fs->blocksize/512;
	retval = ext2fs_devread(block * factor , 0, fs->blocksize, buf);
	retval = 0;
	if (retval)
		return retval;

	p = (char *)buf;
	end = (char *)buf + fs->blocksize;
	while (p < end-8) {
		dirent = (struct ext2_dir_entry *)p;
		name_len = dirent->name_len;

		retval = ext2fs_get_rec_len(fs, dirent, &rec_len);

		if (retval != 0)
			return retval;
		if ((rec_len < 8) || (rec_len % 4)) {
			rec_len = 8;
			retval = EXT2_ET_DIR_CORRUPTED;
		} else if (((name_len & 0xFF) + 8) > rec_len) {
			retval = EXT2_ET_DIR_CORRUPTED;
		}
		p += rec_len;
	}
	return retval;
}

errcode_t ext2fs_read_dir_block(ext2_filsys fs, blk_t block,
				 void *buf)
{
	return ext2fs_read_dir_block2(fs, block, buf, 0);
}

void ext2fs_inode_alloc_stats2(ext2_filsys fs, ext2_ino_t ino,
			       int inuse, int isdir)
{
	int	group = ext2fs_group_of_ino(fs, ino);
#ifndef OMIT_COM_ERR
	if (ino > fs->super->s_inodes_count) {
		printf("ext2fs_inode_alloc_stats2 Illegal ");
		printf("inode number: %lu", (unsigned long) ino);
		return;
	}
#endif
	if (inuse > 0)
		ext2fs_mark_inode_bitmap(fs->inode_map, ino);
	else
		ext2fs_unmark_inode_bitmap(fs->inode_map, ino);
	fs->group_desc[group].bg_free_inodes_count -= inuse;
	if (isdir)
		fs->group_desc[group].bg_used_dirs_count += inuse;

	/* We don't strictly need to be clearing the uninit flag if inuse < 0
	 * (i.e. freeing inodes) but it also means something is bad. */
	fs->group_desc[group].bg_flags &= ~EXT2_BG_INODE_UNINIT;
	if (EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
				       EXT4_FEATURE_RO_COMPAT_GDT_CSUM)) {
		ext2_ino_t first_unused_inode =	fs->super->s_inodes_per_group -
			fs->group_desc[group].bg_itable_unused +
			group * fs->super->s_inodes_per_group + 1;
		if (ino >= first_unused_inode)
			fs->group_desc[group].bg_itable_unused =
				group * fs->super->s_inodes_per_group +
				fs->super->s_inodes_per_group - ino;
		ext2fs_group_desc_csum_set(fs, group);
	}

	fs->super->s_free_inodes_count -= inuse;
	ext2fs_mark_super_dirty(fs);
	ext2fs_mark_ib_dirty(fs);
}

void ext2fs_inode_alloc_stats(ext2_filsys fs, ext2_ino_t ino, int inuse)
{
	ext2fs_inode_alloc_stats2(fs, ino, inuse, 0);
}

void ext2fs_block_alloc_stats(ext2_filsys fs, blk_t blk, int inuse)
{
	int	group = ext2fs_group_of_blk(fs, blk);

#ifndef OMIT_COM_ERR
	if (blk >= fs->super->s_blocks_count) {
		printf("ext2fs_block_alloc_stats Illegal ");
		printf("block number: %lu", (unsigned long) blk);
		return;
	}
#endif
	if (inuse > 0)
		ext2fs_mark_block_bitmap(fs->block_map, blk);
	else
		ext2fs_unmark_block_bitmap(fs->block_map, blk);
	fs->group_desc[group].bg_free_blocks_count -= inuse;
	fs->group_desc[group].bg_flags &= ~EXT2_BG_BLOCK_UNINIT;
	ext2fs_group_desc_csum_set(fs, group);

	fs->super->s_free_blocks_count -= inuse;
	ext2fs_mark_super_dirty(fs);
	ext2fs_mark_bb_dirty(fs);
	if (fs->block_alloc_stats)
		(fs->block_alloc_stats)(fs, (blk64_t) blk, inuse);
}

#ifndef EXT2_FT_DIR
#define EXT2_FT_DIR		2
#endif

errcode_t ext2fs_mkdir(ext2_filsys fs, ext2_ino_t parent, ext2_ino_t inum,
		       const char *name)
{
	errcode_t		retval;
	struct ext2fs_inode	parent_inode, inode;
	ext2_ino_t		ino = inum;
	ext2_ino_t		scratch_ino;
	blk_t			blk;
	char			*block = 0;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	/*
	 * Allocate an inode, if necessary
	 */
	if (!ino) {
		retval = ext2fs_new_inode(fs, parent, LINUX_S_IFDIR | 0755,
					  0, &ino);
		if (retval)
			goto cleanup;
	}

	/*
	 * Allocate a data block for the directory
	 */
	retval = ext2fs_new_block(fs, 0, 0, &blk);
	if (retval)
		goto cleanup;

	/*
	 * Create a scratch template for the directory
	 */
	retval = ext2fs_new_dir_block(fs, ino, parent, &block);
	if (retval)
		goto cleanup;

	/*
	 * Get the parent's inode, if necessary
	 */
	if (parent != ino) {
		retval = ext2fs_read_inode(fs, parent, &parent_inode);
		if (retval)
			goto cleanup;
	} else {
		memset(&parent_inode, 0, sizeof(parent_inode));
	}

	/*
	 * Create the inode structure....
	 */
	memset(&inode, 0, sizeof(struct ext2fs_inode));
	inode.i_mode = LINUX_S_IFDIR | (0777 & ~fs->umask);
	inode.i_uid = 0;
	inode.i_gid = 0;
	ext2fs_iblk_set(fs, &inode, 1);
	inode.i_block[0] = blk;
	inode.i_links_count = 2;
	inode.i_size = fs->blocksize;

	/*
	 * Write out the inode and inode data block
	 */
	retval = ext2fs_write_dir_block(fs, blk, block);
	if (retval)
		goto cleanup;
	retval = ext2fs_write_new_inode(fs, ino, &inode);
	if (retval)
		goto cleanup;

	/*
	 * Link the directory into the filesystem hierarchy
	 */
	if (name) {
		retval = ext2fs_lookup(fs, parent, name, strlen(name), 0,
				       &scratch_ino);
		if (!retval) {
			retval = EXT2_ET_DIR_EXISTS;
			name = 0;
			goto cleanup;
		}
		if (retval != EXT2_ET_FILE_NOT_FOUND)
			goto cleanup;
		retval = ext2fs_link(fs, parent, name, ino, EXT2_FT_DIR);
		if (retval)
			goto cleanup;
	}

	/*
	 * Update parent inode's counts
	 */
	if (parent != ino) {
		parent_inode.i_links_count++;
		retval = ext2fs_write_inode(fs, parent, &parent_inode);
		if (retval)
			goto cleanup;
	}

	/*
	 * Update accounting....
	 */
	ext2fs_block_alloc_stats(fs, blk, 1);
	ext2fs_inode_alloc_stats2(fs, ino, 1, 1);

cleanup:
	if (block)
		ext2fs_free_mem(&block);
	return retval;
}

/*
 * This function zeros out the allocated block, and updates all of the
 * appropriate filesystem records.
 */
errcode_t ext2fs_alloc_block(ext2_filsys fs, blk_t goal,
			     char *block_buf, blk_t *ret)
{
	errcode_t	retval;
	blk_t		block;
	char		*buf = 0;

	if (!block_buf) {
		retval = ext2fs_get_mem(fs->blocksize, &buf);
		if (retval)
			return retval;
		block_buf = buf;
	}
	memset(block_buf, 0, fs->blocksize);

	if (fs->get_alloc_block) {
		blk64_t	new;

		retval = (fs->get_alloc_block)(fs, (blk64_t) goal, &new);
		if (retval)
			goto fail;
		block = (blk_t) new;
	}

	retval = ext2fs_new_block(fs, goal, 0, &block);
	if (retval)
		goto fail;

	put_ext4fs(dev_desc, (uint64_t)(block * fs->blocksize),
		   block_buf, (uint32_t)(fs->blocksize));
	retval = 0;

	if (retval)
		goto fail;

	ext2fs_block_alloc_stats(fs, block, 1);
	*ret = block;

fail:
	if (buf)
		ext2fs_free_mem(&buf);
	return retval;
}

/*
 * Iterate through the groups which hold BACKUP superblock/GDT copies in an
 * ext3 filesystem.  The counters should be initialized to 1, 5, and 7 before
 * calling this for the first time.  In a sparse filesystem it will be the
 * sequence of powers of 3, 5, and 7: 1, 3, 5, 7, 9, 25, 27, 49, 81, ...
 * For a non-sparse filesystem it will be every group: 1, 2, 3, 4, ...
 */
static unsigned int list_backups(ext2_filsys fs, unsigned int *three,
				 unsigned int *five, unsigned int *seven)
{
	unsigned int *min = three;
	int mult = 3;
	unsigned int ret;

	if (!(fs->super->s_feature_ro_compat &
	      EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)) {
		ret = *min;
		*min += 1;
		return ret;
	}

	if (*five < *min) {
		min = five;
		mult = 5;
	}
	if (*seven < *min) {
		min = seven;
		mult = 7;
	}

	ret = *min;
	*min *= mult;

	return ret;
}

/*
 * This code assumes that the reserved blocks have already been marked in-use
 * during ext2fs_initialize(), so that they are not allocated for other
 * uses before we can add them to the resize inode (which has to come
 * after the creation of the inode table).
 */
errcode_t ext2fs_create_resize_inode(ext2_filsys fs)
{
	errcode_t		retval, retval2;
	struct ext2_super_block	*sb;
	struct ext2fs_inode	inode;
	__u32			*dindir_buf, *gdt_buf;
	unsigned long long	apb, inode_size;
	blk_t			dindir_blk, rsv_off, gdt_off, gdt_blk;
	int			dindir_dirty = 0, inode_dirty = 0;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	sb = fs->super;

	retval = ext2fs_get_array(2, fs->blocksize, &dindir_buf);
	if (retval)
		goto out_free;
	gdt_buf = (__u32 *)((char *)dindir_buf + fs->blocksize);

	retval = ext2fs_read_inode(fs, EXT2_RESIZE_INO, &inode);
	if (retval)
		goto out_free;

	/* Maximum possible file size (we donly use the dindirect blocks) */
	apb = EXT2_ADDR_PER_BLOCK(sb);

	dindir_blk = inode.i_block[EXT2_DIND_BLOCK];
	if (dindir_blk) {
		retval = ext2fs_read_ind_block(fs, dindir_blk, dindir_buf);
		if (retval)
			goto out_inode;
	} else {
		blk_t goal = sb->s_first_data_block + fs->desc_blocks +
			sb->s_reserved_gdt_blocks + 2 +
			fs->inode_blocks_per_group;

		retval = ext2fs_alloc_block(fs, goal, 0, &dindir_blk);
		if (retval)
			goto out_free;
		inode.i_mode = LINUX_S_IFREG | 0600;
		inode.i_links_count = 1;
		inode.i_block[EXT2_DIND_BLOCK] = dindir_blk;
		ext2fs_iblk_set(fs, &inode, 1);
		memset(dindir_buf, 0, fs->blocksize);

		dindir_dirty = 1;
		inode_dirty = 1;
		inode_size = apb*apb + apb + EXT2_NDIR_BLOCKS;
		inode_size *= fs->blocksize;
		inode.i_size = inode_size & 0xFFFFFFFF;
		inode.i_size_high = (inode_size >> 32) & 0xFFFFFFFF;
		if (inode.i_size_high) {
			sb->s_feature_ro_compat |=
				EXT2_FEATURE_RO_COMPAT_LARGE_FILE;
		}
		inode.i_ctime = fs->now ? fs->now : (__u32)NULL;
	}

	for (rsv_off = 0, gdt_off = fs->desc_blocks,
	     gdt_blk = sb->s_first_data_block + 1 + fs->desc_blocks;
	     rsv_off < sb->s_reserved_gdt_blocks;
	     rsv_off++, gdt_off++, gdt_blk++) {
		unsigned int three = 1, five = 5, seven = 7;
		unsigned int grp, last = 0;
		int gdt_dirty = 0;

		gdt_off %= apb;
		if (!dindir_buf[gdt_off]) {
			gdt_dirty = 1;
			dindir_dirty = 1;
			inode_dirty = 1;
			memset(gdt_buf, 0, fs->blocksize);
			dindir_buf[gdt_off] = gdt_blk;
			ext2fs_iblk_add_blocks(fs, &inode, 1);
		} else if (dindir_buf[gdt_off] == gdt_blk) {
			retval = ext2fs_read_ind_block(fs, gdt_blk, gdt_buf);
			if (retval)
				goto out_dindir;
		} else {
			retval = EXT2_ET_RESIZE_INODE_CORRUPT;
			goto out_dindir;
		}

		while ((grp = list_backups(fs, &three, &five, &seven)) <
		       fs->group_desc_count) {
			blk_t expect = gdt_blk + grp * sb->s_blocks_per_group;

			if (!gdt_buf[last]) {
				gdt_buf[last] = expect;
				ext2fs_iblk_add_blocks(fs, &inode, 1);
				gdt_dirty = 1;
				inode_dirty = 1;
			} else if (gdt_buf[last] != expect) {
				retval = EXT2_ET_RESIZE_INODE_CORRUPT;
				goto out_dindir;
			}
			last++;
		}
		if (gdt_dirty) {
			retval = ext2fs_write_ind_block(fs, gdt_blk, gdt_buf);
			if (retval)
				goto out_dindir;
		}
	}

out_dindir:
	if (dindir_dirty) {
		retval2 = ext2fs_write_ind_block(fs, dindir_blk, dindir_buf);
		if (!retval)
			retval = retval2;
	}
out_inode:
	if (inode_dirty) {
		inode.i_atime = fs->now ? fs->now : (__u32)NULL;
		inode.i_mtime = fs->now ? fs->now : (__u32)NULL;
		retval2 = ext2fs_write_new_inode(fs, EXT2_RESIZE_INO, &inode);
		if (!retval)
			retval = retval2;
	}
out_free:
	ext2fs_free_mem(&dindir_buf);
	return retval;
}

/*
 * Helper function for creating the journal using direct I/O routines
 */
struct mkjournal_struct {
	int		num_blocks;
	int		newblocks;
	blk_t		goal;
	blk_t		blk_to_zero;
	int		zero_count;
	char		*buf;
	errcode_t	err;
};

static int mkjournal_proc(ext2_filsys	fs,
			   blk_t	*blocknr,
			   e2_blkcnt_t	blockcnt,
			   blk_t	ref_block,
			   int		ref_offset,
			   void		*priv_data)
{
	struct mkjournal_struct *es = (struct mkjournal_struct *)priv_data;
	blk_t	new_blk;
	errcode_t	retval;

	if (*blocknr) {
		es->goal = *blocknr;
		return 0;
	}
	retval = ext2fs_new_block(fs, es->goal, 0, &new_blk);
	if (retval) {
		es->err = retval;
		return BLOCK_ABORT;
	}

	if (blockcnt >= 0)
		es->num_blocks--;

	es->newblocks++;
	retval = 0;
	if (blockcnt <= 0) {
		put_ext4fs(dev_desc, (uint64_t)(new_blk * fs->blocksize),
			   es->buf, (uint32_t) fs->blocksize);
		retval = 0;
	} else {
		if (es->zero_count) {
			if ((es->blk_to_zero + es->zero_count == new_blk) &&
			    (es->zero_count < 1024)) {
					es->zero_count++;
			} else {
				retval = ext2fs_zero_blocks(fs,
							    es->blk_to_zero,
							    es->zero_count,
							    0, 0, dev_desc);
				es->zero_count = 0;
			}
		}
		if (es->zero_count == 0) {
			es->blk_to_zero = new_blk;
			es->zero_count = 1;
		}
	}

	if (blockcnt == 0)
		memset(es->buf, 0, fs->blocksize);

	if (retval) {
		es->err = retval;
		return BLOCK_ABORT;
	}
	*blocknr = es->goal = new_blk;
	ext2fs_block_alloc_stats(fs, new_blk, 1);
	if (es->num_blocks == 0)
		return BLOCK_CHANGED | BLOCK_ABORT;
	else
		return BLOCK_CHANGED;
}

/*
 * This function automatically sets up the journal superblock and
 * returns it as an allocated block.
 */
errcode_t ext2fs_create_journal_superblock(ext2_filsys fs,
					   __u32 size, int flags,
					   char  **ret_jsb)
{
	errcode_t		retval;
	struct journal_superblock_t	*jsb;

	if (size < 1024)
		return EXT2_ET_JOURNAL_TOO_SMALL;

	retval = ext2fs_get_mem(fs->blocksize, &jsb);
	if (retval)
		return retval;

	memset(jsb, 0, fs->blocksize);

	jsb->s_header.h_magic = htonl(JFS_MAGIC_NUMBER);
	if (flags & EXT2_MKJOURNAL_V1_SUPER)
		jsb->s_header.h_blocktype = htonl(JFS_SUPERBLOCK_V1);
	else
		jsb->s_header.h_blocktype = htonl(JFS_SUPERBLOCK_V2);
	jsb->s_blocksize = htonl(fs->blocksize);
	jsb->s_maxlen = htonl(size);
	jsb->s_nr_users = htonl(1);
	jsb->s_first = htonl(1);
	jsb->s_sequence = htonl(1);
	memcpy(jsb->s_uuid, fs->super->s_uuid, sizeof(fs->super->s_uuid));
	/*
	 * If we're creating an external journal device, we need to
	 * adjust these fields.
	 */
	if (fs->super->s_feature_incompat &
	    EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) {
		jsb->s_nr_users = 0;
		if (fs->blocksize == 1024)
			jsb->s_first = htonl(3);
		else
			jsb->s_first = htonl(2);
	}

	*ret_jsb = (char *)jsb;
	return 0;
}


/*
 * This function creates a journal using direct I/O routines.
 */
static errcode_t write_journal_inode(ext2_filsys fs, ext2_ino_t journal_ino,
				     blk_t size, int flags)
{
	char			*buf;
	dgrp_t			group, start, end, i, log_flex;
	errcode_t		retval;
	struct ext2fs_inode	inode;
	struct mkjournal_struct	es;


	retval = ext2fs_create_journal_superblock(fs, size, flags, &buf);
	if (retval)
		return retval;

	retval = ext2fs_read_inode(fs, journal_ino, &inode);
	if (retval)
		return retval;

	if (inode.i_blocks > 0)
		return 17;

	es.num_blocks = size;
	es.newblocks = 0;
	es.buf = buf;
	es.err = 0;
	es.zero_count = 0;

	if (fs->super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS) {
		inode.i_flags |= EXT4_EXTENTS_FL;
		retval = ext2fs_write_inode(fs, journal_ino, &inode);
		if (retval)
			return retval;
	}

	/*
	 * Set the initial goal block to be roughly at the middle of
	 * the filesystem.  Pick a group that has the largest number
	 * of free blocks.
	 */
	group = ext2fs_group_of_blk(fs, (fs->super->s_blocks_count -
					 fs->super->s_first_data_block) / 2);
	log_flex = 1 << fs->super->s_log_groups_per_flex;
	if (fs->super->s_log_groups_per_flex && (group > log_flex)) {
		group = group & ~(log_flex - 1);
		while ((group < fs->group_desc_count) &&
		       fs->group_desc[group].bg_free_blocks_count == 0)
			group++;
		if (group == fs->group_desc_count)
			group = 0;
		start = group;
	} else {
		start = (group > 0) ? group-1 : group;
	}
	end = ((group+1) < fs->group_desc_count) ? group+1 : group;
	group = start;
	for (i = start+1; i <= end; i++)
		if (fs->group_desc[i].bg_free_blocks_count >
		    fs->group_desc[group].bg_free_blocks_count)
			group = i;

	es.goal = (fs->super->s_blocks_per_group * group) +
		fs->super->s_first_data_block;
	retval = ext2fs_block_iterate2(fs, journal_ino, BLOCK_FLAG_APPEND,
				       0, mkjournal_proc, &es);
	if (es.err) {
		retval = es.err;
		goto errout;
	}
	if (es.zero_count) {
		retval = ext2fs_zero_blocks(fs, es.blk_to_zero,
					    es.zero_count, 0, 0, NULL);
		if (retval)
			goto errout;
	}

	retval = ext2fs_read_inode(fs, journal_ino, &inode);
	if (retval)
		goto errout;

	inode.i_size += fs->blocksize * size;
	ext2fs_iblk_add_blocks(fs, &inode, es.newblocks);
	inode.i_ctime = fs->now ? fs->now : (__u32)NULL;
	inode.i_mtime = fs->now ? fs->now : (__u32)NULL;
	inode.i_links_count = 1;
	inode.i_mode = LINUX_S_IFREG | 0600;

	retval = ext2fs_write_new_inode(fs, journal_ino, &inode);
	if (retval)
		goto errout;
	retval = 0;
	memcpy(fs->super->s_jnl_blocks, inode.i_block, EXT2_N_BLOCKS*4);
	fs->super->s_jnl_blocks[16] = inode.i_size;
	fs->super->s_jnl_backup_type = EXT3_JNL_BACKUP_BLOCKS;
	ext2fs_mark_super_dirty(fs);

errout:
	ext2fs_free_mem(&buf);
	return retval;
}

/*
 * This function adds a journal inode to a filesystem, using either
 * POSIX routines if the filesystem is mounted, or using direct I/O
 * functions if it is not.
 */
errcode_t ext2fs_add_journal_inode(ext2_filsys fs, blk_t size, int flags)
{
	errcode_t		retval;
	ext2_ino_t		journal_ino;

	journal_ino = EXT2_JOURNAL_INO;
	retval = write_journal_inode(fs, journal_ino, size, flags);
	if (retval)
		return retval;

	fs->super->s_journal_inum = journal_ino;
	fs->super->s_journal_dev = 0;
	memset(fs->super->s_journal_uuid, 0,
	       sizeof(fs->super->s_journal_uuid));

	fs->super->s_feature_compat |= EXT3_FEATURE_COMPAT_HAS_JOURNAL;

	ext2fs_mark_super_dirty(fs);
	return 0;
}

static errcode_t write_backup_super(ext2_filsys fs, dgrp_t group,
				    blk_t group_block,
				    struct ext2_super_block *super_shadow)
{
	dgrp_t	sgrp = group;
	int retval;

	if (sgrp > ((1 << 16) - 1))
		sgrp = (1 << 16) - 1;
	fs->super->s_block_group_nr = sgrp;

	put_ext4fs(dev_desc, (uint64_t)(group_block * fs->blocksize),
		   super_shadow, (uint32_t)(SUPERBLOCK_SIZE));
	retval = 0;
	return retval;
}

/*
 * This function forces out the primary superblock.  We need to only
 * write out those fields which we have changed, since if the
 * filesystem is mounted, it may have changed some of the other
 * fields.
 *
 * It takes as input a superblock which has already been byte swapped
 * (if necessary).
 *
 */
static errcode_t write_primary_superblock(ext2_filsys fs,
					  struct ext2_super_block *super)
{
	errcode_t	retval;

	put_ext4fs(dev_desc, (uint64_t)(1 * 1024),
		   super, (uint32_t)(SUPERBLOCK_SIZE));
	retval = 0;
	return retval;
}

errcode_t ext2fs_flush(ext2_filsys fs)
{
	dgrp_t		i;
	errcode_t	retval;
	unsigned long	fs_state;
	__u32		feature_incompat;
	struct ext2_super_block *super_shadow = 0;
	struct ext2_group_desc *group_shadow = 0;
	char	*group_ptr;
	int	old_desc_blocks;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	fs_state = fs->super->s_state;
	feature_incompat = fs->super->s_feature_incompat;

	fs->super->s_wtime = fs->now ? fs->now : (__u32)NULL;
	fs->super->s_block_group_nr = 0;
	super_shadow = fs->super;
	group_shadow = fs->group_desc;

	/*
	 * Set the state of the FS to be non-valid.  (The state has
	 * already been backed up earlier, and will be restored after
	 * we write out the backup superblocks.)
	 */
	fs->super->s_state &= ~EXT2_VALID_FS;
	fs->super->s_feature_incompat &= ~EXT3_FEATURE_INCOMPAT_RECOVER;

	/*
	 * If this is an external journal device, don't write out the
	 * block group descriptors or any of the backup superblocks
	 */
	if (fs->super->s_feature_incompat &
	    EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)
		goto write_primary_superblock_only;

	/*
	 * Write out the master group descriptors, and the backup
	 * superblocks and group descriptors.
	 */
	group_ptr = (char *)group_shadow;
	if (fs->super->s_feature_incompat & EXT2_FEATURE_INCOMPAT_META_BG)
		old_desc_blocks = fs->super->s_first_meta_bg;
	else
		old_desc_blocks = fs->desc_blocks;

	for (i = 0; i < fs->group_desc_count; i++) {
		blk_t	super_blk, old_desc_blk, new_desc_blk;
		int	meta_bg;

		ext2fs_super_and_bgd_loc(fs, i, &super_blk, &old_desc_blk,
					 &new_desc_blk, &meta_bg);

		if (!(fs->flags & EXT2_FLAG_MASTER_SB_ONLY) && i && super_blk) {
			retval = write_backup_super(fs, i, super_blk,
						    super_shadow);
			if (retval)
				goto errout;
		}
		if (fs->flags & EXT2_FLAG_SUPER_ONLY)
			continue;
		if ((old_desc_blk) &&
		    (!(fs->flags & EXT2_FLAG_MASTER_SB_ONLY) || (i == 0))) {
			put_ext4fs(dev_desc,
				   (uint64_t)(old_desc_blk * fs->blocksize),
				   group_ptr,
				   (uint32_t)(fs->blocksize * old_desc_blocks));
			retval = 0;

			if (retval)
				goto errout;
		}
		if (new_desc_blk) {
			put_ext4fs(dev_desc,
				   (uint64_t)(new_desc_blk * fs->blocksize),
				   group_ptr + (meta_bg*fs->blocksize),
				   (uint32_t)(fs->blocksize));

			retval = 0;
			if (retval)
				goto errout;
		}
	}

	if (fs->write_bitmaps) {
		retval = fs->write_bitmaps(fs);

		if (retval)
			goto errout;
	}

write_primary_superblock_only:
	/*
	 * Write out master superblock.  This has to be done
	 * separately, since it is located at a fixed location
	 * (SUPERBLOCK_OFFSET).  We flush all other pending changes
	 * out to disk first, just to avoid a race condition with an
	 * insy-tinsy window....
	 */

	fs->super->s_block_group_nr = 0;
	fs->super->s_state = fs_state;
	fs->super->s_feature_incompat = feature_incompat;

	retval = write_primary_superblock(fs, super_shadow);
	if (retval)
		goto errout;

	fs->flags &= ~EXT2_FLAG_DIRTY;
errout:
	fs->super->s_state = fs_state;
	return retval;
}

/* typedef struct struct_io_stats *io_stats; */
#define io_stats struct struct_io_stats*

struct struct_io_stats {
	int			num_fields;
	int			reserved;
	unsigned long long	bytes_read;
	unsigned long long	bytes_written;
};

errcode_t ext4fs_close_clean(ext2_filsys fs)
{
	errcode_t	retval;
	int		meta_blks;
	io_stats stats = 0;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);
	if (fs->write_bitmaps) {
		retval = fs->write_bitmaps(fs);
		if (retval)
			return retval;
	}
	if (stats && stats->bytes_written && (fs->flags & EXT2_FLAG_RW)) {
		fs->super->s_kbytes_written += stats->bytes_written >> 10;
		meta_blks = fs->desc_blocks + 1;
		if (!(fs->flags & EXT2_FLAG_SUPER_ONLY))
			fs->super->s_kbytes_written += meta_blks /
				(fs->blocksize / 1024);
		if ((fs->flags & EXT2_FLAG_DIRTY) == 0)
			fs->flags |= EXT2_FLAG_SUPER_ONLY | EXT2_FLAG_DIRTY;
	}

	if (fs->flags & EXT2_FLAG_DIRTY) {
		retval = ext2fs_flush(fs);
		if (retval)
			return retval;
	}

	ext2fs_free(fs);
	return 0;
}

