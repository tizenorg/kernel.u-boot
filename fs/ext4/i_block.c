/*
 * i_block.c --- Manage the i_block field for i_blocks
 *
 * Copyright (C) 2008 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Library
 * General Public License, version 2.
 * %End-Header%
 */

#include "ext2_fs.h"
#include "ext2fs.h"

errcode_t ext2fs_iblk_add_blocks(ext2_filsys fs, struct ext2_inode *inode,
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
	} else if (b > 0xFFFFFFFF)
		//uma return EOVERFLOW;
		return 75;
	inode->i_blocks = b & 0xFFFFFFFF;
	return 0;
}

errcode_t ext2fs_iblk_sub_blocks(ext2_filsys fs, struct ext2_inode *inode,
				 blk64_t num_blocks)
{
	unsigned long long b = inode->i_blocks;

	if (!(fs->super->s_feature_ro_compat &
	      EXT4_FEATURE_RO_COMPAT_HUGE_FILE) ||
	    !(inode->i_flags & EXT4_HUGE_FILE_FL))
	    num_blocks *= fs->blocksize / 512;

	if (num_blocks > b)
		//uma return EOVERFLOW;
		return 75;

	b -= num_blocks;

	if (fs->super->s_feature_ro_compat &
	    EXT4_FEATURE_RO_COMPAT_HUGE_FILE) {
		b += ((long long) inode->osd2.linux2.l_i_blocks_hi) << 32;
		inode->osd2.linux2.l_i_blocks_hi = b >> 32;
	}
	inode->i_blocks = b & 0xFFFFFFFF;
	return 0;
}

errcode_t ext2fs_iblk_set(ext2_filsys fs, struct ext2_inode *inode, blk64_t b)
{
	if (!(fs->super->s_feature_ro_compat &
	      EXT4_FEATURE_RO_COMPAT_HUGE_FILE) ||
	    !(inode->i_flags & EXT4_HUGE_FILE_FL))
		b *= fs->blocksize / 512;

	inode->i_blocks = b & 0xFFFFFFFF;
	if (fs->super->s_feature_ro_compat & EXT4_FEATURE_RO_COMPAT_HUGE_FILE)
		inode->osd2.linux2.l_i_blocks_hi = b >> 32;
	else if (b >> 32)
		//uma return EOVERFLOW;
		return 75;
	return 0;
}