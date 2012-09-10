/*
 * mke2fs.c - Make a ext2fs filesystem.
 *
 * Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002,
 * 	2003, 2004, 2005 by Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

/* Usage: mke2fs [options] device
 *
 * The device may be a block device or a image of one, but this isn't
 * enforced (but it's not much fun on a character device :-).
 */

#include "ext2_fs.h"
#include "ext2fs.h"
#include "util.h"
#include "profile.h"
#include <common.h>
#include <ext2fs.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/stat.h>
#include <linux/time.h>

#define STRIDE_LENGTH 8
block_dev_desc_t *dev_desc=NULL;
extern void *my_buffer;
extern unsigned long my_block_nr;
unsigned int my_journal_blocks;

extern int isatty(int);
#define EFBIG 27

const char * program_name = "mke2fs";
const char * device_name /* = NULL */;

/* Command line options */
int	cflag;
int	quiet;
int	super_only;
int	discard = 1;	/* attempt to discard device before fs creation */
int	force;
int	noaction;
int	journal_size;
int	journal_flags;
int	lazy_itable_init;
char	*bad_blocks_filename;
__u32	fs_stride;

struct ext2_super_block fs_param;
char *fs_uuid = NULL;
char *creator_os;
char *volume_label;
char *mount_dir;
char *journal_device;
int sync_kludge;	/* Set using the MKE2FS_SYNC env. option */

profile_t	profile;

int sys_page_size = 4096;
int linux_version_code = 0;
/* Added to wipe out old file system headers */
#define FS_TYPE_VFAT	(1 << 1)
#define FS_TYPE_EXT4	(1 << 2)
#define FS_BLOCK_SIZE 512

static unsigned long part_offset;
struct fs_check {
	unsigned int type;
	char *name;
	unsigned int offset;
	unsigned int magic_sz;
	char magic[16];
};

static struct fs_check fs_types[2] = {
	{
		FS_TYPE_EXT4,
		"ext4",
		0x438,
		2,
		{0x53, 0xef}
	},
	{
		FS_TYPE_VFAT,
		"vfat",
		0x52,
		5,
		{'F', 'A', 'T', '3', '2'}
	},
};

static int int_log2(int arg)
{
	int	l = 0;

	arg >>= 1;
	while (arg) {
		l++;
		arg >>= 1;
	}
	return l;
}

static int int_log10(unsigned int arg)
{
	int	l;

	for (l=0; arg ; l++)
		arg = arg / 10;
	return l;
}
static void handle_bad_blocks(ext2_filsys fs, badblocks_list bb_list)
{
	dgrp_t			i;
	blk_t			j;
	unsigned 		must_be_good;
	blk_t			blk;
	badblocks_iterate	bb_iter;
	errcode_t		retval;
	blk_t			group_block;
	int			group;
	int			group_bad;

	if (!bb_list)
		return;

	/*
	 * The primary superblock and group descriptors *must* be
	 * good; if not, abort.
	 */
	must_be_good = fs->super->s_first_data_block + 1 + fs->desc_blocks;
	for (i = fs->super->s_first_data_block; i <= must_be_good; i++) {
		if (ext2fs_badblocks_list_test(bb_list, i)) {
			printf("Block %d in primary "
				"superblock/group descriptor area bad.\n", i);
			printf("Blocks %u through %u must be good "
				"in order to build a filesystem.\n",
				fs->super->s_first_data_block, must_be_good);
			printf("Aborting....\n");
			return;
		}
	}

	/*
	 * See if any of the bad blocks are showing up in the backup
	 * superblocks and/or group descriptors.  If so, issue a
	 * warning and adjust the block counts appropriately.
	 */
	group_block = fs->super->s_first_data_block +
		fs->super->s_blocks_per_group;

	for (i = 1; i < fs->group_desc_count; i++) {
		group_bad = 0;
		for (j=0; j < fs->desc_blocks+1; j++) {
			if (ext2fs_badblocks_list_test(bb_list,
						       group_block + j)) {
				if (!group_bad)
					printf("Warning: the backup superblock/group descriptors at block %u contain\n"
					"	bad blocks.\n\n",
						group_block);
				group_bad++;
				group = ext2fs_group_of_blk(fs, group_block+j);
				fs->group_desc[group].bg_free_blocks_count++;
				ext2fs_group_desc_csum_set(fs, group);
				fs->super->s_free_blocks_count++;
			}
		}
		group_block += fs->super->s_blocks_per_group;
	}

	/*
	 * Mark all the bad blocks as used...
	 */
	retval = ext2fs_badblocks_list_iterate_begin(bb_list, &bb_iter);
	if (retval) {
		printf("while marking bad blocks as used\n");
		return;
	}
	while (ext2fs_badblocks_list_iterate(bb_iter, &blk))
		ext2fs_mark_block_bitmap(fs->block_map, blk);
	ext2fs_badblocks_list_iterate_end(bb_iter);
}

/*
 * These functions implement a generalized progress meter.
 */
struct progress_struct {
	char		format[20];
	char		backup[80];
	__u32		max;
	int		skip_progress;
};

static void progress_init(struct progress_struct *progress,
			  const char *label,__u32 max)
{
	int	i;

	memset(progress, 0, sizeof(struct progress_struct));
	if (quiet)
		return;

	/*
	 * Figure out how many digits we need
	 */
	i = int_log10(max);
	sprintf(progress->format, "%%%dd/%%%dld", i, i);
	memset(progress->backup, '\b', sizeof(progress->backup)-1);
	progress->backup[sizeof(progress->backup)-1] = 0;
	if ((2*i)+1 < (int) sizeof(progress->backup))
		progress->backup[(2*i)+1] = 0;
	progress->max = max;

	progress->skip_progress = 0;
	if (getenv("MKE2FS_SKIP_PROGRESS"))
		progress->skip_progress++;

	printf("%s",label);
}

static void progress_update(struct progress_struct *progress, __u32 val)
{
	if ((progress->format[0] == 0) || progress->skip_progress)
		return;
	printf(progress->format, val, progress->max);
	printf("%s\n",progress->backup);
}

static void progress_close(struct progress_struct *progress)
{
	if (progress->format[0] == 0)
		return;
	printf("done\n");
}

static void write_inode_tables(ext2_filsys fs, int lazy_flag, int itable_zeroed)
{
	errcode_t	retval;
	blk_t		blk;
	dgrp_t		i;
	int		num, ipb;
	struct progress_struct progress;

	if (quiet)
		memset(&progress, 0, sizeof(progress));
	else
		progress_init(&progress, "Writing inode tables: ",
			      fs->group_desc_count);

	for (i = 0; i < fs->group_desc_count; i++) {
		//progress_update(&progress, i); commented by manju

		blk = fs->group_desc[i].bg_inode_table;
		num = fs->inode_blocks_per_group;

		if (lazy_flag) {
			ipb = fs->blocksize / EXT2_INODE_SIZE(fs->super);
			num = ((((fs->super->s_inodes_per_group -
				  fs->group_desc[i].bg_itable_unused) *
				 EXT2_INODE_SIZE(fs->super)) +
				EXT2_BLOCK_SIZE(fs->super) - 1) /
			       EXT2_BLOCK_SIZE(fs->super));
		}
		if (!lazy_flag || itable_zeroed) {
			/* The kernel doesn't need to zero the itable blocks */
			fs->group_desc[i].bg_flags |= EXT2_BG_INODE_ZEROED;
			ext2fs_group_desc_csum_set(fs, i);
		}
		retval = ext2fs_zero_blocks(fs, blk, num, &blk, &num,dev_desc);

		if (retval) {
			printf("\nCould not write %d "
				  "blocks in inode table starting at %u: \n",	num, blk);
			return ;
		}
		if (sync_kludge) {
			if (sync_kludge == 1)
				sync();
			else if ((i % sync_kludge) == 0)
				sync();
		}
	}

	ext2fs_zero_blocks(0, 0, 0, 0, 0,dev_desc);
	progress_close(&progress);
}

static int geteuid(void)
{
	return 1;
}

static int getuid(void)
{
	return 0;
}

static int getgid(void)
{
	return 0;
}

static void create_root_dir(ext2_filsys fs)
{
	errcode_t		retval;
	struct ext2_inode	inode;
	__u32			uid, gid;

	retval = ext2fs_mkdir(fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 0);
	if (retval) {
		printf("while creating root dir \n");
		return;
	}

	if (geteuid()) {
		retval = ext2fs_read_inode(fs, EXT2_ROOT_INO, &inode);
		if (retval) {
			printf("while reading root inode");
			return ;
		}

		uid = getuid();
		inode.i_uid = uid;
		ext2fs_set_i_uid_high(inode, uid >> 16);
		if (uid) {
			gid = getgid();
			inode.i_gid = gid;
			ext2fs_set_i_gid_high(inode, gid >> 16);
		}

		retval = ext2fs_write_new_inode(fs, EXT2_ROOT_INO, &inode);
		if (retval) {
			printf("while setting root inode ownership");
			return ;
		}

	}
}

static void create_lost_and_found(ext2_filsys fs)
{
	unsigned int		lpf_size = 0;
	errcode_t		retval;
	ext2_ino_t		ino;
	const char		*name = "lost+found";
	int			i;

	fs->umask = 077;
	retval = ext2fs_mkdir(fs, EXT2_ROOT_INO, 0, name);
	if (retval) {
		printf("while creating /lost+found");
		return;
	}

	retval = ext2fs_lookup(fs, EXT2_ROOT_INO, name, strlen(name), 0, &ino);
	if (retval) {
		printf("while looking up /lost+found");
		return ;
	}

	for (i=1; i < EXT2_NDIR_BLOCKS; i++) {
		/* Ensure that lost+found is at least 2 blocks, so we always
		 * test large empty blocks for big-block filesystems.  */
		if ((lpf_size += fs->blocksize) >= 16*1024 &&
		    lpf_size >= 2 * fs->blocksize)
			break;
		retval = ext2fs_expand_dir(fs, ino);
		if (retval) {
			printf("while expanding /lost+found");
			return ;
		}
	}
}

static void create_bad_block_inode(ext2_filsys fs, badblocks_list bb_list)
{
	errcode_t	retval;

	ext2fs_mark_inode_bitmap(fs->inode_map, EXT2_BAD_INO);
	ext2fs_inode_alloc_stats2(fs, EXT2_BAD_INO, +1, 0);
	retval = ext2fs_update_bb_inode(fs, bb_list);
	if (retval) {
		printf("while setting bad block inode");
		return;
	}
}

static void reserve_inodes(ext2_filsys fs)
{
	ext2_ino_t	i;

	for (i = EXT2_ROOT_INO + 1; i < EXT2_FIRST_INODE(fs->super); i++)
		ext2fs_inode_alloc_stats2(fs, i, +1, 0);
	ext2fs_mark_ib_dirty(fs);
}

static void show_stats(ext2_filsys fs)
{
	struct ext2_super_block *s = fs->super;
	char 			buf[80];
	blk_t			group_block;
	dgrp_t			i;
	int			need, col_left;
	u16 a=5;

	if (fs_param.s_blocks_count != s->s_blocks_count)
		printf("warning: %u blocks unused.\n\n",
		       fs_param.s_blocks_count - s->s_blocks_count);

	memset(buf, 0, sizeof(buf));
	strncpy(buf, s->s_volume_name, sizeof(s->s_volume_name));
	printf("Filesystem label=%s\n", buf);
	printf("OS type:=Linux");
	printf("\n");
	printf("Block size=%u (log=%u)\n", fs->blocksize,
		s->s_log_block_size);
	printf("Fragment size=%u (log=%u)\n", fs->fragsize,
		s->s_log_frag_size);
	printf("Stride=%u blocks, Stripe width=%u blocks\n",
	       s->s_raid_stride, s->s_raid_stripe_width);
	printf("%u inodes, %u blocks\n", s->s_inodes_count,
	       s->s_blocks_count);
	printf("%u blocks [%u%s reserved for the super user\n",
		s->s_r_blocks_count,a,"%]");
	printf("First data block=%u\n", s->s_first_data_block);
	if (s->s_reserved_gdt_blocks)
		printf("Maximum filesystem blocks=%lu\n",
		       (s->s_reserved_gdt_blocks + fs->desc_blocks) *
		       EXT2_DESC_PER_BLOCK(s) * s->s_blocks_per_group);
	if (fs->group_desc_count > 1)
		printf("%u block groups\n", fs->group_desc_count);
	else
		printf("%u block group\n", fs->group_desc_count);
	printf("%u blocks per group, %u fragments per group\n",
	       s->s_blocks_per_group, s->s_frags_per_group);
	printf("%u inodes per group\n", s->s_inodes_per_group);

	if (fs->group_desc_count == 1) {
		printf("\n");
		return;
	}

	printf("Superblock backups stored on blocks: ");
	group_block = s->s_first_data_block;
	col_left = 0;
	for (i = 1; i < fs->group_desc_count; i++) {
		group_block += s->s_blocks_per_group;
		if (!ext2fs_bg_has_super(fs, i))
			continue;
		if (i != 1)
			printf(", ");
		need = int_log10(group_block) + 2;
		if (need > col_left) {
			printf("\n\t");
			col_left = 72;
		}
		col_left -= need;
		printf("%u", group_block);
	}
	printf("\n\n");
}

static __u32 ok_features[3] = {
	/* Compat */
	EXT3_FEATURE_COMPAT_HAS_JOURNAL |
		EXT2_FEATURE_COMPAT_RESIZE_INODE |
		EXT2_FEATURE_COMPAT_DIR_INDEX |
		EXT2_FEATURE_COMPAT_EXT_ATTR,
	/* Incompat */
	EXT2_FEATURE_INCOMPAT_FILETYPE|
		EXT3_FEATURE_INCOMPAT_EXTENTS|
		EXT3_FEATURE_INCOMPAT_JOURNAL_DEV|
		EXT2_FEATURE_INCOMPAT_META_BG|
		EXT4_FEATURE_INCOMPAT_FLEX_BG,
	/* R/O compat */
	EXT2_FEATURE_RO_COMPAT_LARGE_FILE|
		EXT4_FEATURE_RO_COMPAT_HUGE_FILE|
		EXT4_FEATURE_RO_COMPAT_DIR_NLINK|
		EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE|
		EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER|
		EXT4_FEATURE_RO_COMPAT_GDT_CSUM
};


static void edit_feature(const char *str, __u32 *compat_array)
{
	if (!str)
		return;

	if (e2p_edit_feature(str, compat_array, ok_features)) {
		printf("Invalid filesystem option set: %s\n",
			str);
	return ;
	}
}

struct str_list {
	char **list;
	int num;
	int max;
};
unsigned int num_blk=0;

static void PRS(block_dev_desc_t *dev_desc, int part_no)
{
		char		*tmp;
		int		blocksize = 0;
		int		inode_ratio = 0;
		int		inode_size = 0;
		unsigned long	flex_bg_size = 0;
		double		reserved_ratio = 5.0;
		int		lsector_size = 0, psector_size = 0;
		unsigned long long num_inodes = 0; /* unsigned long long to catch too-large input */
		unsigned long	retval;
		blk_t 	dev_size;
		int		use_bsize;
		__u64 partinfo_size=0;
		disk_partition_t info;
		#define M 1024 * 1024

		if (!get_partition_info(dev_desc, part_no, &info)) {
			partinfo_size = (info.size * info.blksz);
			part_offset = info.start;
		} else {
			printf("error : get partition info\n");
			return ;
		}

		if (blocksize <= 0)
		{
			if(partinfo_size<=(3* M))
				{
				use_bsize=1024;
				inode_size = 128;
				inode_ratio = 8192;
				}
			else if(partinfo_size>(3 * M )&& partinfo_size <= (512 * M ))
				{
				use_bsize=1024;
				inode_size = 128;
				inode_ratio = 4096;
				}
			else
				{
				use_bsize=4096;
				inode_size = 256;
				inode_ratio = 16384;
				}

		}
		blocksize = use_bsize;
		num_blk= partinfo_size/blocksize;
		memset(&fs_param, 0, sizeof(struct ext2_super_block));
		fs_param.s_rev_level = 1;	/* Create revision 1 filesystems now */

		program_name = "mkfs.ext4";

		/* If called as mkfs.ext3, create a journal inode */
		if (!strcmp(program_name, "mkfs.ext3") ||
			!strcmp(program_name, "mke3fs"))
			journal_size = -1;

		/*currently hardcode to ext4.img */
		device_name = "ext4.img";

		fs_param.s_log_block_size =
					int_log2(blocksize >>
						 EXT2_MIN_BLOCK_LOG_SIZE);

		fs_param.s_log_frag_size = fs_param.s_log_block_size;
		if (noaction && fs_param.s_blocks_count) {
			dev_size = fs_param.s_blocks_count;
			retval = 0;
		} else {
			dev_size=partinfo_size/blocksize;
		}

		if (!fs_param.s_blocks_count) {
			if (retval == EXT2_ET_UNIMPLEMENTED) {
				printf("Couldn't determine device size; you "
					"must specify\nthe size of the "
					"filesystem\n");
				return ;
			} else {
				if (dev_size == 0) {
					printf("Device size reported to be zero.  "
					  "Invalid partition specified, or\n\t"
					  "partition table wasn't reread "
					  "after running fdisk, due to\n\t"
					  "a modified partition being busy "
					  "and in use.  You may need to reboot\n\t"
					  "to re-read your partition table.\n");
					return ;
				}
				fs_param.s_blocks_count = dev_size;
				if (sys_page_size > EXT2_BLOCK_SIZE(&fs_param))
					fs_param.s_blocks_count &= ~((sys_page_size /
							EXT2_BLOCK_SIZE(&fs_param))-1);
			}

		} else if (!force && (fs_param.s_blocks_count > dev_size)) {
			printf("Filesystem larger than apparent device size.");
		}

		/* Figure out what features should be enabled */
		tmp = NULL;
		if (fs_param.s_rev_level != EXT2_GOOD_OLD_REV) {
			tmp="sparse_super,filetype,resize_inode,dir_index,ext_attr";
			edit_feature(tmp, &fs_param.s_feature_compat);
			// free(tmp); //manju
			//tmp="has_journal,extent,huge_file,flex_bg,uninit_bg,dir_nlink,extra_isize";
			tmp="has_journal,extent,flex_bg,uninit_bg,dir_nlink,extra_isize";
			edit_feature(tmp, &fs_param.s_feature_compat);
		}

		/* Older kernels may not have physical/logical distinction */
		if (!psector_size)
			psector_size = lsector_size;

		if (blocksize <= 0)
		{
			if(partinfo_size<=(3* M))
			{
				use_bsize=1024;
				inode_size = 128;
				inode_ratio = 8192;
			}
			else if(partinfo_size>(3 * M )&& partinfo_size <= (512 * M ))
			{
				use_bsize=1024;
				inode_size = 128;
				inode_ratio = 4096;
			}
			else
			{
				use_bsize=4096;
				inode_size = 256;
				inode_ratio = 16384;
			}

			if (use_bsize == -1)
			{
				use_bsize = sys_page_size;
				if ((linux_version_code < (2*65536 + 6*256)) &&
				(use_bsize > 4096))
				use_bsize = 4096;
			}
			if (lsector_size && use_bsize < lsector_size)
				use_bsize = lsector_size;
			if ((blocksize < 0) && (use_bsize < (-blocksize)))
				use_bsize = -blocksize;
			blocksize = use_bsize;

			dev_size= blocksize; //umaa ***
		}
		else
		{
			if (blocksize < lsector_size) {			/* Impossible */
				printf("while setting blocksize; too small "
				"for device\n");
			} else if ((blocksize < psector_size) &&
				(psector_size <= sys_page_size)) {	/* Suboptimal */
					printf("Warning: specified blocksize %d is "
					"less than device physical sectorsize %d\n",
					blocksize, psector_size);
			}
		}
		fs_param.s_log_frag_size = fs_param.s_log_block_size =
			int_log2(blocksize >> EXT2_MIN_BLOCK_LOG_SIZE);

		blocksize = EXT2_BLOCK_SIZE(&fs_param);

		lazy_itable_init = 0;
		/* Since sparse_super is the default, we would only have a problem
		 * here if it was explicitly disabled.
		 */
		if ((fs_param.s_feature_compat & EXT2_FEATURE_COMPAT_RESIZE_INODE) &&
			 !(fs_param.s_feature_ro_compat&EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)) {
		printf("reserved online resize blocks not supported "
				  "on non-sparse filesystem");
			return ;
		}

		if (fs_param.s_blocks_per_group) {
			if (fs_param.s_blocks_per_group < 256 ||
				 fs_param.s_blocks_per_group > 8 * (unsigned) blocksize) {
		printf("blocks per group count out of range");
				return ;
			}
		}
		if (!flex_bg_size && (fs_param.s_feature_incompat &
						EXT4_FEATURE_INCOMPAT_FLEX_BG))
					flex_bg_size = 16;

		if (flex_bg_size) {
			if (!(fs_param.s_feature_incompat &
					EXT4_FEATURE_INCOMPAT_FLEX_BG)) {
		printf("Flex_bg feature not enabled, so "
					  "flex_bg size may not be specified");
				return ;
			}
			fs_param.s_log_groups_per_flex = int_log2(flex_bg_size);
		}
		if (inode_size && fs_param.s_rev_level >= EXT2_DYNAMIC_REV) {
			if (inode_size < EXT2_GOOD_OLD_INODE_SIZE ||
				 inode_size > EXT2_BLOCK_SIZE(&fs_param) ||
				 inode_size & (inode_size - 1)) {
		printf("invalid inode size %d (min %d/max %d)",
					inode_size, EXT2_GOOD_OLD_INODE_SIZE,
					blocksize);
				return ;
			}
			fs_param.s_inode_size = inode_size;
		}
		/* Make sure number of inodes specified will fit in 32 bits */
		if (num_inodes == 0) {
			unsigned long long n;
			n = (unsigned long long) fs_param.s_blocks_count * blocksize / inode_ratio;
			if (n > ~0U) {
		printf("too many inodes (%llu), raise inode ratio?", n);
				return ;
			}
		} else if (num_inodes > ~0U) {
		printf("too many inodes (%llu), specify < 2^32 inodes",num_inodes);
			return ;
		}
		/*
		 * Calculate number of inodes based on the inode ratio
		 */
		fs_param.s_inodes_count = num_inodes ? num_inodes :
			((__u64) fs_param.s_blocks_count * blocksize)
				/ inode_ratio;

		if ((((long long)fs_param.s_inodes_count) *
			  (inode_size ? inode_size : EXT2_GOOD_OLD_INODE_SIZE)) >=
			 (((long long)fs_param.s_blocks_count) *
			  EXT2_BLOCK_SIZE(&fs_param))) {
			printf("inode_size (%u) * inodes_count "
						  "(%u) too big for a\n\t"
						  "filesystem with %lu blocks, "
						  "specify higher inode_ratio (-i)\n\t"
						  "or lower inode count (-N).\n",
				inode_size ? inode_size : EXT2_GOOD_OLD_INODE_SIZE,
				fs_param.s_inodes_count,
				(unsigned long) fs_param.s_blocks_count);
			return ;
		}

		/*
		 * Calculate number of blocks to reserve
		 */
		fs_param.s_r_blocks_count = (unsigned int) (reserved_ratio *
						fs_param.s_blocks_count / 100.0);
}

/*
 * Find a reasonable journal file size (in blocks) given the number of blocks
 * in the filesystem.  For very small filesystems, it is not reasonable to
 * have a journal that fills more than half of the filesystem.
 */
int ext2fs_default_journal_size(__u64 blocks)
{
	if (blocks < 2048)
		return -1;
	if (blocks < 32768)
		return (1024);
	if (blocks < 256*1024)
		return (4096);
	if (blocks < 512*1024)
		return (8192);
	if (blocks < 1024*1024)
		return (16384);
	return 32768;
}

void print_check_message(ext2_filsys fs)
{
	printf("\nThis filesystem will be automatically "
		 "checked every %d mounts or\n"
		 "%u days, whichever comes first.  "
		 "Use tune2fs -c or -i to override.\n",
			 fs->super->s_max_mnt_count,
			 180);
}


/*
 * Determine the number of journal blocks to use, either via
 * user-specified # of megabytes, or via some intelligently selected
 * defaults.
 *
 * Find a reasonable journal file size (in blocks) given the number of blocks
 * in the filesystem.  For very small filesystems, it is not reasonable to
 * have a journal that fills more than half of the filesystem.
 */
unsigned int figure_journal_size(int size, ext2_filsys fs)
{
	int j_blocks;

	j_blocks = ext2fs_default_journal_size(fs->super->s_blocks_count);
	if (j_blocks < 0) {
		printf("\nFilesystem too small for a journal\n");
		return 0;
	}

	if (size > 0) {
		j_blocks = size * 1024 / (fs->blocksize	/ 1024);
		if (j_blocks < 1024 || j_blocks > 10240000) {
			printf("\nThe requested journal "
				"size is %d blocks; it must be\n"
				"between 1024 and 10240000 blocks.	"
				"Aborting.\n",j_blocks);
			return -1;
		}
		if ((unsigned) j_blocks > fs->super->s_free_blocks_count / 2) {
		printf("Journal size too big for filesystem.\n");
			return -1;
		}
	}
	return j_blocks;
}

int  mkfs_ext4(block_dev_desc_t *dev_desc_t, int part_no)
{
	unsigned long	retval = 0;
	ext2_filsys	fs;
	badblocks_list	bb_list = 0;
	unsigned int	journal_blocks;
	unsigned int	i;
	int		val;
	io_manager	io_ptr=NULL;
	char		tdb_string[40];
	int		itable_zeroed = 0;

	dev_desc=NULL;
	dev_desc = dev_desc_t;
	PRS(dev_desc, part_no);
	memset(&fs,'\0',sizeof(ext2_filsys));
	/* Added to wipe out old file system headers */
	u32 ofs;
	u8 buf[FS_BLOCK_SIZE];

	if (!dev_desc->block_read)
		return -1;

	for (i = 0; i < ARRAY_SIZE(fs_types); i++) {
		ofs = fs_types[i].offset;

		dev_desc->block_read(dev_desc->dev,
			part_offset + ofs / FS_BLOCK_SIZE, 1, (void *)buf);
		
		/*Wiping out old File System header */
		memset(&buf[ofs % FS_BLOCK_SIZE],'\0',fs_types[i].magic_sz);
		dev_desc->block_write(dev_desc->dev,part_offset + ofs / FS_BLOCK_SIZE,
							FS_BLOCK_SIZE, buf);
	}
	/*
	 * Initialize the superblock....
	 */
	retval = ext2fs_initialize(device_name, EXT2_FLAG_EXCLUSIVE, &fs_param,
				   io_ptr, &fs);
	if (retval) {
		printf("err while setting up superblock\n");
		return -1;

	}
	sprintf(tdb_string, "tdb_data_size=%d", fs->blocksize <= 4096 ?
		32768 : fs->blocksize * 8);

	if (fs_param.s_flags & EXT2_FLAGS_TEST_FILESYS)
		fs->super->s_flags |= EXT2_FLAGS_TEST_FILESYS;

	if ((fs_param.s_feature_incompat &
	     (EXT3_FEATURE_INCOMPAT_EXTENTS|EXT4_FEATURE_INCOMPAT_FLEX_BG)) ||
	    (fs_param.s_feature_ro_compat &
	     (EXT4_FEATURE_RO_COMPAT_HUGE_FILE|EXT4_FEATURE_RO_COMPAT_GDT_CSUM|
	      EXT4_FEATURE_RO_COMPAT_DIR_NLINK|
	      EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE)))
		fs->super->s_kbytes_written = 1;

	//Currently uuid & hash seed is hard code. To do: FIX it
	unsigned char hard_uuid[16] = { 0x4f, 0x25, 0xe8, 0xcf, 0xe7, 0x97, 0x48, 0x23,
			      0xbe, 0xfa, 0xa7, 0x88, 0x4b, 0xae, 0xec, 0xdb };
	memcpy(fs->super->s_uuid, hard_uuid, 16);
	memcpy(fs->super->s_hash_seed,"06ad5de1-cbce-4d9a-a8ba-5c1b34ce916d",32 *sizeof(char));
	//Added to resolve checksum bug
	fs->super->s_desc_size=0;

	/* Fixed the hash version warning*/
	fs->super->s_def_hash_version = EXT2_HASH_HALF_MD4;

	/*
	 * Add "jitter" to the superblock's check interval so that we
	 * don't check all the filesystems at the same time.  We use a
	 * kludgy hack of using the UUID to derive a random jitter value.
	 */
	for (i = 0, val = 0 ; i < sizeof(fs->super->s_uuid); i++)
		val += fs->super->s_uuid[i];
	fs->super->s_max_mnt_count += val % EXT2_DFL_MAX_MNT_COUNT;
	fs->super->s_creator_os = EXT2_OS_LINUX;

	/*
	 * Set the volume label...
	 */
	if (volume_label) {
		memset(fs->super->s_volume_name, 0,
		       sizeof(fs->super->s_volume_name));
		strncpy(fs->super->s_volume_name, volume_label,
			sizeof(fs->super->s_volume_name));
	}

	/*
	 * Set the last mount directory
	 */
	if (mount_dir) {
		memset(fs->super->s_last_mounted, 0,
		       sizeof(fs->super->s_last_mounted));
		strncpy(fs->super->s_last_mounted, mount_dir,
			sizeof(fs->super->s_last_mounted));
	}

	if (!quiet || noaction)
		show_stats(fs);

	handle_bad_blocks(fs, bb_list);
	fs->stride = fs_stride = fs->super->s_raid_stride;
	retval = ext2fs_allocate_tables(fs);
	if (retval) {
		printf("while trying to allocate filesystem tables\n");
		return -1;
	}
	if (super_only) {
		fs->super->s_state |= EXT2_ERROR_FS;
		fs->flags &= ~(EXT2_FLAG_IB_DIRTY|EXT2_FLAG_BB_DIRTY);
	} else {
		/* rsv must be a power of two (64kB is MD RAID sb alignment) */
		unsigned int rsv = 65536 / fs->blocksize;
		unsigned long blocks = fs->super->s_blocks_count;
		unsigned long start;
		blk_t ret_blk;

		/*
		 * Wipe out any old MD RAID (or other) metadata at the end
		 * of the device.  This will also verify that the device is
		 * as large as we think.  Be careful with very small devices.
		 */

		start = (blocks & ~(rsv - 1));
		if (start > rsv)
			start -= rsv;
		if (start > 0)
			retval = ext2fs_zero_blocks(fs, start, blocks - start,
						    &ret_blk, NULL,dev_desc);

		if (retval) {
			printf("while zeroing block %u at end of filesystem\n",ret_blk);
		}
		write_inode_tables(fs, lazy_itable_init, itable_zeroed);
		create_root_dir(fs);
		create_lost_and_found(fs);
		reserve_inodes(fs);
		create_bad_block_inode(fs, bb_list);
		if (fs->super->s_feature_compat &
		    EXT2_FEATURE_COMPAT_RESIZE_INODE) {
			retval = ext2fs_create_resize_inode(fs);
			if (retval) {
				printf("ext2fs_create_resize_inode while reserving blocks for online resize \n");
				return -1;
			}
		}
	}

	if ((journal_size) ||
		   (fs_param.s_feature_compat &
		    EXT3_FEATURE_COMPAT_HAS_JOURNAL)) {
		journal_blocks = figure_journal_size(journal_size, fs);

		if (super_only) {
			printf("Skipping journal creation in super-only mode\n");
			fs->super->s_journal_inum = EXT2_JOURNAL_INO;
			goto no_journal;
		}
		fs->super->s_default_mount_opts=0;

		if (!journal_blocks) {
			fs->super->s_feature_compat &=
				~EXT3_FEATURE_COMPAT_HAS_JOURNAL;
			goto no_journal;
		}
		if (!quiet) {
			printf("Creating journal %u blocks: ",journal_blocks);
		}

		my_journal_blocks = journal_blocks;
		retval = ext2fs_add_journal_inode(fs, journal_blocks,
						  journal_flags);
		if (retval) {
			printf("while trying to create journal");
			return -1;
		}
		/*added by manju for journaling optimization */
		put_ext4(dev_desc, (uint64_t)(my_block_nr * fs->blocksize), my_buffer,(uint32_t)(fs->blocksize));
		if(my_buffer)
		{
			free(my_buffer);
			my_buffer=NULL;
		}
		if (!quiet)
			printf("done\n");
	}

no_journal:

	if (!quiet)
		printf("Writing superblocks and "
		       "filesystem accounting information: ");

	retval = ext2fs_flush(fs);

	if (retval) {
		printf("Warning, had trouble writing out superblocks.\n");
	}
	if (!quiet) {
		printf("done\n");
		print_check_message(fs);
	}
	val = ext4fs_close_clean(fs);
	printf("mkfs.ext4 complete\n\n");
	return (retval || val) ? 1 : 0;
}
