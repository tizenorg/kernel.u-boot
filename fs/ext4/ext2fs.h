#include <common.h>
#include <ext_common.h>
#include <ext4fs.h>
#include "ext4_common.h"

/* typedef long errcode_t; */
#define errcode_t	long

typedef __u32		ext2_ino_t;
typedef __u32		blk_t;
typedef __u64		blk64_t;
typedef __u32		dgrp_t;
typedef __u32		ext2_off_t;
typedef __s64		e2_blkcnt_t;

/*
 * Constants relative to the data blocks
 */
#define EXT2_NDIR_BLOCKS		12
#define EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

/*
 * Structure of the super block
 */
struct ext2_super_block {
	__u32	s_inodes_count;		/* Inodes count */
	__u32	s_blocks_count;		/* Blocks count */
	__u32	s_r_blocks_count;	/* Reserved blocks count */
	__u32	s_free_blocks_count;	/* Free blocks count */
	__u32	s_free_inodes_count;	/* Free inodes count */
	__u32	s_first_data_block;	/* First Data Block */
	__u32	s_log_block_size;	/* Block size */
	__s32	s_log_frag_size;	/* Fragment size */
	__u32	s_blocks_per_group;	/* # Blocks per group */
	__u32	s_frags_per_group;	/* # Fragments per group */
	__u32	s_inodes_per_group;	/* # Inodes per group */
	__u32	s_mtime;		/* Mount time */
	__u32	s_wtime;		/* Write time */
	__u16	s_mnt_count;		/* Mount count */
	__s16	s_max_mnt_count;	/* Maximal mount count */
	__u16	s_magic;		/* Magic signature */
	__u16	s_state;		/* File system state */
	__u16	s_errors;		/* Behaviour when detecting errors */
	__u16	s_minor_rev_level;	/* minor revision level */
	__u32	s_lastcheck;		/* time of last check */
	__u32	s_checkinterval;	/* max. time between checks */
	__u32	s_creator_os;		/* OS */
	__u32	s_rev_level;		/* Revision level */
	__u16	s_def_resuid;		/* Default uid for reserved blocks */
	__u16	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 *
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	__u32	s_first_ino;		/* First non-reserved inode */
	__u16   s_inode_size;		/* size of inode structure */
	__u16	s_block_group_nr;	/* block group # of this superblock */
	__u32	s_feature_compat;	/* compatible feature set */
	__u32	s_feature_incompat;	/* incompatible feature set */
	__u32	s_feature_ro_compat;	/* readonly-compatible feature set */
	__u8	s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16];	/* volume name */
	char	s_last_mounted[64];	/* directory where last mounted */
	__u32	s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT2_FEATURE_COMPAT_DIR_PREALLOC flag is on.
	 */
	__u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	__u16	s_reserved_gdt_blocks;	/* Per group table for online growth */
	/*
	 * Journaling support valid if EXT2_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	__u8	s_journal_uuid[16];	/* uuid of journal superblock */
	__u32	s_journal_inum;		/* inode number of journal file */
	__u32	s_journal_dev;		/* device number of journal file */
	__u32	s_last_orphan;		/* start of list of inodes to delete */
	__u32	s_hash_seed[4];		/* HTREE hash seed */
	__u8	s_def_hash_version;	/* Default hash version to use */
	__u8	s_jnl_backup_type;	/* Default type of journal backup */
	__u16	s_desc_size;		/* Group desc. size: INCOMPAT_64BIT */
	__u32	s_default_mount_opts;
	__u32	s_first_meta_bg;	/* First metablock group */
	__u32	s_mkfs_time;		/* When the filesystem was created */
	__u32	s_jnl_blocks[17];	/* Backup of the journal inode */
	__u32	s_blocks_count_hi;	/* Blocks count high 32bits */
	__u32	s_r_blocks_count_hi;	/* Reserved blocks count high 32 bits*/
	__u32	s_free_blocks_hi;	/* Free blocks count */
	__u16	s_min_extra_isize;	/* All inodes have at least # bytes */
	__u16	s_want_extra_isize;	/* New inodes should reserve # bytes */
	__u32	s_flags;		/* Miscellaneous flags */
	__u16   s_raid_stride;		/* RAID stride */
	__u16   s_mmp_interval;         /* # seconds to wait in MMP checking */
	__u64   s_mmp_block;            /* Block for multi-mount protection */
	__u32   s_raid_stripe_width;    /* blocks on all data disks (N*stride)*/
	__u8	s_log_groups_per_flex;	/* FLEX_BG group size */
	__u8    s_reserved_char_pad;
	__u16	s_reserved_pad;		/* Padding to next 32bits */
	__u64	s_kbytes_written;	/* nr of lifetime kilobytes written */
	__u32	s_snapshot_inum;	/* Inode number of active snapshot */
	__u32	s_snapshot_id;		/* sequential ID of active snapshot */
	__u64	s_snapshot_r_blocks_count; /* reserved blocks for active
					      snapshot's future use */
	__u32	s_snapshot_list;	/* inode number of the head of
						  the on-disk snapshot list */
#define EXT4_S_ERR_START ext4_offsetof(struct ext2_super_block, s_error_count)
	__u32	s_error_count;		/* number of fs errors */
	__u32	s_first_error_time;	/* first time an error happened */
	__u32	s_first_error_ino;	/* inode involved in first error */
	__u64	s_first_error_block;	/* block involved of first error */
	__u8	s_first_error_func[32];	/* function where the error happened */
	__u32	s_first_error_line;	/* line number where error happened */
	__u32	s_last_error_time;	/* most recent time of an error */
	__u32	s_last_error_ino;	/* inode involved in last error */
	__u32	s_last_error_line;	/* line number where error happened */
	__u64	s_last_error_block;	/* block involved of last error */
	__u8	s_last_error_func[32];	/* function where the error happened */
#define EXT4_S_ERR_END ext4_offsetof(struct ext2_super_block, s_mount_opts)
	__u8	s_mount_opts[64];
	__u32   s_reserved[112];        /* Padding to the end of the block */
};

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc {
	__u32	bg_block_bitmap;	/* Blocks bitmap block */
	__u32	bg_inode_bitmap;	/* Inodes bitmap block */
	__u32	bg_inode_table;		/* Inodes table block */
	__u16	bg_free_blocks_count;	/* Free blocks count */
	__u16	bg_free_inodes_count;	/* Free inodes count */
	__u16	bg_used_dirs_count;	/* Directories count */
	__u16	bg_flags;
	__u32	bg_reserved[2];
	__u16	bg_itable_unused;	/* Unused inodes count */
	__u16	bg_checksum;		/* crc16(s_uuid+grouo_num+group_desc)*/
};

#define i_size_high	i_dir_acl

/*
 * Structure of an inode on the disk
 */
struct ext2fs_inode {
	__u16	i_mode;		/* File mode */
	__u16	i_uid;		/* Low 16 bits of Owner Uid */
	__u32	i_size;		/* Size in bytes */
	__u32	i_atime;	/* Access time */
	__u32	i_ctime;	/* Inode change time */
	__u32	i_mtime;	/* Modification time */
	__u32	i_dtime;	/* Deletion Time */
	__u16	i_gid;		/* Low 16 bits of Group Id */
	__u16	i_links_count;	/* Links count */
	__u32	i_blocks;	/* Blocks count */
	__u32	i_flags;	/* File flags */
	union {
		struct {
			__u32	l_i_version; /* was l_i_reserved1 */
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
	} osd1;				/* OS dependent 1 */
	__u32	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	__u32	i_generation;	/* File version (for NFS) */
	__u32	i_file_acl;	/* File ACL */
	__u32	i_dir_acl;	/* Directory ACL */
	__u32	i_faddr;	/* Fragment address */
	union {
		struct {
			__u16	l_i_blocks_hi;
			__u16	l_i_file_acl_high;
			__u16	l_i_uid_high;	/* these 2 fields    */
			__u16	l_i_gid_high;	/* were reserved2[0] */
			__u32	l_i_reserved2;
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
	} osd2;				/* OS dependent 2 */
};

/*
typedef struct struct_ext2_filsys *ext2_filsys;

typedef struct ext2fs_struct_generic_bitmap *ext2fs_generic_bitmap;
typedef struct ext2fs_struct_generic_bitmap *ext2fs_inode_bitmap;
typedef struct ext2fs_struct_generic_bitmap *ext2fs_block_bitmap;
*/

#define ext2_filsys		struct struct_ext2_filsys*
#define ext2fs_generic_bitmap	struct ext2fs_struct_generic_bitmap *
#define ext2fs_inode_bitmap	struct ext2fs_struct_generic_bitmap *
#define ext2fs_block_bitmap	struct ext2fs_struct_generic_bitmap *

/*
 * Inode cache structure
 */
struct ext2_inode_cache {
	void				*buffer;
	blk_t				buffer_blk;
	int				cache_last;
	int				cache_size;
	int				refcount;
	struct ext2_inode_cache_ent	*cache;
};

struct ext2_inode_cache_ent {
	ext2_ino_t		ino;
	struct ext2fs_inode inode;
};

/*
 * Badblocks list
 */
struct ext2_struct_u32_list {
	int	magic;
	int	num;
	int	size;
	__u32 *list;
	int	badblocks_flags;
};

/*
 * Badblocks list definitions
 */

/*
typedef struct ext2_struct_u32_list *ext2_badblocks_list;
typedef struct ext2_struct_u32_list *ext2_u32_list;
typedef struct ext2_struct_dblist *ext2_dblist;
*/

#define ext2_badblocks_list	struct ext2_struct_u32_list *
#define ext2_u32_list		struct ext2_struct_u32_list *
#define ext2_dblist		struct ext2_struct_dblist *

struct struct_ext2_filsys {
	errcode_t			magic;
	int				flags;
	char				*device_name;
	struct ext2_super_block		*super;
	unsigned int			blocksize;
	int				fragsize;
	dgrp_t				group_desc_count;
	unsigned long			desc_blocks;
	struct ext2_group_desc		*group_desc;
	int				inode_blocks_per_group;
	ext2fs_inode_bitmap		inode_map;
	ext2fs_block_bitmap		block_map;
	errcode_t (*get_blocks)(ext2_filsys fs, ext2_ino_t ino, blk_t *blocks);
	errcode_t (*check_directory)(ext2_filsys fs, ext2_ino_t ino);
	errcode_t (*write_bitmaps)(ext2_filsys fs);
	errcode_t (*read_inode)(ext2_filsys fs, ext2_ino_t ino,
				struct ext2fs_inode *inode);
	errcode_t (*write_inode)(ext2_filsys fs, ext2_ino_t ino,
				struct ext2fs_inode *inode);
	ext2_badblocks_list		badblocks;
	ext2_dblist		dblist;
	__u32			stride;	/* for mke2fs */
	struct ext2_super_block		*orig_super;
	__u32			umask;
	time_t				now;
	/*
	 * Reserved for future expansion
	 */
	__u32			reserved[7];

	/*
	 * Reserved for the use of the calling application.
	 */
	void				*priv_data;

	/*
	 * Inode cache
	 */
	struct ext2_inode_cache		*icache;

	/*
	 * More callback functions
	 */
	errcode_t (*get_alloc_block)(ext2_filsys fs, blk64_t goal,
					  blk64_t *ret);
	void (*block_alloc_stats)(ext2_filsys fs, blk64_t blk, int inuse);
};

/*
 * Directory block iterator definition
 */
struct ext2_struct_dblist {
	int			magic;
	ext2_filsys		fs;
	ext2_ino_t		size;
	ext2_ino_t		count;
	int			sorted;
	struct ext2_db_entry	*list;
};


/*
 * ext2_dblist structure and abstractions (see dblist.c)
 */
struct ext2_db_entry {
	ext2_ino_t	ino;
	blk_t	blk;
	int	blockcnt;
};

struct ext2fs_struct_generic_bitmap {
	errcode_t	magic;
	ext2_filsys	fs;
	__u32	start, end;
	__u32	real_end;
	char		*description;
	char		*bitmap;
	errcode_t	base_error_code;
	__u32	reserved[7];
};

struct ext2_struct_u32_iterate {
	int			magic;
	ext2_u32_list		bb;
	int			ptr;
};

/*
typedef struct ext2_struct_u32_iterate *ext2_badblocks_iterate;
typedef struct ext2_struct_u32_iterate *ext2_u32_iterate;

typedef struct ext2_struct_u32_list *badblocks_list;
typedef struct ext2_struct_u32_iterate *badblocks_iterate;
*/

#define ext2_badblocks_iterate	struct ext2_struct_u32_iterate *
#define ext2_u32_iterate	struct ext2_struct_u32_iterate *

#define badblocks_list		struct ext2_struct_u32_list *
#define badblocks_iterate	struct ext2_struct_u32_iterate *

#define EXT4_S_ERR_LEN (EXT4_S_ERR_END - EXT4_S_ERR_START)

/*
 * Codes for operating systems
 */
#define EXT2_OS_LINUX		0
#define EXT2_OS_HURD		1
#define EXT2_OBSO_OS_MASIX	2
#define EXT2_OS_FREEBSD		3
#define EXT2_OS_LITES		4

/*
 * Revision levels
 */
#define EXT2_GOOD_OLD_REV	0	/* The good old (original) format */
#define EXT2_DYNAMIC_REV	1	/* V2 format w/ dynamic inode sizes */

#define EXT2_CURRENT_REV	EXT2_GOOD_OLD_REV
#define EXT2_MAX_SUPP_REV	EXT2_DYNAMIC_REV


/* ext2_err */

#define EXT2_ET_BASE                             (2133571328L)
#define EXT2_ET_MAGIC_EXT2FS_FILSYS              (2133571329L)
#define EXT2_ET_MAGIC_BADBLOCKS_LIST             (2133571330L)
#define EXT2_ET_MAGIC_BADBLOCKS_ITERATE          (2133571331L)
#define EXT2_ET_MAGIC_INODE_SCAN                 (2133571332L)
#define EXT2_ET_MAGIC_IO_CHANNEL                 (2133571333L)
#define EXT2_ET_MAGIC_UNIX_IO_CHANNEL            (2133571334L)
#define EXT2_ET_MAGIC_IO_MANAGER                 (2133571335L)
#define EXT2_ET_MAGIC_BLOCK_BITMAP               (2133571336L)
#define EXT2_ET_MAGIC_INODE_BITMAP               (2133571337L)
#define EXT2_ET_MAGIC_GENERIC_BITMAP             (2133571338L)
#define EXT2_ET_MAGIC_TEST_IO_CHANNEL            (2133571339L)
#define EXT2_ET_MAGIC_DBLIST                     (2133571340L)
#define EXT2_ET_MAGIC_ICOUNT                     (2133571341L)
#define EXT2_ET_MAGIC_PQ_IO_CHANNEL              (2133571342L)
#define EXT2_ET_MAGIC_EXT2_FILE                  (2133571343L)
#define EXT2_ET_MAGIC_E2IMAGE                    (2133571344L)
#define EXT2_ET_MAGIC_INODE_IO_CHANNEL           (2133571345L)
#define EXT2_ET_MAGIC_EXTENT_HANDLE              (2133571346L)
#define EXT2_ET_BAD_MAGIC                        (2133571347L)
#define EXT2_ET_REV_TOO_HIGH                     (2133571348L)
#define EXT2_ET_RO_FILSYS                        (2133571349L)
#define EXT2_ET_GDESC_READ                       (2133571350L)
#define EXT2_ET_GDESC_WRITE                      (2133571351L)
#define EXT2_ET_GDESC_BAD_BLOCK_MAP              (2133571352L)
#define EXT2_ET_GDESC_BAD_INODE_MAP              (2133571353L)
#define EXT2_ET_GDESC_BAD_INODE_TABLE            (2133571354L)
#define EXT2_ET_INODE_BITMAP_WRITE               (2133571355L)
#define EXT2_ET_INODE_BITMAP_READ                (2133571356L)
#define EXT2_ET_BLOCK_BITMAP_WRITE               (2133571357L)
#define EXT2_ET_BLOCK_BITMAP_READ                (2133571358L)
#define EXT2_ET_INODE_TABLE_WRITE                (2133571359L)
#define EXT2_ET_INODE_TABLE_READ                 (2133571360L)
#define EXT2_ET_NEXT_INODE_READ                  (2133571361L)
#define EXT2_ET_UNEXPECTED_BLOCK_SIZE            (2133571362L)
#define EXT2_ET_DIR_CORRUPTED                    (2133571363L)
#define EXT2_ET_SHORT_READ                       (2133571364L)
#define EXT2_ET_SHORT_WRITE                      (2133571365L)
#define EXT2_ET_DIR_NO_SPACE                     (2133571366L)
#define EXT2_ET_NO_INODE_BITMAP                  (2133571367L)
#define EXT2_ET_NO_BLOCK_BITMAP                  (2133571368L)
#define EXT2_ET_BAD_INODE_NUM                    (2133571369L)
#define EXT2_ET_BAD_BLOCK_NUM                    (2133571370L)
#define EXT2_ET_EXPAND_DIR_ERR                   (2133571371L)
#define EXT2_ET_TOOSMALL                         (2133571372L)
#define EXT2_ET_BAD_BLOCK_MARK                   (2133571373L)
#define EXT2_ET_BAD_BLOCK_UNMARK                 (2133571374L)
#define EXT2_ET_BAD_BLOCK_TEST                   (2133571375L)
#define EXT2_ET_BAD_INODE_MARK                   (2133571376L)
#define EXT2_ET_BAD_INODE_UNMARK                 (2133571377L)
#define EXT2_ET_BAD_INODE_TEST                   (2133571378L)
#define EXT2_ET_FUDGE_BLOCK_BITMAP_END           (2133571379L)
#define EXT2_ET_FUDGE_INODE_BITMAP_END           (2133571380L)
#define EXT2_ET_BAD_IND_BLOCK                    (2133571381L)
#define EXT2_ET_BAD_DIND_BLOCK                   (2133571382L)
#define EXT2_ET_BAD_TIND_BLOCK                   (2133571383L)
#define EXT2_ET_NEQ_BLOCK_BITMAP                 (2133571384L)
#define EXT2_ET_NEQ_INODE_BITMAP                 (2133571385L)
#define EXT2_ET_BAD_DEVICE_NAME                  (2133571386L)
#define EXT2_ET_MISSING_INODE_TABLE              (2133571387L)
#define EXT2_ET_CORRUPT_SUPERBLOCK               (2133571388L)
#define EXT2_ET_BAD_GENERIC_MARK                 (2133571389L)
#define EXT2_ET_BAD_GENERIC_UNMARK               (2133571390L)
#define EXT2_ET_BAD_GENERIC_TEST                 (2133571391L)
#define EXT2_ET_SYMLINK_LOOP                     (2133571392L)
#define EXT2_ET_CALLBACK_NOTHANDLED              (2133571393L)
#define EXT2_ET_BAD_BLOCK_IN_INODE_TABLE         (2133571394L)
#define EXT2_ET_UNSUPP_FEATURE                   (2133571395L)
#define EXT2_ET_RO_UNSUPP_FEATURE                (2133571396L)
#define EXT2_ET_LLSEEK_FAILED                    (2133571397L)
#define EXT2_ET_NO_MEMORY                        (2133571398L)
#define EXT2_ET_INVALID_ARGUMENT                 (2133571399L)
#define EXT2_ET_BLOCK_ALLOC_FAIL                 (2133571400L)
#define EXT2_ET_INODE_ALLOC_FAIL                 (2133571401L)
#define EXT2_ET_NO_DIRECTORY                     (2133571402L)
#define EXT2_ET_TOO_MANY_REFS                    (2133571403L)
#define EXT2_ET_FILE_NOT_FOUND                   (2133571404L)
#define EXT2_ET_FILE_RO                          (2133571405L)
#define EXT2_ET_DB_NOT_FOUND                     (2133571406L)
#define EXT2_ET_DIR_EXISTS                       (2133571407L)
#define EXT2_ET_UNIMPLEMENTED                    (2133571408L)
#define EXT2_ET_CANCEL_REQUESTED                 (2133571409L)
#define EXT2_ET_FILE_TOO_BIG                     (2133571410L)
#define EXT2_ET_JOURNAL_NOT_BLOCK                (2133571411L)
#define EXT2_ET_NO_JOURNAL_SB                    (2133571412L)
#define EXT2_ET_JOURNAL_TOO_SMALL                (2133571413L)
#define EXT2_ET_JOURNAL_UNSUPP_VERSION           (2133571414L)
#define EXT2_ET_LOAD_EXT_JOURNAL                 (2133571415L)
#define EXT2_ET_NO_JOURNAL                       (2133571416L)
#define EXT2_ET_DIRHASH_UNSUPP                   (2133571417L)
#define EXT2_ET_BAD_EA_BLOCK_NUM                 (2133571418L)
#define EXT2_ET_TOO_MANY_INODES                  (2133571419L)
#define EXT2_ET_NOT_IMAGE_FILE                   (2133571420L)
#define EXT2_ET_RES_GDT_BLOCKS                   (2133571421L)
#define EXT2_ET_RESIZE_INODE_CORRUPT             (2133571422L)
#define EXT2_ET_SET_BMAP_NO_IND                  (2133571423L)
#define EXT2_ET_TDB_SUCCESS                      (2133571424L)
#define EXT2_ET_TDB_ERR_CORRUPT                  (2133571425L)
#define EXT2_ET_TDB_ERR_IO                       (2133571426L)
#define EXT2_ET_TDB_ERR_LOCK                     (2133571427L)
#define EXT2_ET_TDB_ERR_OOM                      (2133571428L)
#define EXT2_ET_TDB_ERR_EXISTS                   (2133571429L)
#define EXT2_ET_TDB_ERR_NOLOCK                   (2133571430L)
#define EXT2_ET_TDB_ERR_EINVAL                   (2133571431L)
#define EXT2_ET_TDB_ERR_NOEXIST                  (2133571432L)
#define EXT2_ET_TDB_ERR_RDONLY                   (2133571433L)
#define EXT2_ET_DBLIST_EMPTY                     (2133571434L)
#define EXT2_ET_RO_BLOCK_ITERATE                 (2133571435L)
#define EXT2_ET_MAGIC_EXTENT_PATH                (2133571436L)
#define EXT2_ET_MAGIC_RESERVED_10                (2133571437L)
#define EXT2_ET_MAGIC_RESERVED_11                (2133571438L)
#define EXT2_ET_MAGIC_RESERVED_12                (2133571439L)
#define EXT2_ET_MAGIC_RESERVED_13                (2133571440L)
#define EXT2_ET_MAGIC_RESERVED_14                (2133571441L)
#define EXT2_ET_MAGIC_RESERVED_15                (2133571442L)
#define EXT2_ET_MAGIC_RESERVED_16                (2133571443L)
#define EXT2_ET_MAGIC_RESERVED_17                (2133571444L)
#define EXT2_ET_MAGIC_RESERVED_18                (2133571445L)
#define EXT2_ET_MAGIC_RESERVED_19                (2133571446L)
#define EXT2_ET_EXTENT_HEADER_BAD                (2133571447L)
#define EXT2_ET_EXTENT_INDEX_BAD                 (2133571448L)
#define EXT2_ET_EXTENT_LEAF_BAD                  (2133571449L)
#define EXT2_ET_EXTENT_NO_SPACE                  (2133571450L)
#define EXT2_ET_INODE_NOT_EXTENT                 (2133571451L)
#define EXT2_ET_EXTENT_NO_NEXT                   (2133571452L)
#define EXT2_ET_EXTENT_NO_PREV                   (2133571453L)
#define EXT2_ET_EXTENT_NO_UP                     (2133571454L)
#define EXT2_ET_EXTENT_NO_DOWN                   (2133571455L)
#define EXT2_ET_NO_CURRENT_NODE                  (2133571456L)
#define EXT2_ET_OP_NOT_SUPPORTED                 (2133571457L)
#define EXT2_ET_CANT_INSERT_EXTENT               (2133571458L)
#define EXT2_ET_CANT_SPLIT_EXTENT                (2133571459L)
#define EXT2_ET_EXTENT_NOT_FOUND                 (2133571460L)
#define EXT2_ET_EXTENT_NOT_SUPPORTED             (2133571461L)
#define EXT2_ET_EXTENT_INVALID_LENGTH            (2133571462L)
#define EXT2_ET_IO_CHANNEL_NO_SUPPORT_64         (2133571463L)
#define EXT2_NO_MTAB_FILE                        (2133571464L)

/*
 * Macro-instructions used to manage several block sizes
 */

#define EXT2_SB_BLOCK_SIZE(s)	(EXT2_MIN_BLOCK_SIZE << (s)->s_log_block_size)

/*
 * Feature set definitions
 */

#define EXT2_HAS_COMPAT_FEATURE(sb, mask)			\
	(EXT2_SB(sb)->s_feature_compat & (mask))
#define EXT2_HAS_RO_COMPAT_FEATURE(sb, mask)			\
	(EXT2_SB(sb)->s_feature_ro_compat & (mask))
#define EXT2_HAS_INCOMPAT_FEATURE(sb, mask)			\
	(EXT2_SB(sb)->s_feature_incompat & (mask))

#define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES	0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR		0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INODE	0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX		0x0020
#define EXT2_FEATURE_COMPAT_LAZY_BG		0x0040
#define EXT2_FEATURE_COMPAT_EXCLUDE_INODE	0x0080

#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
/* #define EXT2_FEATURE_RO_COMPAT_BTREE_DIR	0x0004 not used */
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE	0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM		0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK	0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE	0x0040
#define EXT4_FEATURE_RO_COMPAT_HAS_SNAPSHOT	0x0080

#define EXT2_FEATURE_INCOMPAT_COMPRESSION	0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE		0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER		0x0004 /* Needs recovery */
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV	0x0008 /* Journal device */
#define EXT2_FEATURE_INCOMPAT_META_BG		0x0010
#define EXT3_FEATURE_INCOMPAT_EXTENTS		0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT		0x0080
#define EXT4_FEATURE_INCOMPAT_MMP		0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG		0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE		0x0400
#define EXT4_FEATURE_INCOMPAT_DIRDATA		0x1000


#define EXT2_FEATURE_COMPAT_SUPP	0
#define EXT2_FEATURE_INCOMPAT_SUPP	(EXT2_FEATURE_INCOMPAT_FILETYPE)
#define EXT2_FEATURE_RO_COMPAT_SUPP	(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER| \
					 EXT2_FEATURE_RO_COMPAT_LARGE_FILE| \
					 EXT4_FEATURE_RO_COMPAT_DIR_NLINK| \
					 EXT2_FEATURE_RO_COMPAT_BTREE_DIR)

/*
 * Flags for the ext2_filsys structure and for ext2fs_open()
 */
#define EXT2_FLAG_RW			0x01
#define EXT2_FLAG_CHANGED		0x02
#define EXT2_FLAG_DIRTY			0x04
#define EXT2_FLAG_VALID			0x08
#define EXT2_FLAG_IB_DIRTY		0x10
#define EXT2_FLAG_BB_DIRTY		0x20
#define EXT2_FLAG_SWAP_BYTES		0x40
#define EXT2_FLAG_SWAP_BYTES_READ	0x80
#define EXT2_FLAG_SWAP_BYTES_WRITE	0x100
#define EXT2_FLAG_MASTER_SB_ONLY	0x200
#define EXT2_FLAG_FORCE			0x400
#define EXT2_FLAG_SUPER_ONLY		0x800
#define EXT2_FLAG_JOURNAL_DEV_OK	0x1000
#define EXT2_FLAG_IMAGE_FILE		0x2000
#define EXT2_FLAG_EXCLUSIVE		0x4000
#define EXT2_FLAG_SOFTSUPP_FEATURES	0x8000
#define EXT2_FLAG_NOFREE_ON_ERROR	0x10000
#define EXT2_FLAG_DIRECT_IO		0x80000

#define IO_FLAG_RW		0x0001
#define IO_FLAG_EXCLUSIVE	0x0002
#define IO_FLAG_DIRECT_IO	0x0004

#define EXT2_SUPER_MAGIC	0xEF53
/*
 * File system states
 */
#define EXT2_VALID_FS			0x0001	/* Unmounted cleanly */
#define EXT2_ERROR_FS			0x0002	/* Errors detected */
#define EXT3_ORPHAN_FS			0x0004	/* Orphans being recovered */

/*
 * Maximal mount counts between two filesystem checks
 */
#define EXT2_DFL_MAX_MNT_COUNT		20	/* Allow 20 mounts */
#define EXT2_DFL_CHECKINTERVAL		0	/* Don't use interval check */

/* This #ifdef is temporary until compression is fully supported */
#ifdef ENABLE_COMPRESSION
#ifndef I_KNOW_THAT_COMPRESSION_IS_EXPERIMENTAL
/* If the below warning bugs you, then have
   `CPPFLAGS=-DI_KNOW_THAT_COMPRESSION_IS_EXPERIMENTAL' in your
   environment at configure time. */
 #warning "Compression support is experimental"
#endif
#define EXT2_LIB_FEATURE_INCOMPAT_SUPP	(EXT2_FEATURE_INCOMPAT_FILETYPE|\
					 EXT2_FEATURE_INCOMPAT_COMPRESSION|\
					 EXT3_FEATURE_INCOMPAT_JOURNAL_DEV|\
					 EXT2_FEATURE_INCOMPAT_META_BG|\
					 EXT3_FEATURE_INCOMPAT_RECOVER|\
					 EXT3_FEATURE_INCOMPAT_EXTENTS|\
					 EXT4_FEATURE_INCOMPAT_FLEX_BG)
#else
#define EXT2_LIB_FEATURE_INCOMPAT_SUPP	(EXT2_FEATURE_INCOMPAT_FILETYPE|\
					 EXT3_FEATURE_INCOMPAT_JOURNAL_DEV|\
					 EXT2_FEATURE_INCOMPAT_META_BG|\
					 EXT3_FEATURE_INCOMPAT_RECOVER|\
					 EXT3_FEATURE_INCOMPAT_EXTENTS|\
					 EXT4_FEATURE_INCOMPAT_FLEX_BG)
#endif
#define EXT2_LIB_FEATURE_RO_COMPAT_SUPP	(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER|\
					 EXT4_FEATURE_RO_COMPAT_HUGE_FILE|\
					 EXT2_FEATURE_RO_COMPAT_LARGE_FILE|\
					 EXT4_FEATURE_RO_COMPAT_DIR_NLINK|\
					 EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE|\
					 EXT4_FEATURE_RO_COMPAT_GDT_CSUM)


/*
 * Permanent part of an large inode on the disk
 */
struct ext2_inode_large {
	__u16	i_mode;		/* File mode */
	__u16	i_uid;		/* Low 16 bits of Owner Uid */
	__u32	i_size;		/* Size in bytes */
	__u32	i_atime;	/* Access time */
	__u32	i_ctime;	/* Inode Change time */
	__u32	i_mtime;	/* Modification time */
	__u32	i_dtime;	/* Deletion Time */
	__u16	i_gid;		/* Low 16 bits of Group Id */
	__u16	i_links_count;	/* Links count */
	__u32	i_blocks;	/* Blocks count */
	__u32	i_flags;	/* File flags */
	union {
		struct {
			__u32	l_i_version; /* was l_i_reserved1 */
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
	} osd1;				/* OS dependent 1 */
	__u32	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	__u32	i_generation;	/* File version (for NFS) */
	__u32	i_file_acl;	/* File ACL */
	__u32	i_dir_acl;	/* Directory ACL */
	__u32	i_faddr;	/* Fragment address */
	union {
		struct {
			__u16	l_i_blocks_hi;
			__u16	l_i_file_acl_high;
			__u16	l_i_uid_high;	/* these 2 fields    */
			__u16	l_i_gid_high;	/* were reserved2[0] */
			__u32	l_i_reserved2;
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
	} osd2;				/* OS dependent 2 */
	__u16	i_extra_isize;
	__u16	i_pad1;
	__u32	i_ctime_extra;	/* extra Change time (nsec << 2 | epoch) */
	__u32	i_mtime_extra;	/* extra Modification time
						(nsec << 2 | epoch) */
	__u32	i_atime_extra;	/* extra Access time (nsec << 2 | epoch) */
	__u32	i_crtime;	/* File creation time */
	__u32	i_crtime_extra;	/* extra File creation time
						(nsec << 2 | epoch)*/
	__u32	i_version_hi;	/* high 32 bits for 64-bit version */
};

/* First non-reserved inode for old ext2 filesystems */
#define EXT2_GOOD_OLD_FIRST_INO	11
#define EXT2_GOOD_OLD_INODE_SIZE 128

#define EXT2_SB(sb)	(sb)

/*
 * Macro-instructions used to manage fragments
 */
#define EXT2_MIN_FRAG_SIZE		EXT2_MIN_BLOCK_SIZE
#define EXT2_MAX_FRAG_SIZE		EXT2_MAX_BLOCK_SIZE
#define EXT2_MIN_FRAG_LOG_SIZE		EXT2_MIN_BLOCK_LOG_SIZE

# define EXT2_FRAG_SIZE(s)		(EXT2_MIN_FRAG_SIZE << \
						 (s)->s_log_frag_size)
# define EXT2_FRAGS_PER_BLOCK(s)	(EXT2_SB_BLOCK_SIZE(s) / \
						EXT2_FRAG_SIZE(s))

/*
 * Where the master copy of the superblock is located, and how big
 * superblocks are supposed to be.  We define SUPERBLOCK_SIZE because
 * the size of the superblock structure is not necessarily trustworthy
 * (some versions have the padding set up so that the superblock is
 * 1032 bytes long).
 */
#define SUPERBLOCK_OFFSET	1024

/*
 * Behaviour when detecting errors
 */
#define EXT2_ERRORS_CONTINUE		1	/* Continue execution */
#define EXT2_ERRORS_RO			2	/* Remount fs read-only */
#define EXT2_ERRORS_PANIC		3	/* Panic */
#define EXT2_ERRORS_DEFAULT		EXT2_ERRORS_CONTINUE

#define CREATOR_OS EXT2_OS_LINUX

/*
 * Macro-instructions used to manage group descriptors
 */
#define EXT2_MIN_DESC_SIZE             32
#define EXT2_MIN_DESC_SIZE_64BIT       64
#define EXT2_MAX_DESC_SIZE             EXT2_MIN_BLOCK_SIZE
#define EXT2_DESC_SIZE(s)	       ((EXT2_SB(s)->s_feature_incompat & \
				       EXT4_FEATURE_INCOMPAT_64BIT) ? \
				       (s)->s_desc_size : EXT2_MIN_DESC_SIZE)

#define EXT2_BLOCKS_PER_GROUP(s)	(EXT2_SB(s)->s_blocks_per_group)
#define EXT2_INODES_PER_GROUP(s)	(EXT2_SB(s)->s_inodes_per_group)
#define EXT2_INODES_PER_BLOCK(s)	(EXT2_SB_BLOCK_SIZE(s) /\
							EXT2_INODE_SIZE(s))
/* limits imposed by 16-bit value gd_free_{blocks,inode}_count */
#define EXT2_MAX_BLOCKS_PER_GROUP(s)	((1 << 16) - 8)
#define EXT2_MAX_INODES_PER_GROUP(s)	((1 << 16) - EXT2_INODES_PER_BLOCK(s))

#define EXT2_DESC_PER_BLOCK(s)		(EXT2_SB_BLOCK_SIZE(s) /\
							EXT2_DESC_SIZE(s))

#define EXT2_BLOCK_SIZE_BITS(s)	((s)->s_log_block_size + 10)
#define EXT2_INODE_SIZE(s)	(((s)->s_rev_level == EXT2_GOOD_OLD_REV) ? \
				 EXT2_GOOD_OLD_INODE_SIZE : (s)->s_inode_size)
#define EXT2_FIRST_INO(s)	(((s)->s_rev_level == EXT2_GOOD_OLD_REV) ? \
				 EXT2_GOOD_OLD_FIRST_INO : (s)->s_first_ino)
#define EXT2_FIRST_INODE(s)	EXT2_FIRST_INO(s)

#define EXT2_ADDR_PER_BLOCK(s)	(EXT2_SB_BLOCK_SIZE(s) / sizeof(__u32))

#define EXT2_BG_INODE_UNINIT	0x0001 /* Inode table/bitmap not initialized */
#define EXT2_BG_BLOCK_UNINIT	0x0002 /* Block bitmap not initialized */
#define EXT2_BG_INODE_ZEROED	0x0004 /* On-disk itable initialized to zero */

/*
 * Misc. filesystem flags
 */
#define EXT2_FLAGS_SIGNED_HASH		0x0001  /* Signed dirhash in use */
#define EXT2_FLAGS_UNSIGNED_HASH	0x0002  /* Unsigned dirhash in use */
#define EXT2_FLAGS_TEST_FILESYS		0x0004	/* OK for use on develop code */
#define EXT2_FLAGS_IS_SNAPSHOT		0x0010	/* This is a snapshot image */
#define EXT2_FLAGS_FIX_SNAPSHOT		0x0020	/* Snapshot inodes corrupted */
#define EXT2_FLAGS_FIX_EXCLUDE		0x0040	/* Exclude bitmaps corrupted */

/*
 * For checking structure magic numbers...
 */

#define EXT2_CHECK_MAGIC(struct, code) \
	  if ((struct)->magic != (code)) \
		return (code)

#define EXT2_HASH_LEGACY		0
#define EXT2_HASH_HALF_MD4		1
#define EXT2_HASH_TEA			2
#define EXT2_HASH_LEGACY_UNSIGNED	3 /* reserved for userspace lib */
#define EXT2_HASH_HALF_MD4_UNSIGNED	4 /* reserved for userspace lib */
#define EXT2_HASH_TEA_UNSIGNED		5 /* reserved for userspace lib */


/*
 * Structure of a directory entry
 */
#define EXT2_NAME_LEN 255

struct ext2_dir_entry {
	__u32	inode;			/* Inode number */
	__u16	rec_len;		/* Directory entry length */
	__u16	name_len;		/* Name length */
	char	name[EXT2_NAME_LEN];	/* File name */
};

/*
 * Ext2/linux mode flags.  We define them here so that we don't need
 * to depend on the OS's sys/stat.h, since we may be compiling on a
 * non-Linux system.
 */
#define LINUX_S_IFMT  00170000
#define LINUX_S_IFSOCK 0140000
#define LINUX_S_IFLNK	 0120000
#define LINUX_S_IFREG  0100000
#define LINUX_S_IFBLK  0060000
#define LINUX_S_IFDIR  0040000
#define LINUX_S_IFCHR  0020000
#define LINUX_S_IFIFO  0010000
#define LINUX_S_ISUID  0004000
#define LINUX_S_ISGID  0002000
#define LINUX_S_ISVTX  0001000

#define LINUX_S_IRWXU 00700
#define LINUX_S_IRUSR 00400
#define LINUX_S_IWUSR 00200
#define LINUX_S_IXUSR 00100

#define LINUX_S_IRWXG 00070
#define LINUX_S_IRGRP 00040
#define LINUX_S_IWGRP 00020
#define LINUX_S_IXGRP 00010

#define LINUX_S_IRWXO 00007
#define LINUX_S_IROTH 00004
#define LINUX_S_IWOTH 00002
#define LINUX_S_IXOTH 00001

#define LINUX_S_ISLNK(m)	(((m) & LINUX_S_IFMT) == LINUX_S_IFLNK)
#define LINUX_S_ISREG(m)	(((m) & LINUX_S_IFMT) == LINUX_S_IFREG)
#define LINUX_S_ISDIR(m)	(((m) & LINUX_S_IFMT) == LINUX_S_IFDIR)
#define LINUX_S_ISCHR(m)	(((m) & LINUX_S_IFMT) == LINUX_S_IFCHR)
#define LINUX_S_ISBLK(m)	(((m) & LINUX_S_IFMT) == LINUX_S_IFBLK)
#define LINUX_S_ISFIFO(m)	(((m) & LINUX_S_IFMT) == LINUX_S_IFIFO)
#define LINUX_S_ISSOCK(m)	(((m) & LINUX_S_IFMT) == LINUX_S_IFSOCK)

/*
 * EXT2_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define EXT2_DIR_PAD			4
#define EXT2_DIR_ROUND			(EXT2_DIR_PAD - 1)
#define EXT2_DIR_REC_LEN(name_len)	(((name_len) + 8 + EXT2_DIR_ROUND) & \
					 ~EXT2_DIR_ROUND)

/*
 * Inode flags
 */
#define EXT2_SECRM_FL			0x00000001 /* Secure deletion */
#define EXT2_UNRM_FL			0x00000002 /* Undelete */
#define EXT2_COMPR_FL			0x00000004 /* Compress file */
#define EXT2_SYNC_FL			0x00000008 /* Synchronous updates */
#define EXT2_IMMUTABLE_FL		0x00000010 /* Immutable file */
#define EXT2_APPEND_FL			0x00000020 /* writes to file may
							only append */
#define EXT2_NODUMP_FL			0x00000040 /* do not dump file */
#define EXT2_NOATIME_FL			0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define EXT2_DIRTY_FL			0x00000100
#define EXT2_COMPRBLK_FL		0x00000200 /* One or more
							compressed clusters */
#define EXT2_NOCOMPR_FL			0x00000400 /* Access raw compressed
							data */
#define EXT2_ECOMPR_FL			0x00000800 /* Compression error */
/* End compression flags --- maybe not all used */
#define EXT2_BTREE_FL			0x00001000 /* btree format dir */
#define EXT2_INDEX_FL			0x00001000 /* hash-indexed directory */
#define EXT2_IMAGIC_FL			0x00002000
#define EXT3_JOURNAL_DATA_FL		0x00004000 /* file data should be
							journaled */
#define EXT2_NOTAIL_FL			0x00008000 /* file tail should not be
							merged */
#define EXT2_DIRSYNC_FL			0x00010000 /* Synchronous directory
							modifications */
#define EXT2_TOPDIR_FL			0x00020000 /* Top of dir hierarchies */
#define EXT4_HUGE_FILE_FL               0x00040000 /* Set to each huge file */
#define EXT4_EXTENTS_FL			0x00080000 /* Inode uses extents */
#define EXT4_EA_INODE_FL	        0x00200000 /* Inode used for large EA */
#define EXT4_EOFBLOCKS_FL		0x00400000 /* Blocks alloc beyond EOF */
#define EXT4_SNAPFILE_FL		0x01000000 /* Inode is a snapshot */
#define EXT4_SNAPFILE_DELETED_FL	0x04000000 /* Snapshot is deleted */
#define EXT4_SNAPFILE_SHRUNK_FL		0x08000000 /* Snapshot shrink has
							completed */
#define EXT2_RESERVED_FL		0x80000000 /* reserved for ext2 lib */

#define EXT2_FL_USER_VISIBLE		0x004BDFFF /* User visible flags */
#define EXT2_FL_USER_MODIFIABLE		0x004B80FF /* User modifiable flags */

/*
 * Return flags for the directory iterator functions
 */
#define DIRENT_CHANGED	1
#define DIRENT_ABORT	2
#define DIRENT_ERROR	3

#define BLOCK_FLAG_APPEND	1
#define BLOCK_FLAG_HOLE		1
#define BLOCK_FLAG_DEPTH_TRAVERSE	2
#define BLOCK_FLAG_DATA_ONLY	4
#define BLOCK_FLAG_READ_ONLY	8

#define BLOCK_FLAG_NO_LARGE	0x1000

#define EXT2_EXTENT_FLAGS_LEAF		0x0001
#define EXT2_EXTENT_FLAGS_UNINIT	0x0002
#define EXT2_EXTENT_FLAGS_SECOND_VISIT	0x0004

struct ext2fs_extent {
	blk64_t	e_pblk;		/* first physical block */
	blk64_t	e_lblk;		/* first logical block extent covers */
	__u32	e_len;		/* number of blocks covered by extent */
	__u32	e_flags;	/* extent flags */
};

/*
 * Return flags for the block iterator functions
 */
#define BLOCK_CHANGED	1
#define BLOCK_ABORT	2
#define BLOCK_ERROR	4

/*
 * Flags used by ext2fs_extent_get()
 */
#define EXT2_EXTENT_CURRENT	0x0000
#define EXT2_EXTENT_MOVE_MASK	0x000F
#define EXT2_EXTENT_ROOT	0x0001
#define EXT2_EXTENT_LAST_LEAF	0x0002
#define EXT2_EXTENT_FIRST_SIB	0x0003
#define EXT2_EXTENT_LAST_SIB	0x0004
#define EXT2_EXTENT_NEXT_SIB	0x0005
#define EXT2_EXTENT_PREV_SIB	0x0006
#define EXT2_EXTENT_NEXT_LEAF	0x0007
#define EXT2_EXTENT_PREV_LEAF	0x0008
#define EXT2_EXTENT_NEXT	0x0009
#define EXT2_EXTENT_PREV	0x000A
#define EXT2_EXTENT_UP		0x000B
#define EXT2_EXTENT_DOWN	0x000C
#define EXT2_EXTENT_DOWN_AND_LAST 0x000D
/*
 * Flags used by ext2fs_extent_insert()
 */
#define EXT2_EXTENT_INSERT_AFTER	0x0001 /* insert after handle loc'n */
#define EXT2_EXTENT_INSERT_NOSPLIT	0x0002 /* insert may not cause split */

/*
 * Flags used by ext2fs_extent_delete()
 */
#define EXT2_EXTENT_DELETE_KEEP_EMPTY	0x001 /* keep node if last extnt gone */

/*
 * Flags used by ext2fs_extent_set_bmap()
 */
#define EXT2_EXTENT_SET_BMAP_UNINIT	0x0001

/*
 * For directory iterators
 */
struct dir_context {
	ext2_ino_t		dir;
	int		flags;
	char		*buf;
	int (*func)(ext2_ino_t	dir,
			 int	entry,
			 struct ext2_dir_entry *dirent,
			 int	offset,
			 int	blocksize,
			 char *buf,
			 void *priv_data);
	void		*priv_data;
	errcode_t	errcode;
};

/*
 * Private definitions
 */

struct extent_path {
	char		*buf;
	int		entries;
	int		max_entries;
	int		left;
	int		visit_num;
	int		flags;
	blk64_t		end_blk;
	void		*curr;
};

struct ext2_extent_handle {
	errcode_t		magic;
	ext2_filsys		fs;
	ext2_ino_t		ino;
	struct ext2fs_inode	*inode;
	int			type;
	int			level;
	int			max_depth;
	struct extent_path	*path;
};

struct ext2_extent_path {
	errcode_t		magic;
	int			leaf_height;
	blk64_t			lblk;
};

/*
typedef struct ext2_extent_handle *ext2_extent_handle_t;
typedef struct ext2_extent_path *ext2_extent_path_t;
*/

#define ext2_extent_handle_t	struct ext2_extent_handle *
#define ext2_extent_path_t	struct ext2_extent_path *

/*
 * each block (leaves and indexes), even inode-stored has header
 */
struct ext3_extent_header {
	__u16	eh_magic;	/* probably will support different formats */
	__u16	eh_entries;	/* number of valid entries */
	__u16	eh_max;		/* capacity of store in entries */
	__u16	eh_depth;	/* has tree real underlaying blocks? */
	__u32	eh_generation;	/* generation of the tree */
};

/*
 * this is extent on-disk structure
 * it's used at the bottom of the tree
 */
struct ext3_extent {
	__u32	ee_block;	/* first logical block extent covers */
	__u16	ee_len;		/* number of blocks covered by extent */
	__u16	ee_start_hi;	/* high 16 bits of physical block */
	__u32	ee_start;	/* low 32 bigs of physical block */
};

/*
 * this is index on-disk structure
 * it's used at all the levels, but the bottom
 */
struct ext3_extent_idx {
	__u32	ei_block;	/* index covers logical blocks from 'block' */
	__u32	ei_leaf;	/* pointer to the physical block of the next *
				 * level. leaf or next index could bet here */
	__u16	ei_leaf_hi;	/* high 16 bits of physical block */
	__u16	ei_unused;
};

#define ext2fs_cpu_to_le64(x) ((__u64)(x))
#define ext2fs_le64_to_cpu(x) ((__u64)(x))
#define ext2fs_cpu_to_le32(x) ((__u32)(x))
#define ext2fs_le32_to_cpu(x) ((__u32)(x))
#define ext2fs_cpu_to_le16(x) ((__u16)(x))
#define ext2fs_le16_to_cpu(x) ((__u16)(x))

#define EXT3_EXT_MAGIC		0xf30a

#define EXT_INIT_MAX_LEN	(1UL << 15)
#define EXT_UNINIT_MAX_LEN	(EXT_INIT_MAX_LEN - 1)

#define EXT_FIRST_EXTENT(__hdr__) \
	((struct ext3_extent *)(((char *)(__hdr__)) +		\
				 sizeof(struct ext3_extent_header)))
#define EXT_FIRST_INDEX(__hdr__) \
	((struct ext3_extent_idx *)(((char *)(__hdr__)) +	\
				     sizeof(struct ext3_extent_header)))
#define EXT_HAS_FREE_INDEX(__path__) \
	((__path__)->p_hdr->eh_entries < (__path__)->p_hdr->eh_max)
#define EXT_LAST_EXTENT(__hdr__) \
	(EXT_FIRST_EXTENT((__hdr__)) + (__hdr__)->eh_entries - 1)
#define EXT_LAST_INDEX(__hdr__) \
	(EXT_FIRST_INDEX((__hdr__)) + (__hdr__)->eh_entries - 1)
#define EXT_MAX_EXTENT(__hdr__) \
	(EXT_FIRST_EXTENT((__hdr__)) + (__hdr__)->eh_max - 1)
#define EXT_MAX_INDEX(__hdr__) \
	(EXT_FIRST_INDEX((__hdr__)) + (__hdr__)->eh_max - 1)

/*
 * Data structure returned by ext2fs_extent_get_info()
 */
struct ext2_extent_info {
	int		curr_entry;
	int		curr_level;
	int		num_entries;
	int		max_entries;
	int		max_depth;
	int		bytes_avail;
	blk64_t		max_lblk;
	blk64_t		max_pblk;
	__u32		max_len;
	__u32		max_uninit_len;
};

/*
 * Magic "block count" return values for the block iterator function.
 */
#define BLOCK_COUNT_IND		(-1)
#define BLOCK_COUNT_DIND	(-2)
#define BLOCK_COUNT_TIND	(-3)
#define BLOCK_COUNT_TRANSLATOR	(-4)

/*
 * Directory iterator flags
 */

#define DIRENT_FLAG_INCLUDE_EMPTY	1
#define DIRENT_FLAG_INCLUDE_REMOVED	2

#define DIRENT_DOT_FILE		1
#define DIRENT_DOT_DOT_FILE	2
#define DIRENT_OTHER_FILE	3
#define DIRENT_DELETED_FILE	4

#define ext2fs_set_i_uid_high(inode, x) ((inode).osd2.linux2.l_i_uid_high = (x))
#define ext2fs_set_i_gid_high(inode, x) ((inode).osd2.linux2.l_i_gid_high = (x))

/*
 * Special inode numbers
 */
#define EXT2_BAD_INO		 1	/* Bad blocks inode */
#define EXT2_ROOT_INO		 2	/* Root inode */
#define EXT2_ACL_IDX_INO	 3	/* ACL inode */
#define EXT2_ACL_DATA_INO	 4	/* ACL inode */
#define EXT2_BOOT_LOADER_INO	 5	/* Boot loader inode */
#define EXT2_UNDEL_DIR_INO	 6	/* Undelete directory inode */
#define EXT2_RESIZE_INO		 7	/* Reserved group descriptors inode */
#define EXT2_JOURNAL_INO	 8	/* Journal inode */
#define EXT2_EXCLUDE_INO	 9	/* The "exclude" inode, for snapshots */

/*
 * Journal inode backup types
 */
#define EXT3_JNL_BACKUP_BLOCKS	1

/*
 * Flags for mkjournal
 *
 * EXT2_MKJOURNAL_V1_SUPER	Make a (deprecated) V1 journal superblock
 */
#define EXT2_MKJOURNAL_V1_SUPER	0x0000001

/*
 * Internal structures used by the logging mechanism:
 */

#define JFS_MAGIC_NUMBER 0xc03b3998U /* The first 4 bytes of /dev/random! */

/*
 * Descriptor block types:
 */

#define JFS_DESCRIPTOR_BLOCK	1
#define JFS_COMMIT_BLOCK	2
#define JFS_SUPERBLOCK_V1	3
#define JFS_SUPERBLOCK_V2	4
#define JFS_REVOKE_BLOCK	5

extern block_dev_desc_t *dev_desc;

int ext2fs_bg_has_super(ext2_filsys fs, int group_block);
void ext2fs_group_desc_csum_set(ext2_filsys fs, dgrp_t group);

errcode_t ext2fs_zero_blocks(ext2_filsys fs, blk_t blk, int num,
				blk_t *ret_blk, int *ret_count,
				block_dev_desc_t *dev_desc);

errcode_t ext2fs_mkdir(ext2_filsys fs, ext2_ino_t parent, ext2_ino_t inum,
		       const char *name);

errcode_t ext2fs_read_inode(ext2_filsys fs, ext2_ino_t ino,
			    struct ext2fs_inode *inode);

errcode_t ext2fs_write_inode(ext2_filsys fs, ext2_ino_t ino,
			     struct ext2fs_inode *inode);

errcode_t ext2fs_write_new_inode(ext2_filsys fs, ext2_ino_t ino,
				 struct ext2fs_inode *inode);

void ext2fs_inode_alloc_stats2(ext2_filsys fs, ext2_ino_t ino,
			       int inuse, int isdir);

void ext2fs_mark_ib_dirty(ext2_filsys fs);

errcode_t ext2fs_initialize(const char *name, int flags,
			    struct ext2_super_block *param,
			    ext2_filsys *ret_fs);

errcode_t ext2fs_allocate_tables(ext2_filsys fs);
errcode_t ext2fs_create_resize_inode(ext2_filsys fs);
errcode_t ext2fs_add_journal_inode(ext2_filsys fs, blk_t size, int flags);
errcode_t ext2fs_flush(ext2_filsys fs);
errcode_t ext4fs_close_clean(ext2_filsys fs);
errcode_t ext2fs_get_mem(unsigned long size, void *ptr);
errcode_t ext2fs_read_dir_block(ext2_filsys fs, blk_t block, void *buf);
errcode_t ext2fs_free_mem(void *ptr);
errcode_t ext2fs_get_array(unsigned long count, unsigned long size, void *ptr);
errcode_t ext2fs_write_dir_block(ext2_filsys fs, blk_t block, void *inbuf);

errcode_t ext2fs_lookup(ext2_filsys fs, ext2_ino_t dir, const char *name,
			int namelen, char *buf, ext2_ino_t *inode);

errcode_t ext2fs_expand_dir(ext2_filsys fs, ext2_ino_t dir);
errcode_t ext2fs_set_rec_len(ext2_filsys fs,
			     unsigned int len,
			     struct ext2_dir_entry *dirent);

errcode_t ext2fs_get_rec_len(ext2_filsys fs,
			     struct ext2_dir_entry *dirent,
			     unsigned int *rec_len);

int ext2fs_devread(int sector, int byte_offset, int byte_len, char *buf);
errcode_t ext2fs_link(ext2_filsys fs, ext2_ino_t dir, const char *name,
		      ext2_ino_t ino, int flags);

errcode_t ext2fs_read_ind_block(ext2_filsys fs, blk_t blk, void *buf);
errcode_t ext2fs_iblk_add_blocks(ext2_filsys fs, struct ext2fs_inode *inode,
				 blk64_t num_blocks);

errcode_t ext2fs_write_ind_block(ext2_filsys fs, blk_t blk, void *buf);
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
				void *priv_data);

void put_ext4fs(block_dev_desc_t *dev_desc, uint64_t off,
				void *buf, uint32_t size);

int e2p_edit_feature(const char *str, __u32 *compat_array, __u32 *ok_array);
int ext2fs_group_of_blk(ext2_filsys fs, blk_t blk);

void ext2fs_extent_free(ext2_extent_handle_t handle);
void ext2fs_block_alloc_stats(ext2_filsys fs, blk_t blk, int inuse);
errcode_t ext2fs_new_block(ext2_filsys fs, blk_t goal,
			   ext2fs_block_bitmap map, blk_t *ret);

errcode_t ext2fs_new_dir_block(ext2_filsys fs, ext2_ino_t dir_ino,
			       ext2_ino_t parent_ino, char **block);

int ext2fs_badblocks_list_test(ext2_badblocks_list bb, blk_t blk);
errcode_t ext2fs_update_bb_inode(ext2_filsys fs, ext2_badblocks_list bb_list);
errcode_t ext2fs_badblocks_list_iterate_begin(ext2_badblocks_list bb,
					      ext2_badblocks_iterate *ret);

int ext2fs_badblocks_list_iterate(ext2_badblocks_iterate iter, blk_t *blk);
int ext2fs_mark_block_bitmap(ext2fs_block_bitmap bitmap,
				       blk_t block);

void ext2fs_badblocks_list_iterate_end(ext2_badblocks_iterate iter);
errcode_t ext2fs_iblk_set(ext2_filsys fs,
					struct ext2fs_inode *inode, blk64_t b);

int ext2fs_mark_inode_bitmap(ext2fs_inode_bitmap bitmap,
				       ext2_ino_t inode);

int ext2fs_test_generic_bitmap(ext2fs_generic_bitmap bitmap, blk_t bitno);
