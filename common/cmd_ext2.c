/*
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * made from cmd_reiserfs by
 *
 * (C) Copyright 2003 - 2004
 * Sysgo Real-Time Solutions, AG <www.elinos.com>
 * Pavel Bartusek <pba@sysgo.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/*
 * Ext2fs support
 */
#include <common.h>
#include <part.h>
#include <config.h>
#include <command.h>
#include <image.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <ext2fs.h>
#include <linux/stat.h>
#include "../disk/part_dos.h"
#include <malloc.h>
#include <div64.h>

#if defined(CONFIG_CMD_USB) && defined(CONFIG_USB_STORAGE)
#include <usb.h>
#endif

#if !defined(CONFIG_DOS_PARTITION) && !defined(CONFIG_EFI_PARTITION)
#error DOS or EFI partition support must be selected
#endif

/* #define	EXT2_DEBUG */

#ifdef	EXT2_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

static int total_sector;
static block_dev_desc_t *cur_dev = NULL;
static unsigned long part_offset = 0;
static unsigned long part_size = 0;

static int cur_part = 1;

#define DOS_PART_MAGIC_OFFSET	0x1fe
#define DOS_FS_TYPE_OFFSET	0x36
#define DOS_FS32_TYPE_OFFSET	0x52

/*
 * Constants relative to the data blocks
 */
#define EXT2_NDIR_BLOCKS	12
#define EXT2_IND_BLOCK		EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK		(EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK		(EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS		(EXT2_TIND_BLOCK + 1)

/*
 * Macro-instructions used to manage several block sizes
 */
#define EXT2_MIN_BLOCK_LOG_SIZE	10 /* 1024 */
#define EXT2_MAX_BLOCK_LOG_SIZE	16 /* 65536 */
#define EXT2_MIN_BLOCK_SIZE	(1 << EXT2_MIN_BLOCK_LOG_SIZE)
#define EXT2_MAX_BLOCK_SIZE	(1 << EXT2_MAX_BLOCK_LOG_SIZE)

#define ENABLE_FEATURE_MKFS_EXT2_RESERVED_GDT	0
#define ENABLE_FEATURE_MKFS_EXT2_DIR_INDEX	1
/* First non-reserved inode for old ext2 filesystems */
#define EXT2_GOOD_OLD_FIRST_INO	11

#define SWAP_LE16(x) (x)
#define SWAP_LE32(x) (x)

#define EXT2_DYNAMIC_REV	1	/* V2 format w/ dynamic inode sizes */
#define EXT2_FEATURE_COMPAT_RESIZE_INODE	0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX		0x0020
/*
 * The second extended file system magic number
 */
#define EXT2_SUPER_MAGIC	0xEF53
#define EXT2_OS_LINUX		0
#define EXT2_HASH_HALF_MD4	1

#define EXT2_HASH_HALF_MD4 		1
#define EXT2_FLAGS_SIGNED_HASH		0x0001
#define EXT2_FLAGS_UNSIGNED_HASH	0x0002
#define EXT2_ROOT_INO			2 /* Root inode */

#define EXT2_FEATURE_INCOMPAT_FILETYPE		0x0002
#define EXT2_FEATURE_COMPAT_SUPP		0
#define EXT2_DFL_MAX_MNT_COUNT			20 /* Allow 20 mounts */
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001

/*
 * Behaviour when detecting errors
 */
#define EXT2_ERRORS_DEFAULT	1

/*
 * Ext2 directory file types.  Only the low 3 bits are used.  The
 * other bits are reserved for now.
 */
enum {
	EXT2_FT_UNKNOWN,
	EXT2_FT_REG_FILE,
	EXT2_FT_DIR,
	EXT2_FT_CHRDEV,
	EXT2_FT_BLKDEV,
	EXT2_FT_FIFO,
	EXT2_FT_SOCK,
	EXT2_FT_SYMLINK,
	EXT2_FT_MAX
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
	uint32_t s_blocks_per_group;
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

/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
	uint16_t i_mode;		/* File mode */
	uint16_t i_uid;		/* Low 16 bits of Owner Uid */
	uint32_t i_size;		/* Size in bytes */
	uint32_t i_atime; /* Access time */
	uint32_t i_ctime; /* Creation time */
	uint32_t i_mtime; /* Modification time */
	uint32_t i_dtime; /* Deletion Time */
	uint16_t i_gid;		/* Low 16 bits of Group Id */
	uint16_t i_links_count; /* Links count */
	uint32_t i_blocks;	/* Blocks count */
	uint32_t i_flags; /* File flags */
	union {
		struct {
			uint32_t  l_i_reserved1;
		} linux1;
		struct {
			uint32_t  h_i_translator;
		} hurd1;
		struct {
			uint32_t  m_i_reserved1;
		} masix1;
	} osd1;				/* OS dependent 1 */
	uint32_t i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	uint32_t i_generation;	/* File version (for NFS) */
	uint32_t i_file_acl; /* File ACL */
	uint32_t i_dir_acl;	/* Directory ACL */
	uint32_t i_faddr; /* Fragment address */
	union {
		struct {
			uint8_t		l_i_frag;	/* Fragment number */
			uint8_t		l_i_fsize;	/* Fragment size */
			uint16_t i_pad1;
			uint16_t l_i_uid_high;	/* these 2 fields 	*/
			uint16_t l_i_gid_high;	/* were reserved2[0] */
			uint32_t l_i_reserved2;
		} linux2;
		struct {
			uint8_t		h_i_frag;	/* Fragment number */
			uint8_t		h_i_fsize;	/* Fragment size */
			uint16_t h_i_mode_high;
			uint16_t h_i_uid_high;
			uint16_t h_i_gid_high;
			uint32_t h_i_author;
		} hurd2;
		struct {
			uint8_t		m_i_frag;	/* Fragment number */
			uint8_t		m_i_fsize;	/* Fragment size */
			uint16_t m_pad1;
			uint32_t m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc {
	uint32_t bg_block_bitmap;	/* Blocks bitmap block */
	uint32_t bg_inode_bitmap;	/* Inodes bitmap block */
	uint32_t bg_inode_table;		/* Inodes table block */
	uint16_t bg_free_blocks_count;	/* Free blocks count */
	uint16_t bg_free_inodes_count;	/* Free inodes count */
	uint16_t bg_used_dirs_count;	/* Directories count */
	uint16_t bg_pad;
	uint32_t bg_reserved[3];
};

// All fields are little-endian
struct ext2_dir {
	uint32_t inode1;
	uint16_t rec_len1;
	uint8_t	name_len1;
	uint8_t	file_type1;
	char		name1[4];
	uint32_t inode2;
	uint16_t rec_len2;
	uint8_t	name_len2;
	uint8_t	file_type2;
	char		name2[4];
	uint32_t inode3;
	uint16_t rec_len3;
	uint8_t	name_len3;
	uint8_t	file_type3;
	char		name3[12];
};

#define FETCH_LE32(field) \
	(sizeof(field) == 4 ? SWAP_LE32(field) : BUG_wrong_field_size())

// storage helpers
char BUG_wrong_field_size(void);
#define STORE_LE(field, value) \
do { \
	if (sizeof(field) == 4) \
		field = SWAP_LE32(value); \
	else if (sizeof(field) == 2) \
		field = SWAP_LE16(value); \
	else if (sizeof(field) == 1) \
		field = (value); \
	else \
		BUG_wrong_field_size(); \
} while (0)

int do_ext2ls (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *filename = "/";
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;
	int part_length;

	if (argc < 3)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (argc == 4)
		filename = argv[3];

	PRINTF("Using device %s %d:%d, directory: %s\n", argv[1], dev, part, filename);

	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return 1;
	}

	if (!ext2fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return 1;
	}

	if (ext2fs_ls (filename)) {
		printf ("** Error ext2fs_ls() **\n");
		ext2fs_close();
		return 1;
	};

	ext2fs_close();

	return 0;
}

U_BOOT_CMD(
	ext2ls,	4,	1,	do_ext2ls,
	"list files in a directory (default /)",
	"<interface> <dev[:part]> [directory]\n"
	"    - list files from 'dev' on 'interface' in a 'directory'"
);

int do_ext2write(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	block_dev_desc_t *dev_desc=NULL;
	char *filename = "/";
	int part_length;
	int part=1;
	int dev=0;
	char *ep;
	unsigned long ram_address;
	unsigned long file_size;
	disk_partition_t info;

	if (argc < 6)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	/*get the filename*/
	filename = argv[3];

	/*get the address in hexadecimal format (string to int)*/
	ram_address = simple_strtoul (argv[4], NULL, 16);

	/*get the filesize in base 10 format*/
	file_size=simple_strtoul (argv[5], NULL, 10);


	PRINTF("Using device %s %d:%d, directory: %s\n", argv[1], dev, part, filename);

	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return 1;
	}

	if (ext2_register_device(dev_desc,part)!=0)
	{
		printf("\n** Unable to use %s %d:%d for fattable **\n",
			argv[1], dev, part);
		return 1;
	}

	if (!get_partition_info(dev_desc, part, &info))
	{
		total_sector = (info.size * info.blksz) / SECTOR_SIZE;
	}
	else
	{
		printf("error : get partition info\n");
		return 1;
	}
	if (!ext2fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return 1;
	}

	if (ext2fs_write(dev_desc,part,filename,(unsigned char*)ram_address,file_size))
	{
			printf ("** Error ext2fs_write() **\n");
			ext2fs_close();
			return 1;
	}

	return 0;
}

U_BOOT_CMD(
	ext2write,	6,	1,	do_ext2write,
	"create a file in the root directory",
	"<interface> <dev[:part]> [filename] [Address] [sizebytes]\n"
	"    - create a file in / directory"
);

/******************************************************************************
 * Ext2fs boot command intepreter. Derived from diskboot
 */
int do_ext2load (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *filename = NULL;
	char *ep;
	int dev, part = 1;
	ulong addr = 0, part_length;
	int filelen;
	disk_partition_t info;
	block_dev_desc_t *dev_desc = NULL;
	char buf [12];
	unsigned long count;
	char *addr_str;

	switch (argc) {
	case 3:
		addr_str = getenv("loadaddr");
		if (addr_str != NULL)
			addr = simple_strtoul (addr_str, NULL, 16);
		else
			addr = CONFIG_SYS_LOAD_ADDR;

		filename = getenv ("bootfile");
		count = 0;
		break;
	case 4:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = getenv ("bootfile");
		count = 0;
		break;
	case 5:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = argv[4];
		count = 0;
		break;
	case 6:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = argv[4];
		count = simple_strtoul (argv[5], NULL, 16);
		break;

	default:
		return cmd_usage(cmdtp);
	}

	if (!filename) {
		puts ("** No boot file defined **\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		printf ("** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts ("** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	PRINTF("Using device %s%d, partition %d\n", argv[1], dev, part);

	if (part != 0) {
		if (get_partition_info (dev_desc, part, &info)) {
			printf ("** Bad partition %d **\n", part);
			return 1;
		}

		if (strncmp((char *)info.type, BOOT_PART_TYPE, sizeof(info.type)) != 0) {
			printf ("** Invalid partition type \"%.32s\""
				" (expect \"" BOOT_PART_TYPE "\")\n",
				info.type);
			return 1;
		}
		printf ("Loading file \"%s\" "
			"from %s device %d:%d (%.32s)\n",
			filename,
			argv[1], dev, part, info.name);
	} else {
		printf ("Loading file \"%s\" from %s device %d\n",
			filename, argv[1], dev);
	}


	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return 1;
	}

	if (!ext2fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %s %d:%d **\n",
			argv[1], dev, part);
		ext2fs_close();
		return 1;
	}

	filelen = ext2fs_open(filename);
	if (filelen < 0) {
		printf("** File not found %s\n", filename);
		ext2fs_close();
		return 1;
	}
	if ((count < filelen) && (count != 0)) {
	    filelen = count;
	}

	if (ext2fs_read((char *)addr, filelen) != filelen) {
		printf("** Unable to read \"%s\" from %s %d:%d **\n",
			filename, argv[1], dev, part);
		ext2fs_close();
		return 1;
	}

	ext2fs_close();

	/* Loading ok, update default load address */
	load_addr = addr;

	printf ("%d bytes read\n", filelen);
	sprintf(buf, "%X", filelen);
	setenv("filesize", buf);

	return 0;
}

U_BOOT_CMD(
	ext2load,	6,	0,	do_ext2load,
	"load binary file from a Ext2 filesystem",
	"<interface> <dev[:part]> [addr] [filename] [bytes]\n"
	"    - load binary file 'filename' from 'dev' on 'interface'\n"
	"      to address 'addr' from ext2 filesystem"
);
static unsigned int_log2(unsigned arg)
{
	unsigned r = 0;
	while ((arg >>= 1) != 0)
		r++;
	return r;
}
// taken from mkfs_minix.c. libbb candidate?
// "uint32_t size", since we never use it for anything >32 bits
uint32_t div_roundup(uint32_t size, uint32_t n)
{
	// Overflow-resistant
	uint32_t res = size / n;
	if (res * n != size)
		res++;
	return res;
}

static uint32_t has_super(uint32_t x)
{
	// 0, 1 and powers of 3, 5, 7 up to 2^32 limit
	static const uint32_t supers[] = {
		0, 1, 3, 5, 7, 9, 25, 27, 49, 81, 125, 243, 343, 625, 729,
		2187, 2401, 3125, 6561, 15625, 16807, 19683, 59049, 78125,
		117649, 177147, 390625, 531441, 823543, 1594323, 1953125,
		4782969, 5764801, 9765625, 14348907, 40353607, 43046721,
		48828125, 129140163, 244140625, 282475249, 387420489,
		1162261467, 1220703125, 1977326743, 3486784401/* >2^31 */,
	};
	const uint32_t *sp = supers + ARRAY_SIZE(supers);
	while (1) {
		sp--;
		if (x == *sp)
			return 1;
		if (x > *sp)
			return 0;
	}
}

// Die if we can't allocate size bytes of memory.
void* xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL && size != 0)
		printf("bb_msg_memory_exhausted\n");
	return ptr;
}

// Die if we can't allocate and zero size bytes of memory.
void* xzalloc(size_t size)
{
	void *ptr = xmalloc(size);
	memset(ptr, 0, size);
	return ptr;
}
static void allocate(uint8_t *bitmap, uint32_t blocksize, uint32_t start, uint32_t end)
{
	uint32_t i;
	memset(bitmap, 0, blocksize);
	i = start / 8;
	memset(bitmap, 0xFF, i);
	bitmap[i] = (1 << (start & 7)) - 1;
	i = end / 8;
	bitmap[blocksize - i - 1] |= 0x7F00 >> (end & 7);
	memset(bitmap + blocksize - i, 0xFF, i);
}

/* Like strncpy but make sure the resulting string is always 0 terminated. */
char* safe_strncpy(char *dst, const char *src, size_t size)
{
	if (!size) return dst;
	dst[--size] = '\0';
	return strncpy(dst, src, size);
}

void PUT(block_dev_desc_t *dev_desc,uint64_t off, void *buf, uint32_t size)
{
	uint64_t startblock,remainder;
	unsigned int sector_size = 512;
	unsigned block_len;
	unsigned char *temp_ptr=NULL;
	char sec_buf[SECTOR_SIZE];
	startblock = lldiv(off, SECTOR_SIZE);
	startblock += part_offset;
	remainder = do_div(off, SECTOR_SIZE);
	remainder &= SECTOR_SIZE - 1;
	if (dev_desc == NULL)
		return ;

	if ((startblock + (size/SECTOR_SIZE)) > (part_offset +total_sector)) {
		printf("part_offset is %d\n",part_offset);
		printf("total_sector is %d\n",total_sector);
		printf("error: overflow occurs\n");
		return ;
	}

	if(remainder){
		if (dev_desc->block_read) {
			dev_desc->block_read(dev_desc->dev, startblock, 1,
				(unsigned char *) sec_buf);
			temp_ptr=(unsigned char*)sec_buf;
			memcpy((temp_ptr + remainder),(unsigned char*)buf,size);
			dev_desc->block_write(dev_desc->dev, startblock, 1,
				(unsigned char *)sec_buf);
		}
	}
	else
	{
		if(size/SECTOR_SIZE!=0)
		{
			dev_desc->block_write(dev_desc->dev, startblock,
			size/SECTOR_SIZE,(unsigned long *) buf);
		}
		else
		{
			dev_desc->block_read(dev_desc->dev, startblock, 1,
			(unsigned char *) sec_buf);
			temp_ptr=(unsigned char*)sec_buf;
			memcpy(temp_ptr,buf,size);
			dev_desc->block_write(dev_desc->dev, startblock,
			1,(unsigned long *) sec_buf);
		}
	}

	return;
}

int mkfs_ext2(block_dev_desc_t *dev_desc, int part_no)
{
	disk_partition_t info;
	unsigned i, pos, n;
	unsigned bs, bpi;
	unsigned blocksize, blocksize_log2;
	unsigned inodesize, user_inodesize;
	unsigned reserved_percent = 5;
	unsigned long long kilobytes;
	uint32_t nblocks, nblocks_full;
	uint32_t nreserved;
	uint32_t ngroups;
	uint32_t bytes_per_inode;
	uint32_t first_block;
	uint32_t inodes_per_group;
	uint32_t group_desc_blocks;
	uint32_t inode_table_blocks;
	uint32_t lost_and_found_blocks;
	time_t timestamp;
	const char *label = "";
	struct ext2_sblock *sb; // superblock
	struct ext2_group_desc *gd; // group descriptors
	struct ext2_inode *inode;
	struct ext2_dir *dir;
	uint8_t *buf;
	__u32 volume_id;
	__u32 bytes_per_sect;
	__u32 volume_size_sect;
	__u64 volume_size_bytes;
	unsigned char *buffer;
	unsigned char *temp_buffer;
	dos_partition_t *pt,*temp_pt;
	pt=xzalloc(sizeof(dos_partition_t));

	/* volume_id is fixed */
	volume_id = 0x386d43bf;

	/* Get image size and sector size */
	bytes_per_sect = SECTOR_SIZE;

	if (!get_partition_info(dev_desc, part_no, &info)) {
		kilobytes = (info.size * info.blksz)/1024;
		volume_size_bytes = info.size * info.blksz;
		volume_size_sect = volume_size_bytes / bytes_per_sect;

		total_sector = volume_size_sect;
	} else {
		printf("error : get partition info\n");
		return 1;
	}

	bytes_per_inode = 16384;
	if (kilobytes < 512*1024)
		bytes_per_inode = 4096;
	if (kilobytes < 3*1024)
		bytes_per_inode = 8192;

	// Determine block size and inode size
	// block size is a multiple of 1024
	// inode size is a multiple of 128

	blocksize = 1024;
	inodesize = sizeof(struct ext2_inode); // 128
	if (kilobytes >= 512*1024) { // mke2fs 1.41.9 compat
		blocksize = 4096;
		inodesize = 256;
	}

	if (EXT2_MAX_BLOCK_SIZE > 4096) {
		while ((kilobytes >> 22) >= blocksize)
			blocksize *= 2;
	}

	if ((int32_t)bytes_per_inode < blocksize)
		printf("-%c is bad", 'i');
	// number of bits in one block, i.e. 8*blocksize
#define blocks_per_group (8 * blocksize)
	first_block = (EXT2_MIN_BLOCK_SIZE == blocksize);
	blocksize_log2 = int_log2(blocksize);

	// Determine number of blocks
	kilobytes >>= (blocksize_log2 - EXT2_MIN_BLOCK_LOG_SIZE);
	nblocks = kilobytes;
	if (nblocks != kilobytes)
		printf("block count doesn't fit in 32 bits");
#define kilobytes kilobytes_unused_after_this
	// Experimentally, standard mke2fs won't work on images smaller than 60k
	if (nblocks < 60)
		printf("need >= 60 blocks");

	// How many reserved blocks?
	if (reserved_percent > 50)
		printf("-%c is bad", 'm');
	nreserved = (uint64_t)nblocks * reserved_percent / 100;

	// N.B. killing e2fsprogs feature! Unused blocks don't account in calculations
	nblocks_full = nblocks;

	// If last block group is too small, nblocks may be decreased in order
	// to discard it, and control returns here to recalculate some
	// parameters.
	// Note: blocksize and bytes_per_inode are never recalculated.
 retry:
	// N.B. a block group can have no more than blocks_per_group blocks
	ngroups = div_roundup(nblocks - first_block, blocks_per_group);

	group_desc_blocks = div_roundup(ngroups, blocksize / sizeof(*gd));

	{
		// N.B. e2fsprogs does as follows!
		uint32_t overhead, remainder;
		// ninodes is the max number of inodes in this filesystem
		uint32_t ninodes =
			((uint64_t) nblocks_full * blocksize) / bytes_per_inode;
		if (ninodes < EXT2_GOOD_OLD_FIRST_INO+1)
			ninodes = EXT2_GOOD_OLD_FIRST_INO+1;
		inodes_per_group = div_roundup(ninodes, ngroups);
		// minimum number because the first EXT2_GOOD_OLD_FIRST_INO-1 are reserved
		if (inodes_per_group < 16)
			inodes_per_group = 16;
		// a block group can't have more inodes than blocks
		if (inodes_per_group > blocks_per_group)
			inodes_per_group = blocks_per_group;
		// adjust inodes per group so they completely fill the inode table blocks in the descriptor
		inodes_per_group =
			(div_roundup(inodes_per_group * inodesize,
				blocksize) * blocksize) / inodesize;
		// make sure the number of inodes per group is a multiple of 8
		inodes_per_group &= ~7;
		inode_table_blocks = div_roundup(inodes_per_group * inodesize,
			blocksize);

		// to be useful, lost+found should occupy at least 2 blocks (but not exceeding 16*1024 bytes),
		// and at most EXT2_NDIR_BLOCKS. So reserve these blocks right now
		/* Or e2fsprogs comment verbatim (what does it mean?):
		 * Ensure that lost+found is at least 2 blocks, so we always
		 * test large empty blocks for big-block filesystems. */
		lost_and_found_blocks = MIN(EXT2_NDIR_BLOCKS,
			16 >> (blocksize_log2 - EXT2_MIN_BLOCK_LOG_SIZE));

		// the last group needs more attention: isn't it too small for possible overhead?
		overhead = (has_super(ngroups - 1) ?
			(1/*sb*/ + group_desc_blocks) : 0) + 1/*bbmp*/ + 1/*ibmp*/ + inode_table_blocks;
		remainder = (nblocks - first_block) % blocks_per_group;
		if (remainder && (remainder < overhead + 50)) {
			nblocks -= remainder;
			goto retry;
		}
	}

	if (nblocks_full - nblocks)
		printf("warning: %u blocks unused\n\n", nblocks_full - nblocks);
	printf(
		"Filesystem label=%s\n"
		"OS type: Linux\n"
		"Block size=%u (log=%u)\n"
		"Fragment size=%u (log=%u)\n"
		"%u inodes, %u blocks\n"
		"%u blocks (%u%%) reserved for the super user\n"
		"First data block=%u\n"
		"Maximum filesystem blocks=%u\n"
		"%u block groups\n"
		"%u blocks per group, %u fragments per group\n"
		"%u inodes per group"
		, label
		, blocksize, blocksize_log2 - EXT2_MIN_BLOCK_LOG_SIZE
		, blocksize, blocksize_log2 - EXT2_MIN_BLOCK_LOG_SIZE
		, inodes_per_group * ngroups, nblocks
		, nreserved, reserved_percent
		, first_block
		, group_desc_blocks * (blocksize / (unsigned)sizeof(*gd)) * blocks_per_group
		, ngroups
		, blocks_per_group, blocks_per_group
		, inodes_per_group
	);
	{
		const char *fmt = "\nSuperblock backups stored on blocks:\n"
			"\t%u";
		pos = first_block;
		for (i = 1; i < ngroups; i++) {
			pos += blocks_per_group;
			if (has_super(i)) {
				printf(fmt, (unsigned)pos);
				fmt = ", %u";
			}
		}
	}
	printf("\n");

	// fill the superblock
	sb = xzalloc(1024);
	STORE_LE(sb->revision_level, EXT2_DYNAMIC_REV); // revision 1 filesystem
	STORE_LE(sb->magic, EXT2_SUPER_MAGIC);
	STORE_LE(sb->inode_size, inodesize);
	// set "Required extra isize" and "Desired extra isize" fields to 28
	//if (inodesize != sizeof(*inode))
		//STORE_LE(sb->s_reserved[21], 0x001C001C);
	STORE_LE(sb->first_inode, EXT2_GOOD_OLD_FIRST_INO);
	STORE_LE(sb->log2_block_size, blocksize_log2 - EXT2_MIN_BLOCK_LOG_SIZE);
	STORE_LE(sb->log2_fragment_size,
		blocksize_log2 - EXT2_MIN_BLOCK_LOG_SIZE);
	// first 1024 bytes of the device are for boot record. If block size is 1024 bytes, then
	// the first block is 1, otherwise 0
	STORE_LE(sb->first_data_block, first_block);
	// block and inode bitmaps occupy no more than one block, so maximum number of blocks is
	STORE_LE(sb->s_blocks_per_group, blocks_per_group);
	STORE_LE(sb->fragments_per_group, blocks_per_group);
	// blocks
	STORE_LE(sb->total_blocks, nblocks);
	// reserve blocks for superuser
	STORE_LE(sb->reserved_blocks, nreserved);
	// ninodes
	STORE_LE(sb->inodes_per_group, inodes_per_group);
	STORE_LE(sb->total_inodes, inodes_per_group * ngroups);
	STORE_LE(sb->free_inodes,
		inodes_per_group * ngroups - EXT2_GOOD_OLD_FIRST_INO);
	/* TODO: Implement timestamps, currently commented*/
	#if 0
	// timestamps
	timestamp = time(NULL);
	STORE_LE(sb->s_mkfs_time, timestamp);
	STORE_LE(sb->s_wtime, timestamp);
	STORE_LE(sb->s_lastcheck, timestamp);
	#endif
	// misc. Values are chosen to match mke2fs 1.41.9
	STORE_LE(sb->fs_state, 1); //  TODO: what's 1?
	STORE_LE(sb->creator_os, EXT2_OS_LINUX);
	STORE_LE(sb->checkinterval, 24*60*60 * 180); // 180 days
	STORE_LE(sb->error_handling, EXT2_ERRORS_DEFAULT);
	// mke2fs 1.41.9 also sets EXT3_FEATURE_COMPAT_RESIZE_INODE
	// and if >= 0.5GB, EXT3_FEATURE_RO_COMPAT_LARGE_FILE.
	// we use values which match "mke2fs -O ^resize_inode":
	// in this case 1.41.9 never sets EXT3_FEATURE_RO_COMPAT_LARGE_FILE.
	STORE_LE(sb->feature_compatibility, EXT2_FEATURE_COMPAT_SUPP
		//| (EXT2_FEATURE_COMPAT_RESIZE_INO * ENABLE_FEATURE_MKFS_EXT2_RESERVED_GDT)
		| (ENABLE_FEATURE_MKFS_EXT2_RESERVED_GDT)
		| (EXT2_FEATURE_COMPAT_DIR_INDEX * ENABLE_FEATURE_MKFS_EXT2_DIR_INDEX));
	STORE_LE(sb->feature_incompat, EXT2_FEATURE_INCOMPAT_FILETYPE);
	STORE_LE(sb->feature_ro_compat, EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER);
	//STORE_LE(sb->s_flags, EXT2_FLAGS_UNSIGNED_HASH * ENABLE_FEATURE_MKFS_EXT2_DIR_INDEX);
	//generate_uuid(sb->s_uuid);
	//if (ENABLE_FEATURE_MKFS_EXT2_DIR_INDEX) {
		//STORE_LE(sb->s_def_hash_version, EXT2_HASH_HALF_MD4);
	//	generate_uuid((uint8_t *)sb->s_hash_seed);
	//}
	/*
	 * From e2fsprogs: add "jitter" to the superblock's check interval so that we
	 * don't check all the filesystems at the same time.	We use a
	 * kludgy hack of using the UUID to derive a random jitter value.
	 */
	STORE_LE(sb->max_mnt_count,EXT2_DFL_MAX_MNT_COUNT);
		//+ (sb->s_uuid[ARRAY_SIZE(sb->s_uuid)-1] % EXT2_DFL_MAX_MNT_COUNT));

	// write the label
	safe_strncpy((char *)sb->volume_name, label, sizeof(sb->volume_name));

	// calculate filesystem skeleton structures
	gd = xzalloc(group_desc_blocks * blocksize);
	buf = xmalloc(blocksize);
	sb->free_blocks = 0;
	for (i = 0, pos = first_block, n = nblocks - first_block;
		i < ngroups;
		i++, pos += blocks_per_group, n -= blocks_per_group
	) {
		memset(buf,'\0',blocksize);

		uint32_t overhead = pos + (has_super(i) ?
			(1/*sb*/ + group_desc_blocks) : 0);
		uint32_t free_blocks;
		// fill group descriptors
		STORE_LE(gd[i].bg_block_bitmap, overhead + 0);
		STORE_LE(gd[i].bg_inode_bitmap, overhead + 1);
		STORE_LE(gd[i].bg_inode_table, overhead + 2);
		overhead =
			overhead - pos + 1/*bbmp*/ + 1/*ibmp*/ + inode_table_blocks;
		gd[i].bg_free_inodes_count = inodes_per_group;
		//STORE_LE(gd[i].bg_used_dirs_count, 0);
		// N.B. both "/" and "/lost+found" are within the first block group
		// "/" occupies 1 block, "/lost+found" occupies lost_and_found_blocks...
		if (0 == i) {
			// ... thus increased overhead for the first block group ...
			overhead += 1 + lost_and_found_blocks;
			// ... and 2 used directories
			STORE_LE(gd[i].bg_used_dirs_count, 2);
			// well known reserved inodes belong to the first block too
			gd[i].bg_free_inodes_count -= EXT2_GOOD_OLD_FIRST_INO;
		}

		// cache free block count of the group
		free_blocks = (n < blocks_per_group ? n : blocks_per_group)
			- overhead;

		// mark preallocated blocks as allocated
		allocate(buf, blocksize,
			// reserve "overhead" blocks
			overhead,
			// mark unused trailing blocks
			blocks_per_group - (free_blocks + overhead)
		);
		// dump block bitmap
		PUT(dev_desc,(uint64_t)(FETCH_LE32(gd[i].bg_block_bitmap)) * blocksize,
			buf, blocksize);
		STORE_LE(gd[i].bg_free_blocks_count, free_blocks);
		memset(buf,'\0',blocksize);

		// mark preallocated inodes as allocated
		allocate(buf, blocksize,
			// mark reserved inodes
			inodes_per_group - gd[i].bg_free_inodes_count,
			// mark unused trailing inodes
			blocks_per_group - inodes_per_group
		);
		// dump inode bitmap, but it's right after block bitmap, so we can just:
		//Modified to write in immediate block after block bitmap
		PUT(dev_desc,((uint64_t)(FETCH_LE32(gd[i].bg_block_bitmap))+1) * blocksize,
			buf, blocksize);
		STORE_LE(gd[i].bg_free_inodes_count, gd[i].bg_free_inodes_count);

		// count overall free blocks
		sb->free_blocks += free_blocks;
	}
	STORE_LE(sb->free_blocks, sb->free_blocks);

	// dump filesystem skeleton structures
	for (i = 0, pos = first_block; i < ngroups; i++, pos += blocks_per_group) {
		// dump superblock and group descriptors and their backups
		if (has_super(i)) {
			// N.B. 1024 byte blocks are special
			PUT(dev_desc,((uint64_t)pos * blocksize) +
				((0 == i && 1024 != blocksize) ? 1024 : 0),
				sb, 1024);
			PUT(dev_desc,((uint64_t)pos * blocksize) + blocksize,
				gd, group_desc_blocks * blocksize);
		}
	}

	// zero boot sectors
	/* Magic number is written in first block*/
	buffer= (char *)xmalloc(1024);
	temp_buffer= (char *)xmalloc(1024);
	memset(buffer, 0, 1024);
	buffer[DOS_PART_MAGIC_OFFSET] = 0x55;
	buffer[DOS_PART_MAGIC_OFFSET + 1] = 0xaa;

	/* Read the first block and restore the partition info into first block*/
	dev_desc->block_read (dev_desc->dev, 0, 1, (ulong *) temp_buffer);
	temp_pt = (dos_partition_t *) (temp_buffer + DOS_PART_TBL_OFFSET);
	pt->boot_ind = 0x0;
	pt->sys_ind = 0x83;
	memcpy(buffer+DOS_PART_TBL_OFFSET,pt,sizeof(dos_partition_t));
	PUT(dev_desc,0, buffer, 1024);
	memset(buf, 0, blocksize);

#if 0
	// zero inode tables
	for (i = 0; i < ngroups; ++i)
		for (n = 0; n < inode_table_blocks; ++n){
			PUT(dev_desc,
				(uint64_t)(FETCH_LE32(gd[i].bg_inode_table) + n)
					* blocksize, buf, blocksize);
			}
#endif

	// prepare directory inode
	inode = (struct ext2_inode *)buf;
	STORE_LE(inode->i_mode, S_IFDIR | S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH);
	STORE_LE(inode->i_mtime, timestamp);
	STORE_LE(inode->i_atime, timestamp);
	STORE_LE(inode->i_ctime, timestamp);
	STORE_LE(inode->i_size, blocksize);

	// inode->i_blocks stores the number of 512 byte data blocks
	// (512, because it goes directly to struct stat without scaling)
	STORE_LE(inode->i_blocks, blocksize / 512);

	// dump root dir inode
	STORE_LE(inode->i_links_count, 3); // "/.", "/..", "/lost+found/.." point to this inode
	STORE_LE(inode->i_block[0],
		FETCH_LE32(gd[0].bg_inode_table) + inode_table_blocks);
	PUT(dev_desc,((uint64_t)FETCH_LE32(gd[0].bg_inode_table) * blocksize) +
		(EXT2_ROOT_INO-1) * inodesize,buf, inodesize);

	// dump lost+found dir inode
	STORE_LE(inode->i_links_count, 2); // both "/lost+found" and "/lost+found/." point to this inode
	STORE_LE(inode->i_size, lost_and_found_blocks * blocksize);
	STORE_LE(inode->i_blocks, (lost_and_found_blocks * blocksize) / 512);
	n = FETCH_LE32(inode->i_block[0]) + 1;

	for (i = 0; i < lost_and_found_blocks; ++i)
		STORE_LE(inode->i_block[i], i + n); // use next block
	PUT(dev_desc,((uint64_t)FETCH_LE32(gd[0].bg_inode_table) * blocksize) +
		(EXT2_GOOD_OLD_FIRST_INO-1) * inodesize, buf, inodesize);

	// dump directories
	memset(buf, 0, blocksize);
	dir = (struct ext2_dir *)buf;

	// dump 2nd+ blocks of "/lost+found"
	STORE_LE(dir->rec_len1, blocksize); // e2fsck 1.41.4 compat (1.41.9 does not need this)
	for (i = 1; i < lost_and_found_blocks; ++i) {
		PUT(dev_desc,(uint64_t)(FETCH_LE32(gd[0].bg_inode_table) +
			inode_table_blocks + 1+i) * blocksize, buf, blocksize);
	}
	// dump 1st block of "/lost+found"
	STORE_LE(dir->inode1, EXT2_GOOD_OLD_FIRST_INO);
	STORE_LE(dir->rec_len1, 12);
	STORE_LE(dir->name_len1, 1);
	STORE_LE(dir->file_type1, EXT2_FT_DIR);
	dir->name1[0] = '.';
	STORE_LE(dir->inode2, EXT2_ROOT_INO);
	STORE_LE(dir->rec_len2, blocksize - 12);
	STORE_LE(dir->name_len2, 2);
	STORE_LE(dir->file_type2, EXT2_FT_DIR);
	dir->name2[0] = '.'; dir->name2[1] = '.';
	PUT(dev_desc,(uint64_t)(FETCH_LE32(gd[0].bg_inode_table) +
		inode_table_blocks + 1) * blocksize, buf, blocksize);

	// dump root dir block
	STORE_LE(dir->inode1, EXT2_ROOT_INO);
	STORE_LE(dir->rec_len2, 12);
	STORE_LE(dir->inode3, EXT2_GOOD_OLD_FIRST_INO);
	STORE_LE(dir->rec_len3, blocksize - 12 - 12);
	STORE_LE(dir->name_len3, 10);
	STORE_LE(dir->file_type3, EXT2_FT_DIR);
	strcpy(dir->name3, "lost+found");
	PUT(dev_desc,(uint64_t)(FETCH_LE32(gd[0].bg_inode_table) +
		inode_table_blocks + 0) * blocksize, buf, blocksize);
	free(pt);
	free(buffer);
	free(temp_buffer);

	return 0;
}

int ext2_register_device (block_dev_desc_t * dev_desc, int part_no)
{
	unsigned char buffer[SECTOR_SIZE];

	disk_partition_t info;

	if (!dev_desc->block_read)
		return -1;

	/* check if we have a MBR (on floppies we have only a PBR) */
	if (dev_desc->block_read(dev_desc->dev, 0, 1, (ulong *)buffer) != 1) {
		printf("** Can't read from device %d **\n",
			dev_desc->dev);
		return -1;
	}
	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		 buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
		/* no signature found */
		return -1;
	}

	/* First we assume there is a MBR */
	if (!get_partition_info(dev_desc, part_no, &info)) {
		part_offset = info.start;
		cur_part = part_no;
		part_size = info.size;
	} else if ((strncmp((char *)&buffer[DOS_FS_TYPE_OFFSET], "FAT", 3) == 0) ||
			(strncmp((char *)&buffer[DOS_FS32_TYPE_OFFSET], "FAT32", 5) == 0)) {
		/* ok, we assume we are on a PBR only */
		cur_part = 1;
		part_offset = 0;
	} else {
		printf("** Partition %d not valid on device %d **\n",
			part_no, dev_desc->dev);
		return -1;
	}

	return 0;
}

int do_ext2_format (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *filename = "/";
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;
	int part_length;
	disk_partition_t info;

	if (argc < 3)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (argc == 4)
		filename = argv[3];

	PRINTF("Using device %s %d:%d, directory: %s\n", argv[1], dev, part, filename);

	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return 1;
	}

	if (ext2_register_device(dev_desc,part)!=0) {
		printf("\n** Unable to use %s %d:%d for fattable **\n",
			argv[1], dev, part);
		return 1;
	}

	if (!get_partition_info(dev_desc, part, &info)) {
		total_sector = (info.size * info.blksz) / SECTOR_SIZE;
	} else {
		printf("error : get partition info\n");
		return 1;
	}

	printf("formatting\n");
	if (mkfs_ext2(dev_desc, part))
		return 1;

	return 0;
}

U_BOOT_CMD(
	ext2format,	3, 1, do_ext2_format,
	"format device as EXT2 filesystem",
	"<interface> <dev[:part]>\n"
	"	  - format device as EXT2 filesystem on 'dev'"
);
