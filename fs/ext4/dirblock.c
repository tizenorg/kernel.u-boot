/*
 * dirblock.c --- directory block routines.
 *
 * Copyright (C) 1995, 1996 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Library
 * General Public License, version 2.
 * %End-Header%
 */

#include "ext2_fs.h"
#include "ext2fs.h"

extern block_dev_desc_t *dev_desc;


errcode_t ext2fs_read_dir_block2(ext2_filsys fs, blk_t block,
				 void *buf, int flags EXT2FS_ATTR((unused)))
{
	errcode_t	retval;
	char		*p, *end;
	struct ext2_dir_entry *dirent;
	unsigned int	name_len, rec_len;

 	//uma retval = io_channel_read_blk(fs->io, block, 1, buf);
	int factor = fs->blocksize/512;
	retval= ext2fs_devread(block * factor , 0, fs->blocksize,	buf);
	retval=0; //uma hack
	if (retval)
		return retval;

	p = (char *) buf;
	end = (char *) buf + fs->blocksize;
	while (p < end-8) {
		dirent = (struct ext2_dir_entry *) p;
#ifdef WORDS_BIGENDIAN
		dirent->inode = ext2fs_swab32(dirent->inode);
		dirent->rec_len = ext2fs_swab16(dirent->rec_len);
		dirent->name_len = ext2fs_swab16(dirent->name_len);
#endif
		name_len = dirent->name_len;
#ifdef WORDS_BIGENDIAN
		if (flags & EXT2_DIRBLOCK_V2_STRUCT)
			dirent->name_len = ext2fs_swab16(dirent->name_len);
#endif
		if ((retval = ext2fs_get_rec_len(fs, dirent, &rec_len)) != 0)
			return retval;
		if ((rec_len < 8) || (rec_len % 4)) {
			rec_len = 8;
			retval = EXT2_ET_DIR_CORRUPTED;
		} else if (((name_len & 0xFF) + 8) > rec_len)
			retval = EXT2_ET_DIR_CORRUPTED;
		p += rec_len;
	}
	return retval;
}

errcode_t ext2fs_read_dir_block(ext2_filsys fs, blk_t block,
				 void *buf)
{
	return ext2fs_read_dir_block2(fs, block, buf, 0);
}


errcode_t ext2fs_write_dir_block2(ext2_filsys fs, blk_t block,
				  void *inbuf, int flags EXT2FS_ATTR((unused)))
{
#ifdef WORDS_BIGENDIAN
	errcode_t	retval;
	char		*p, *end;
	char		*buf = 0;
	unsigned int	rec_len;
	struct ext2_dir_entry *dirent;
	retval = ext2fs_get_mem(fs->blocksize, &buf);
	if (retval)
		return retval;
	memcpy(buf, inbuf, fs->blocksize);
	p = buf;
	end = buf + fs->blocksize;
	while (p < end) {
		dirent = (struct ext2_dir_entry *) p;
		if ((retval = ext2fs_get_rec_len(fs, dirent, &rec_len)) != 0)
			return retval;
		if ((rec_len < 8) ||
		    (rec_len % 4)) {
			ext2fs_free_mem(&buf);
			return (EXT2_ET_DIR_CORRUPTED);
		}
		p += rec_len;
		dirent->inode = ext2fs_swab32(dirent->inode);
		dirent->rec_len = ext2fs_swab16(dirent->rec_len);
		dirent->name_len = ext2fs_swab16(dirent->name_len);

		if (flags & EXT2_DIRBLOCK_V2_STRUCT)
			dirent->name_len = ext2fs_swab16(dirent->name_len);
	}
	#if 0
	//uma
 	retval = io_channel_write_blk(fs->io, block, 1, buf);
	#endif
	retval=0;	
	put_ext4(dev_desc, (uint64_t)(block * fs->blocksize), buf,(uint32_t)(fs->blocksize));
	ext2fs_free_mem(&buf);
	return retval;
#else

#if 0
//uma
 	return io_channel_write_blk(fs->io, block, 1, (char *) inbuf);
#endif	
	put_ext4(dev_desc, (uint64_t)(block * fs->blocksize), inbuf,(uint32_t)(fs->blocksize));
	return 0;

#endif
}


errcode_t ext2fs_write_dir_block(ext2_filsys fs, blk_t block,
				 void *inbuf)
{
	return ext2fs_write_dir_block2(fs, block, inbuf, 0);
}
