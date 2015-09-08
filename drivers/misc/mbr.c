/*
 * Copyright (C) 2010 Samsung Electrnoics
 * Kyungmin Park <kyungmin.park@samsung.com>
 */

#include <common.h>
#include <mbr.h>
#include <mmc.h>

#define SIGNATURE	((unsigned short) 0xAA55)

unsigned int mbr_offset[16];
unsigned int mbr_parts = 0;

static int logical = 4;
static int extended_lba;

static int mbr_dev;

void set_mbr_dev(int dev)
{
	mbr_dev = dev;
}

static inline void set_chs_value(struct mbr_partition *part, int part_num)
{
	/* FIXME */
	if (part_num == 0) {
		part->f_chs[0] = 0x01;
		part->f_chs[1] = 0x01;
		part->f_chs[2] = 0x00;
	} else {
		part->f_chs[0] = 0x03;
		part->f_chs[1] = 0xd0;
		part->f_chs[2] = 0xff;
	}
	part->l_chs[0] = 0x03;
	part->l_chs[1] = 0xd0;
	part->l_chs[2] = 0xff;
}

void set_mbr_table(unsigned int start_addr, int parts,
		unsigned int *blocks, unsigned int *part_offset)
{
	struct mmc *mmc = find_mmc_device(mbr_dev);
	struct mbr mbr_table;
	unsigned int offset = start_addr;
	unsigned int max = 0;
	unsigned int size = 0;
	int i, j;

	max = mmc->capacity / mmc->read_bl_len;

	memset(&mbr_table, 0, sizeof(struct mbr));

	mbr_table.signature = SIGNATURE;

	if (blocks[parts - 1] == 0) {
		size = start_addr;
		for (i = 0; i < parts; i++)
			size += blocks[i];
		blocks[parts - 1] = max - size;
	}

	/* Primary */
	for (i = 0; i < 3; i++) {
		mbr_table.parts[i].partition_type = 0x83;	/* Linux */
		mbr_table.parts[i].lba = offset;
		mbr_table.parts[i].nsectors = blocks[i];
		part_offset[i] = offset;
		offset += blocks[i];

		set_chs_value(&mbr_table.parts[i], i);
	}

	if (parts < 4) {
		mmc->block_dev.block_write(mbr_dev, 0, 1, &mbr_table);
		return;
	}

	/* Extended */
	mbr_table.parts[i].partition_type = 0x05;	/* Extended */
	mbr_table.parts[i].lba = offset;
	mbr_table.parts[i].nsectors = max - offset;
	set_chs_value(&mbr_table.parts[i], i);

	mmc->block_dev.block_write(mbr_dev, 0, 1, &mbr_table);
	extended_lba = mbr_table.parts[i].lba;

	for (; i < parts; i++) {
		struct mbr ebr_table;
		memset(&ebr_table, 0, sizeof(struct mbr));

		ebr_table.signature = SIGNATURE;

		for (j = 0; j < 2; j++) {
			if (j == 0) {
				blocks[parts - 1] -= 0x10;
				ebr_table.parts[j].partition_type = 0x83;
				ebr_table.parts[j].lba = 0x10;
				ebr_table.parts[j].nsectors = blocks[i];
				set_chs_value(&ebr_table.parts[j], i);
			} else {
				if (parts != i + 1) {
					ebr_table.parts[j].partition_type = 0x05;
					ebr_table.parts[j].lba = offset +
						ebr_table.parts[0].lba +
						ebr_table.parts[0].nsectors -
						extended_lba;
					ebr_table.parts[j].nsectors = 0x10 + blocks[i + 1];
					set_chs_value(&ebr_table.parts[j], i);
				}
			}
		}

		mmc->block_dev.block_write(mbr_dev, offset, 1, &ebr_table);

		offset += ebr_table.parts[0].lba;
		part_offset[i] = offset;
		offset += ebr_table.parts[0].nsectors;
	}
}

static int get_ebr_table(struct mmc *mmc, struct mbr_partition *mp,
		int ebr_next, unsigned int *part_offset, int parts)
{
	struct mbr *ebr;
	struct mbr_partition *p;
	char buf[512];
	int ret, i;
	int lba = 0;

	if (ebr_next)
		lba = extended_lba;

	lba += mp->lba;
	ret = mmc->block_dev.block_read(mbr_dev, lba, 1, buf);
	if (ret != 1) {
		printf("%s[%d] mmc_read_blocks %d\n", __func__, __LINE__, ret);
		return 0;
	}
	ebr = (struct mbr *) buf;

	if (ebr->signature != SIGNATURE) {
		printf("Signature error 0x%x\n", ebr->signature);
		return 0;
	}

	for (i = 0; i < 2; i++) {
		p = (struct mbr_partition *) &ebr->parts[i];

		if (i == 0) {
			logical++;
			lba += p->lba;
			if (p->partition_type == 0x83) {
				part_offset[parts] = lba;
				parts++;
			}
		}
	}

	if (p->lba && p->partition_type == 0x5)
		parts = get_ebr_table(mmc, p, 1, part_offset, parts);

	return parts;
}

int get_mbr_table(unsigned int *part_offset)
{
	struct mmc *mmc = find_mmc_device(mbr_dev);
	struct mbr_partition *mp;
	struct mbr *mbr;
	char buf[512];
	int ret, i;
	int parts = 0;

	ret = mmc->block_dev.block_read(mbr_dev, 0, 1, buf);
	if (ret != 1) {
		printf("%s[%d] mmc_read_blocks %d\n", __func__, __LINE__, ret);
		return 0;
	}

	mbr = (struct mbr *) buf;

	if (mbr->signature != SIGNATURE)
		printf("Signature error 0x%x\n", mbr->signature);

	logical = 4;

	for (i = 0; i < 4; i++) {
		mp = (struct mbr_partition *) &mbr->parts[i];

		if (!mp->partition_type)
			continue;

		if (mp->partition_type == 0x83) {
			part_offset[parts] = mp->lba;
			parts++;
		}

		if (mp->lba && mp->partition_type == 0x5) {
			extended_lba = mp->lba;
			parts = get_ebr_table(mmc, mp, 0, part_offset, parts);
		}
	}

	return parts;
}

static inline int get_cylinder(char chs1, char chs2, int *sector)
{
	*sector = chs1 & 0x3f;

	return ((chs1 & 0xc0) << 2) | chs2;
}

static void ebr_show(struct mmc *mmc, struct mbr_partition *mp, int ebr_next)
{
	struct mbr *ebr;
	struct mbr_partition *p;
	char buf[512];
	int ret, i, sector, cylinder;
	int lba = 0;

	if (ebr_next)
		lba = extended_lba;

	lba += mp->lba;
	printf(">>> Read sector from 0x%08x (LBA: 0x%08x + 0x%x)\n",
		lba, mp->lba, (lba == mp->lba) ? 0 : lba - mp->lba);

	ret = mmc->block_dev.block_read(mbr_dev, lba, 1, buf);
	if (ret != 1) {
		printf("%s[%d] mmc_read_blocks %d\n", __func__, __LINE__, ret);
		return;
	}
	ebr = (struct mbr *) buf;

	if (ebr->signature != SIGNATURE) {
		printf("Signature error 0x%x\n", ebr->signature);
		return;
	}

	for (i = 0; i < 2; i++) {
		p = (struct mbr_partition *) &ebr->parts[i];

		if (i == 0) {
			logical++;
			printf("Extended Part %d\n", logical);
		} else
			printf("Extended Part next\n");

		printf("status   0x%02x\n", p->status);
		printf("head     0x%02x,", p->f_chs[0]);
		cylinder = get_cylinder(p->f_chs[1], p->f_chs[2], &sector);
		printf("\tsector   0x%02x,", sector);
		printf("\tcylinder 0x%04x\n", cylinder);
		printf("type     0x%02x\n", p->partition_type);
		printf("head     0x%02x,", p->l_chs[0]);
		cylinder = get_cylinder(p->l_chs[1], p->l_chs[2], &sector);
		printf("\tsector   0x%02x,", sector);
		printf("\tcylinder 0x%04x\n", cylinder);
		printf("lba      0x%08x (%d), ", p->lba, p->lba);
		printf("nsectors 0x%08x (%d)\n", p->nsectors, p->nsectors);
	}

	if (p->lba && p->partition_type == 0x5)
		ebr_show(mmc, p, 1);
}

static void mbr_show(void)
{
	struct mmc *mmc = find_mmc_device(mbr_dev);
	struct mbr_partition *mp;
	struct mbr *mbr;
	char buf[512];
	int ret, i;
	int cylinder;

	ret = mmc->block_dev.block_read(mbr_dev, 0, 1, buf);
	if (ret != 1) {
		printf("%s[%d] mmc_read_blocks %d\n", __func__, __LINE__, ret);
		return;
	}

	mbr = (struct mbr *) buf;

	if (mbr->signature != SIGNATURE)
		printf("Signature error 0x%x\n", mbr->signature);

	logical = 4;
	printf("MBR partition info\n");
	for (i = 0; i < 4; i++) {
		mp = (struct mbr_partition *) &mbr->parts[i];

		if (!mp->partition_type)
			continue;

		printf("Part %d\n", i + 1);
		printf("status   0x%02x\n", mp->status);
		printf("head     0x%02x,", mp->f_chs[0]);
		printf("\tsector   0x%02x,", mp->f_chs[1] & 0x3f);
		cylinder = (mp->f_chs[1] & 0xc0) << 2;
		cylinder |= mp->f_chs[2];
		printf("\tcylinder 0x%04x\n", cylinder);
		printf("type     0x%02x\n", mp->partition_type);
		printf("head     0x%02x,", mp->l_chs[0]);
		printf("\tsector   0x%02x,", mp->l_chs[1] & 0x3f);
		cylinder = (mp->l_chs[1] & 0xc0) << 2;
		cylinder |= mp->l_chs[2];
		printf("\tcylinder 0x%04x\n", cylinder);
		printf("lba      0x%08x (%d), ", mp->lba, mp->lba);
		printf("nsectors 0x%08x (%d)\n", mp->nsectors, mp->nsectors);

		if (mp->lba && mp->partition_type == 0x5) {
			extended_lba = mp->lba;
			ebr_show(mmc, mp, 0);
		}
	}
}

static unsigned long memsize_to_blocks(const char *const ptr, const char **retptr)
{
	unsigned long ret = simple_strtoul(ptr, (char **)retptr, 0);
	int shift = 0;

	switch (**retptr) {
		case 'G':
		case 'g':
			shift += 10;
		case 'M':
		case 'm':
			shift += 10;
		case 'K':
		case 'k':
			shift += 10;
			(*retptr)++;
		default:
			shift -= 9;
			break;
	}

	if (shift > 0)
		ret <<= shift;
	else
		ret >>= shift;

	return ret;
}

void set_mbr_info(char *ramaddr, unsigned int len, unsigned int reserved)
{
	char mbr_str[256];
	char save[16][16];
	char *p;
	char *tok;
	unsigned int size[16];
	int i = 0;

	strncpy(mbr_str, ramaddr, len);
	p = mbr_str;

	for (i = 0; ; i++, p = NULL) {
		tok = strtok(p, ",");
		if (tok == NULL)
			break;
		strcpy(save[i], tok);
		printf("part%d: %s\n", i, save[i]);
	}

	mbr_parts = i;
	printf("find %d partitions\n", mbr_parts);

	for (i = 0; i < mbr_parts; i++) {
		p = save[i];
		size[i] = memsize_to_blocks(p, (const char **)&p);
	}

	puts("save the MBR Table...\n");
	set_mbr_table(reserved, mbr_parts, size, mbr_offset);
}

static void mbr_default(void)
{
	char *mbrparts;

	puts("using default MBR\n");

	mbrparts = getenv("mbrparts");
	set_mbr_info(mbrparts, strlen(mbrparts), MBR_START_OFS_BLK);
}

static int do_mbr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	switch (argc) {
	case 2:
		if (strncmp(argv[1], "show", 2) == 0)
			mbr_show();
		else if (strncmp(argv[1], "default", 3) == 0)
			mbr_default();
		break;
	case 3:
		if (strncmp(argv[1], "dev", 3) == 0)
			set_mbr_dev(simple_strtoul(argv[2], NULL, 10));
		break;
	default:
		cmd_usage(cmdtp);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	mbr,	CONFIG_SYS_MAXARGS,	1, do_mbr,
	"Master Boot Record",
	"show - show MBR\n"
	"mbr default - reset MBR partition to defaults\n"
	"mbr dev #num - set device number\n"
);
