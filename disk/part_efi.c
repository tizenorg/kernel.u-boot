/*
 * Copyright (C) 2008 RuggedCom, Inc.
 * Richard Retanubun <RichardRetanubun@RuggedCom.com>
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
 */

/*
 * Problems with CONFIG_SYS_64BIT_LBA:
 *
 * struct disk_partition.start in include/part.h is sized as ulong.
 * When CONFIG_SYS_64BIT_LBA is activated, lbaint_t changes from ulong to uint64_t.
 * For now, it is cast back to ulong at assignment.
 *
 * This limits the maximum size of addressable storage to < 2 Terra Bytes
 */
#include <common.h>
#include <command.h>
#include <ide.h>
#include <malloc.h>
#include "part_efi.h"
#include <linux/ctype.h>
#include <mmc.h>
#include <compiler.h>

#if defined(CONFIG_CMD_IDE) || \
    defined(CONFIG_CMD_MG_DISK) || \
    defined(CONFIG_CMD_SATA) || \
    defined(CONFIG_CMD_SCSI) || \
    defined(CONFIG_CMD_USB) || \
    defined(CONFIG_MMC) || \
    defined(CONFIG_SYSTEMACE)

/**
 * efi_crc32() - EFI version of crc32 function
 * @buf: buffer to calculate crc32 of
 * @len - length of buf
 *
 * Description: Returns EFI-style CRC32 value for @buf
 */
static inline u32 efi_crc32(const void *buf, u32 len)
{
	return crc32(0, buf, len);
}

/*
 * Private function prototypes
 */

static int pmbr_part_valid(struct partition *part);
static int is_pmbr_valid(legacy_mbr * mbr);

static int is_gpt_valid(block_dev_desc_t * dev_desc, unsigned long long lba,
				gpt_header * pgpt_head, gpt_entry ** pgpt_pte);

static gpt_entry *alloc_read_gpt_entries(block_dev_desc_t * dev_desc,
				gpt_header * pgpt_head);

static int is_pte_valid(gpt_entry * pte);
static int strlen_efi16(efi_char16_t * str);

static char *print_efiname(gpt_entry *pte)
{
	static char name[PARTNAME_SZ + 1];
	int i;
	for (i = 0; i < PARTNAME_SZ; i++) {
		u8 c;
		c = pte->partition_name[i] & 0xff;
		c = (c && !isprint(c)) ? '.' : c;
		name[i] = c;
	}
	name[PARTNAME_SZ] = 0;
	return name;
}

/*
 * Public Functions (include/part.h)
 */

void print_part_efi(block_dev_desc_t * dev_desc)
{
	ALLOC_CACHE_ALIGN_BUFFER(gpt_header, gpt_head, 1);
	gpt_entry *gpt_pte = NULL;
	int i = 0;

	if (!dev_desc) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return;
	}
	/* This function validates AND fills in the GPT header and PTE */
	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
			 gpt_head, &gpt_pte) != 1) {
		printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
		return;
	}

	debug("%s: gpt-entry at %p\n", __func__, gpt_pte);

	printf("Part\tName\t\t\tStart LBA\tEnd LBA\n");
	for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {

		if (is_pte_valid(&gpt_pte[i])) {
			printf("%3d\t%-18s\t0x%08llX\t0x%08llX\n", (i + 1),
				print_efiname(&gpt_pte[i]),
				le64_to_cpu(gpt_pte[i].starting_lba),
				le64_to_cpu(gpt_pte[i].ending_lba));
		} else {
			break;	/* Stop at the first non valid PTE */
		}
	}

	/* Remember to free pte */
	free(gpt_pte);
	return;
}

int get_partition_info_efi(block_dev_desc_t * dev_desc, int part,
				disk_partition_t * info)
{
	ALLOC_CACHE_ALIGN_BUFFER(gpt_header, gpt_head, 1);
	gpt_entry *gpt_pte = NULL;

	/* "part" argument must be at least 1 */
	if (!dev_desc || !info || part < 1) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return -1;
	}

	/* This function validates AND fills in the GPT header and PTE */
	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
			gpt_head, &gpt_pte) != 1) {
		printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
		return -1;
	}

	/* The ulong casting limits the maximum disk size to 2 TB */
	info->start = (ulong) le64_to_cpu(gpt_pte[part - 1].starting_lba);
	/* The ending LBA is inclusive, to calculate size, add 1 to it */
	info->size = ((ulong) le64_to_cpu(gpt_pte[part - 1].ending_lba) + 1)
		     - info->start;
	info->blksz = GPT_BLOCK_SIZE;

	sprintf((char *)info->name, "%s",
			print_efiname(&gpt_pte[part - 1]));
	sprintf((char *)info->type, "U-Boot");

	debug("%s: start 0x%lX, size 0x%lX, name %s", __func__,
		info->start, info->size, info->name);

	/* Remember to free pte */
	free(gpt_pte);
	return 0;
}

int test_part_efi(block_dev_desc_t * dev_desc)
{
	ALLOC_CACHE_ALIGN_BUFFER(legacy_mbr, legacymbr, 1);

	/* Read legacy MBR from block 0 and validate it */
	if ((dev_desc->block_read(dev_desc->dev, 0, 1, (ulong *)legacymbr) != 1)
		|| (is_pmbr_valid(legacymbr) != 1)) {
		return -1;
	}
	return 0;
}

/*
 * Private functions
 */
/*
 * pmbr_part_valid(): Check for EFI partition signature
 *
 * Returns: 1 if EFI GPT partition type is found.
 */
static int pmbr_part_valid(struct partition *part)
{
	if (part->sys_ind == EFI_PMBR_OSTYPE_EFI_GPT &&
		le32_to_cpu(part->start_sect) == 1UL) {
		return 1;
	}

	return 0;
}

/*
 * is_pmbr_valid(): test Protective MBR for validity
 *
 * Returns: 1 if PMBR is valid, 0 otherwise.
 * Validity depends on two things:
 *  1) MSDOS signature is in the last two bytes of the MBR
 *  2) One partition of type 0xEE is found, checked by pmbr_part_valid()
 */
static int is_pmbr_valid(legacy_mbr * mbr)
{
	int i = 0;

	if (!mbr || le16_to_cpu(mbr->signature) != MSDOS_MBR_SIGNATURE) {
		return 0;
	}

	for (i = 0; i < 4; i++) {
		if (pmbr_part_valid(&mbr->partition_record[i])) {
			return 1;
		}
	}
	return 0;
}

/**
 * is_gpt_valid() - tests one GPT header and PTEs for validity
 *
 * lba is the logical block address of the GPT header to test
 * gpt is a GPT header ptr, filled on return.
 * ptes is a PTEs ptr, filled on return.
 *
 * Description: returns 1 if valid,  0 on error.
 * If valid, returns pointers to PTEs.
 */
static int is_gpt_valid(block_dev_desc_t * dev_desc, unsigned long long lba,
			gpt_header * pgpt_head, gpt_entry ** pgpt_pte)
{
	u32 crc32_backup = 0;
	unsigned long calc_crc32;
	unsigned long long lastlba;

	if (!dev_desc || !pgpt_head) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return 0;
	}

	/* Read GPT Header from device */
	if (dev_desc->block_read(dev_desc->dev, lba, 1, pgpt_head) != 1) {
		printf("*** ERROR: Can't read GPT header ***\n");
		return 0;
	}

	/* Check the GPT header signature */
	if (le64_to_cpu(pgpt_head->signature) != GPT_HEADER_SIGNATURE) {
		printf("GUID Partition Table Header signature is wrong:"
			"0x%llX != 0x%llX\n",
			(unsigned long long)le64_to_cpu(pgpt_head->signature),
			(unsigned long long)GPT_HEADER_SIGNATURE);
		return 0;
	}

	/* Check the GUID Partition Table CRC */
	crc32_backup = le32_to_cpu(pgpt_head->header_crc32);
	pgpt_head->header_crc32 = 0;

	calc_crc32 = efi_crc32((const unsigned char *)pgpt_head,
		le32_to_cpu(pgpt_head->header_size));

	pgpt_head->header_crc32 = cpu_to_le32(crc32_backup);

	if (calc_crc32 != le32_to_cpu(crc32_backup)) {
		printf("GUID Partition Table Header CRC is wrong:"
			"0x%08lX != 0x%08lX\n",
			le32_to_cpu(crc32_backup), calc_crc32);
		return 0;
	}

	/* Check that the my_lba entry points to the LBA that contains the GPT */
	if (le64_to_cpu(pgpt_head->my_lba) != lba) {
		printf("GPT: my_lba incorrect: %llX != %llX\n",
			(unsigned long long)le64_to_cpu(pgpt_head->my_lba),
			(unsigned long long)lba);
		return 0;
	}

	/* Check the first_usable_lba and last_usable_lba are within the disk. */
	lastlba = (unsigned long long)dev_desc->lba;
	if (le64_to_cpu(pgpt_head->first_usable_lba) > lastlba) {
		printf("GPT: first_usable_lba incorrect: %llX > %llX\n",
			le64_to_cpu(pgpt_head->first_usable_lba), lastlba);
		return 0;
	}
	if (le64_to_cpu(pgpt_head->last_usable_lba) > lastlba) {
		printf("GPT: last_usable_lba incorrect: %llX > %llX\n",
			le64_to_cpu(pgpt_head->last_usable_lba), lastlba);
		return 0;
	}

	debug("GPT: first_usable_lba: %llX last_usable_lba %llX last lba %llX\n",
		le64_to_cpu(pgpt_head->first_usable_lba),
		le64_to_cpu(pgpt_head->last_usable_lba), lastlba);

	/* Read and allocate Partition Table Entries */
	*pgpt_pte = alloc_read_gpt_entries(dev_desc, pgpt_head);
	if (*pgpt_pte == NULL) {
		printf("GPT: Failed to allocate memory for PTE\n");
		return 0;
	}

	/* Check the GUID Partition Table Entry Array CRC */
	calc_crc32 = efi_crc32((const unsigned char *)*pgpt_pte,
		le32_to_cpu(pgpt_head->num_partition_entries) *
		le32_to_cpu(pgpt_head->sizeof_partition_entry));

	if (calc_crc32 != le32_to_cpu(pgpt_head->partition_entry_array_crc32)) {
		printf("GUID Partition Table Entry Array CRC is wrong:"
			"0x%08lX != 0x%08lX\n",
			le32_to_cpu(pgpt_head->partition_entry_array_crc32),
			calc_crc32);

		free(*pgpt_pte);
		return 0;
	}

	/* We're done, all's well */
	return 1;
}

/**
 * alloc_read_gpt_entries(): reads partition entries from disk
 * @dev_desc
 * @gpt - GPT header
 *
 * Description: Returns ptes on success,  NULL on error.
 * Allocates space for PTEs based on information found in @gpt.
 * Notes: remember to free pte when you're done!
 */
static gpt_entry *alloc_read_gpt_entries(block_dev_desc_t * dev_desc,
					 gpt_header * pgpt_head)
{
	size_t count = 0;
	gpt_entry *pte = NULL;

	if (!dev_desc || !pgpt_head) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return NULL;
	}

	count = le32_to_cpu(pgpt_head->num_partition_entries) *
		le32_to_cpu(pgpt_head->sizeof_partition_entry);

	debug("%s: count = %lu * %lu = %u\n", __func__,
		le32_to_cpu(pgpt_head->num_partition_entries),
		le32_to_cpu(pgpt_head->sizeof_partition_entry), count);

	/* Allocate memory for PTE, remember to FREE */
	if (count != 0) {
		pte = memalign(ARCH_DMA_MINALIGN, count);
	}

	if (count == 0 || pte == NULL) {
		printf("%s: ERROR: Can't allocate 0x%X bytes for GPT Entries\n",
			__func__, count);
		return NULL;
	}

	/* Read GPT Entries from device */
	if (dev_desc->block_read (dev_desc->dev,
		(unsigned long)le64_to_cpu(pgpt_head->partition_entry_lba),
		(lbaint_t) (count / GPT_BLOCK_SIZE), pte)
		!= (count / GPT_BLOCK_SIZE)) {

		printf("*** ERROR: Can't read GPT Entries ***\n");
		free(pte);
		return NULL;
	}
	return pte;
}

static unsigned char DISK_GUID[16] = {
	'e', 'M', 'M', 'C', ' ', 'D', 'I', 'S',
	'K', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
};

/**
 * write_gpt_table(): Writes the GPT for the given block device
 * @block_dev_desc_t - Pointer to a block device which will be used to generate
 * 		the GPT, and the GPT will be written to this block device
 * @gpt_entry - Pointer to an array of Partition Table Entry(PTE)s.
 * 		If the number of partition is less than GPT_ENTRY_NUMBERS
 * 		make sure the unused partition are zero filled.
 *
 * Description: The calling method needs to make sure the gpt_entry is filled
 * 		and if necessary, the calling method must free the gpt_entry.
 */
void write_gpt_table(block_dev_desc_t *dev_desc, gpt_entry *p_gpt_entry)
{
	u32 tmp_val, crc32_tmp;
	u64 tmp_64_val;

	gpt_header p_gpt_head, s_gpt_head;
	gpt_entry s_gpt_entry[GPT_ENTRY_NUMBERS];
	legacy_mbr protective_mbr;

	memset(&protective_mbr, 0, sizeof(legacy_mbr));
	memset(&p_gpt_head, 0, sizeof(gpt_header));
	memset(&s_gpt_head, 0, sizeof(gpt_header));
	memset(s_gpt_entry, 0, GPT_ENTRY_NUMBERS * sizeof(gpt_entry));

	/* Write to LBA0 the Protective MBR aka Legacy MBR */
	protective_mbr.signature = MSDOS_MBR_SIGNATURE;
	protective_mbr.partition_record[0].sys_ind = EFI_PMBR_OSTYPE_EFI_GPT;
	tmp_val = 1UL;
	memcpy(&protective_mbr.partition_record[0].start_sect, &tmp_val, 4);
	tmp_val = (unsigned long) (dev_desc->lba - 1);
	memcpy(&protective_mbr.partition_record[0].nr_sects, &tmp_val, 4);
	dev_desc->block_write(dev_desc->dev, 0, 1, &protective_mbr);

	/* Generate Primary GPT header (LBA1) */
	p_gpt_head.signature = cpu_to_le64(GPT_HEADER_SIGNATURE);
	p_gpt_head.revision = cpu_to_le32(GPT_HEADER_REVISION_V1);
	p_gpt_head.header_size = cpu_to_le32(sizeof(gpt_header));
	p_gpt_head.my_lba = cpu_to_le64(1);
	p_gpt_head.alternate_lba = cpu_to_le64(dev_desc->lba - 1);
	p_gpt_head.first_usable_lba = cpu_to_le64(34);
	p_gpt_head.last_usable_lba = cpu_to_le64(dev_desc->lba - 34);
	p_gpt_head.partition_entry_lba = cpu_to_le64(2);
	p_gpt_head.num_partition_entries = cpu_to_le32(GPT_ENTRY_NUMBERS);
	p_gpt_head.sizeof_partition_entry = cpu_to_le32(GPT_ENTRY_SIZE);
	p_gpt_head.header_crc32 = 0;
	p_gpt_head.partition_entry_array_crc32 = 0;
	memcpy(p_gpt_head.disk_guid.b, DISK_GUID, 16);

	/* Generate CRC for the Primary GPT Header */
	crc32_tmp = efi_crc32((const unsigned char *)p_gpt_entry,
			le32_to_cpu(p_gpt_head.num_partition_entries) *
			le32_to_cpu(p_gpt_head.sizeof_partition_entry));
	p_gpt_head.partition_entry_array_crc32 = cpu_to_le32(crc32_tmp);

	crc32_tmp = efi_crc32((const unsigned char *)&p_gpt_head,
			le32_to_cpu(p_gpt_head.header_size));
	p_gpt_head.header_crc32 = cpu_to_le32(crc32_tmp);

	/* Write the First GPT to the block right after the Legacy MBR */
	dev_desc->block_write(dev_desc->dev, 1, 1, &p_gpt_head);
	dev_desc->block_write(dev_desc->dev, 2, 32, p_gpt_entry);

	/* recalculate the values for the Second GPT */
	memcpy(&s_gpt_head, &p_gpt_head, sizeof(gpt_header));
	s_gpt_head.my_lba = p_gpt_head.alternate_lba;
	s_gpt_head.alternate_lba = p_gpt_head.my_lba;
	tmp_64_val = le64_to_cpu(p_gpt_head.last_usable_lba);
	s_gpt_head.partition_entry_lba = cpu_to_le64(tmp_64_val + 1);
	s_gpt_head.header_crc32 = 0;
	s_gpt_head.partition_entry_array_crc32 = 0;

	/* secondary partition entries */
	memcpy(s_gpt_entry, p_gpt_entry,
			GPT_ENTRY_NUMBERS * sizeof(gpt_entry));

	/* Generate CRC for the Secondary Header */
	crc32_tmp = efi_crc32((const unsigned char *)s_gpt_entry,
			le32_to_cpu(s_gpt_head.num_partition_entries) *
			le32_to_cpu(s_gpt_head.sizeof_partition_entry));
	s_gpt_head.partition_entry_array_crc32 = cpu_to_le32(crc32_tmp);

	crc32_tmp = efi_crc32((const unsigned char *)&s_gpt_head,
			le32_to_cpu(s_gpt_head.header_size));
	s_gpt_head.header_crc32 = cpu_to_le32(crc32_tmp);

	/* Write the Second GPT that is located at the very back of the Disk */
	dev_desc->block_write(dev_desc->dev,
			le64_to_cpu(s_gpt_head.partition_entry_lba),
			32, s_gpt_entry);
	dev_desc->block_write(dev_desc->dev,
			le64_to_cpu(s_gpt_head.partition_entry_lba) + 32,
			1, &s_gpt_head);
	printf("GPT successfully written to block device!\n");
}

/**
 * is_pte_valid(): validates a single Partition Table Entry
 * @gpt_entry - Pointer to a single Partition Table Entry
 *
 * Description: returns 1 if valid,  0 on error.
 */
static int is_pte_valid(gpt_entry * pte)
{
	efi_guid_t unused_guid;

	if (!pte) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return 0;
	}

	/* Only one validation for now:
	 * The GUID Partition Type != Unused Entry (ALL-ZERO)
	 */
	memset(unused_guid.b, 0, sizeof(unused_guid.b));

	if (memcmp(pte->partition_type_guid.b, unused_guid.b,
		sizeof(unused_guid.b)) == 0) {

		debug("%s: Found an unused PTE GUID at 0x%08X\n", __func__,
		(unsigned int)pte);

		return 0;
	} else {
		return 1;
	}
}

int set_gpt_table(block_dev_desc_t *dev_desc, unsigned int part_start_lba,
		int parts, unsigned int *blocks, u16 **block_labels,
		unsigned int *part_offset)
{
	int i;
	unsigned int offset = part_start_lba;
	gpt_entry p_gpt_entry[GPT_ENTRY_NUMBERS];

	memset(p_gpt_entry, 0, GPT_ENTRY_NUMBERS * sizeof(gpt_entry));

	/* setup partition entries from block_labels information */
	for (i = 0; i < parts; i++) {
		memcpy(p_gpt_entry[i].partition_type_guid.b,
			&PARTITION_BASIC_DATA_GUID, 16);
		memcpy(p_gpt_entry[i].unique_partition_guid.b,
			block_labels[i],
			(strlen_efi16(block_labels[i]) + 1) * 2);
		memcpy(p_gpt_entry[i].partition_name,
			block_labels[i],
			(strlen_efi16(block_labels[i]) + 1) * 2);

		p_gpt_entry[i].starting_lba = cpu_to_le32(offset);
		/* allocate remaining memory in last partition */
		if (i != parts - 1)
			p_gpt_entry[i].ending_lba = cpu_to_le32(offset + blocks[i] - 1);
		else
			p_gpt_entry[i].ending_lba = cpu_to_le32(dev_desc->lba - 33);

		memset(&p_gpt_entry[i].attributes, 0, sizeof(gpt_entry_attributes));
		part_offset[i] = offset;
		offset += blocks[i];
	}

	/* pass the partition information from PIT and write gpt table */
	write_gpt_table(dev_desc, p_gpt_entry);
	return 0;
}

unsigned int gpt_offset[16];
unsigned int gpt_parts;

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

int get_gpt_table(block_dev_desc_t * dev_desc, unsigned int *part_offset)
{
	gpt_header gpt_head;
	gpt_entry *pgpt_pte = NULL;
	int ret, i;
	int parts = 0;

	ret = dev_desc->block_read(dev_desc->dev, 1, 1, &gpt_head);
	if (ret != 1) {
		printf("%s[%d] mmc_read_blocks %d\n", __func__, __LINE__, ret);
		return 0;
	}

	pgpt_pte = alloc_read_gpt_entries(dev_desc, &gpt_head);
	if (pgpt_pte == NULL) {
		printf("GPT: Failed to allocate memory for PTE\n");
		free(pgpt_pte);
		return 0;
	}

	for (i = 0; i < le32_to_cpu(gpt_head.num_partition_entries); i++) {
		if (is_pte_valid(&pgpt_pte[i])) {
			part_offset[parts] = le64_to_cpu(pgpt_pte[i].starting_lba);
			parts++;
			printf("%s%d (%s)\t   0x%llX\t0x%llX\n", GPT_ENTRY_NAME,
				(i + 1), (char *)pgpt_pte[i].partition_name,
				le64_to_cpu(pgpt_pte[i].starting_lba),
				le64_to_cpu(pgpt_pte[i].ending_lba));
		} else {
			break;
		}
	}
	free(pgpt_pte);

	return parts;
}

static int strlen_efi16(efi_char16_t * str)
{
	int count = 0;
	while (*str != '\0') {
		count++;
		str++;
	}
	return count;
}

static efi_char16_t *copy_label_to_efi16(char *p)
{
	/* the p string will be in (label) format */
	efi_char16_t *label;
	int len, i = 0;
	p++; /* skip the '(' */
	if (*p == '(')
		p++; /* in case we're at the last -(ums) */
	len = strlen(p); /* the last ')' will be changed to '\0' */

	label = (efi_char16_t *)malloc(sizeof(efi_char16_t) * len);
	while (p[i] != ')') {
		label[i] = p[i];
		i++;
	}
	label[i] = '\0';

	return label;
}



void set_gpt_info(block_dev_desc_t * dev_desc, char *ramaddr,
		unsigned int len, unsigned int reserved)
{
	char gpt_str[256];
	char save[16][16];
	char *p;
	char *tok;
	unsigned int size[16];
	int i = 0;
	efi_char16_t **labels;

	strncpy(gpt_str, ramaddr, len);
	p = gpt_str;

	for (i = 0; ; i++, p = NULL) {
		tok = strtok(p, ",");
		if (tok == NULL)
			break;
		strcpy(save[i], tok);
		printf("part%d: %s\n", i, save[i]);
	}

	gpt_parts = i;
	printf("found %d partitions\n", gpt_parts);

	labels = (efi_char16_t **)malloc(sizeof(efi_char16_t *) * gpt_parts);

	for (i = 0; i < gpt_parts; i++) {
		p = save[i];
		size[i] = memsize_to_blocks(p, (const char **)&p);
		labels[i] = copy_label_to_efi16(p);
	}

	puts("saving the GPT Table...\n");
	set_gpt_table(dev_desc, reserved, gpt_parts, size, labels, gpt_offset);

	for (i = 0; i < gpt_parts; i++)
		free(labels[i]);
	free(labels);
}

static int gpt_dev;
#define MBR_START_OFS_BLK	(0x500000 / 512)

void set_gpt_dev(int dev)
{
	gpt_dev = dev;
}

static void gpt_show(void)
{
	struct mmc *mmc = find_mmc_device(gpt_dev);

	print_part_efi(&mmc->block_dev);
}

static void gpt_default(void)
{
	char *mbrparts;
	struct mmc *mmc = find_mmc_device(gpt_dev);

	puts("using default GPT\n");

	mbrparts = getenv("mbrparts");
	set_gpt_info(&mmc->block_dev, mbrparts, strlen(mbrparts), MBR_START_OFS_BLK);
}

static int do_gpt(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	switch (argc) {
	case 2:
		if (strncmp(argv[1], "show", 2) == 0)
			gpt_show();
		else if (strncmp(argv[1], "default", 3) == 0)
			gpt_default();
		break;
	case 3:
		if (strncmp(argv[1], "dev", 3) == 0)
			set_gpt_dev(simple_strtoul(argv[2], NULL, 10));
		break;
	default:
		cmd_usage(cmdtp);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	gpt,	CONFIG_SYS_MAXARGS,	1, do_gpt,
	"GUID Partition Table",
	"show - show GPT\n"
	"gpt default - reset GPT partition to defaults\n"
	"gpt dev #num - set device number\n"
);
#endif
