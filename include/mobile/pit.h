/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Sanghee Kim <sh0130.kim@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _PITINFO_H_
#define _PITINFO_H_

#define PIT_PKT_SIZE 		4096 	/* 4KB */
#define PIT_PART_MAX 		32
#define PIT_BOOTPART0_ID	80		/* 80 ~ 89 */
#define PIT_BOOTPART1_ID	90		/* 90 ~ 99 */
#define PIT_BOOTPARTn_GET(id)	((id / 10) - 7)	/* 1: 80 ~ 89, 2: 90 ~ 99 */

#define PIT_MAGIC		0x12349876

enum bin_type {
	PIT_BINTYPE_AP = 0,
	PIT_BINTYPE_CP,
};

enum dev_type {
	PIT_DEVTYPE_ONENAND = 0,
	PIT_DEVTYPE_FILE,		/* writing on fat */
	PIT_DEVTYPE_MMC,
	PIT_DEVTYPE_ALL
};

enum part_type {
	PART_TYPE_NONE = 0,
	PART_TYPE_BCT,
	PART_TYPE_BOOTLOADER,
	PART_TYPE_PARTITION_TABLE,
	PART_TYPE_NVDATA,
	PART_TYPE_DATA,
	PART_TYPE_MBR,
	PART_TYPE_EBR,
	PART_TYPE_GP1,
	PART_TYPE_GPT,
};

enum part_filesys {
	PART_FS_TYPE_NONE = 0,
	PART_FS_TYPE_BASIC,
	PART_FS_TYPE_ENHANCED,		/* FAT */
	PART_FS_TYPE_EXT2,
	PART_FS_TYPE_YAFFS2,
	PART_FS_TYPE_CEXT4,
	PART_FS_TYPE_EXT4,
};

/* PIT Structure = pit_header + partition_info + ... */
typedef struct {
	unsigned int magic;		/* magic code */
	int count;			/* partition (OneNAND + MOVINAND) */
	char gang_format[8];		/* COM_TAR2 */
	char model[8];
	char dummy[4];
} __attribute__((packed)) pit_header_t;

typedef struct {
	enum bin_type bin_type;		/* binary type (AP or CP?) */
	enum dev_type dev_type;		/* partition device type */
	int	id;			/* partition id */
	enum part_type part_type;		/* partition type */
	enum part_filesys filesys;		/* partition filesystem type */
	unsigned int blk_start;		/* start sector (sector is 512 B) */
	unsigned int blk_num;		/* sector number */
	unsigned int offset;		/* file offset (in tar) */
	unsigned int file_size;		/* file size */
	char name[32];			/* partition name */
	char file_name[32];		/* file name */
	char delta_name[32];		/* delta file name for FOTA */
} __attribute__((packed)) partition_info_t;

struct pitpart_data {
	int valid;			/* valid flag data */
	int dev_type;			/* partition device type */
	int id;				/* partition id */
	int	part_type;		/* partition type */
	int	filesys;		/* partition filesystem type */
	char *name;			/* partition name */
	char *file_name;		/* binary file name */
	u64 size;			/* total size of the partition */
	u64 offset;			/* offset within device */
	u32 sector_size;		/* size of sector */
} __attribute__((packed));

/* specially for qboot, magic code is located at 0xff0 */
#define QBOOT_ERASE_SIZE	0x1000
extern int qboot_erase;

extern int get_pitpart_count(void);
extern int get_pitpart_id(char *name);
extern int get_pitpart_id_by_filename(char *file_name);
extern int get_pitpart_id_for_pit(char *file_name);
extern char *get_pitpart_name(int id);
extern u32 get_pitpart_size(int id);
extern int pit_get_version(void);
extern int pit_check_version(char *file_name);
extern int pit_no_partition_support(void);
extern int pit_get_partition_base(void);
extern int pit_adjust_id_on_mmc(int index);
extern int do_pit(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern void erase_given_area(char *name, u32 size);
#endif /* _PITINFO_H_ */
