/*
 * flash.c -- board flashing routines
 *
 * Copyright (C) 2011 Samsung Electronics
 * author: Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <malloc.h>
#include <mbr.h>
#include <mmc.h>
#include <fat.h>
#include <flash_entity.h>
#include <linux/types.h>
#include <jffs2/load_kernel.h>

#define ONENAND_BLOCK_SZ	(256 * 1024)
#define MMC_BLOCK_SZ		ONENAND_BLOCK_SZ
#define MMC_FAT_BLOCK_SZ	(4 * 1024 * 1024)
#define MMC_SECTOR_SZ		512
#define MMC_UBOOT_OFFSET	128
#define MMC_UBOOT_SZ		512
#define MMC_BACKUP_PART_NUM	2
#define MMC_BACKUP_OFFSET	0
#define MMC_BACKUP_SIZE		2048
#define PARAM_LEN		12
#define SHORT_PART_NAME		15
#define LONG_PART_NAME		20
#define SIGNATURE		((unsigned short) 0xAA55)
#define CALLOC_STRUCT(n, type)	(struct type * ) calloc(n, sizeof(struct type))
#define DEFAULT_MMC_PART_NAME	"mmc-default-part"
#define BOOT_PART_NUM_FILES	1

#define USE_ONENAND
/* #define USE_MMC_BACKUP */
#define USE_MMC_UBOOT
#define USE_MMC

typedef int (*rw_op)(void *buf, unsigned int len, unsigned long from);
typedef int (*erase_op)(unsigned int len, unsigned long from);

struct flash_entity_ctx {
	unsigned long		offset;	/* offset into the device */
	unsigned long		length; /* size of the entity */
	u8			*buf; /* buffer for one chunk */
	unsigned long		buf_len; /* one chunk length */
	unsigned int		buffered; /* # available bytes in buf */
	unsigned int		num_done; /* total bytes handled */
	rw_op			read; /* chunk read op for this medium */
	rw_op			write; /* chunk write op for this medium */
	erase_op		erase; /* erase op for this medium or NULL */
	struct flash_entity	*this_entity; /* the containing entity */
	void			*associated; /* related entity, if any */
};

struct mbr_part_data {
	unsigned long		offset; /* #sectors from mmc begin */
	unsigned long		length; /* #sectors in this partition*/
	u8			primary; /* != 0 if primary, 0 if extended */
};

u8 entity_by_name(struct flash_entity *fe, u8 n_ents, const char *name)
{
	while (n_ents--)
		if (!strcmp(fe[n_ents].name, name))
			return n_ents;

	return -1;
}

/*
 * OneNAND partitions and operations
 */
extern struct list_head devices;

static struct part_info **onenand_part;

static u8 onenand_part_num;
static u8 used_nand_parts;

static u8 onenand_buf[ONENAND_BLOCK_SZ];

static u8 onenand_part_count(void)
{
	struct list_head *dentry, *pentry;
	struct mtd_device *dev;
	u8 p_num = 0;

	/*
	 * Only count number of partitions on a device onenand0, samsung-onenand
	 */
	list_for_each(dentry, &devices) {
		struct mtdids *id;
		dev = list_entry(dentry, struct mtd_device, link);
		id = dev->id;

		if (id->type == MTD_DEV_TYPE_ONENAND && id->num == 0 &&
		    !strcmp(id->mtd_id, "samsung-onenand"))
		    	list_for_each(pentry, &dev->parts)
				++p_num;
	}

	return p_num;
}

static int get_part_info(struct part_info *parts[], u8 p_num)
{
	struct mtd_device *dev;
	u8 out_num;
	int i;
	char *p_name = "onenand0,    ";
	char *p_num_str = p_name + 9;

	for (i = 0; i < p_num; i++) {
		sprintf(p_num_str, "%d", i);

		/* if some partiton cannot be found then give up at all */
		if (find_dev_and_part(p_name, &dev, &out_num, &parts[i]))
			return 1;
	}

	return 0;
}

static int rw_onenand(void *buf, char *op, unsigned int len, unsigned long from)
{
	char ram_addr[PARAM_LEN];
	char offset[PARAM_LEN];
	char length[PARAM_LEN];
	char *argv[] = {"onenand", op, ram_addr, offset, length};
	
	sprintf(ram_addr, "0x%lx", (unsigned long)buf);
	sprintf(offset, "0x%lx", from);
	sprintf(length, "0x%x", len);

	return do_onenand(NULL, 0, 5, argv);
}

static inline int read_onenand(void *buf, unsigned int len, unsigned long from)
{
	return rw_onenand(buf, "read", len, from);
}

static inline int write_onenand(void *buf, unsigned int len, unsigned long from)
{
	return rw_onenand(buf, "write", len, from);
}

static int erase_onenand(unsigned int len, unsigned long from)
{
	char offset[PARAM_LEN];
	char length[PARAM_LEN];
	char *argv[] = {"onenand", "erase", offset, length};
	
	sprintf(offset, "0x%lx", from);
	sprintf(length, "0x%x", len);

	return do_onenand(NULL, 0, 4, argv);
}

int use_onenand(const char *name)
{
	if (!strcmp(name, "bootloader"))
		return 1;
	if (!strcmp(name, "params"))
		return 1;
#if 0
	if (!strcmp(name, "config"))
		return 1;
	if (!strcmp(name, "csa"))
		return 1;
#endif
	if (!strcmp(name, "kernel"))
		return 1;
#if 0
	if (!strcmp(name, "log"))
		return 1;
	if (!strcmp(name, "modem"))
		return 1;
#endif
	if (!strcmp(name, "qboot"))
		return 1;
	if (!strcmp(name, "UBI"))
		return 1;
	
	return 0;
}

/*
 * MMC u-boot partitions
 */
static struct mbr_part_data *uboot_pdata;

static u8 uboot_part_num;
static u8 used_uboot_parts;

int use_uboot(struct mbr_part_data *pdata, u8 i)
{
	/*
	 * Use i and pdata[i] members to decide if the partition is used
	 */
	return 1;
}

char *alloc_uboot_name(u8 i)
{
	char *name = calloc(SHORT_PART_NAME, 1);

	if (name) {
		sprintf(name, "mmc-u-boot");
		return name;
	}

	return DEFAULT_MMC_PART_NAME;
}

/*
 * MMC backup partitions
 */
static struct mbr_part_data *backup_pdata;

static u8 backup_part_num;
static u8 used_backup_parts;

int use_backup(struct mbr_part_data *pdata, u8 i)
{
	/*
	 * Use i and pdata[i] members to decide if the partition is used
	 */
	return 1;
}

char *alloc_backup_name(u8 i)
{
	char *name = calloc(SHORT_PART_NAME, 1);

	if (name) {
		sprintf(name, "mmc-backup-%d", i + 1);
		return name;
	}

	return DEFAULT_MMC_PART_NAME;
}

/*
 * MMC partitions and operations
 */
struct mmc *mmc;

static struct mbr_part_data *mmc_pdata;

static u8 mmc_part_num;
static u8 used_mmc_parts;

static u8 mmc_buf[MMC_BLOCK_SZ];

static int extended_lba;

static int mmc_mbr_dev = 0;

static u8 pri_count = 0;
static u8 ext_count = 4;

static int read_ebr(struct mmc *mmc, struct mbr_partition *mp,
		int ebr_next, struct mbr_part_data *pd, int parts)
{
	struct mbr *ebr;
	struct mbr_partition *p;
	char buf[512];
	int ret, i;
	int lba = 0;

	if (ebr_next)
		lba = extended_lba;

	lba += mp->lba;
	ret = mmc->block_dev.block_read(mmc_mbr_dev, lba, 1, buf);
	if (ret != 1)
		return 0;
	
	ebr = (struct mbr *) buf;

	if (ebr->signature != SIGNATURE) {
		printf("Signature error 0x%x\n", ebr->signature);
		return 0;
	}

	for (i = 0; i < 2; i++) {
		p = (struct mbr_partition *) &ebr->parts[i];

		if (i == 0) {
			lba += p->lba;
			if (p->partition_type == 0x83) {
				if (pd) {
					pd[parts].offset = lba;
					pd[parts].length = p->nsectors;
					pd[parts].primary = 0;
				}
				parts++;
			}
		}
	}

	if (p->lba && p->partition_type == 0x5)
		parts = read_ebr(mmc, p, 1, pd, parts);

	return parts;
}

static int read_mbr(struct mmc *mmc, struct mbr_part_data *pd)
{
	struct mbr_partition *mp;
	struct mbr *mbr;
	char buf[512];
	int ret, i;
	int parts = 0;

	ret = mmc->block_dev.block_read(mmc_mbr_dev, 0, 1, buf);
	if (ret != 1)
		return 0;

	mbr = (struct mbr *) buf;

	if (mbr->signature != SIGNATURE) {
		printf("Signature error 0x%x\n", mbr->signature);
		return 0;
	}

	for (i = 0; i < 4; i++) {
		mp = (struct mbr_partition *) &mbr->parts[i];

		if (!mp->partition_type)
			continue;

		if (mp->partition_type == 0x83) {
			if (pd) {
				pd[parts].offset = mp->lba;
				pd[parts].length = mp->nsectors;
				pd[parts].primary = 1;
			}
			parts++;
		}

		if (mp->lba && mp->partition_type == 0x5) {
			extended_lba = mp->lba;
			parts = read_ebr(mmc, mp, 0, pd, parts);
		}
	}

	return parts;
}

static int rw_mmc(void *buf, char *op, unsigned int len, unsigned long from)
{
	char dev_num[PARAM_LEN];
	char ram_addr[PARAM_LEN];
	char offset[PARAM_LEN];
	char length[PARAM_LEN];
	char *argv[] = {"mmc", op, dev_num, ram_addr, offset, length};
	
	sprintf(dev_num, "0x%x", mmc_mbr_dev);
	sprintf(ram_addr, "0x%lx", (unsigned long)buf);
	sprintf(offset, "0x%lx", from / MMC_SECTOR_SZ); /* guaranteed integer */
	sprintf(length, "0x%x", (len + MMC_SECTOR_SZ - 1) / MMC_SECTOR_SZ);

	return do_mmcops(NULL, 0, 6, argv);
}

static inline int read_mmc(void *buf, unsigned int len, unsigned long from)
{
	return rw_mmc(buf, "read", len, from);
}

static inline int write_mmc(void *buf, unsigned int len, unsigned long from)
{
	return rw_mmc(buf, "write", len, from);
}

static int switch_mmc(int n)
{
	char number[PARAM_LEN];
	char *argv[] = {"mmc", "boot", "0", "1", "1", number};

	sprintf(number, "%d", n);

	return do_mmcops(NULL, 0, 6, argv);
}

/*
 * Return number of flash entities per this partition
 */
u8 use_mmc(struct mbr_part_data *pdata, u8 i)
{
	/*
	 * Use i and pdata[i] members to decide if the partition is used
	 */
	if (i == 4)
		return 0; /* do not expose UMS; there is a separate command */
	if (i == 1)
		return BOOT_PART_NUM_FILES;
	return 1;
}

char *alloc_mmc_name(struct mbr_part_data *pdata, u8 i, u8 l)
{
	char *name = calloc(PARAM_LEN, 1);

	if (name) {
		sprintf(name, "mmc0-");
		if (pdata[i].primary)
			sprintf(name + strlen(name), "pri-%d", l ? pri_count : ++pri_count);
		else
			sprintf(name + strlen(name), "ext-%d", l ? ext_count : ++ext_count);
		
		return name;
	}

	return DEFAULT_MMC_PART_NAME;
}

/*
 * FAT operations
 */
static u8 fat_buf[MMC_FAT_BLOCK_SZ];

static char *fat_filename;
static int fat_part_id;

static int read_fat(void *buf, unsigned int len, unsigned long from)
{
	int ret;

	ret = fat_register_device(&mmc->block_dev, fat_part_id);
	if (ret < 0) {
		printf("error : fat_register_divce\n");
		return 0;
	}

	return file_fat_read(fat_filename, buf, len);
}

static int write_fat(void *buf, unsigned int len, unsigned long from)
{
#ifdef CONFIG_FAT_WRITE
	int ret;

	ret = fat_register_device(&mmc->block_dev, fat_part_id);
	if (ret < 0) {
		printf("error : fat_register_divce\n");
		return 0;
	}

	ret = file_fat_write(fat_filename, buf, len);

	/* format and write again */
	if (ret == 1) {
		printf("formatting\n");
		if (mkfs_vfat(&mmc->block_dev, fat_part_id)) {
			printf("error formatting device\n");
			return 0;
		}
		ret = file_fat_write(fat_filename, buf, len);
	}

	if (ret < 0) {
		printf("error : writing %s\n", fat_filename);
		return 0;
	}
#else
	printf("error : FAT write not supported\n");
	return 0;
#endif
	return len;
}

/*
 * Entity-specific prepare and finish
 */
static void reset_ctx(struct flash_entity_ctx *ctx)
{
	ctx->buffered = 0;
	ctx->num_done = 0;
}

static int generic_prepare(void *ctx, u8 mode)
{
	struct flash_entity_ctx *ct = ctx;

	reset_ctx(ct);
	if (mode == FLASH_WRITE) {
		if (ct->erase) {
			printf("Erase partition: %s ", ct->this_entity->name);
			ct->erase(ct->length, ct->offset);
		}
		printf("Write partition: %s ", ct->this_entity->name);
	} else if (mode == FLASH_READ) {
		printf("Read partition: %s ", ct->this_entity->name);
	}
	return 0;
}

static int generic_finish(void *ctx, u8 mode)
{
	struct flash_entity_ctx *ct = ctx;

	if (mode == FLASH_WRITE && ct->buffered > 0)
		ct->write(ct->buf, ct->buffered, ct->offset + ct->num_done);

	return 0;
}

static int prepare_and_erase_associated(void *ctx, u8 mode)
{
	struct flash_entity_ctx *ct = ctx;
	struct flash_entity_ctx *associated_ct;
	struct flash_entity *associated;

	if (mode == FLASH_WRITE) {
		associated = ct->associated;
		associated_ct = associated->ctx;
		if (associated_ct->erase) {
			printf("Erase associated partition: %s ",
				associated->name);
			associated_ct->erase(associated_ct->length,
					     associated_ct->offset);
		}
	}
	generic_prepare(ct, mode);

	return 0;
}

static int prepare_mmc_backup(void *ctx, u8 mode)
{
	struct flash_entity_ctx *ct = ctx;

	switch_mmc(((char)ct->associated) - '1' + 1);
	return generic_prepare(ctx, mode);
}

static int finish_mmc_backup(void *ctx, u8 mode)
{
	int ret;

	ret = generic_finish(ctx, mode);

	switch_mmc(0);

	return ret;
}

static int prepare_fat(void *ctx, u8 mode)
{
	struct flash_entity_ctx *ct = ctx;

	fat_filename = ct->associated;
	/*
	 * This is a hack. It is assumed, that partition name
	 * follows "mmc0-pri-X-<name>" where X is one digit
	 */
	fat_part_id = ct->this_entity->name[9] - '0';
	return generic_prepare(ctx, mode);
}

/*
 * Transport layer to storage adaptation
 */

/* 
 * Adapt transport layer buffer size to storage chunk size
 *
 * return < n to indicate no more data to read
 */
static int read_block(void *ctx, unsigned int n, void *buf)
{
	struct flash_entity_ctx *ct = ctx;
	unsigned int nread = 0;

	if (n == 0)
		return n;
	
	while (nread < n) {
		unsigned int copy;

		if (ct->num_done >= ct->length)
			break;
		if (ct->buffered == 0) {
			ct->read(ct->buf, ct->buf_len,
				 ct->offset + ct->num_done);
			ct->buffered = ct->buf_len;
		}
		copy = min(n - nread, ct->buffered);

		memcpy(buf + nread, ct->buf + ct->buf_len - ct->buffered, copy);
		nread += copy;
		ct->buffered -= copy;
		ct->num_done += copy;
	}

	return nread;
}

/* 
 * Adapt transport layer buffer size to storage chunk size
 */
static int write_block(void *ctx, unsigned int n, void *buf)
{
	struct flash_entity_ctx *ct = ctx;
	unsigned int nwritten = 0;

	if (n == 0)
		return n;

	while (nwritten < n) {
		unsigned int copy;

		if (ct->num_done >= ct->length)
			break;
		if (ct->buffered >= ct->buf_len) {
			ct->write(ct->buf, ct->buf_len,
				  ct->offset + ct->num_done);
			ct->buffered = 0;
			ct->num_done += ct->buf_len;
			if (ct->num_done >= ct->length)
				break;
		}
		copy = min(n - nwritten, ct->buf_len - ct->buffered);

		memcpy(ct->buf + ct->buffered, buf + nwritten, copy);
		nwritten += copy;
		ct->buffered += copy;
	}

	return nwritten;
}

/* 
 * Flash entities definitions for this board
 */
static struct flash_entity *flash_ents;

void customize_entities(struct flash_entity *fe, u8 n_ents)
{
	int i;
	for (i = 0; i < n_ents; ++i) {
		if (!strcmp(fe[i].name, "bootloader")) {
			struct flash_entity_ctx *ctx;

			fe[i].prepare = prepare_and_erase_associated;
			ctx = fe[i].ctx;
			ctx->associated = &fe[entity_by_name(fe,
					      n_ents, "params")];

			continue;
		}
		if (!strcmp(fe[i].name, "kernel")) {
			struct flash_entity_ctx *ctx;

			fe[i].prepare = prepare_and_erase_associated;
			ctx = fe[i].ctx;
			ctx->associated = &fe[entity_by_name(fe,
					      n_ents, "qboot")];

			continue;
		}
		if (!strcmp(fe[i].name, "UBI")) {
			struct flash_entity_ctx *ctx;

			fe[i].prepare = prepare_and_erase_associated;
			ctx = fe[i].ctx;
			ctx->associated = &fe[entity_by_name(fe,
					      n_ents, "qboot")];

			continue;
		}
		if (!strcmp(fe[i].name, "mmc0-pri-2")) {
			struct flash_entity_ctx *ctx;
			char *name;

			fe[i].prepare = prepare_fat;
			ctx = fe[i].ctx;
			ctx->length = min(ctx->length, MMC_FAT_BLOCK_SZ);
			ctx->buf = fat_buf;
			ctx->buf_len = ctx->length;
			ctx->read = read_fat;
			ctx->write = write_fat;
			if ((char)ctx->associated == 0)
				ctx->associated = "uImage";
			else
				ctx->associated = "u-boot";

			name = calloc(LONG_PART_NAME, 1);
			if (name) {
				sprintf(name, "%s-%s", fe[i].name,
					(char * )ctx->associated);
				free(fe[i].name);
				fe[i].name = name;
			}

			continue;
		}
		if (!strcmp(fe[i].name, "mmc-backup-1")) {
			struct flash_entity_ctx *ctx;
			
			fe[i].prepare = prepare_mmc_backup;
			fe[i].finish = finish_mmc_backup;
			ctx = fe[i].ctx;
			ctx->associated = (void * )'1';

			continue;
		}
		if (!strcmp(fe[i].name, "mmc-backup-2")) {
			struct flash_entity_ctx *ctx;
			
			fe[i].prepare = prepare_mmc_backup;
			fe[i].finish = finish_mmc_backup;
			ctx = fe[i].ctx;
			ctx->associated = (void * )'2';

			continue;
		}
	}
}

void register_flash_areas(void)
{
	u8 i, j;

#ifdef USE_ONENAND
	if (!mtdparts_init()) {
		onenand_part_num = onenand_part_count();
		if (onenand_part_num) {
			onenand_part = CALLOC_STRUCT(onenand_part_num,
						     part_info * );
			if (onenand_part)
				if (get_part_info(onenand_part,
						   onenand_part_num)) {
					free(onenand_part);
					onenand_part = NULL;
				}
		}
	}
	if (onenand_part)
		for (i = 0; i < onenand_part_num; ++i)
			if (use_onenand(onenand_part[i]->name))
				++used_nand_parts;
#endif
	
#ifdef USE_MMC
	mmc = find_mmc_device(mmc_mbr_dev);
	if (mmc) {
		mmc_part_num = read_mbr(mmc, NULL);
		if (mmc_part_num) {
			mmc_pdata = CALLOC_STRUCT(mmc_part_num, mbr_part_data);
			if (mmc_pdata)
				if (!read_mbr(mmc, mmc_pdata)) {
					free(mmc_pdata);
					mmc_pdata = NULL;
				}
		}
	}
	if (mmc_pdata) {
		used_mmc_parts = BOOT_PART_NUM_FILES - 1;
		for (i = 0; i < mmc_part_num; ++i)
			if (use_mmc(mmc_pdata, i))
				++used_mmc_parts;
	}
	pri_count = 0;
	ext_count = 4;
#endif

#ifdef USE_MMC_BACKUP
	if (mmc) {
		backup_part_num = MMC_BACKUP_PART_NUM;
		if (backup_part_num) {
			backup_pdata = CALLOC_STRUCT(backup_part_num,
						     mbr_part_data);
			if (backup_pdata)
				for (i = 0; i < backup_part_num; ++i) {
					backup_pdata[i].offset =
						MMC_BACKUP_OFFSET;
					backup_pdata[i].length =
						MMC_BACKUP_SIZE;
					backup_pdata[i].primary = 0;
				}
		}
	}
	if (backup_pdata)
		for (i = 0; i < backup_part_num; ++i)
			if (use_backup(backup_pdata, i))
				++used_backup_parts;
#endif

#ifdef USE_MMC_UBOOT
	if (mmc) {
		uboot_part_num = 1;
		if (uboot_part_num) {
			uboot_pdata = CALLOC_STRUCT(uboot_part_num,
						    mbr_part_data);
			if (uboot_pdata)
				for (i = 0; i < uboot_part_num; ++i) {
					uboot_pdata[i].offset =
						MMC_UBOOT_OFFSET;
					uboot_pdata[i].length =
						MMC_UBOOT_SZ;
					uboot_pdata[i].primary = 0;
				}
		}
	}
	if (uboot_pdata)
		for (i = 0; i < uboot_part_num; ++i)
			if (use_uboot(uboot_pdata, i))
				++used_uboot_parts;
#endif

	flash_ents = CALLOC_STRUCT(used_nand_parts + used_backup_parts +
				   used_uboot_parts + used_mmc_parts,
				   flash_entity);
	if (!flash_ents)
		goto partinfo_alloc_rollback;

	j = 0;
	for (i = 0; i < onenand_part_num; ++i)
		if (use_onenand(onenand_part[i]->name)) {
			struct flash_entity_ctx *ctx;

			flash_ents[j].name = onenand_part[i]->name;
			flash_ents[j].prepare = generic_prepare;
			flash_ents[j].finish = generic_finish;
			flash_ents[j].read_block = read_block;
			flash_ents[j].write_block = write_block;

			ctx = CALLOC_STRUCT(1, flash_entity_ctx);
			if (!ctx)
				goto flash_ents_alloc_rollback;
			ctx->offset = onenand_part[i]->offset;
			ctx->length = onenand_part[i]->size;
			ctx->buf = onenand_buf;
			ctx->buf_len = ONENAND_BLOCK_SZ;
			ctx->read = read_onenand;
			ctx->write = write_onenand;
			ctx->erase = erase_onenand;
			ctx->this_entity = &flash_ents[j];
			flash_ents[j++].ctx = ctx;
		}
	onenand_part_num = used_nand_parts;

	for (i = 0; i < backup_part_num; ++i)
		if (use_backup(backup_pdata, i)) {
			struct flash_entity_ctx *ctx;

			flash_ents[j].name = alloc_backup_name(i);
			flash_ents[j].prepare = generic_prepare;
			flash_ents[j].finish = generic_finish;
			flash_ents[j].read_block = read_block;
			flash_ents[j].write_block = write_block;

			ctx = CALLOC_STRUCT(1, flash_entity_ctx);
			if (!ctx)
				goto flash_ents_alloc_rollback;
			ctx->offset = backup_pdata[i].offset * MMC_SECTOR_SZ;
			ctx->length = backup_pdata[i].length * MMC_SECTOR_SZ;
			ctx->buf = mmc_buf;
			ctx->buf_len = MMC_BLOCK_SZ;
			ctx->read = read_mmc;
			ctx->write = write_mmc;
			ctx->erase = NULL;
			ctx->this_entity = &flash_ents[j];
			flash_ents[j++].ctx = ctx;
		}
	backup_part_num = used_backup_parts;

	for (i = 0; i < uboot_part_num; ++i)
		if (use_uboot(uboot_pdata, i)) {
			struct flash_entity_ctx *ctx;

			flash_ents[j].name = alloc_uboot_name(i);
			flash_ents[j].prepare = generic_prepare;
			flash_ents[j].finish = generic_finish;
			flash_ents[j].read_block = read_block;
			flash_ents[j].write_block = write_block;

			ctx = CALLOC_STRUCT(1, flash_entity_ctx);
			if (!ctx)
				goto flash_ents_alloc_rollback;
			ctx->offset = uboot_pdata[i].offset * MMC_SECTOR_SZ;
			ctx->length = uboot_pdata[i].length * MMC_SECTOR_SZ;
			ctx->buf = mmc_buf;
			ctx->buf_len = MMC_BLOCK_SZ;
			ctx->read = read_mmc;
			ctx->write = write_mmc;
			ctx->erase = NULL;
			ctx->this_entity = &flash_ents[j];
			flash_ents[j++].ctx = ctx;
		}
	uboot_part_num = used_uboot_parts;

	for (i = 0; i < mmc_part_num; ++i) {
		u8 k = use_mmc(mmc_pdata, i);
		u8 l;
		for (l = 0; l < k; ++l) {
			struct flash_entity_ctx *ctx;

			flash_ents[j].name = alloc_mmc_name(mmc_pdata, i, l);
			flash_ents[j].prepare = generic_prepare;
			flash_ents[j].finish = generic_finish;
			flash_ents[j].read_block = read_block;
			flash_ents[j].write_block = write_block;

			ctx = CALLOC_STRUCT(1, flash_entity_ctx);
			if (!ctx)
				goto flash_ents_alloc_rollback;
			ctx->offset = mmc_pdata[i].offset * MMC_SECTOR_SZ;
			ctx->length = mmc_pdata[i].length * MMC_SECTOR_SZ;
			ctx->buf = mmc_buf;
			ctx->buf_len = MMC_BLOCK_SZ;
			ctx->read = read_mmc;
			ctx->write = write_mmc;
			ctx->erase = NULL;
			ctx->this_entity = &flash_ents[j];
			ctx->associated = (void * )l;
			flash_ents[j++].ctx = ctx;
		}
	}
	mmc_part_num = used_mmc_parts;
	customize_entities(flash_ents, onenand_part_num + backup_part_num +
			   uboot_part_num + mmc_part_num);
	register_flash_entities(flash_ents, onenand_part_num + backup_part_num +
				uboot_part_num + mmc_part_num);

	return;

flash_ents_alloc_rollback:
	while (j--) {
		if (j >= onenand_part_num)
			free(flash_ents[j].name);	
		free(flash_ents[j].ctx);
	}

partinfo_alloc_rollback:
	free(onenand_part);
	free(backup_pdata);
	free(uboot_pdata);
	free(mmc_pdata);
}

void unregister_flash_areas(void)
{
	int j = onenand_part_num + backup_part_num + 
		uboot_part_num + mmc_part_num;
	while (j--) {
		if (j >= onenand_part_num)
			free(flash_ents[j].name);	
		free(flash_ents[j].ctx);
	}
	free(onenand_part);
	free(backup_pdata);
	free(uboot_pdata);
	free(mmc_pdata);
}

