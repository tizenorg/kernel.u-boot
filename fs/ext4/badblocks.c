#include <common.h>
#include <ext_common.h>
#include <ext4fs.h>
#include "ext2fs.h"

/*
 * This procedure finds a particular block is on a badblocks
 * list.
 */
int ext2fs_u32_list_find(ext2_u32_list bb, __u32 blk)
{
	int	low, high, mid;

	if (bb->magic != EXT2_ET_MAGIC_BADBLOCKS_LIST)
		return -1;

	if (bb->num == 0)
		return -1;

	low = 0;
	high = bb->num-1;
	if (blk == bb->list[low])
		return low;
	if (blk == bb->list[high])
		return high;

	while (low < high) {
		mid = (low+high)/2;
		if (mid == low || mid == high)
			break;
		if (blk == bb->list[mid])
			return mid;
		if (blk < bb->list[mid])
			high = mid;
		else
			low = mid;
	}
	return -1;
}

/*
 * This procedure tests to see if a particular block is on a badblocks
 * list.
 */
int ext2fs_u32_list_test(ext2_u32_list bb, __u32 blk)
{
	if (ext2fs_u32_list_find(bb, blk) < 0)
		return 0;
	else
		return 1;
}

int ext2fs_badblocks_list_test(ext2_badblocks_list bb, blk_t blk)
{
	return ext2fs_u32_list_test((ext2_u32_list) bb, (__u32) blk);
}

errcode_t ext2fs_u32_list_iterate_begin(ext2_u32_list bb,
					ext2_u32_iterate *ret)
{
	ext2_u32_iterate iter;
	errcode_t		retval;

	EXT2_CHECK_MAGIC(bb, EXT2_ET_MAGIC_BADBLOCKS_LIST);

	retval = ext2fs_get_mem(sizeof(struct ext2_struct_u32_iterate), &iter);
	if (retval)
		return retval;

	iter->magic = EXT2_ET_MAGIC_BADBLOCKS_ITERATE;
	iter->bb = bb;
	iter->ptr = 0;
	*ret = iter;
	return 0;
}

errcode_t ext2fs_badblocks_list_iterate_begin(ext2_badblocks_list bb,
					      ext2_badblocks_iterate *ret)
{
	return ext2fs_u32_list_iterate_begin((ext2_u32_list) bb,
					      (ext2_u32_iterate *)ret);
}

void ext2fs_u32_list_iterate_end(ext2_u32_iterate iter)
{
	if (!iter || (iter->magic != EXT2_ET_MAGIC_BADBLOCKS_ITERATE))
		return;

	iter->bb = 0;
	ext2fs_free_mem(&iter);
}

void ext2fs_badblocks_list_iterate_end(ext2_badblocks_iterate iter)
{
	ext2fs_u32_list_iterate_end((ext2_u32_iterate) iter);
}

int ext2fs_u32_list_iterate(ext2_u32_iterate iter, __u32 *blk)
{
	ext2_u32_list	bb;

	if (iter->magic != EXT2_ET_MAGIC_BADBLOCKS_ITERATE)
		return 0;

	bb = iter->bb;

	if (bb->magic != EXT2_ET_MAGIC_BADBLOCKS_LIST)
		return 0;

	if (iter->ptr < bb->num) {
		*blk = bb->list[iter->ptr++];
		return 1;
	}
	*blk = 0;
	return 0;
}

int ext2fs_badblocks_list_iterate(ext2_badblocks_iterate iter, blk_t *blk)
{
	return ext2fs_u32_list_iterate((ext2_u32_iterate) iter,
				       (__u32 *)blk);
}

struct set_badblock_record {
	ext2_badblocks_iterate	bb_iter;
	int		bad_block_count;
	blk_t		*ind_blocks;
	int		max_ind_blocks;
	int		ind_blocks_size;
	int		ind_blocks_ptr;
	char		*block_buf;
	errcode_t	err;
};

static int set_bad_block_proc(ext2_filsys fs, blk_t *block_nr,
			      e2_blkcnt_t blockcnt,
			      blk_t ref_block, int ref_offset,
			      void *priv_data);
static int clear_bad_block_proc(ext2_filsys fs, blk_t *block_nr,
				e2_blkcnt_t blockcnt,
				blk_t ref_block, int ref_offset,
				void *priv_data);
/*
 * Given a bad blocks bitmap, update the bad blocks inode to reflect
 * the map.
 */
errcode_t ext2fs_update_bb_inode(ext2_filsys fs, ext2_badblocks_list bb_list)
{
	errcode_t			retval;
	struct set_badblock_record	rec;
	struct ext2fs_inode		inode;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!fs->block_map)
		return EXT2_ET_NO_BLOCK_BITMAP;

	rec.bad_block_count = 0;
	rec.ind_blocks_size = 0;
	rec.ind_blocks_ptr = 0;
	rec.max_ind_blocks = 10;
	retval = ext2fs_get_array(rec.max_ind_blocks, sizeof(blk_t),
				&rec.ind_blocks);
	if (retval)
		return retval;
	memset(rec.ind_blocks, 0, rec.max_ind_blocks * sizeof(blk_t));
	retval = ext2fs_get_mem(fs->blocksize, &rec.block_buf);
	if (retval)
		goto cleanup;
	memset(rec.block_buf, 0, fs->blocksize);
	rec.err = 0;

	/*
	 * First clear the old bad blocks (while saving the indirect blocks)
	 */
	retval = ext2fs_block_iterate2(fs, EXT2_BAD_INO,
				       BLOCK_FLAG_DEPTH_TRAVERSE, 0,
				       clear_bad_block_proc, &rec);
	if (retval)
		goto cleanup;
	if (rec.err) {
		retval = rec.err;
		goto cleanup;
	}

	/*
	 * Now set the bad blocks!
	 *
	 * First, mark the bad blocks as used.  This prevents a bad
	 * block from being used as an indirecto block for the bad
	 * block inode (!).
	 */
	if (bb_list) {
		retval = ext2fs_badblocks_list_iterate_begin(bb_list,
							     &rec.bb_iter);
		if (retval)
			goto cleanup;
		retval = ext2fs_block_iterate2(fs, EXT2_BAD_INO,
					       BLOCK_FLAG_APPEND, 0,
					       set_bad_block_proc, &rec);
		ext2fs_badblocks_list_iterate_end(rec.bb_iter);
		if (retval)
			goto cleanup;
		if (rec.err) {
			retval = rec.err;
			goto cleanup;
		}
	}

	/*
	 * Update the bad block inode's mod time and block count
	 * field.
	 */
	retval = ext2fs_read_inode(fs, EXT2_BAD_INO, &inode);
	if (retval)
		goto cleanup;

	inode.i_atime = fs->now ? fs->now : (__u32) NULL;
	inode.i_mtime = fs->now ? fs->now : (__u32) NULL;
	if (!inode.i_ctime)
		inode.i_ctime = fs->now ? fs->now : (__u32) NULL;
	ext2fs_iblk_set(fs, &inode, rec.bad_block_count);
	inode.i_size = rec.bad_block_count * fs->blocksize;

	retval = ext2fs_write_inode(fs, EXT2_BAD_INO, &inode);
	if (retval)
		goto cleanup;

cleanup:
	ext2fs_free_mem(&rec.ind_blocks);
	ext2fs_free_mem(&rec.block_buf);
	return retval;
}

/*
 *  Resize memory
 */
errcode_t ext2fs_resize_mem(unsigned long old_size,
				     unsigned long size, void *ptr)
{
	void *p;

	/* Use "memcpy" for pointer assignments here to avoid problems
	 * with C99 strict type aliasing rules. */
	memcpy(&p, ptr, sizeof(p));
	p = realloc(p, size);
	if (!p)
		return EXT2_ET_NO_MEMORY;
	memcpy(ptr, &p, sizeof(p));
	return 0;
}

/*
 * Given a bad blocks bitmap, update the bad blocks inode to reflect
 * the map.*/
static int clear_bad_block_proc(ext2_filsys fs, blk_t *block_nr,
				e2_blkcnt_t blockcnt,
				blk_t ref_block,
				int ref_offset,
				void *priv_data)
{
	struct set_badblock_record *rec = (struct set_badblock_record *)
		priv_data;
	errcode_t	retval;
	unsigned long	old_size;

	if (!*block_nr)
		return 0;

	/*
	 * If the block number is outrageous, clear it and ignore it.
	 */
	if (*block_nr >= fs->super->s_blocks_count ||
	    *block_nr < fs->super->s_first_data_block) {
		*block_nr = 0;
		return BLOCK_CHANGED;
	}

	if (blockcnt < 0) {
		if (rec->ind_blocks_size >= rec->max_ind_blocks) {
			old_size = rec->max_ind_blocks * sizeof(blk_t);
			rec->max_ind_blocks += 10;
			retval = ext2fs_resize_mem(old_size,
				   rec->max_ind_blocks * sizeof(blk_t),
				   &rec->ind_blocks);
			if (retval) {
				rec->max_ind_blocks -= 10;
				rec->err = retval;
				return BLOCK_ABORT;
			}
		}
		rec->ind_blocks[rec->ind_blocks_size++] = *block_nr;
	}

	/*
	 * Mark the block as unused, and update accounting information
	 */
	ext2fs_block_alloc_stats(fs, *block_nr, -1);

	*block_nr = 0;
	return BLOCK_CHANGED;
}

int ext2fs_test_block_bitmap(ext2fs_block_bitmap bitmap,
				       blk_t block)
{
	return ext2fs_test_generic_bitmap((ext2fs_generic_bitmap) bitmap,
					  block);
}

/*
 * Helper function for update_bb_inode()
 *
 * Set the block list in the bad block inode, using the supplied bitmap.
 */
static int set_bad_block_proc(ext2_filsys fs, blk_t *block_nr,
			      e2_blkcnt_t blockcnt,
			      blk_t ref_block,
			      int ref_offset,
			      void *priv_data)
{
	struct set_badblock_record *rec = (struct set_badblock_record *)
		priv_data;
	errcode_t	retval;
	blk_t		blk;

	if (blockcnt >= 0) {
		/*
		 * Get the next bad block.
		 */
		if (!ext2fs_badblocks_list_iterate(rec->bb_iter, &blk))
			return BLOCK_ABORT;
		rec->bad_block_count++;
	} else {
		/*
		 * An indirect block; fetch a block from the
		 * previously used indirect block list.  The block
		 * most be not marked as used; if so, get another one.
		 * If we run out of reserved indirect blocks, allocate
		 * a new one.
		 */
retry:
		if (rec->ind_blocks_ptr < rec->ind_blocks_size) {
			blk = rec->ind_blocks[rec->ind_blocks_ptr++];
			if (ext2fs_test_block_bitmap(fs->block_map, blk))
				goto retry;
		} else {
			retval = ext2fs_new_block(fs, 0, 0, &blk);
			if (retval) {
				rec->err = retval;
				return BLOCK_ABORT;
			}
		}
		put_ext4fs(dev_desc, (uint64_t)(blk * fs->blocksize),
			   rec->block_buf, (uint32_t)(fs->blocksize));
		retval = 0;
		if (retval) {
			rec->err = retval;
			return BLOCK_ABORT;
		}
	}

	/*
	 * Update block counts
	 */
	ext2fs_block_alloc_stats(fs, blk, 1);

	*block_nr = blk;
	return BLOCK_CHANGED;
}
