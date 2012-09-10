
#ifndef _JFS_COMPAT_H
#define _JFS_COMPAT_H

#include "kernel-list.h"
//uma #include <errno.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#define printk printf
#define KERN_ERR ""
#define KERN_DEBUG ""

#define READ 0
#define WRITE 1

//#define cpu_to_be32(n) htonl(n)
//#define be32_to_cpu(n) ntohl(n)

typedef unsigned int tid_t;
typedef struct journal_s journal_t;

//umaa
/* The revoke table is just a simple hash table of revoke records. */
struct jbd_revoke_table_s
{
	/* It is conceivable that we might want a larger hash table
	 * for recovery.	Must be a power of two. */
	int		  hash_size;
	int		  hash_shift;
	struct list_head *hash_table;
};

/* This describes an entire blkid cache file and probed devices.
 * We can traverse all of the found devices via bic_list.
 * We can traverse all of the tag types by bic_tags, which hold empty tags
 * for each tag type.  Those tags can be used as list_heads for iterating
 * through all devices with a specific tag type (e.g. LABEL).
 */
struct blkid_struct_cache
{
	struct list_head	bic_devs;	/* List head of all devices */
	struct list_head	bic_tags;	/* List head of all tag types */
	time_t			bic_time;	/* Last probe time */
	time_t			bic_ftime;	/* Mod time of the cachefile */
	unsigned int		bic_flags;	/* Status flags of the cache */
	char			*bic_filename; /* filename of cache */
};

/*
 * The strategy we use for keeping track of EA refcounts is as
 * follows.  We keep a sorted array of first EA blocks and its
 * reference counts.  Once the refcount has dropped to zero, it is
 * removed from the array to save memory space.  Once the EA block is
 * checked, its bit is set in the block_ea_map bitmap.
 */
struct ea_refcount_el {
	blk_t ea_blk;
	int	ea_count;
};

struct ea_refcount {
	blk_t 	count;
	blk_t 	size;
	blk_t 	cursor;
	struct ea_refcount_el	*list;
};

#define MAX_EXTENT_DEPTH_COUNT 5

/*
 * This is the global e2fsck structure.
 */
typedef struct e2fsck_struct *e2fsck_t;

struct e2fsck_struct{
	ext2_filsys fs;
	const char *program_name;
	char *filesystem_name;
	char *device_name;
	char *io_options;
	int	flags;		/* E2fsck internal flags */
	int	options;
	blk_t use_superblock;	/* sb requested by user */
	blk_t superblock; /* sb used to open fs */
	int	blocksize;	/* blocksize */
	blk64_t	num_blocks; /* Total number of blocks */
	int	mount_flags;
	struct blkid_struct_cache blkid;	/* blkid cache */

#ifdef HAVE_SETJMP_H
	jmp_buf	abort_loc;
#endif
	unsigned long abort_code;

	int (*progress)(e2fsck_t ctx, int pass, unsigned long cur,
			unsigned long max);

	ext2fs_inode_bitmap inode_used_map; /* Inodes which are in use */
	ext2fs_inode_bitmap inode_bad_map; /* Inodes which are bad somehow */
	ext2fs_inode_bitmap inode_dir_map; /* Inodes which are directories */
	ext2fs_inode_bitmap inode_bb_map; /* Inodes which are in bad blocks */
	ext2fs_inode_bitmap inode_imagic_map; /* AFS inodes */
	ext2fs_inode_bitmap inode_reg_map; /* Inodes which are regular files*/

	ext2fs_block_bitmap block_found_map; /* Blocks which are in use */
	ext2fs_block_bitmap block_dup_map; /* Blks referenced more than once */
	ext2fs_block_bitmap block_ea_map; /* Blocks which are used by EA's */

	/*
	 * Inode count arrays
	 */
	ext2_icount_t	inode_count;
	ext2_icount_t inode_link_info;

	struct ea_refcount	refcount;
	struct ea_refcount refcount_extra;

	/*
	 * Array of flags indicating whether an inode bitmap, block
	 * bitmap, or inode table is invalid
	 */
	int *invalid_inode_bitmap_flag;
	int *invalid_block_bitmap_flag;
	int *invalid_inode_table_flag;
	int invalid_bitmaps; /* There are invalid bitmaps/itable */

	/*
	 * Block buffer
	 */
	char *block_buf;

	/*
	 * For pass1_check_directory and pass1_get_blocks
	 */
	ext2_ino_t stashed_ino;
	struct ext2_inode *stashed_inode;

	/*
	 * Location of the lost and found directory
	 */
	ext2_ino_t lost_and_found;
	int bad_lost_and_found;

	/*
	 * Directory information
	 */
	//umac *** struct dir_info_db	*dir_info;
	 void	*dir_info;

	/*
	 * Indexed directory information
	 */
	int		dx_dir_info_count;
	int		dx_dir_info_size;
	struct dx_dir_info *dx_dir_info;

	/*
	 * Directories to hash
	 */
	ext2_u32_list	dirs_to_hash;

	/*
	 * Tuning parameters
	 */
	int process_inode_size;
	int inode_buffer_blocks;
	unsigned int htree_slack_percentage;

	/*
	 * ext3 journal support
	 */
	io_channel	journal_io;
	char	*journal_name;

#ifdef RESOURCE_TRACK
	/*
	 * For timing purposes
	 */
	struct resource_track	global_rtrack;
#endif

	/*
	 * How we display the progress update (for unix)
	 */
	int progress_fd;
	int progress_pos;
	int progress_last_percent;
	unsigned int progress_last_time;
	int interactive;	/* Are we connected directly to a tty? */
	char start_meta[2], stop_meta[2];

	/* File counts */
	__u32 fs_directory_count;
	__u32 fs_regular_count;
	__u32 fs_blockdev_count;
	__u32 fs_chardev_count;
	__u32 fs_links_count;
	__u32 fs_symlinks_count;
	__u32 fs_fast_symlinks_count;
	__u32 fs_fifo_count;
	__u32 fs_total_count;
	__u32 fs_badblocks_count;
	__u32 fs_sockets_count;
	__u32 fs_ind_count;
	__u32 fs_dind_count;
	__u32 fs_tind_count;
	__u32 fs_fragmented;
	__u32 fs_fragmented_dir;
	__u32 large_files;
	__u32 fs_ext_attr_inodes;
	__u32 fs_ext_attr_blocks;
	__u32 extent_depth_count[MAX_EXTENT_DEPTH_COUNT];

	/* misc fields */
	time_t now;
	time_t time_fudge;	/* For working around buggy init scripts */
	int ext_attr_ver;
	profile_t	profile;
	int blocks_per_page;

	/*
	 * For the use of callers of the e2fsck functions; not used by
	 * e2fsck functions themselves.
	 */
	void *priv_data;
};

struct buffer_head{
	e2fsck_t b_ctx;
	io_channel	b_io;
	int		b_size;
	blk_t 	b_blocknr;
	int		b_dirty;
	int		b_uptodate;
	int		b_err;
	char		b_data[1024];
};

//end
struct inode;

struct journal_s
{
	unsigned long		j_flags;
	int			j_errno;
	struct buffer_head *	j_sb_buffer;
	struct journal_superblock_s *j_superblock;
	int			j_format_version;
	unsigned long		j_head;
	unsigned long		j_tail;
	unsigned long		j_free;
	unsigned long		j_first, j_last;
	kdev_t			j_dev;
	kdev_t			j_fs_dev;
	int			j_blocksize;
	unsigned int		j_blk_offset;
	unsigned int		j_maxlen;
	struct inode *		j_inode;
	tid_t			j_tail_sequence;
	tid_t			j_transaction_sequence;
	__u8			j_uuid[16];
	struct jbd_revoke_table_s *j_revoke;
	tid_t			j_failed_commit;
};

#if 0
//uma 
#define J_ASSERT(assert)						\
	do { if (!(assert)) {						\
		printf ("Assertion failure in %s() at %s line %d: "	\
			"\"%s\"\n",					\
			__FUNCTION__, __FILE__, __LINE__, # assert);	\
		fatal_error(e2fsck_global_ctx, 0);			\
	} } while (0)
#endif
#define is_journal_abort(x) 0

#define BUFFER_TRACE(bh, info)	do {} while (0)

/* Need this so we can compile with configure --enable-gcc-wall */
#ifdef NO_INLINE_FUNCS
#define inline
#endif

#endif /* _JFS_COMPAT_H */
