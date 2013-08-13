#include <common.h>
#include <ext_common.h>
#include <ext4fs.h>
#include <malloc.h>
#include <div64.h>
#include "ext2fs.h"
#include "ext4_common.h"
#include "iterate.h"

#define EXT4_MAX_REC_LEN		((1<<16)-1)

errcode_t ext2fs_get_rec_len(ext2_filsys fs,
			     struct ext2_dir_entry *dirent,
			     unsigned int *rec_len)
{
	unsigned int len = dirent->rec_len;

	if (fs->blocksize < 65536)
		*rec_len = len;
	else if (len == EXT4_MAX_REC_LEN || len == 0)
		*rec_len = fs->blocksize;
	else
		*rec_len = (len & 65532) | ((len & 3) << 16);
	return 0;
}

errcode_t ext2fs_set_rec_len(ext2_filsys fs,
			     unsigned int len,
			     struct ext2_dir_entry *dirent)
{
	if ((len > fs->blocksize) || (fs->blocksize > (1 << 18)) || (len & 3))
		return 22;
	if (len < 65536) {
		dirent->rec_len = len;
		return 0;
	}
	if (len == fs->blocksize) {
		if (fs->blocksize == 65536)
			dirent->rec_len = EXT4_MAX_REC_LEN;
		else
			dirent->rec_len = 0;
	} else {
		dirent->rec_len = (len & 65532) | ((len >> 16) & 3);
	}
	return 0;
}

/* look up */

struct block_context {
	ext2_filsys	fs;
	int (*func)(ext2_filsys	fs,
		    blk_t	*blocknr,
		    e2_blkcnt_t	bcount,
		    blk_t	ref_blk,
		    int		ref_offset,
		    void	*priv_data);
	e2_blkcnt_t	bcount;
	int		bsize;
	int		flags;
	errcode_t	errcode;
	char	*ind_buf;
	char	*dind_buf;
	char	*tind_buf;
	void	*priv_data;
};

struct xlate {
	int (*func)(struct ext2_dir_entry *dirent,
		    int		offset,
		    int		blocksize,
		    char	*buf,
		    void	*priv_data);
	void *real_private;
};

static int xlate_func(ext2_ino_t dir, int entry,
		      struct ext2_dir_entry *dirent, int offset,
		      int blocksize, char *buf, void *priv_data)
{
	struct xlate *xl = (struct xlate *)priv_data;

	return (*xl->func)(dirent, offset, blocksize, buf, xl->real_private);
}


/*
 * Verify the extent header as being sane
 */
errcode_t ext2fs_extent_header_verify(void *ptr, int size)
{
	int eh_max, entry_size;
	struct ext3_extent_header *eh = ptr;

	if (ext2fs_le16_to_cpu(eh->eh_magic) != EXT3_EXT_MAGIC)
		return EXT2_ET_EXTENT_HEADER_BAD;
	if (ext2fs_le16_to_cpu(eh->eh_entries) > ext2fs_le16_to_cpu(eh->eh_max))
		return EXT2_ET_EXTENT_HEADER_BAD;
	if (eh->eh_depth == 0)
		entry_size = sizeof(struct ext3_extent);
	else
		entry_size = sizeof(struct ext3_extent_idx);

	eh_max = (size - sizeof(*eh)) / entry_size;
	/* Allow two extent-sized items at the end of the block, for
	 * ext4_extent_tail with checksum in the future. */
	if ((ext2fs_le16_to_cpu(eh->eh_max) > eh_max) ||
	    (ext2fs_le16_to_cpu(eh->eh_max) < (eh_max - 2)))
		return EXT2_ET_EXTENT_HEADER_BAD;

	return 0;
}

errcode_t ext2fs_extent_open2(ext2_filsys fs, ext2_ino_t ino,
				    struct ext2fs_inode *inode,
				    ext2_extent_handle_t *ret_handle)
{
	struct ext2_extent_handle	*handle;
	errcode_t			retval;
	int				i;
	struct ext3_extent_header	*eh;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!inode)
		if ((ino == 0) || (ino > fs->super->s_inodes_count))
			return EXT2_ET_BAD_INODE_NUM;

	retval = ext2fs_get_mem(sizeof(struct ext2_extent_handle), &handle);
	if (retval)
		return retval;
	memset(handle, 0, sizeof(struct ext2_extent_handle));

	retval = ext2fs_get_mem(sizeof(struct ext2fs_inode), &handle->inode);
	if (retval)
		goto errout;

	handle->ino = ino;
	handle->fs = fs;

	if (inode) {
		memcpy(handle->inode, inode, sizeof(struct ext2fs_inode));
	} else {
		retval = ext2fs_read_inode(fs, ino, handle->inode);
		if (retval)
			goto errout;
	}

	eh = (struct ext3_extent_header *)&handle->inode->i_block[0];

	for (i = 0; i < EXT2_N_BLOCKS; i++)
		if (handle->inode->i_block[i])
			break;
	if (i >= EXT2_N_BLOCKS) {
		eh->eh_magic = ext2fs_cpu_to_le16(EXT3_EXT_MAGIC);
		eh->eh_depth = 0;
		eh->eh_entries = 0;
		i = (sizeof(handle->inode->i_block) - sizeof(*eh)) /
						sizeof(struct ext3_extent);
		eh->eh_max = ext2fs_cpu_to_le16(i);
		handle->inode->i_flags |= EXT4_EXTENTS_FL;
	}

	if (!(handle->inode->i_flags & EXT4_EXTENTS_FL)) {
		retval = EXT2_ET_INODE_NOT_EXTENT;
		goto errout;
	}

	retval = ext2fs_extent_header_verify(eh,
					sizeof(handle->inode->i_block));
	if (retval)
		goto errout;

	handle->max_depth = ext2fs_le16_to_cpu(eh->eh_depth);
	handle->type = ext2fs_le16_to_cpu(eh->eh_magic);

	retval = ext2fs_get_mem(((handle->max_depth+1) *
				 sizeof(struct extent_path)),
				&handle->path);
	memset(handle->path, 0,
	       (handle->max_depth+1) * sizeof(struct extent_path));

	handle->path[0].buf = (char *)handle->inode->i_block;
	handle->path[0].left = ext2fs_le16_to_cpu(eh->eh_entries);
	handle->path[0].entries = ext2fs_le16_to_cpu(eh->eh_entries);
	handle->path[0].max_entries = ext2fs_le16_to_cpu(eh->eh_max);
	handle->path[0].curr = 0;
	handle->path[0].end_blk =
		((((__u64) handle->inode->i_size_high << 32) +
		  handle->inode->i_size + (fs->blocksize - 1))
		 >> EXT2_BLOCK_SIZE_BITS(fs->super));
	handle->path[0].visit_num = 1;
	handle->level = 0;
	handle->magic = EXT2_ET_MAGIC_EXTENT_HANDLE;

	*ret_handle = handle;
	return 0;

errout:
	ext2fs_extent_free(handle);
	return retval;
}

/*
 * This function is responsible for (optionally) moving through the
 * extent tree and then returning the current extent
 */
errcode_t ext2fs_extent_get(ext2_extent_handle_t handle,
			    int flags, struct ext2fs_extent *extent)
{
	struct extent_path	*path, *newpath;
	struct ext3_extent_header	*eh;
	struct ext3_extent_idx		*ix = 0;
	struct ext3_extent		*ex;
	errcode_t			retval;
	blk_t				blk;
	blk64_t				end_blk;
	int				orig_op, op;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	orig_op = flags & EXT2_EXTENT_MOVE_MASK;
	op = orig_op;

retry:
	path = handle->path + handle->level;
	if ((orig_op == EXT2_EXTENT_NEXT) ||
	    (orig_op == EXT2_EXTENT_NEXT_LEAF)) {
		if (handle->level < handle->max_depth) {
			/* interior node */
			if (path->visit_num == 0) {
				path->visit_num++;
				op = EXT2_EXTENT_DOWN;
			} else if (path->left > 0) {
				op = EXT2_EXTENT_NEXT_SIB;
			} else if (handle->level > 0) {
				op = EXT2_EXTENT_UP;
			} else {
				return EXT2_ET_EXTENT_NO_NEXT;
			}
		} else {
			/* leaf node */
			if (path->left > 0)
				op = EXT2_EXTENT_NEXT_SIB;
			else if (handle->level > 0)
				op = EXT2_EXTENT_UP;
			else
				return EXT2_ET_EXTENT_NO_NEXT;
		}
	}

	if ((orig_op == EXT2_EXTENT_PREV) ||
	    (orig_op == EXT2_EXTENT_PREV_LEAF)) {
		if (handle->level < handle->max_depth) {
			/* interior node */
			if (path->visit_num > 0) {
				/* path->visit_num = 0; */
				op = EXT2_EXTENT_DOWN_AND_LAST;
			} else if (path->left < path->entries-1) {
				op = EXT2_EXTENT_PREV_SIB;
			} else if (handle->level > 0) {
				op = EXT2_EXTENT_UP;
			} else {
				return EXT2_ET_EXTENT_NO_PREV;
			}
		} else {
			/* leaf node */
			if (path->left < path->entries-1)
				op = EXT2_EXTENT_PREV_SIB;
			else if (handle->level > 0)
				op = EXT2_EXTENT_UP;
			else
				return EXT2_ET_EXTENT_NO_PREV;
		}
	}
	if (orig_op == EXT2_EXTENT_LAST_LEAF) {
		if ((handle->level < handle->max_depth) &&
		    (path->left == 0))
			op = EXT2_EXTENT_DOWN;
		else
			op = EXT2_EXTENT_LAST_SIB;
	}

	switch (op) {
	case EXT2_EXTENT_CURRENT:
		ix = path->curr;
		break;
	case EXT2_EXTENT_ROOT:
		handle->level = 0;
		path = handle->path + handle->level;
	case EXT2_EXTENT_FIRST_SIB:
		path->left = path->entries;
		path->curr = 0;
	case EXT2_EXTENT_NEXT_SIB:
		if (path->left <= 0)
			return EXT2_ET_EXTENT_NO_NEXT;
		if (path->curr) {
			ix = path->curr;
			ix++;
		} else {
			eh = (struct ext3_extent_header *)path->buf;
			ix = EXT_FIRST_INDEX(eh);
		}
		path->left--;
		path->curr = ix;
		path->visit_num = 0;
		break;
	case EXT2_EXTENT_PREV_SIB:
		if (!path->curr ||
		    path->left+1 >= path->entries)
			return EXT2_ET_EXTENT_NO_PREV;
		ix = path->curr;
		ix--;
		path->curr = ix;
		path->left++;
		if (handle->level < handle->max_depth)
			path->visit_num = 1;
		break;
	case EXT2_EXTENT_LAST_SIB:
		eh = (struct ext3_extent_header *)path->buf;
		path->curr = EXT_LAST_EXTENT(eh);
		ix = path->curr;
		path->left = 0;
		path->visit_num = 0;
		break;
	case EXT2_EXTENT_UP:
		if (handle->level <= 0)
			return EXT2_ET_EXTENT_NO_UP;
		handle->level--;
		path--;
		ix = path->curr;
		if ((orig_op == EXT2_EXTENT_PREV) ||
		    (orig_op == EXT2_EXTENT_PREV_LEAF))
			path->visit_num = 0;
		break;
	case EXT2_EXTENT_DOWN:
	case EXT2_EXTENT_DOWN_AND_LAST:
		if (!path->curr || (handle->level >= handle->max_depth))
			return EXT2_ET_EXTENT_NO_DOWN;

		ix = path->curr;
		newpath = path + 1;
		if (!newpath->buf) {
			retval = ext2fs_get_mem(handle->fs->blocksize,
						&newpath->buf);
			if (retval)
				return retval;
		}
		blk = ext2fs_le32_to_cpu(ix->ei_leaf) +
			((__u64) ext2fs_le16_to_cpu(ix->ei_leaf_hi) << 32);

		int factor = handle->fs->blocksize/512;
		retval = ext2fs_devread(blk * factor , 0,
					handle->fs->blocksize,	newpath->buf);
		retval = 0;
		if (retval)
			return retval;

		handle->level++;

		eh = (struct ext3_extent_header *)newpath->buf;

		retval = ext2fs_extent_header_verify(eh, handle->fs->blocksize);
		if (retval) {
			handle->level--;
			return retval;
		}

		newpath->left = ext2fs_le16_to_cpu(eh->eh_entries);
		newpath->entries = ext2fs_le16_to_cpu(eh->eh_entries);
		newpath->max_entries = ext2fs_le16_to_cpu(eh->eh_max);

		if (path->left > 0) {
			ix++;
			newpath->end_blk = ext2fs_le32_to_cpu(ix->ei_block);
		} else {
			newpath->end_blk = path->end_blk;
		}

		path = newpath;
		if (op == EXT2_EXTENT_DOWN) {
			ix = EXT_FIRST_INDEX((struct ext3_extent_header *)eh);
			path->curr = ix;
			path->left = path->entries - 1;
			path->visit_num = 0;
		} else {
			ix = EXT_LAST_INDEX((struct ext3_extent_header *)eh);
			path->curr = ix;
			path->left = 0;
			if (handle->level < handle->max_depth)
				path->visit_num = 1;
		}
		break;
	default:
		return EXT2_ET_OP_NOT_SUPPORTED;
	}

	if (!ix)
		return EXT2_ET_NO_CURRENT_NODE;

	extent->e_flags = 0;

	if (handle->level == handle->max_depth) {
		ex = (struct ext3_extent *)ix;

		extent->e_pblk = ext2fs_le32_to_cpu(ex->ee_start) +
			((__u64) ext2fs_le16_to_cpu(ex->ee_start_hi) << 32);
		extent->e_lblk = ext2fs_le32_to_cpu(ex->ee_block);
		extent->e_len = ext2fs_le16_to_cpu(ex->ee_len);
		extent->e_flags |= EXT2_EXTENT_FLAGS_LEAF;
		if (extent->e_len > EXT_INIT_MAX_LEN) {
			extent->e_len -= EXT_INIT_MAX_LEN;
			extent->e_flags |= EXT2_EXTENT_FLAGS_UNINIT;
		}
	} else {
		extent->e_pblk = ext2fs_le32_to_cpu(ix->ei_leaf) +
			((__u64) ext2fs_le16_to_cpu(ix->ei_leaf_hi) << 32);
		extent->e_lblk = ext2fs_le32_to_cpu(ix->ei_block);
		if (path->left > 0) {
			ix++;
			end_blk = ext2fs_le32_to_cpu(ix->ei_block);
		} else {
			end_blk = path->end_blk;
		}

		extent->e_len = end_blk - extent->e_lblk;
	}
	if (path->visit_num)
		extent->e_flags |= EXT2_EXTENT_FLAGS_SECOND_VISIT;

	if (((orig_op == EXT2_EXTENT_NEXT_LEAF) ||
	     (orig_op == EXT2_EXTENT_PREV_LEAF)) &&
	    (handle->level != handle->max_depth))
		goto retry;

	if ((orig_op == EXT2_EXTENT_LAST_LEAF) &&
	    ((handle->level != handle->max_depth) ||
	     (path->left != 0)))
		goto retry;

	return 0;
}

#define check_for_ro_violation_return(ctx, ret)				\
	do {								\
		if (((ctx)->flags & BLOCK_FLAG_READ_ONLY) &&		\
		    ((ret) & BLOCK_CHANGED)) {				\
			(ctx)->errcode = EXT2_ET_RO_BLOCK_ITERATE;	\
			ret |= BLOCK_ABORT | BLOCK_ERROR;		\
			return ret;					\
		}							\
	} while (0)

#define check_for_ro_violation_goto(ctx, ret, label)			\
	do {								\
		if (((ctx)->flags & BLOCK_FLAG_READ_ONLY) &&		\
		    ((ret) & BLOCK_CHANGED)) {				\
			(ctx)->errcode = EXT2_ET_RO_BLOCK_ITERATE;	\
			ret |= BLOCK_ABORT | BLOCK_ERROR;		\
			goto label;					\
		}							\
	} while (0)

static errcode_t update_path(ext2_extent_handle_t handle)
{
	blk64_t				blk;
	errcode_t			retval;
	struct ext3_extent_idx		*ix;

	if (handle->level == 0) {
		retval = ext2fs_write_inode(handle->fs, handle->ino,
					    handle->inode);
	} else {
		ix = handle->path[handle->level - 1].curr;
		blk = ext2fs_le32_to_cpu(ix->ei_leaf) +
			((__u64) ext2fs_le16_to_cpu(ix->ei_leaf_hi) << 32);

	put_ext4fs(dev_desc, (uint64_t)(blk * handle->fs->blocksize),
		   handle->path[handle->level].buf,
		   (uint32_t) handle->fs->blocksize);

		retval = 0;
	}
	return retval;
}

/*
 * Go to the node at leaf_level which contains logical block blk.
 *
 * leaf_level is height from the leaf node level, i.e.
 * leaf_level 0 is at leaf node, leaf_level 1 is 1 above etc.
 *
 * If "blk" has no mapping (hole) then handle is left at last
 * extent before blk.
 */
errcode_t ext2fs_extent_replace(ext2_extent_handle_t handle,
				int flags,
				struct ext2fs_extent *extent)
{
	struct extent_path		*path;
	struct ext3_extent_idx		*ix;
	struct ext3_extent		*ex;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	path = handle->path + handle->level;
	if (!path->curr)
		return EXT2_ET_NO_CURRENT_NODE;


	if (handle->level == handle->max_depth) {
		ex = path->curr;

		ex->ee_block = ext2fs_cpu_to_le32(extent->e_lblk);
		ex->ee_start = ext2fs_cpu_to_le32(extent->e_pblk & 0xFFFFFFFF);
		ex->ee_start_hi = ext2fs_cpu_to_le16(extent->e_pblk >> 32);
		if (extent->e_flags & EXT2_EXTENT_FLAGS_UNINIT) {
			if (extent->e_len > EXT_UNINIT_MAX_LEN)
				return EXT2_ET_EXTENT_INVALID_LENGTH;
			ex->ee_len = ext2fs_cpu_to_le16(extent->e_len +
							EXT_INIT_MAX_LEN);
		} else {
			if (extent->e_len > EXT_INIT_MAX_LEN)
				return EXT2_ET_EXTENT_INVALID_LENGTH;
			ex->ee_len = ext2fs_cpu_to_le16(extent->e_len);
		}
	} else {
		ix = path->curr;

		ix->ei_leaf = ext2fs_cpu_to_le32(extent->e_pblk & 0xFFFFFFFF);
		ix->ei_leaf_hi = ext2fs_cpu_to_le16(extent->e_pblk >> 32);
		ix->ei_block = ext2fs_cpu_to_le32(extent->e_lblk);
		ix->ei_unused = 0;
	}
	update_path(handle);
	return 0;
}

errcode_t ext2fs_extent_delete(ext2_extent_handle_t handle, int flags)
{
	struct extent_path		*path;
	char				*cp;
	struct ext3_extent_header	*eh;
	errcode_t			retval = 0;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	path = handle->path + handle->level;
	if (!path->curr)
		return EXT2_ET_NO_CURRENT_NODE;

	cp = path->curr;

	if (path->left) {
		memmove(cp, cp + sizeof(struct ext3_extent_idx),
			path->left * sizeof(struct ext3_extent_idx));
		path->left--;
	} else {
		struct ext3_extent_idx	*ix = path->curr;
		ix--;
		path->curr = ix;
	}
	if (--path->entries == 0)
		path->curr = 0;

	/* if non-root node has no entries left, remove it & parent ptr to it */
	if (path->entries == 0 && handle->level) {
		if (!(flags & EXT2_EXTENT_DELETE_KEEP_EMPTY)) {
			struct ext2fs_extent	extent;

			retval = ext2fs_extent_get(handle, EXT2_EXTENT_UP,
								&extent);
			if (retval)
				return retval;

			retval = ext2fs_extent_delete(handle, flags);
			handle->inode->i_blocks -= handle->fs->blocksize / 512;
			retval = ext2fs_write_inode(handle->fs, handle->ino,
						    handle->inode);
			ext2fs_block_alloc_stats(handle->fs, extent.e_pblk, -1);
		}
	} else {
		eh = (struct ext3_extent_header *)path->buf;
		eh->eh_entries = ext2fs_cpu_to_le16(path->entries);
		if ((path->entries == 0) && (handle->level == 0))
			eh->eh_depth = 0;
			handle->max_depth = 0;
			retval = update_path(handle);
	}
	return retval;
}

errcode_t ext2fs_extent_insert(ext2_extent_handle_t handle, int flags,
				      struct ext2fs_extent *extent)
{
	struct extent_path		*path;
	struct ext3_extent_idx		*ix;
	struct ext3_extent_header	*eh;
	errcode_t			retval;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;


	path = handle->path + handle->level;

	eh = (struct ext3_extent_header *)path->buf;
	if (path->curr) {
		ix = path->curr;
		if (flags & EXT2_EXTENT_INSERT_AFTER) {
			ix++;
			path->left--;
		}
	} else {
		ix = EXT_FIRST_INDEX(eh);
	}

	path->curr = ix;

	if (path->left >= 0)
		memmove(ix + 1, ix,
			(path->left+1) * sizeof(struct ext3_extent_idx));
	path->left++;
	path->entries++;

	eh = (struct ext3_extent_header *)path->buf;
	eh->eh_entries = ext2fs_cpu_to_le16(path->entries);
	retval = ext2fs_extent_replace(handle, 0, extent);
	if (retval)
		goto errout;


	retval = update_path(handle);
	if (retval)
		goto errout;

	return 0;

errout:
	ext2fs_extent_delete(handle, 0);
	return retval;
}

errcode_t ext2fs_extent_get_info(ext2_extent_handle_t handle,
				 struct ext2_extent_info *info)
{
	struct extent_path		*path;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	memset(info, 0, sizeof(struct ext2_extent_info));

	path = handle->path + handle->level;
	if (path) {
		if (path->curr)
			info->curr_entry = ((char *)path->curr - path->buf) /
				sizeof(struct ext3_extent_idx);
		else
			info->curr_entry = 0;
		info->num_entries = path->entries;
		info->max_entries = path->max_entries;
		info->bytes_avail = (path->max_entries - path->entries) *
			sizeof(struct ext3_extent);
	}

	info->curr_level = handle->level;
	info->max_depth = handle->max_depth;
	info->max_lblk = ((__u64) 1 << 32) - 1;
	info->max_pblk = ((__u64) 1 << 48) - 1;
	info->max_len = (1UL << 15);
	info->max_uninit_len = (1UL << 15) - 1;

	return 0;
}

/*
 * Go to the node at leaf_level which contains logical block blk.
 *
 * leaf_level is height from the leaf node level, i.e.
 * leaf_level 0 is at leaf node, leaf_level 1 is 1 above etc.
 *
 * If "blk" has no mapping (hole) then handle is left at last
 * extent before blk.
 */
static errcode_t extent_goto(ext2_extent_handle_t handle,
			     int leaf_level, blk64_t blk)
{
	struct ext2fs_extent	extent;
	errcode_t		retval;
	retval = ext2fs_extent_get(handle, EXT2_EXTENT_ROOT, &extent);
	if (retval) {
		if (retval == EXT2_ET_EXTENT_NO_NEXT)
			retval = EXT2_ET_EXTENT_NOT_FOUND;
		return retval;
	}

	if (leaf_level > handle->max_depth)
		return EXT2_ET_OP_NOT_SUPPORTED;

	while (1) {
		if (handle->max_depth - handle->level == leaf_level) {
			/* block is in this &extent */
			if ((blk >= extent.e_lblk) &&
			    (blk < extent.e_lblk + extent.e_len))
				return 0;

			if (blk < extent.e_lblk) {
				retval = ext2fs_extent_get(handle,
							   EXT2_EXTENT_PREV_SIB,
							   &extent);
				return EXT2_ET_EXTENT_NOT_FOUND;
			}
			retval = ext2fs_extent_get(handle,
						   EXT2_EXTENT_NEXT_SIB,
						   &extent);
			if (retval == EXT2_ET_EXTENT_NO_NEXT)
				return EXT2_ET_EXTENT_NOT_FOUND;

			if (retval)
				return retval;

			continue;
		}
		retval = ext2fs_extent_get(handle, EXT2_EXTENT_NEXT_SIB,
					   &extent);
		if (retval == EXT2_ET_EXTENT_NO_NEXT)
			goto go_down;
		if (retval)
			return retval;

		if (blk == extent.e_lblk)
			goto go_down;
		if (blk > extent.e_lblk)
			continue;

		retval = ext2fs_extent_get(handle, EXT2_EXTENT_PREV_SIB,
					   &extent);
		if (retval)
			return retval;

go_down:
		retval = ext2fs_extent_get(handle, EXT2_EXTENT_DOWN,
					   &extent);
		if (retval)
			return retval;
	}
}

errcode_t ext2fs_extent_goto(ext2_extent_handle_t handle,
			     blk64_t blk)
{
	return extent_goto(handle, 0, blk);
}

/*
 * Traverse back up to root fixing parents of current node as needed.
 *
 * If we changed start of first entry in a node, fix parent index start
 * and so on.
 *
 * Safe to call for any position in node; if not at the first entry,
 * will  simply return.
 */
static errcode_t ext2fs_extent_fix_parents(ext2_extent_handle_t handle)
{
	int				retval = 0;
	blk64_t				start;
	struct extent_path		*path;
	struct ext2fs_extent		extent;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	path = handle->path + handle->level;
	if (!path->curr)
		return EXT2_ET_NO_CURRENT_NODE;

	retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT, &extent);
	if (retval)
		goto done;

	/* modified node's start block */
	start = extent.e_lblk;
	/* traverse up until index not first, or startblk matches, or top */
	while (handle->level > 0 &&
	       (path->left == path->entries - 1)) {
		retval = ext2fs_extent_get(handle, EXT2_EXTENT_UP, &extent);
		if (retval)
			goto done;
		if (extent.e_lblk == start)
			break;
		path = handle->path + handle->level;
		extent.e_len += (extent.e_lblk - start);
		extent.e_lblk = start;
		retval = ext2fs_extent_replace(handle, 0, &extent);
		if (retval)
			goto done;
			update_path(handle);
	}

	/* put handle back to where we started */
		retval = ext2fs_extent_goto(handle, start);
done:
	return retval;
}

/*
 * Sets the physical block for a logical file block in the extent tree.
 *
 * May: map unmapped, unmap mapped, or remap mapped blocks.
 *
 * Mapping an unmapped block adds a single-block extent.
 *
 * Unmapping first or last block modifies extent in-place
 *  - But may need to fix parent's starts too in first-block case
 *
 * Mapping any unmapped block requires adding a (single-block) extent
 * and inserting into proper point in tree.
 *
 * Modifying (unmapping or remapping) a block in the middle
 * of an extent requires splitting the extent.
 *  - Remapping case requires new single-block extent.
 *
 * Remapping first or last block adds an extent.
 *
 * We really need extent adding to be smart about merging.
 */

errcode_t ext2fs_extent_set_bmap(ext2_extent_handle_t handle,
				 blk64_t logical, blk64_t physical, int flags)
{
	errcode_t		retval = 0;
	int			mapped = 1; /* logical is mapped? */
	int			orig_height;
	int			extent_uninit = 0;
	int			next_uninit = 0;
	int			new_uninit = 0;
	int			max_len = EXT_INIT_MAX_LEN;
	int			has_next;
	blk64_t			orig_lblk;
	struct extent_path	*path;
	struct ext2fs_extent	extent, next_extent, prev_extent;
	struct ext2fs_extent	newextent;
	struct ext2_extent_info	info;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	path = handle->path + handle->level;

	if (flags & EXT2_EXTENT_SET_BMAP_UNINIT) {
		new_uninit = 1;
		max_len = EXT_UNINIT_MAX_LEN;
	}

	/* if (re)mapping, set up new extent to insert */
	if (physical) {
		newextent.e_len = 1;
		newextent.e_pblk = physical;
		newextent.e_lblk = logical;
		newextent.e_flags = EXT2_EXTENT_FLAGS_LEAF;
		if (new_uninit)
			newextent.e_flags |= EXT2_EXTENT_FLAGS_UNINIT;
	}

	/* special case if the extent tree is completely empty */
	if ((handle->max_depth == 0) && (path->entries == 0)) {
		retval = ext2fs_extent_insert(handle, 0, &newextent);
		return retval;
	}

	/* save our original location in the extent tree */

	retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT, &extent);
	if (retval) {
		if (retval != EXT2_ET_NO_CURRENT_NODE)
			return retval;
		memset(&extent, 0, sizeof(extent));
	}
	retval = ext2fs_extent_get_info(handle, &info);
	if (retval)
		return retval;
	orig_height = info.max_depth - info.curr_level;
	orig_lblk = extent.e_lblk;

	/* go to the logical spot we want to (re/un)map */
	retval = ext2fs_extent_goto(handle, logical);
	if (retval) {
		if (retval == EXT2_ET_EXTENT_NOT_FOUND) {
			retval = 0;
			mapped = 0;
			if (!physical)
				goto done;
		} else {
			goto done;
		}
	}

	/*
	 * This may be the extent *before* the requested logical,
	 * if it's currently unmapped.
	 *
	 * Get the previous and next leaf extents, if they are present.
	 */
	retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT, &extent);
	if (retval)
		goto done;
	if (extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
		extent_uninit = 1;
	retval = ext2fs_extent_get(handle, EXT2_EXTENT_NEXT_LEAF, &next_extent);
	if (retval) {
		has_next = 0;
		if (retval != EXT2_ET_EXTENT_NO_NEXT)
			goto done;
	} else {
		has_next = 1;
		if (next_extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
			next_uninit = 1;
	}
	retval = ext2fs_extent_goto(handle, logical);
	if (retval && retval != EXT2_ET_EXTENT_NOT_FOUND)
		goto done;
	retval = ext2fs_extent_get(handle, EXT2_EXTENT_PREV_LEAF, &prev_extent);
	if (retval) {
		if (retval != EXT2_ET_EXTENT_NO_PREV)
			goto done;
	}

	retval = ext2fs_extent_goto(handle, logical);
	if (retval && retval != EXT2_ET_EXTENT_NOT_FOUND)
		goto done;

	/* check if already pointing to the requested physical */
	if (mapped && (new_uninit == extent_uninit) &&
	    (extent.e_pblk + (logical - extent.e_lblk) == physical)) {
		goto done;
	}

	if (!mapped) {
		if ((logical == extent.e_lblk + extent.e_len) &&
		    (physical == extent.e_pblk + extent.e_len) &&
		    (new_uninit == extent_uninit) &&
		    ((int) extent.e_len < max_len-1)) {
			extent.e_len++;
			retval = ext2fs_extent_replace(handle, 0, &extent);
		} else if ((logical == extent.e_lblk - 1) &&
			   (physical == extent.e_pblk - 1) &&
			   (new_uninit == extent_uninit) &&
			   ((int) extent.e_len < max_len - 1)) {
			extent.e_len++;
			extent.e_lblk--;
			extent.e_pblk--;
			retval = ext2fs_extent_replace(handle, 0, &extent);
		} else if (has_next &&
			   (logical == next_extent.e_lblk - 1) &&
			   (physical == next_extent.e_pblk - 1) &&
			   (new_uninit == next_uninit) &&
			   ((int) next_extent.e_len < max_len - 1)) {
					retval = ext2fs_extent_get(handle,
						   EXT2_EXTENT_NEXT_LEAF,
						   &next_extent);
			if (retval)
				goto done;
			next_extent.e_len++;
			next_extent.e_lblk--;
			next_extent.e_pblk--;
			retval = ext2fs_extent_replace(handle, 0, &next_extent);
		} else if (logical < extent.e_lblk) {
			retval = ext2fs_extent_insert(handle, 0, &newextent);
		} else {
			retval = ext2fs_extent_insert(handle,
				      EXT2_EXTENT_INSERT_AFTER, &newextent);
		}

		if (retval)
			goto done;
		retval = ext2fs_extent_fix_parents(handle);
		if (retval)
			goto done;
	}
done:
	/* get handle back to its position */
	if (orig_height > handle->max_depth)
		/* In case we shortened the tree */
		orig_height = handle->max_depth;

	extent_goto(handle, orig_height, orig_lblk);

	return retval;
}

errcode_t ext2fs_read_ind_block(ext2_filsys fs, blk_t blk, void *buf)
{
	errcode_t	retval;

	int factor = fs->blocksize/512;
	printf("devread 1\n");
	retval = ext2fs_devread(blk * factor , 0, fs->blocksize, buf);
	retval = 0;
	if (retval)
		return retval;
	return 0;
}

errcode_t ext2fs_write_ind_block(ext2_filsys fs, blk_t blk, void *buf)
{
	int retval = 0;

	if (fs->flags & EXT2_FLAG_IMAGE_FILE)
		return 0;

	put_ext4fs(dev_desc, (uint64_t)(blk * fs->blocksize),
		   buf, (uint32_t)fs->blocksize);
	retval = 0;
	return retval;
}

/*
 * Begin functions to handle an inode's extent information
 */
extern void ext2fs_extent_free(ext2_extent_handle_t handle)
{
	int			i;

	if (!handle)
		return;

	if (handle->inode)
		ext2fs_free_mem(&handle->inode);
	if (handle->path) {
		for (i = 1; i <= handle->max_depth; i++) {
			if (handle->path[i].buf)
				ext2fs_free_mem(&handle->path[i].buf);
		}
		ext2fs_free_mem(&handle->path);
	}
	ext2fs_free_mem(&handle);
}

static int block_iterate_ind(blk_t *ind_block, blk_t ref_block,
			     int ref_offset, struct block_context *ctx)
{
	int	ret = 0, changed = 0;
	int	i, flags, limit, offset;
	blk_t	*block_nr;

	limit = ctx->fs->blocksize >> 2;
	if (!(ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE) &&
	    !(ctx->flags & BLOCK_FLAG_DATA_ONLY))
		ret = (*ctx->func)(ctx->fs, ind_block,
				   BLOCK_COUNT_IND, ref_block,
				   ref_offset, ctx->priv_data);
	check_for_ro_violation_return(ctx, ret);
	if (!*ind_block || (ret & BLOCK_ABORT)) {
		ctx->bcount += limit;
		return ret;
	}
	if (*ind_block >= ctx->fs->super->s_blocks_count ||
	    *ind_block < ctx->fs->super->s_first_data_block) {
		ctx->errcode = EXT2_ET_BAD_IND_BLOCK;
		ret |= BLOCK_ERROR;
		return ret;
	}
	ctx->errcode = ext2fs_read_ind_block(ctx->fs, *ind_block,
					     ctx->ind_buf);
	if (ctx->errcode) {
		ret |= BLOCK_ERROR;
		return ret;
	}

	block_nr = (blk_t *)ctx->ind_buf;
	offset = 0;
	if (ctx->flags & BLOCK_FLAG_APPEND) {
		for (i = 0; i < limit; i++, ctx->bcount++, block_nr++) {
			flags = (*ctx->func)(ctx->fs, block_nr, ctx->bcount,
					     *ind_block, offset,
					     ctx->priv_data);
			changed	|= flags;
			if (flags & BLOCK_ABORT) {
				ret |= BLOCK_ABORT;
				break;
			}
			offset += sizeof(blk_t);
		}
	} else {
		for (i = 0; i < limit; i++, ctx->bcount++, block_nr++) {
			if (*block_nr == 0)
				goto skip_sparse;
			flags = (*ctx->func)(ctx->fs, block_nr, ctx->bcount,
					     *ind_block, offset,
					     ctx->priv_data);
			changed	|= flags;
			if (flags & BLOCK_ABORT) {
				ret |= BLOCK_ABORT;
				break;
			}
skip_sparse:
			offset += sizeof(blk_t);
		}
	}
	check_for_ro_violation_return(ctx, changed);
	if (changed & BLOCK_CHANGED) {
		ctx->errcode = ext2fs_write_ind_block(ctx->fs, *ind_block,
						      ctx->ind_buf);
		if (ctx->errcode)
			ret |= BLOCK_ERROR | BLOCK_ABORT;
	}
	if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE) &&
	    !(ctx->flags & BLOCK_FLAG_DATA_ONLY) &&
	    !(ret & BLOCK_ABORT))
		ret |= (*ctx->func)(ctx->fs, ind_block,
				    BLOCK_COUNT_IND, ref_block,
				    ref_offset, ctx->priv_data);
	check_for_ro_violation_return(ctx, ret);
	return ret;
}

static int block_iterate_dind(blk_t *dind_block, blk_t ref_block,
			      int ref_offset, struct block_context *ctx)
{
	int	ret = 0, changed = 0;
	int	i, flags, limit, offset;
	blk_t	*block_nr;

	limit = ctx->fs->blocksize >> 2;
	if (!(ctx->flags & (BLOCK_FLAG_DEPTH_TRAVERSE |
			    BLOCK_FLAG_DATA_ONLY)))
		ret = (*ctx->func)(ctx->fs, dind_block,
				   BLOCK_COUNT_DIND, ref_block,
				   ref_offset, ctx->priv_data);
	check_for_ro_violation_return(ctx, ret);
	if (!*dind_block || (ret & BLOCK_ABORT)) {
		ctx->bcount += limit*limit;
		return ret;
	}
	if (*dind_block >= ctx->fs->super->s_blocks_count ||
	    *dind_block < ctx->fs->super->s_first_data_block) {
		ctx->errcode = EXT2_ET_BAD_DIND_BLOCK;
		ret |= BLOCK_ERROR;
		return ret;
	}
	ctx->errcode = ext2fs_read_ind_block(ctx->fs, *dind_block,
					     ctx->dind_buf);
	if (ctx->errcode) {
		ret |= BLOCK_ERROR;
		return ret;
	}

	block_nr = (blk_t *)ctx->dind_buf;
	offset = 0;
	if (ctx->flags & BLOCK_FLAG_APPEND) {
		for (i = 0; i < limit; i++, block_nr++) {
			flags = block_iterate_ind(block_nr,
						  *dind_block, offset,
						  ctx);
			changed |= flags;
			if (flags & (BLOCK_ABORT | BLOCK_ERROR)) {
				ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
				break;
			}
			offset += sizeof(blk_t);
		}
	} else {
		for (i = 0; i < limit; i++, block_nr++) {
			if (*block_nr == 0) {
				ctx->bcount += limit;
				continue;
			}
			flags = block_iterate_ind(block_nr,
						  *dind_block, offset,
						  ctx);
			changed |= flags;
			if (flags & (BLOCK_ABORT | BLOCK_ERROR)) {
				ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
				break;
			}
			offset += sizeof(blk_t);
		}
	}
	check_for_ro_violation_return(ctx, changed);
	if (changed & BLOCK_CHANGED) {
		ctx->errcode = ext2fs_write_ind_block(ctx->fs, *dind_block,
						      ctx->dind_buf);
		if (ctx->errcode)
			ret |= BLOCK_ERROR | BLOCK_ABORT;
	}
	if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE) &&
	    !(ctx->flags & BLOCK_FLAG_DATA_ONLY) &&
	    !(ret & BLOCK_ABORT))
		ret |= (*ctx->func)(ctx->fs, dind_block,
				    BLOCK_COUNT_DIND, ref_block,
				    ref_offset, ctx->priv_data);
	check_for_ro_violation_return(ctx, ret);
	return ret;
}

static int block_iterate_tind(blk_t *tind_block, blk_t ref_block,
			      int ref_offset, struct block_context *ctx)
{
	int	ret = 0, changed = 0;
	int	i, flags, limit, offset;
	blk_t	*block_nr;

	limit = ctx->fs->blocksize >> 2;
	if (!(ctx->flags & (BLOCK_FLAG_DEPTH_TRAVERSE |
			    BLOCK_FLAG_DATA_ONLY)))
		ret = (*ctx->func)(ctx->fs, tind_block,
				   BLOCK_COUNT_TIND, ref_block,
				   ref_offset, ctx->priv_data);
	check_for_ro_violation_return(ctx, ret);
	if (!*tind_block || (ret & BLOCK_ABORT)) {
		ctx->bcount += limit*limit*limit;
		return ret;
	}
	if (*tind_block >= ctx->fs->super->s_blocks_count ||
	    *tind_block < ctx->fs->super->s_first_data_block) {
		ctx->errcode = EXT2_ET_BAD_TIND_BLOCK;
		ret |= BLOCK_ERROR;
		return ret;
	}
	ctx->errcode = ext2fs_read_ind_block(ctx->fs, *tind_block,
					     ctx->tind_buf);
	if (ctx->errcode) {
		ret |= BLOCK_ERROR;
		return ret;
	}

	block_nr = (blk_t *)ctx->tind_buf;
	offset = 0;
	if (ctx->flags & BLOCK_FLAG_APPEND) {
		for (i = 0; i < limit; i++, block_nr++) {
			flags = block_iterate_dind(block_nr,
						   *tind_block,
						   offset, ctx);
			changed |= flags;
			if (flags & (BLOCK_ABORT | BLOCK_ERROR)) {
				ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
				break;
			}
			offset += sizeof(blk_t);
		}
	} else {
		for (i = 0; i < limit; i++, block_nr++) {
			if (*block_nr == 0) {
				ctx->bcount += limit*limit;
				continue;
			}
			flags = block_iterate_dind(block_nr,
						   *tind_block,
						   offset, ctx);
			changed |= flags;
			if (flags & (BLOCK_ABORT | BLOCK_ERROR)) {
				ret |= flags & (BLOCK_ABORT | BLOCK_ERROR);
				break;
			}
			offset += sizeof(blk_t);
		}
	}
	check_for_ro_violation_return(ctx, changed);
	if (changed & BLOCK_CHANGED) {
		ctx->errcode = ext2fs_write_ind_block(ctx->fs, *tind_block,
						      ctx->tind_buf);
		if (ctx->errcode)
			ret |= BLOCK_ERROR | BLOCK_ABORT;
	}
	if ((ctx->flags & BLOCK_FLAG_DEPTH_TRAVERSE) &&
	    !(ctx->flags & BLOCK_FLAG_DATA_ONLY) &&
	    !(ret & BLOCK_ABORT))
		ret |= (*ctx->func)(ctx->fs, tind_block,
				    BLOCK_COUNT_TIND, ref_block,
				    ref_offset, ctx->priv_data);
	check_for_ro_violation_return(ctx, ret);
	return ret;
}

errcode_t ext2fs_block_iterate2(ext2_filsys fs,
				ext2_ino_t ino,
				int	flags,
				char *block_buf,
				int (*func)(ext2_filsys fs,
					    blk_t	*blocknr,
					    e2_blkcnt_t	blockcnt,
					    blk_t	ref_blk,
					    int		ref_offset,
					    void	*priv_data),
				void *priv_data)
{
	int	i;
	int	r, ret = 0;
	struct ext2fs_inode inode;
	errcode_t	retval;
	struct block_context ctx;
	int	limit;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);
	ctx.errcode = ext2fs_read_inode(fs, ino, &inode);
	if (ctx.errcode)
		return ctx.errcode;
	/*
	 * Check to see if we need to limit large files
	 */
	if (flags & BLOCK_FLAG_NO_LARGE) {
		if (!LINUX_S_ISDIR(inode.i_mode) &&
		    (inode.i_size_high != 0))
			return EXT2_ET_FILE_TOO_BIG;
	}

	limit = fs->blocksize >> 2;

	ctx.fs = fs;
	ctx.func = func;
	ctx.priv_data = priv_data;
	ctx.flags = flags;
	ctx.bcount = 0;
	if (block_buf) {
		ctx.ind_buf = block_buf;
	} else {
		retval = ext2fs_get_array(3, fs->blocksize, &ctx.ind_buf);
		if (retval)
			return retval;
	}
	ctx.dind_buf = ctx.ind_buf + fs->blocksize;
	ctx.tind_buf = ctx.dind_buf + fs->blocksize;

	if (inode.i_flags & EXT4_EXTENTS_FL) {
		ext2_extent_handle_t	handle;
		struct ext2fs_extent	extent;
		e2_blkcnt_t		blockcnt = 0;
		blk_t			blk, new_blk;
		int			op = EXT2_EXTENT_ROOT;
		int			uninit;

		unsigned int		j;

		ctx.errcode = ext2fs_extent_open2(fs, ino, &inode, &handle);
		if (ctx.errcode)
			goto abort_exit;
		while (1) {
			ctx.errcode = ext2fs_extent_get(handle, op, &extent);
			if (ctx.errcode) {
				if (ctx.errcode != EXT2_ET_EXTENT_NO_NEXT)
					break;
				ctx.errcode = 0;
				if (!(flags & BLOCK_FLAG_APPEND))
					break;

next_block_set:
				blk = 0;
				r = (*ctx.func)(fs, &blk, blockcnt,
						0, 0, priv_data);
				ret |= r;
				check_for_ro_violation_goto(&ctx, ret,
							    extent_errout);
				if (r & BLOCK_CHANGED) {
					ctx.errcode =
						ext2fs_extent_set_bmap(
							handle,
							(blk64_t) blockcnt++,
							(blk64_t) blk, 0);

					if (ctx.errcode ||
					    (ret & BLOCK_ABORT)) {
						journal_broken_cache_exit = 1;
						break;
					}
					if (blk)
						goto next_block_set;
				}
				break;
			}

			op = EXT2_EXTENT_NEXT;
			blk = extent.e_pblk;
			if (!(extent.e_flags & EXT2_EXTENT_FLAGS_LEAF)) {
				if (ctx.flags & BLOCK_FLAG_DATA_ONLY)
					continue;
				if ((!(extent.e_flags &
				       EXT2_EXTENT_FLAGS_SECOND_VISIT) &&
				    !(ctx.flags & BLOCK_FLAG_DEPTH_TRAVERSE)) ||
				    ((extent.e_flags &
				      EXT2_EXTENT_FLAGS_SECOND_VISIT) &&
				     (ctx.flags & BLOCK_FLAG_DEPTH_TRAVERSE))) {
					ret |= (*ctx.func)(fs, &blk,
							   -1, 0, 0, priv_data);
				if (ret & BLOCK_CHANGED) {
					extent.e_pblk = blk;
					ctx.errcode =
					ext2fs_extent_replace(
						handle, 0, &extent);
					if (ctx.errcode)
						break;
					}
				}
				continue;
			}
			uninit = 0;
			if (extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
				uninit = EXT2_EXTENT_SET_BMAP_UNINIT;
			for (blockcnt = extent.e_lblk, j = 0;
			     j < extent.e_len;
			     blk++, blockcnt++, j++) {
				new_blk = blk;
				r = (*ctx.func)(fs, &new_blk, blockcnt,
						0, 0, priv_data);
				ret |= r;
				check_for_ro_violation_goto(&ctx, ret,
							    extent_errout);
				if (r & BLOCK_CHANGED) {
					ctx.errcode =
						ext2fs_extent_set_bmap(
							handle,
							(blk64_t)blockcnt,
							(blk64_t)new_blk,
							uninit);
					if (ctx.errcode)
						goto extent_errout;
				}
				if (ret & BLOCK_ABORT)
					break;
			}
		}
extent_errout:
		ext2fs_extent_free(handle);
		ret |= BLOCK_ERROR | BLOCK_ABORT;
		goto errout;
	}
	/*
	 * Iterate over normal data blocks
	 */
	for (i = 0; i < EXT2_NDIR_BLOCKS; i++, ctx.bcount++) {
		if (inode.i_block[i] || (flags & BLOCK_FLAG_APPEND)) {
			ret |= (*ctx.func)(fs, &inode.i_block[i],
					    ctx.bcount, 0, i, priv_data);
			if (ret & BLOCK_ABORT)
				goto abort_exit;
		}
	}
	check_for_ro_violation_goto(&ctx, ret, abort_exit);
	if (inode.i_block[EXT2_IND_BLOCK] || (flags & BLOCK_FLAG_APPEND)) {
		ret |= block_iterate_ind(&inode.i_block[EXT2_IND_BLOCK],
					 0, EXT2_IND_BLOCK, &ctx);
		if (ret & BLOCK_ABORT)
			goto abort_exit;
	} else {
		ctx.bcount += limit;
	}

	if (inode.i_block[EXT2_DIND_BLOCK] || (flags & BLOCK_FLAG_APPEND)) {
		ret |= block_iterate_dind(&inode.i_block[EXT2_DIND_BLOCK],
					  0, EXT2_DIND_BLOCK, &ctx);
		if (ret & BLOCK_ABORT)
			goto abort_exit;
	} else {
		ctx.bcount += limit * limit;
	}

	if (inode.i_block[EXT2_TIND_BLOCK] || (flags & BLOCK_FLAG_APPEND)) {
		ret |= block_iterate_tind(&inode.i_block[EXT2_TIND_BLOCK],
					  0, EXT2_TIND_BLOCK, &ctx);
		if (ret & BLOCK_ABORT)
			goto abort_exit;
	}
abort_exit:
	if (ret & BLOCK_CHANGED) {
		retval = ext2fs_write_inode(fs, ino, &inode);
		if (retval) {
			ret |= BLOCK_ERROR;
			ctx.errcode = retval;
		}
	}
errout:
	if (!block_buf)
		ext2fs_free_mem(&ctx.ind_buf);

	return (ret & BLOCK_ERROR) ? ctx.errcode : 0;
}

errcode_t ext2fs_check_directory(ext2_filsys fs, ext2_ino_t ino)
{
	struct	ext2fs_inode	inode;
	errcode_t		retval;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (ino > fs->super->s_inodes_count)
		return EXT2_ET_BAD_INODE_NUM;

	if (fs->check_directory) {
		retval = (fs->check_directory)(fs, ino);
		if (retval != EXT2_ET_CALLBACK_NOTHANDLED)
			return retval;
	}
	retval = ext2fs_read_inode(fs, ino, &inode);
	if (retval)
		return retval;
	if (!LINUX_S_ISDIR(inode.i_mode))
		return EXT2_ET_NO_DIRECTORY;
	return 0;
}

/*
 * This function checks to see whether or not a potential deleted
 * directory entry looks valid.  What we do is check the deleted entry
 * and each successive entry to make sure that they all look valid and
 * that the last deleted entry ends at the beginning of the next
 * undeleted entry.  Returns 1 if the deleted entry looks valid, zero
 * if not valid.
 */
static int ext2fs_validate_entry(ext2_filsys fs, char *buf,
				 unsigned int offset,
				 unsigned int final_offset)
{
	struct ext2_dir_entry *dirent;
	unsigned int rec_len;
#define DIRENT_MIN_LENGTH 12

	while ((offset < final_offset) &&
	       (offset <= fs->blocksize - DIRENT_MIN_LENGTH)) {
		dirent = (struct ext2_dir_entry *)(buf + offset);
		if (ext2fs_get_rec_len(fs, dirent, &rec_len))
			return 0;
		offset += rec_len;
		if ((rec_len < 8) ||
		    ((rec_len % 4) != 0) ||
		    ((((unsigned) dirent->name_len & 0xFF)+8) > rec_len))
			return 0;
	}
	return (offset == final_offset);
}

/*
 * Helper function which is private to this module.  Used by
 * ext2fs_dir_iterate() and ext2fs_dblist_dir_iterate()
 */
int ext2fs_process_dir_block(ext2_filsys fs,
				  blk_t	*blocknr,
				  e2_blkcnt_t blockcnt,
				  blk_t	ref_block,
				  int ref_offset,
				  void	*priv_data)
{
	struct dir_context *ctx = (struct dir_context *)priv_data;
	unsigned int	offset = 0;
	unsigned int	next_real_entry = 0;
	int		ret = 0;
	int		changed = 0;
	int		do_abort = 0;
	unsigned int	rec_len, size;
	int		entry;
	struct ext2_dir_entry *dirent;

	if (blockcnt < 0)
		return 0;

	entry = blockcnt ? DIRENT_OTHER_FILE : DIRENT_DOT_FILE;

	ctx->errcode = ext2fs_read_dir_block(fs, *blocknr, ctx->buf);
	if (ctx->errcode)
		return BLOCK_ABORT;

	while (offset < fs->blocksize) {
		dirent = (struct ext2_dir_entry *)(ctx->buf + offset);
		if (ext2fs_get_rec_len(fs, dirent, &rec_len))
			return BLOCK_ABORT;
		if (((offset + rec_len) > fs->blocksize) ||
		    (rec_len < 8) ||
		    ((rec_len % 4) != 0) ||
		    ((((unsigned)dirent->name_len & 0xFF)+8) > rec_len)) {
			ctx->errcode = EXT2_ET_DIR_CORRUPTED;
			return BLOCK_ABORT;
		}
		if (!dirent->inode &&
		    !(ctx->flags & DIRENT_FLAG_INCLUDE_EMPTY))
			goto next;

		ret = (ctx->func)(ctx->dir,
				  (next_real_entry > offset) ?
				  DIRENT_DELETED_FILE : entry,
				  dirent, offset,
				  fs->blocksize, ctx->buf,
				  ctx->priv_data);
		if (entry < DIRENT_OTHER_FILE)
			entry++;

		if (ret & DIRENT_CHANGED) {
			if (ext2fs_get_rec_len(fs, dirent, &rec_len))
				return BLOCK_ABORT;
			changed++;
		}
		if (ret & DIRENT_ABORT) {
			do_abort++;
			break;
		}
next:
		if (next_real_entry == offset)
			next_real_entry += rec_len;

		if (ctx->flags & DIRENT_FLAG_INCLUDE_REMOVED) {
			size = ((dirent->name_len & 0xFF) + 11) & ~3;

			if (rec_len != size)  {
				unsigned int final_offset;

				final_offset = offset + rec_len;
				offset += size;
				while (offset < final_offset &&
					 !ext2fs_validate_entry(fs, ctx->buf,
					offset,
					final_offset))

					offset += 4;
				continue;
			}
		}
		offset += rec_len;
	}

	if (changed) {
		ctx->errcode = ext2fs_write_dir_block(fs, *blocknr, ctx->buf);
		if (ctx->errcode)
			return BLOCK_ABORT;
	}
	if (do_abort)
		return BLOCK_ABORT;
	return 0;
}

errcode_t ext2fs_dir_iterate2(ext2_filsys fs,
			      ext2_ino_t dir,
			      int flags,
			      char *block_buf,
			      int (*func)(ext2_ino_t	dir,
					  int		entry,
					  struct ext2_dir_entry *dirent,
					  int	offset,
					  int	blocksize,
					  char	*buf,
					  void	*priv_data),
			      void *priv_data)
{
	struct		dir_context	ctx;
	errcode_t	retval;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	retval = ext2fs_check_directory(fs, dir);
	if (retval)
		return retval;

	ctx.dir = dir;
	ctx.flags = flags;
	if (block_buf) {
		ctx.buf = block_buf;
	} else {
		retval = ext2fs_get_mem(fs->blocksize, &ctx.buf);
		if (retval)
			return retval;
	}
	ctx.func = func;
	ctx.priv_data = priv_data;
	ctx.errcode = 0;
	retval = ext2fs_block_iterate2(fs, dir, BLOCK_FLAG_READ_ONLY, 0,
				       ext2fs_process_dir_block, &ctx);
	if (!block_buf)
		ext2fs_free_mem(&ctx.buf);
	if (retval)
		return retval;
	return ctx.errcode;
}

errcode_t ext2fs_dir_iterate(ext2_filsys fs,
			      ext2_ino_t dir,
			      int flags,
			      char *block_buf,
			      int (*func)(struct ext2_dir_entry *dirent,
					  int	offset,
					  int	blocksize,
					  char	*buf,
					  void	*priv_data),
			      void *priv_data)
{
	struct xlate xl;

	xl.real_private = priv_data;
	xl.func = func;

	return ext2fs_dir_iterate2(fs, dir, flags, block_buf,
				   xlate_func, &xl);
}

struct lookup_struct  {
	const char	*name;
	int		len;
	ext2_ino_t	*inode;
	int		found;
};

static int lookup_proc(struct ext2_dir_entry *dirent,
		       int	offset,
		       int	blocksize,
		       char	*buf,
		       void	*priv_data)
{
	struct lookup_struct *ls = (struct lookup_struct *)priv_data;

	if (ls->len != (dirent->name_len & 0xFF))
		return 0;
	if (strncmp(ls->name, dirent->name, (dirent->name_len & 0xFF)))
		return 0;
	*ls->inode = dirent->inode;
	ls->found++;
	return DIRENT_ABORT;
}

errcode_t ext2fs_lookup(ext2_filsys fs, ext2_ino_t dir, const char *name,
			int namelen, char *buf, ext2_ino_t *inode)
{
	errcode_t	retval;
	struct lookup_struct ls;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	ls.name = name;
	ls.len = namelen;
	ls.inode = inode;
	ls.found = 0;

	retval = ext2fs_dir_iterate(fs, dir, 0, buf, lookup_proc, &ls);
	if (retval)
		return retval;

	return (ls.found) ? 0 : EXT2_ET_FILE_NOT_FOUND;
}

struct link_struct  {
	ext2_filsys	fs;
	const char	*name;
	int		namelen;
	ext2_ino_t	inode;
	int		flags;
	int		done;
	unsigned int	blocksize;
	errcode_t	err;
	struct ext2_super_block *sb;
};

static int link_proc(struct ext2_dir_entry *dirent,
		     int	offset,
		     int	blocksize,
		     char	*buf,
		     void	*priv_data)
{
	struct link_struct *ls = (struct link_struct *)priv_data;
	struct ext2_dir_entry *next;
	unsigned int rec_len, min_rec_len, curr_rec_len;
	int ret = 0;

	rec_len = EXT2_DIR_REC_LEN(ls->namelen);

	ls->err = ext2fs_get_rec_len(ls->fs, dirent, &curr_rec_len);
	if (ls->err)
		return DIRENT_ABORT;

	/*
	 * See if the following directory entry (if any) is unused;
	 * if so, absorb it into this one.
	 */
	next = (struct ext2_dir_entry *)(buf + offset + curr_rec_len);
	if ((offset + curr_rec_len < blocksize - 8) &&
	    (next->inode == 0) &&
	    (offset + curr_rec_len + next->rec_len <= blocksize)) {
		curr_rec_len += next->rec_len;
		ls->err = ext2fs_set_rec_len(ls->fs, curr_rec_len, dirent);
		if (ls->err)
			return DIRENT_ABORT;
		ret = DIRENT_CHANGED;
	}

	/*
	 * If the directory entry is used, see if we can split the
	 * directory entry to make room for the new name.  If so,
	 * truncate it and return.
	 */
	if (dirent->inode) {
		min_rec_len = EXT2_DIR_REC_LEN(dirent->name_len & 0xFF);
		if (curr_rec_len < (min_rec_len + rec_len))
			return ret;
		rec_len = curr_rec_len - min_rec_len;
		ls->err = ext2fs_set_rec_len(ls->fs, min_rec_len, dirent);
		if (ls->err)
			return DIRENT_ABORT;
		next = (struct ext2_dir_entry *)(buf + offset +
						 dirent->rec_len);
		next->inode = 0;
		next->name_len = 0;
		ls->err = ext2fs_set_rec_len(ls->fs, rec_len, next);
		if (ls->err)
			return DIRENT_ABORT;
		return DIRENT_CHANGED;
	}

	/*
	 * If we get this far, then the directory entry is not used.
	 * See if we can fit the request entry in.  If so, do it.
	 */
	if (curr_rec_len < rec_len)
		return ret;
	dirent->inode = ls->inode;
	dirent->name_len = ls->namelen;
	strncpy(dirent->name, ls->name, ls->namelen);
	if (ls->sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE)
		dirent->name_len |= (ls->flags & 0x7) << 8;

	ls->done++;
	return DIRENT_ABORT|DIRENT_CHANGED;
}

/*
 * Note: the low 3 bits of the flags field are used as the directory
 * entry filetype.
 */
errcode_t ext2fs_link(ext2_filsys fs, ext2_ino_t dir, const char *name,
		      ext2_ino_t ino, int flags)
{
	errcode_t		retval;
	struct link_struct	ls;
	struct ext2fs_inode	inode;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!(fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	ls.fs = fs;
	ls.name = name;
	ls.namelen = name ? strlen(name) : 0;
	ls.inode = ino;
	ls.flags = flags;
	ls.done = 0;
	ls.sb = fs->super;
	ls.blocksize = fs->blocksize;
	ls.err = 0;

	retval = ext2fs_dir_iterate(fs, dir, DIRENT_FLAG_INCLUDE_EMPTY,
				    0, link_proc, &ls);
	if (retval)
		return retval;
	if (ls.err)
		return ls.err;

	if (!ls.done)
		return EXT2_ET_DIR_NO_SPACE;

	retval = ext2fs_read_inode(fs, dir, &inode);
	if (retval != 0)
		return retval;

	if (inode.i_flags & EXT2_INDEX_FL) {
		inode.i_flags &= ~EXT2_INDEX_FL;
		retval = ext2fs_write_inode(fs, dir, &inode);
		if (retval != 0)
			return retval;
	}

	return 0;
}

errcode_t ext2fs_iblk_add_blocks(ext2_filsys fs, struct ext2fs_inode *inode,
				 blk64_t num_blocks)
{
	unsigned long long b = inode->i_blocks;

	if (!(fs->super->s_feature_ro_compat &
	    EXT4_FEATURE_RO_COMPAT_HUGE_FILE) ||
	    !(inode->i_flags & EXT4_HUGE_FILE_FL))
		num_blocks *= fs->blocksize / 512;

	b += num_blocks;

	if (fs->super->s_feature_ro_compat &
	    EXT4_FEATURE_RO_COMPAT_HUGE_FILE) {
		b += ((long long) inode->osd2.linux2.l_i_blocks_hi) << 32;
		inode->osd2.linux2.l_i_blocks_hi = b >> 32;
	} else if (b > 0xFFFFFFFF) {
		return 75;
	}
	inode->i_blocks = b & 0xFFFFFFFF;
	return 0;
}

struct expand_dir_struct {
	int		done;
	int		newblocks;
	errcode_t	err;
};

static int expand_dir_proc(ext2_filsys	fs,
			   blk_t	*blocknr,
			   e2_blkcnt_t	blockcnt,
			   blk_t	ref_block,
			   int		ref_offset,
			   void		*priv_data)
{
	struct expand_dir_struct *es = (struct expand_dir_struct *)priv_data;
	blk_t	new_blk;
	static blk_t	last_blk;
	char		*block;
	errcode_t	retval;

	if (*blocknr) {
		last_blk = *blocknr;
		return 0;
	}
	retval = ext2fs_new_block(fs, last_blk, 0, &new_blk);
	if (retval) {
		es->err = retval;
		return BLOCK_ABORT;
	}
	if (blockcnt > 0) {
		retval = ext2fs_new_dir_block(fs, 0, 0, &block);
		if (retval) {
			es->err = retval;
			return BLOCK_ABORT;
		}
		es->done = 1;
		retval = ext2fs_write_dir_block(fs, new_blk, block);
	} else {
		retval = ext2fs_get_mem(fs->blocksize, &block);
		if (retval) {
			es->err = retval;
			return BLOCK_ABORT;
		}
		memset(block, 0, fs->blocksize);

	put_ext4fs(dev_desc, (uint64_t)(new_blk * fs->blocksize),
		   block, (uint32_t)fs->blocksize);
	retval = 0;
	}
	if (retval) {
		es->err = retval;
		return BLOCK_ABORT;
	}
	ext2fs_free_mem(&block);
	*blocknr = new_blk;
	ext2fs_block_alloc_stats(fs, new_blk, 1);
	es->newblocks++;

	if (es->done)
		return BLOCK_CHANGED | BLOCK_ABORT;
	else
		return BLOCK_CHANGED;
}

errcode_t ext2fs_expand_dir(ext2_filsys fs, ext2_ino_t dir)
{
	errcode_t	retval;
	struct expand_dir_struct es;
	struct ext2fs_inode	inode;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!(fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!fs->block_map)
		return EXT2_ET_NO_BLOCK_BITMAP;

	retval = ext2fs_check_directory(fs, dir);
	if (retval)
		return retval;

	es.done = 0;
	es.err = 0;
	es.newblocks = 0;

	retval = ext2fs_block_iterate2(fs, dir, BLOCK_FLAG_APPEND,
				       0, expand_dir_proc, &es);

	if (es.err)
		return es.err;
	if (!es.done)
		return EXT2_ET_EXPAND_DIR_ERR;

	/*
	 * Update the size and block count fields in the inode.
	 */
	retval = ext2fs_read_inode(fs, dir, &inode);
	if (retval)
		return retval;

	inode.i_size += fs->blocksize;
	ext2fs_iblk_add_blocks(fs, &inode, es.newblocks);
	retval = ext2fs_write_inode(fs, dir, &inode);
	if (retval)
		return retval;

	return 0;
}
