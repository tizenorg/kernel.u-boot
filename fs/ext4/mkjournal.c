/*
 * mkjournal.c --- make a journal for a filesystem
 *
 * Copyright (C) 2000 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Library
 * General Public License, version 2.
 * %End-Header%
 */

#include "util.h"
#include "profile.h"
#include <common.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/stat.h>
#include <linux/time.h>
#include "ext2_fs.h"
#include "e2p.h"
#include "ext2fs.h"
#include "jfs_user.h"

/*
 * This function automatically sets up the journal superblock and
 * returns it as an allocated block.
 */
errcode_t ext2fs_create_journal_superblock(ext2_filsys fs,
					   __u32 size, int flags,
					   char  **ret_jsb)
{
	errcode_t		retval;
	journal_superblock_t	*jsb;

	if (size < 1024)
		return EXT2_ET_JOURNAL_TOO_SMALL;

	if ((retval = ext2fs_get_mem(fs->blocksize, &jsb)))
		return retval;

	memset (jsb, 0, fs->blocksize);

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

	*ret_jsb = (char *) jsb;
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
//#define STRIDE_LENGTH 8
#define STRIDE_LENGTH 128

errcode_t ext2fs_zero_blocks(ext2_filsys fs, blk_t blk, int num,
			     blk_t *ret_blk, int *ret_count,block_dev_desc_t *dev_desc)
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
		if (!buf)
		{
			printf("no memory \n");			
			return -1;
		}
		memset(buf, 0, fs->blocksize * STRIDE_LENGTH);
	}
	
	/* OK, do the write loop */
	j=0;
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
		put_ext4(dev_desc, (uint64_t)(blk *fs->blocksize), buf,(uint32_t)(count *fs->blocksize));
		j += count; blk += count;
	}
	if(buf)
	{
		free(buf);
		buf=NULL;
	}
	return 0;
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
			   blk_t	ref_block EXT2FS_ATTR((unused)),
			   int		ref_offset EXT2FS_ATTR((unused)),
			   void		*priv_data)
{
	struct mkjournal_struct *es = (struct mkjournal_struct *) priv_data;
	blk_t	new_blk;
	errcode_t	retval;
	//static int FLAG=0;
	//FLAG++;
	//printf("FLAG-%d\n",FLAG);
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
	if (blockcnt <= 0)
	{		
		put_ext4(dev_desc, (uint64_t)(new_blk* fs->blocksize), es->buf,(uint32_t) fs->blocksize);
		retval=0;
	}
	else {		
		if (es->zero_count) {
			if ((es->blk_to_zero + es->zero_count == new_blk) &&
			    (es->zero_count < 1024))
				{				
					es->zero_count++;
				}
			else {				
				retval = ext2fs_zero_blocks(fs,
							    es->blk_to_zero,
							    es->zero_count,
							    0, 0,dev_desc); 
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
	ext2fs_block_alloc_stats(fs, new_blk, +1);	
	if (es->num_blocks == 0)
		return (BLOCK_CHANGED | BLOCK_ABORT);
	else
		return BLOCK_CHANGED;

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
	struct ext2_inode	inode;
	struct mkjournal_struct	es;	
	if ((retval = ext2fs_create_journal_superblock(fs, size, flags, &buf)))
		return retval;	
	if ((retval = ext2fs_read_bitmaps(fs)))
		return retval;	
	if ((retval = ext2fs_read_inode(fs, journal_ino, &inode)))
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
		if ((retval = ext2fs_write_inode(fs, journal_ino, &inode)))
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
	} else
		start = (group > 0) ? group-1 : group;
	end = ((group+1) < fs->group_desc_count) ? group+1 : group;
	group = start;
	for (i=start+1; i <= end; i++)
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
					    es.zero_count, 0, 0,NULL); //FOR now NULL is used for dev_desc uma ****
		if (retval)
			goto errout;
	}
	
	if ((retval = ext2fs_read_inode(fs, journal_ino, &inode)))
		goto errout;
	
 	inode.i_size += fs->blocksize * size;
	ext2fs_iblk_add_blocks(fs, &inode, es.newblocks);	
	inode.i_mtime = inode.i_ctime = fs->now ? fs->now : (__u32)NULL;
	inode.i_links_count = 1;
	inode.i_mode = LINUX_S_IFREG | 0600;
	
	if ((retval = ext2fs_write_new_inode(fs, journal_ino, &inode)))
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
		if ((retval = write_journal_inode(fs, journal_ino,
						  size, flags)))					
			return retval;		
	
	fs->super->s_journal_inum = journal_ino;
	fs->super->s_journal_dev = 0;
	memset(fs->super->s_journal_uuid, 0,
	       sizeof(fs->super->s_journal_uuid));
	fs->super->s_feature_compat |= EXT3_FEATURE_COMPAT_HAS_JOURNAL;

	ext2fs_mark_super_dirty(fs);
	return 0;	
}

