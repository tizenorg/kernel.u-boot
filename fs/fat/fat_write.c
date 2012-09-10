/*
 * fat_write.c
 *
 * R/W (V)FAT 12/16/32 filesystem implementation by Donggun Kim
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

#include <common.h>
#include <command.h>
#include <config.h>
#include <fat.h>
#include <asm/byteorder.h>
#include <part.h>
#include "fat.c"

static int fragmented_count;

static __u32 dir_curclust;

static void uppercase (char *str, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		TOUPPER(*str);
		str++;
	}
}

static int total_sector;
static int disk_write (__u32 startblock, __u32 getsize, __u8 * bufptr)
{
	if (cur_dev == NULL)
		return -1;

	if (startblock + getsize > total_sector) {
		printf("error: overflow occurs\n");
		return -1;
	}

	if (getsize == 0) {
		printf("warning: attempted to write zero size\n");
		return 1;
	}

	startblock += part_offset;

	if (cur_dev->block_write) {
		return cur_dev->block_write(cur_dev->dev, startblock, getsize,
					   (unsigned long *) bufptr);
	}
	return -1;
}

/*
 * Set short name in directory entry
 */
static void set_name (dir_entry *dirent, const char *filename)
{
	char s_name[256];
	char *period;
	int period_location, len, i, ext_num;

	if (filename == NULL)
		return;

	len = strlen(filename);
	if (len == 0)
		return;

	memcpy(s_name, filename, len);
	uppercase(s_name, len);

	period = strchr(s_name, '.');
	if (period == NULL) {
		period_location = len;
		ext_num = 0;
	} else {
		period_location = period - s_name;
		ext_num = len - period_location - 1;
	}

	/* Pad spaces when file name length is shortter than eight */
	if (period_location < 8) {
		memcpy(dirent->name, s_name, period_location);
		for (i = period_location; i < 8; i++)
			dirent->name[i] = ' ';
	} else if (period_location == 8) {
		memcpy(dirent->name, s_name, period_location);
	} else {
		memcpy(dirent->name, s_name, 6);
		dirent->name[6] = '~';
		dirent->name[7] = '1';
	}

	if (ext_num < 3) {
		memcpy(dirent->ext, s_name + period_location + 1, ext_num);
		for (i = ext_num; i < 3; i++)
			dirent->ext[i] = ' ';
	} else
		memcpy(dirent->ext, s_name + period_location + 1, 3);

	debug("name : %s\n", dirent->name);
	debug("ext : %s\n", dirent->ext);
}

/*
 * Write fat buffer into block device
 */
static int flush_fat_buffer(fsdata *mydata)
{
	int getsize = FATBUFSIZE / FS_BLOCK_SIZE;
	__u32 fatlength = mydata->fatlength;
	__u8 *bufptr = mydata->fatbuf;
	__u32 startblock = mydata->fatbufnum * FATBUFBLOCKS;

	fatlength *= SECTOR_SIZE;
	startblock += mydata->fat_sect;

	if (getsize > fatlength)
		getsize = fatlength;

	/* Write FAT buf */
	if (disk_write(startblock, getsize, bufptr) < 0) {
		debug("error: writing FAT blocks\n");
		return -1;
	}

	return 0;
}

/*
 * Get the entry at index 'entry' in a FAT (12/16/32) table.
 * On failure 0x00 is returned.
 * When bufnum is changed, write back the previous fatbuf to the disk.
 */
static __u32 get_fatent_value (fsdata *mydata, __u32 entry)
{
	__u32 bufnum;
	__u32 off16, offset;
	__u32 ret = 0x00;
	__u16 val1, val2;

	switch (mydata->fatsize) {
	case 32:
		bufnum = entry / FAT32BUFSIZE;
		offset = entry - bufnum * FAT32BUFSIZE;
		break;
	case 16:
		bufnum = entry / FAT16BUFSIZE;
		offset = entry - bufnum * FAT16BUFSIZE;
		break;
	case 12:
		bufnum = entry / FAT12BUFSIZE;
		offset = entry - bufnum * FAT12BUFSIZE;
		break;

	default:
		/* Unsupported FAT size */
		return ret;
	}

	debug("FAT%d: entry: 0x%04x = %d, offset: 0x%04x = %d\n",
	       mydata->fatsize, entry, entry, offset, offset);

	/* Read a new block of FAT entries into the cache. */
	if (bufnum != mydata->fatbufnum) {
		int getsize = FATBUFSIZE / FS_BLOCK_SIZE;
		__u8 *bufptr = mydata->fatbuf;
		__u32 fatlength = mydata->fatlength;
		__u32 startblock = bufnum * FATBUFBLOCKS;

		fatlength *= SECTOR_SIZE;	/* We want it in bytes now */
		startblock += mydata->fat_sect;	/* Offset from start of disk */

		if (getsize > fatlength)
			getsize = fatlength;
		/* Write back the fatbuf to the disk */
		if (mydata->fatbufnum != -1) {
			if (flush_fat_buffer(mydata) < 0)
				return -1;
		}

		if (disk_read(startblock, getsize, bufptr) < 0) {
			debug("Error reading FAT blocks\n");
			return ret;
		}
		mydata->fatbufnum = bufnum;
	}

	/* Get the actual entry from the table */
	switch (mydata->fatsize) {
	case 32:
		ret = FAT2CPU32(((__u32 *) mydata->fatbuf)[offset]);
		break;
	case 16:
		ret = FAT2CPU16(((__u16 *) mydata->fatbuf)[offset]);
		break;
	case 12:
		off16 = (offset * 3) / 4;

		switch (offset & 0x3) {
		case 0:
			ret = FAT2CPU16(((__u16 *) mydata->fatbuf)[off16]);
			ret &= 0xfff;
			break;
		case 1:
			val1 = FAT2CPU16(((__u16 *)mydata->fatbuf)[off16]);
			val1 &= 0xf000;
			val2 = FAT2CPU16(((__u16 *)mydata->fatbuf)[off16 + 1]);
			val2 &= 0x00ff;
			ret = (val2 << 4) | (val1 >> 12);
			break;
		case 2:
			val1 = FAT2CPU16(((__u16 *)mydata->fatbuf)[off16]);
			val1 &= 0xff00;
			val2 = FAT2CPU16(((__u16 *)mydata->fatbuf)[off16 + 1]);
			val2 &= 0x000f;
			ret = (val2 << 8) | (val1 >> 8);
			break;
		case 3:
			ret = FAT2CPU16(((__u16 *)mydata->fatbuf)[off16]);
			ret = (ret & 0xfff0) >> 4;
			break;
		default:
			break;
		}
		break;
	}
	debug("FAT%d: ret: %08x, entry: %08x, offset: %04x\n",
	       mydata->fatsize, ret, entry, offset);

	return ret;
}

#ifdef CONFIG_SUPPORT_VFAT
/*
 * Set the file name information from 'name' into 'slot',
 */
static int str2slot (dir_slot *slotptr, const char *name, int *idx)
{
	int j, end_idx = 0;

	for (j = 0; j <= 8; j += 2) {
		if (name[*idx] == 0x00) {
			slotptr->name0_4[j] = 0;
			slotptr->name0_4[j + 1] = 0;
			end_idx++;
			goto name0_4;
		}
		slotptr->name0_4[j] = name[*idx];
		(*idx)++;
		end_idx++;
	}
	for (j = 0; j <= 10; j += 2) {
		if (name[*idx] == 0x00) {
			slotptr->name5_10[j] = 0;
			slotptr->name5_10[j + 1] = 0;
			end_idx++;
			goto name5_10;
		}
		slotptr->name5_10[j] = name[*idx];
		(*idx)++;
		end_idx++;
	}
	for (j = 0; j <= 2; j += 2) {
		if (name[*idx] == 0x00) {
			slotptr->name11_12[j] = 0;
			slotptr->name11_12[j + 1] = 0;
			end_idx++;
			goto name11_12;
		}
		slotptr->name11_12[j] = name[*idx];
		(*idx)++;
		end_idx++;
	}

	if (name[*idx] == 0x00)
		return 1;

	return 0;
/* Not used characters are filled with 0xff 0xff */
name0_4:
	for (; end_idx < 5; end_idx++) {
		slotptr->name0_4[end_idx * 2] = 0xff;
		slotptr->name0_4[end_idx * 2 + 1] = 0xff;
	}
	end_idx = 5;
name5_10:
	end_idx -= 5;
	for (; end_idx < 6; end_idx++) {
		slotptr->name5_10[end_idx * 2] = 0xff;
		slotptr->name5_10[end_idx * 2 + 1] = 0xff;
	}
	end_idx = 11;
name11_12:
	end_idx -= 11;
	for (; end_idx < 2; end_idx++) {
		slotptr->name11_12[end_idx * 2] = 0xff;
		slotptr->name11_12[end_idx * 2 + 1] = 0xff;
	}

	return 1;
}

static int is_next_clust (fsdata *mydata, dir_entry *dentptr);
static void flush_dir_table (fsdata *mydata, dir_entry **dentptr);

/*
 * Fill dir_slot entries with appropriate name, id, and attr
 * The real directory entry is returned by dentptr
 */
static void
fill_dir_slot (fsdata *mydata, dir_entry **dentptr, const char *l_name)
{
	dir_slot *slotptr = (dir_slot *)get_vfatname_block;
	__u8 counter=0, checksum;
	int idx = 0, ret;
	char s_name[16];

	/* Get short file name and checksum value*/
	strncpy(s_name, (*dentptr)->name, 16);
	checksum = mkcksum(s_name);

	do {
		memset(slotptr, 0x00, sizeof(dir_slot));
		ret = str2slot(slotptr, l_name, &idx);
		slotptr->id = ++counter;
		slotptr->attr = ATTR_VFAT;
		slotptr->alias_checksum = checksum;
		slotptr++;
	} while (ret == 0);

	slotptr--;
	slotptr->id |= LAST_LONG_ENTRY_MASK;

	while (counter >= 1) {
		if (is_next_clust(mydata, *dentptr)) {
			/* A new cluster is allocated for directory table */
			flush_dir_table (mydata, dentptr);
		}
		memcpy(*dentptr, slotptr, sizeof(dir_slot));
		(*dentptr)++;
		slotptr--;
		counter--;
	}

	if (is_next_clust(mydata, *dentptr)) {
		/* A new cluster is allocated for directory table */
		flush_dir_table (mydata, dentptr);
	}
}

static int
get_long_file_name (fsdata *mydata, int curclust, __u8 *cluster,
	      dir_entry **retdent, char *l_name)
{
	dir_entry *realdent;
	dir_slot *slotptr = (dir_slot *)(*retdent);
	dir_slot *slotptr2 = NULL;
	__u8 *nextclust = cluster + mydata->clust_size * SECTOR_SIZE;
	__u8 counter = (slotptr->id & ~LAST_LONG_ENTRY_MASK) & 0xff;
	int idx = 0, cur_position = 0;

	while ((__u8 *)slotptr < nextclust) {
		if (counter == 0)
			break;
		if (((slotptr->id & ~LAST_LONG_ENTRY_MASK) & 0xff) != counter)
			return -1;
		slotptr++;
		counter--;
	}

	if ((__u8 *)slotptr >= nextclust) {
		curclust = get_fatent_value(mydata, dir_curclust);
		if (CHECK_CLUST(curclust, mydata->fatsize)) {
			debug("curclust: 0x%x\n", curclust);
			printf("Invalid FAT entry\n");
			return -1;
		}

		dir_curclust = curclust;

		if (get_cluster(mydata, curclust, get_vfatname_block,
				mydata->clust_size * SECTOR_SIZE) != 0) {
			debug("Error: reading directory block\n");
			return -1;
		}

		slotptr2 = (dir_slot *)get_vfatname_block;
		while (counter > 0) {
			slotptr2++;
			counter--;
		}

		/* Save the real directory entry */
		realdent = (dir_entry *)slotptr2;
		while ((__u8 *)slotptr2 > get_vfatname_block) {
			slotptr2--;
			slot2str(slotptr2, l_name, &idx);
		}
	} else {
		/* Save the real directory entry */
		realdent = (dir_entry *)slotptr;
	}

	do {
		slotptr--;
		if (slot2str(slotptr, l_name, &idx))
			break;
	} while (!(slotptr->id & LAST_LONG_ENTRY_MASK));

	l_name[idx] = '\0';
	if (*l_name == DELETED_FLAG)
		*l_name = '\0';
	else if (*l_name == aRING)
		*l_name = DELETED_FLAG;
	downcase(l_name);

	/* Return the real directory entry */
	*retdent = realdent;

	if (slotptr2) {
		memcpy(get_dentfromdir_block, get_vfatname_block,
			mydata->clust_size * SECTOR_SIZE);
		cur_position = (__u8 *)realdent - get_vfatname_block;
		*retdent = (dir_entry *) &get_dentfromdir_block[cur_position];
	}

	return 0;
}

#endif
/*
 * Set the entry at index 'entry' in a FAT (16/32) table.
 */
static int set_fatent_value (fsdata *mydata, __u32 entry, __u32 entry_value)
{
	__u32 bufnum, offset;

	switch (mydata->fatsize) {
	case 32:
		bufnum = entry / FAT32BUFSIZE;
		offset = entry - bufnum * FAT32BUFSIZE;
		break;
	case 16:
		bufnum = entry / FAT16BUFSIZE;
		offset = entry - bufnum * FAT16BUFSIZE;
		break;
	default:
		/* Unsupported FAT size */
		return -1;
	}

	/* Read a new block of FAT entries into the cache. */
	if (bufnum != mydata->fatbufnum) {
		int getsize = FATBUFSIZE / FS_BLOCK_SIZE;
		__u8 *bufptr = mydata->fatbuf;
		__u32 fatlength = mydata->fatlength;
		__u32 startblock = bufnum * FATBUFBLOCKS;

		fatlength *= SECTOR_SIZE;
		startblock += mydata->fat_sect;

		if (getsize > fatlength)
			getsize = fatlength;

		if (mydata->fatbufnum != -1) {
			if (flush_fat_buffer(mydata) < 0)
				return -1;
		}

		if (disk_read(startblock, getsize, bufptr) < 0) {
			debug("Error reading FAT blocks\n");
			return -1;
		}
		mydata->fatbufnum = bufnum;
	}

	/* Set the actual entry */
	switch (mydata->fatsize) {
	case 32:
		((__u32 *) mydata->fatbuf)[offset] = cpu_to_le32(entry_value);
		break;
	case 16:
		((__u16 *) mydata->fatbuf)[offset] = cpu_to_le16(entry_value);
		break;
	default:
		return -1;
	}

	return 0;
}

/*
 * Determine the entry value at index 'entry' in a FAT (16/32) table
 */
static __u32 determine_fatent (fsdata *mydata, __u32 entry)
{
	__u32 next_fat, next_entry = entry + 1;

	while (1) {
		next_fat = get_fatent_value(mydata, next_entry);
		if (next_fat == 0) {
			set_fatent_value(mydata, entry, next_entry);
			break;
		}
		next_entry++;
	}
	debug("FAT%d: entry: %08x, entry_value: %04x\n",
	       mydata->fatsize, entry, next_entry);

	return next_entry;
}

/*
 * Write at most 'size' bytes from 'buffer' into the specified cluster.
 * Return 0 on success, -1 otherwise.
 */
static int
set_cluster (fsdata *mydata, __u32 clustnum, __u8 *buffer,
	     unsigned long size)
{
	int idx = 0;
	__u32 startsect;

	if (clustnum > 0) {
		startsect = mydata->data_begin +
				clustnum * mydata->clust_size;
	} else {
		startsect = mydata->rootdir_sect;
	}

	debug("gc - clustnum: %d, startsect: %d\n", clustnum, startsect);

	if (disk_write(startsect, size / FS_BLOCK_SIZE, buffer) < 0) {
		debug("Error writing data\n");
		return -1;
	}

	if (size % FS_BLOCK_SIZE) {
		__u8 tmpbuf[FS_BLOCK_SIZE];

		idx = size / FS_BLOCK_SIZE;
		buffer += idx * FS_BLOCK_SIZE;
		memcpy(tmpbuf, buffer, size % FS_BLOCK_SIZE);

		if (disk_write(startsect + idx, 1, tmpbuf) < 0) {
			debug("Error writing data\n");
			return -1;
		}

		return 0;
	}

	return 0;
}

/*
 * Find the first empty cluster starting from cluster #2
 */
static int find_empty_cluster(fsdata *mydata)
{
	__u32 fat_val, entry = 3;

	while (1) {
		fat_val = get_fatent_value(mydata, entry);
		if (fat_val == 0)
			break;
		entry++;
	}

	return entry;
}

static void flush_dir_table (fsdata *mydata, dir_entry **dentptr)
{
	int dir_newclust = 0;

	if (set_cluster(mydata, dir_curclust,
		    get_dentfromdir_block,
		    mydata->clust_size * SECTOR_SIZE) != 0) {
		printf("error: wrinting directory entry\n");
		return;
	}
	dir_newclust = find_empty_cluster(mydata);
	set_fatent_value(mydata, dir_curclust, dir_newclust);
	if (mydata->fatsize == 32)
		set_fatent_value(mydata, dir_newclust, 0xffffff8);
	else if (mydata->fatsize == 16)
		set_fatent_value(mydata, dir_newclust, 0xfff8);

	dir_curclust = dir_newclust;

	if (flush_fat_buffer(mydata) < 0)
		return;

	memset(get_dentfromdir_block, 0x00,
		mydata->clust_size * SECTOR_SIZE);

	*dentptr = (dir_entry *) get_dentfromdir_block;
}

/*
 * Set empty cluster from 'entry' to the end of a file
 */
static int clear_fatent(fsdata *mydata, __u32 entry)
{
	__u32 fat_val;

	while (1) {
		fat_val = get_fatent_value(mydata, entry);
		if (fat_val != 0)
			set_fatent_value(mydata, entry, 0);
		else
			break;

		if (fat_val == 0xfffffff || fat_val == 0xffff)
			break;

		entry = fat_val;
	}

	/* Flush fat buffer */
	if (flush_fat_buffer(mydata) < 0)
		return -1;

	return 0;
}

/*
 * Write at most 'maxsize' bytes from 'buffer' into
 * the file associated with 'dentptr'
 * Return the number of bytes read or -1 on fatal errors.
 */
static int
set_contents (fsdata *mydata, dir_entry *dentptr, __u8 *buffer,
	      unsigned long maxsize)
{
	unsigned long filesize = FAT2CPU32(dentptr->size), gotsize = 0;
	unsigned int bytesperclust = mydata->clust_size * SECTOR_SIZE;
	__u32 curclust = START(dentptr);
	__u32 endclust = 0, newclust = 0;
	unsigned long actsize;

	debug("Filesize: %ld bytes\n", filesize);

	if (maxsize > 0 && filesize > maxsize)
		filesize = maxsize;

	debug("%ld bytes\n", filesize);

	actsize = bytesperclust;
	endclust = curclust;
	do {
		/* search for consecutive clusters */
		while (actsize < filesize) {
			newclust = determine_fatent(mydata, endclust);

			if ((newclust - 1) != endclust)
				goto getit;

			if (CHECK_CLUST(newclust, mydata->fatsize)) {
				debug("curclust: 0x%x\n", newclust);
				debug("Invalid FAT entry\n");
				return gotsize;
			}
			endclust = newclust;
			actsize += bytesperclust;
		}
		/* actsize >= file size */
		actsize -= bytesperclust;
		/* set remaining clusters */
		if (set_cluster(mydata, curclust, buffer, (int)actsize) != 0) {
			debug("error: writing cluster\n");
			return -1;
		}

		/* set remaining bytes */
		gotsize += (int)actsize;
		filesize -= actsize;
		buffer += actsize;
		actsize = filesize;

		if (set_cluster(mydata, endclust, buffer, (int)actsize) != 0) {
			debug("error: writing cluster\n");
			return -1;
		}
		gotsize += actsize;

		/* Mark end of file in FAT */
		if (mydata->fatsize == 16)
			newclust = 0xffff;
		else if (mydata->fatsize == 32)
			newclust = 0xfffffff;
		set_fatent_value(mydata, endclust, newclust);

		return gotsize;
getit:
		if (set_cluster(mydata, curclust, buffer, (int)actsize) != 0) {
			debug("error: writing cluster\n");
			return -1;
		}
		gotsize += (int)actsize;
		filesize -= actsize;
		buffer += actsize;

		if (CHECK_CLUST(curclust, mydata->fatsize)) {
			debug("curclust: 0x%x\n", curclust);
			debug("Invalid FAT entry\n");
			return gotsize;
		}
		actsize = bytesperclust;
		curclust = endclust = newclust;
		fragmented_count++;
	} while (1);
}

/*
 * Fill dir_entry entry
 */
static void fill_dentry (fsdata *mydata, dir_entry *dentptr,
	const char *filename, __u32 start_cluster, __u32 size, __u8 attr)
{
	if (mydata->fatsize == 32)
		dentptr->starthi =
			cpu_to_le16((start_cluster & 0xffff0000) >> 16);
	dentptr->start = cpu_to_le16(start_cluster & 0xffff);
	dentptr->size = cpu_to_le32(size);

	dentptr->attr = attr;

	set_name(dentptr, filename);
}

/* 
 * Check whether adding a file makes the file system to
 * exceed the size of the block device
 * Return 1 when overflow occurs, otherwise return 0
 */
static int check_overflow(fsdata *mydata, __u32 clustnum, unsigned long size)
{
	__u32 startsect, sect_num;

	if (clustnum > 0) {
		startsect = mydata->data_begin +
				clustnum * mydata->clust_size;
	} else {
		startsect = mydata->rootdir_sect;
	}

	sect_num = size / FS_BLOCK_SIZE;
	if (size % FS_BLOCK_SIZE)
		sect_num++;

	if (startsect + sect_num > total_sector)
		return 1;

	return 0;
}

static void find_fragmented_area (fsdata *mydata, __u32 *start,
			__u32 *middle, __u32 *end);

static void print_fat_range (fsdata *mydata, dir_entry *dentptr)
{
	char s_name[16];
	__u32 start = 0, middle = 0, end = 0, pre_end = 0;

	start = end = START(dentptr);

	get_name(dentptr, s_name);
	printf("%16s : ", s_name);
	printf("%5d - ", start);

	end = get_fatent_value(mydata, start);
	if ((end >= 0xffffff8) || (end >= 0xfff8)) {
		printf("%5d ", start);
		printf("(size : %d)\n", FAT2CPU32(dentptr->size));
		return;
	}

	while (1) {
		find_fragmented_area(mydata, &start, &middle, &end);
		if (middle == 0) {
			if (start != end)
				printf("%5d ", end);
			break;
		}

		if (pre_end != start)
			printf("%5d ", start);
		printf(", %5d - %5d ", middle, end);

		start = end;
		pre_end = end;
		middle = 0;
	}
	printf("(size : %d)\n", FAT2CPU32(dentptr->size));
}

static dir_entry *empty_dentptr = NULL;

static int is_next_clust (fsdata *mydata, dir_entry *dentptr)
{
	int cur_position;

	cur_position = (__u8 *)dentptr - get_dentfromdir_block;

	if (cur_position >= mydata->clust_size * SECTOR_SIZE)
		return 1;
	else
		return 0;
}

/*
 * Find a directory entry based on filename or start cluster number
 * If the directory entry is not found,
 * the new position for writing a directory entry will be returned
 */
static dir_entry *find_directory_entry (fsdata *mydata, int startsect,
	char *filename, dir_entry *retdent, __u32 start, int find_name,
	int scan_table)
{
	__u16 prevcksum = 0xffff;
	__u32 curclust = (startsect - mydata->data_begin) / mydata->clust_size;

	debug("get_dentfromdir: %s\n", filename);

	if (scan_table)
		printf("%16s : start -  end (cluster #)\n", "file name");

	while (1) {
		dir_entry *dentptr;

		int i;

		if (get_cluster(mydata, curclust, get_dentfromdir_block,
			    mydata->clust_size * SECTOR_SIZE) != 0) {
			printf("Error: reading directory block\n");
			return NULL;
		}

		dentptr = (dir_entry *)get_dentfromdir_block;

		dir_curclust = curclust;

		for (i = 0; i < DIRENTSPERCLUST; i++) {
			char s_name[14], l_name[256];

			l_name[0] = '\0';
			if (dentptr->name[0] == DELETED_FLAG) {
				dentptr++;
				if (is_next_clust(mydata, dentptr))
					break;
				continue;
			}
			if ((dentptr->attr & ATTR_VOLUME)) {
#ifdef CONFIG_SUPPORT_VFAT
				if ((dentptr->attr & ATTR_VFAT) &&
				    (dentptr-> name[0] & LAST_LONG_ENTRY_MASK)) {
					prevcksum = ((dir_slot *)dentptr)->alias_checksum;
					get_long_file_name(mydata, curclust,
						     get_dentfromdir_block,
						     &dentptr, l_name);
					debug("vfatname: |%s|\n", l_name);
				} else
#endif
				{
					/* Volume label or VFAT entry */
					dentptr++;
					if (is_next_clust(mydata, dentptr))
						break;
					continue;
				}
			}
			if (dentptr->name[0] == 0) {
				debug("Dentname == NULL - %d\n", i);
				empty_dentptr = dentptr;
				return NULL;
			}
			if (find_name) {
				get_name(dentptr, s_name);

				if (scan_table)
					print_fat_range(mydata, dentptr);

				if (strcmp(filename, s_name)
				    && strcmp(filename, l_name)) {
					debug("Mismatch: |%s|%s|\n", s_name, l_name);
					dentptr++;
					if (is_next_clust(mydata, dentptr))
						break;
					continue;
				}

				memcpy(retdent, dentptr, sizeof(dir_entry));

				debug("DentName: %s", s_name);
				debug(", start: 0x%x", START(dentptr));
				debug(", size:  0x%x %s\n",
				      FAT2CPU32(dentptr->size),
				      (dentptr->attr & ATTR_DIR) ? "(DIR)" : "");

				return dentptr;
			} else {
				if (start == START(dentptr))
					return dentptr;
				else {
					dentptr++;
					if (is_next_clust(mydata, dentptr))
						break;
				}
				continue;
			}
		}

		curclust = get_fatent_value(mydata, dir_curclust);
		if ((curclust >= 0xffffff8) || (curclust >= 0xfff8)) {
			empty_dentptr = dentptr;
			return NULL;
		}
		if (CHECK_CLUST(curclust, mydata->fatsize)) {
			debug("curclust: 0x%x\n", curclust);
			debug("Invalid FAT entry\n");
			return NULL;
		}
	}

	return NULL;
}

/*
 * Find a fregmented area for a file
 * An area is seperated by three parameters (start, middle, end)
 * The data for a file is located from middle to end
 * The data for other files is located from start + 1 to middle - 1
 */
static void find_fragmented_area (fsdata *mydata, __u32 *start,
			__u32 *middle, __u32 *end)
{
	__u32 entry = *start;
	__u32 fat_val;
	int fragmented = 0;

	while (1) {
		fat_val = get_fatent_value(mydata, entry);
		if (fat_val == 0xfffffff || fat_val == 0xffff) {
			*end = entry;
			break;
		}

		if ((fat_val - 1) != entry) {
			if (!fragmented) {
				*start = entry;
				*middle = fat_val;
				fragmented = 1;

			} else {
				*end = entry;
				break;
			}
		}

		entry = fat_val;
	}

	debug("start : %d, middle : %d, end : %d\n", *start, *middle, *end);
}

static void *memory_for_swap;

/*
 * Change location between the data for other files and the data for target file
 * Fetch the whole data of the fragmented area from a block device to memory
 * Write the target file data into the first part of the area
 * and write the data for other files into the second part of the area 
 */
static void swap_file_contents (fsdata *mydata,
		__u32 start, __u32 middle, __u32 end)
{
	int first_area_size, second_area_size, total_size;
	__u32 clust_num_first, clust_num_second;

	first_area_size = (middle - 1 - start) *
			mydata->clust_size * FS_BLOCK_SIZE;
	second_area_size = (end + 1 - middle) *
			mydata->clust_size * FS_BLOCK_SIZE;
	total_size = first_area_size + second_area_size;

	clust_num_first = start + 1;
	clust_num_second = clust_num_first + end + 1 - middle;

	memset(memory_for_swap, 0x0, 0x2800000); /* 40 MB */
	get_cluster(mydata, start + 1, memory_for_swap, total_size);

	set_cluster(mydata, clust_num_second,
		memory_for_swap, first_area_size);

	memory_for_swap += first_area_size;
	set_cluster(mydata, clust_num_first,
		memory_for_swap, second_area_size);

}

/*
 * Get the fat entry based on the fat entry value
 */
static __u32 get_reverse_fatent (fsdata *mydata, __u32 entry_value)
{
	__u32 entry = entry_value - 1;
	__u32 fat_val = get_fatent_value(mydata, entry);

	if (entry_value == 0)
		return 0;

	if (fat_val == entry_value) {
		debug("entry : %d, entry_value : %d\n", entry, entry_value);
		return entry;
	}

	entry = 3;
	while (1) {
		if (entry > entry_value) {
			entry = 1;
			break;
		}
		fat_val = get_fatent_value(mydata, entry);
		if (entry_value == fat_val)
			break;
		entry++;
	}

	debug("entry : %d, entry_value : %d\n", entry, entry_value);
	return entry;
}

/*
 * Swap the fat entry value for a target file
 * with the fat entry value for the other files
 */
static __u32 swap_fat_values (fsdata *mydata,
		__u32 start, __u32 middle, __u32 end)
{
	__u32 start_second = start + 1 + (end + 1 - middle);
	__u32 start_first = middle - 1;
	__u32 num_fatent_first = middle - (start + 1);
	__u32 num_fatent_second = (end + 1) - middle;
	__u32 reverse_fatent, new_fat_val;
	__u32 end_second_fatval = get_fatent_value(mydata, end);
	__u32 startsect = mydata->rootdir_sect;
	__u32 fat_val;
	dir_entry *dentptr;
	int i;

	for (i = 0; i < num_fatent_first; i++) {
		fat_val = get_fatent_value(mydata, start_first - i);

		if (fat_val == 0)
			continue;

		if (fat_val != start_first - i + 1)
			set_fatent_value(mydata, end - i, fat_val);

		reverse_fatent = get_reverse_fatent(mydata, start_first - i);

		new_fat_val = end - i;

		/* find directory entry to update the start cluster number */
		if (reverse_fatent == 1) {
			dentptr = find_directory_entry(mydata, startsect,
					"", dentptr, start_first - i, 0, 0);
			if (dentptr == NULL) {
				printf("error : no matching dir entry\n");
				return 0;
			}

			if (mydata->fatsize == 32)
				dentptr->starthi =
				    cpu_to_le16(new_fat_val & 0xffff0000);
			dentptr->start = cpu_to_le16(new_fat_val & 0xffff);

			if (set_cluster(mydata, dir_curclust,
				    get_dentfromdir_block,
				    mydata->clust_size * SECTOR_SIZE) != 0) {
				printf("error: wrinting directory entry\n");
				return 0;
			}
		} else {
			if (reverse_fatent == (start_first - i - 1))
				set_fatent_value(mydata,
					new_fat_val - 1, new_fat_val);
			else
				set_fatent_value(mydata,
					reverse_fatent, new_fat_val);
		}
	}

	for (i = 0; i < num_fatent_second; i++)
		set_fatent_value(mydata, start + i, start + i + 1);
	set_fatent_value(mydata, start + i, end_second_fatval);

	flush_fat_buffer(mydata);

	return start_second;
}

/*
 * Remove fragments for a file
 */
static void defragment_file (fsdata *mydata, dir_entry *dentptr)
{
	__u32 start = 0, middle = 0, end = 0;

	start = end = START(dentptr);
	while (1) {
		find_fragmented_area(mydata, &start, &middle, &end);
		if (middle == 0)
			break;
		swap_file_contents(mydata, start, middle, end);
		swap_fat_values(mydata, start, middle, end);
		middle = 0;
	}
}

static int do_fat_write (const char *filename, void *buffer,
	unsigned long size)
{
	dir_entry *dentptr, *retdent;
	dir_slot *slotptr;
	__u32 startsect;
	__u32 start_cluster;
	boot_sector bs;
	volume_info volinfo;
	fsdata datablock;
	fsdata *mydata = &datablock;
	int cursect;
	int root_cluster, ret, name_len;
	char l_filename[256];

	fragmented_count = 0;
	dir_curclust = 0;

	if (read_bootsectandvi(&bs, &volinfo, &mydata->fatsize)) {
		debug("error: reading boot sector\n");
		return 1;
	}

	total_sector = bs.total_sect;
	if (total_sector == 0) {
		total_sector = part_size;
	}

	root_cluster = bs.root_cluster;

	if (mydata->fatsize == 32)
		mydata->fatlength = bs.fat32_length;
	else
		mydata->fatlength = bs.fat_length;

	mydata->fat_sect = bs.reserved;

	cursect = mydata->rootdir_sect
		= mydata->fat_sect + mydata->fatlength * bs.fats;

	mydata->clust_size = bs.cluster_size;

	if (mydata->fatsize == 32) {
		mydata->data_begin = mydata->rootdir_sect -
					(mydata->clust_size * 2);
	} else {
		int rootdir_size;

		rootdir_size = ((bs.dir_entries[1]  * (int)256 +
				 bs.dir_entries[0]) *
				 sizeof(dir_entry)) /
				 SECTOR_SIZE;
		mydata->data_begin = mydata->rootdir_sect +
					rootdir_size -
					(mydata->clust_size * 2);
	}

	mydata->fatbufnum = -1;
	startsect = mydata->rootdir_sect;

	if (disk_read(cursect, mydata->clust_size, do_fat_read_block) < 0) {
		printf("error: reading rootdir block\n");
		return -1;
	}
	dentptr = (dir_entry *) do_fat_read_block;

	memory_for_swap = buffer + size;

	name_len = strlen(filename);
	memcpy(l_filename, filename, name_len);
	downcase(l_filename);

	retdent = find_directory_entry(mydata, startsect,
				l_filename, dentptr, 0, 1, 0);
	if (retdent) {
		/* Update file size and start_cluster in a directory entry */
		retdent->size = cpu_to_le32(size);
		start_cluster = FAT2CPU16(retdent->start);
		if (mydata->fatsize == 32)
			start_cluster |=
				(FAT2CPU16(retdent->starthi) << 16);

		ret = check_overflow(mydata, start_cluster, size);
		if (ret) {
			printf("error: %ld overflow\n", size);
			return -1;
		}

		ret = clear_fatent(mydata, start_cluster);
		if (ret < 0) {
			printf("error: clearing FAT entries\n");
			return -1;
		}

		ret = set_contents(mydata, retdent, buffer, size);
		if (ret < 0) {
			printf("error: writing contents\n");
			return -1;
		}

		/* Flush fat buffer */
		if (flush_fat_buffer(mydata) < 0)
			return -1;

#if 0
		if (fragmented_count > 0) {
			defragment_file(mydata, retdent);
			retdent->size = cpu_to_le32(size);
		}
#endif

		/* Write directory table to device */
		if (set_cluster(mydata, dir_curclust,
			    get_dentfromdir_block,
			    mydata->clust_size * SECTOR_SIZE) != 0) {
			printf("error: wrinting directory entry\n");
			return -1;
		}
	} else {
		slotptr = (dir_slot *)empty_dentptr;

		/* Set short name to set alias checksum field in dir_slot */
		set_name(empty_dentptr, filename);
		fill_dir_slot(mydata, &empty_dentptr, filename);

		ret = start_cluster = find_empty_cluster(mydata);
		if (ret < 0) {
			printf("error: finding empty cluster\n");
			return -1;
		}

		ret = check_overflow(mydata, start_cluster, size);
		if (ret) {
			printf("error: %ld overflow\n", size);
			return -1;
		}

		/* Set attribute as archieve for regular file */
		fill_dentry(mydata, empty_dentptr, filename,
			start_cluster, size, 0x20);

		ret = set_contents(mydata, empty_dentptr, buffer, size);
		if (ret < 0) {
			printf("error: writing contents\n");
			return -1;
		}

		/* Flush fat buffer */
		if (flush_fat_buffer(mydata) < 0)
			return -1;

		/* Write directory table to device */
		if (set_cluster(mydata, dir_curclust,
			    get_dentfromdir_block,
			    mydata->clust_size * SECTOR_SIZE) != 0) {
			printf("error: writing directory entry\n");
			return -1;
		}

#if 0
		if (fragmented_count > 0) {
			defragment_file(mydata, empty_dentptr);
			empty_dentptr->size = cpu_to_le32(size);

			/* Write directory table to device */
			if (set_cluster(mydata, dir_curclust,
				    get_dentfromdir_block,
				    mydata->clust_size * SECTOR_SIZE) != 0) {
				printf("error: writing directory entry\n");
				return -1;
			}

		}
#endif
	}

	return 0;
}

int file_fat_write (const char *filename, void *buffer, unsigned long maxsize)
{
	printf("writing %s\n", filename);
	return do_fat_write(filename, buffer, maxsize);
}

int do_fat_fswrite (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	long size;
	unsigned long addr;
	unsigned long offset;
	unsigned long count;
	char buf [12];
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 6) {
		printf( "usage: fatwrite <interface> <dev[:part]> "
			"<addr> <filename> <bytes>\n");
		return 1;
	}

	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if (fat_register_device(dev_desc,part)!=0) {
		printf("\n** Unable to use %s %d:%d for fatload **\n",
			argv[1], dev, part);
		return 1;
	}
	addr = simple_strtoul(argv[3], NULL, 16);
	count = simple_strtoul(argv[5], NULL, NULL);

	size = file_fat_write(argv[4], (unsigned char *)addr, count);

	if(size==-1) {
		printf("\n** Unable to write \"%s\" from %s %d:%d **\n",
			argv[4], argv[1], dev, part);
		return 1;
	}

	printf("%ld bytes write\n", size);

	sprintf(buf, "%lX", size);
	setenv("filesize", buf);

	return 0;
}

U_BOOT_CMD(
	fatwrite,	7,	0,	do_fat_fswrite,
	"write binary file into a dos filesystem",
	"<interface> <dev[:part]>  <addr> <filename> <bytes>\n"
	"    - write binary file 'filename' from the address 'addr'\n"
	"      to dos filesystem 'dev' on 'interface'"
);

#if  1
__u8 fsinfo[FS_BLOCK_SIZE];

#define CREATE_DIR 0
#define CREATE_FILE 1

typedef struct _fat_boot_fsinfo {
	__le32   signature1;	/* 0x41615252L */
	__le32   reserved1[120];	/* Nothing as far as I can tell */
	__le32   signature2;	/* 0x61417272L */
	__le32   free_clusters;	/* Free cluster count.  -1 if unknown */
	__le32   next_cluster;	/* Most recently allocated cluster */
	__le32   reserved2[4];
}fat_boot_fsinfo;

#define START(dent)	(FAT2CPU16((dent)->start) \
			+ (fs->fatsize != 32 ? 0 : \
			  (FAT2CPU16((dent)->starthi) << 16)))
char **dir_level;

static void*
xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL && size != 0)
		printf("malloc failed !!!\n");
	return ptr;
}

static void*
xzalloc(size_t size)
{
	void *ptr = xmalloc(size);
	memset(ptr, 0, size);
	return ptr;
}

int
read_FSInfo(unsigned int sect_no)
{
	fat_boot_fsinfo *p_fsnfo;
	if (disk_read (sect_no, 1, fsinfo) < 0) {
		debug("Error: reading block\n");
		return -1;
	}
	p_fsnfo = (fat_boot_fsinfo *)fsinfo;
	return 0;
}

void
update_free_clusters_count(fsdata *fs,struct
				dir_entry* dentry, unsigned long new_size)
{
	fat_boot_fsinfo *p_fsnfo;
	p_fsnfo=(fat_boot_fsinfo *)fsinfo;
	unsigned int existing_clusters = 0;
	unsigned int current_clusters = 0;
	unsigned int cluster_size = 0;

	if(fs == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return;
	}

	cluster_size = fs->clust_size*SECTOR_SIZE;

	if(dentry != NULL) {
		existing_clusters = dentry->size /cluster_size;
		if(dentry->size % cluster_size)
			existing_clusters++;
	}

	current_clusters = new_size/cluster_size;
	if(new_size % cluster_size)
		current_clusters++;

	p_fsnfo->free_clusters = p_fsnfo->free_clusters +
				existing_clusters - current_clusters;

	disk_write(1,1,fsinfo);
	return;
}

int
init_directory_entry_cluster(fsdata *fs,char *buffer,
		unsigned int start_cluster,unsigned int parent_dir_cluster)
{
	dir_entry dot_dirent,dot_dot_dirent;
	short int i=0;

	if(buffer == NULL || fs == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	/*fill the pattern ".          " till 11th byte*/
	for ( i = 0 ; i < 11 ; i++)
			dot_dirent.name[i] = ' ';
	dot_dirent.name[0]='.';

	/*fill the pattern ". .        " till 11th byte*/
	for ( i = 0 ; i < 11 ; i++)
			dot_dot_dirent.name[i] = ' ';
	dot_dot_dirent.name[0]='.';
	dot_dot_dirent.name[1]='.';

	if (fs->fatsize == 32) {
		dot_dirent.starthi =
			cpu_to_le16((start_cluster & 0xffff0000) >> 16);
		dot_dot_dirent.starthi =
			cpu_to_le16((start_cluster & 0xffff0000) >> 16);
	}
	dot_dirent.size = cpu_to_le32(0);
	dot_dirent.attr = 0x10; /*its a Directory*/
	dot_dirent.start = cpu_to_le16(start_cluster & 0xffff);/*its cluster*/

	dot_dot_dirent.size = cpu_to_le32(0);
	dot_dot_dirent.attr = 0x10; /*its a Directory*/

	dot_dot_dirent.start = cpu_to_le16(parent_dir_cluster & 0xffff);

	memcpy(buffer,&dot_dirent,sizeof(struct dir_entry));
	memcpy(buffer+sizeof(struct dir_entry),
				&dot_dot_dirent,sizeof(struct dir_entry));

	return 0; /*success*/
}

int
get_path_depth(char *path)
{
	if(path == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return NULL;
	}

	char *token = strtok(path, "/");
	int count = 0;
	while(token != NULL) {
		token = strtok(NULL,"/");
		count++;
	}
	return count + 1 + 1;
	/*
	*for example  for string /home/temp
	*depth=/(1)+home(1)+temp(1)+1 extra for NULL;
	*so depth returned is 4;
	*/
}


char *
get_duplicate_path(char *path)
{
	if(path == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return NULL;
	}

	char *dup_path;
	dup_path = (char *)xzalloc(strlen(path)+1);
	if(dup_path == NULL){
			return NULL;
	}
	memcpy(dup_path,path,strlen(path));
	return dup_path;
}


/*
*parse_path construct the path table
*Example: for the path "/home/user/"
*arr[0]="/"
*arr[1]="home"
*arr[2]="user"
*arr[3]="NULL"
*/
int
_parse_FAT32_path(char **arr,char *path)
{
	if(path == NULL || arr == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	char *token = strtok(path, "/");
	int i = 0;

	/*add root*/
	arr[i] = xzalloc(strlen("/")+1);
	if(!arr[i])
		return -1;
	memcpy(arr[i++],"/",strlen("/"));

	/*add each path entry after root*/
	while(token != NULL) {
		arr[i] = xzalloc(strlen(token)+1);
		if(!arr[i])
			return -1;
		memcpy(arr[i++],token,strlen(token));
		token = strtok(NULL,"/");
	}
	arr[i] = NULL;
	return 0; /*success*/
}


int
parse_FAT32_path(char *absolute_path)
{
	char *path = NULL;
	int depth,i;

	if(absolute_path == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	if(*absolute_path != '/') { /*first charector*/
		printf("Info: Please supply Absolute path\n");
		return -1;
	}

	/*get depth*/
	path = get_duplicate_path(absolute_path);
	if(path == NULL)  return -1;
	depth = get_path_depth(path);
	free(path);

	/*construct directory level table*/
	path = get_duplicate_path(absolute_path);
	if(path == NULL)  return -1;
	dir_level = (char**)xzalloc((depth)* sizeof(char *));
	if(_parse_FAT32_path(dir_level,path) != 0) {
		free(path);
		return -1;
	}
#if 0
	for(i=0;i<depth;i++) {
		printf("directory entry is %s\n",dir_level[i]);
	}
#endif
	free(path);
	return 0;
}


/*
*populate and return filesystem info i.e., fsdata 
*/
int
init_FAT32fs(fsdata *fs)
{
	volume_info volinfo;
	boot_sector bs;

	if(fs == NULL ) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	memset(&bs,'\0',sizeof(struct boot_sector));
	memset(&volinfo,'\0',sizeof(struct volume_info));

	if (read_bootsectandvi(&bs, &volinfo, &fs->fatsize)) {
		printf("error: reading boot sector\n");
		return -1;
	}

	if(read_FSInfo(bs.info_sector) != 0) {
		printf("error: reading FSinfo\n");
		return -1;
	}

	total_sector = bs.total_sect;
	if (total_sector == 0) {
		total_sector = part_size;
	}

	fragmented_count = 0;
	dir_curclust = 0;

	memset(get_vfatname_block,'\0',MAX_CLUSTSIZE);
	memset(get_dentfromdir_block,'\0',MAX_CLUSTSIZE);
	if(empty_dentptr) {
		memset(empty_dentptr,'\0',sizeof( struct  dir_entry));
	}

	if (fs->fatsize == 32) {
		fs->fatlength = bs.fat32_length;
	}
	else {
		fs->fatlength = bs.fat_length;
		printf("Error: Not a FAT32 Partition\n");
		return -1;
	}
	fs->fat_sect = bs.reserved;
	fs->rootdir_sect	= fs->fat_sect + fs->fatlength * bs.fats;
	fs->clust_size = bs.cluster_size;

	if (fs->fatsize == 32) {
		fs->data_begin = fs->rootdir_sect -
					(fs->clust_size * 2);
	} else {
		int rootdir_size;
		rootdir_size = ((bs.dir_entries[1]  * (int)256 +
				 bs.dir_entries[0]) *
				 sizeof(dir_entry)) /
				 SECTOR_SIZE;
		fs->data_begin = fs->rootdir_sect +
					rootdir_size -
					(fs->clust_size * 2);
	}
	fs->fatbufnum = -1;
	return 0;
}


int
deinit_FAT32fs(fsdata *fs)
{
	short int i = 0;
	if(fs == NULL ) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	/*free directory table*/
	for(i = 0;i < dir_level[i] != NULL;i++) {
		if(dir_level[i])
		free(dir_level[i]);
	}
	if(dir_level)
		free(dir_level);
	return 0;
}


/*
*Description : return cluster number from sector number
[input ] fs is filesystem info
[input ] sector_num is sector number
*/
__u32
get_clusterNum(fsdata *fs ,unsigned int sector_num)
{
	return (sector_num - fs->data_begin) / fs->clust_size;
}


/*Description
*[input   ] fs filesystem info
*[Output ] isit_rootdir can we create in rootdir
*[Output ] filename or directory name to be created
*[Output ] parent_cluster cluster number of parent directory
*[Output ] direntry is either empty (if not matched) or matched directory entry
*/
int
get_parentdir_info(fsdata* fs,int *isit_rootdir,char *filename,
				__u32 *parent_clus,dir_entry **direntry,int operation)
{
	dir_entry *dentptr = NULL, *retdent = NULL;
	__u32 startsect = 0;
	__u32 start_cluster = 0;
	__u32 parent_cluster = 0;
	char l_filename[256] = {0,};
	int i = 0;
	int create_in_rootdir = 0;
	unsigned int root_dir_cluster;

	if(fs == NULL || filename == NULL || direntry == NULL) 	{
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	startsect = fs->rootdir_sect;
	start_cluster = get_clusterNum(fs,startsect);
	parent_cluster = root_dir_cluster = start_cluster;

	for(i = 1;dir_level[i] != NULL;i++)
	{
		memset(do_fat_read_block,'\0',MAX_CLUSTSIZE);
		if (disk_read(startsect, fs->clust_size, do_fat_read_block) < 0) {
			printf("Error: reading rootdir block\n");
			return -1;
		}

		dentptr = (dir_entry *) do_fat_read_block;
		memset(l_filename,'\0',256);
		memcpy(l_filename, dir_level[i], strlen(dir_level[i]));
		downcase(l_filename);
		memset(get_dentfromdir_block,'\0',MAX_CLUSTSIZE);

		retdent = find_directory_entry(fs, startsect,
				l_filename, dentptr, 0, 1, 0);
		if(retdent) {
			/*directory entry  found in start_cluster/startsect*/
			/*checking is it file*/
			if(retdent->attr & ATTR_ARCH) {
				if(dir_level[i+1] != NULL) {
					printf("Path Does not exists\n");
					goto fail;
					break;
				}
				if(dir_level[i+1] == NULL) {
					printf("%s : Already existing file\n",dir_level[i]);
					if(operation == CREATE_FILE) {
						printf("Overwritting %s ... \n",dir_level[i]);
					}
					if(operation == CREATE_DIR) {
						printf("Info: File & Directory name matches\n");
						goto fail;
					}
					break;
				}
			}
			/*check is it Directorry*/
			else if (retdent->attr & ATTR_DIR) {
				if(dir_level[i+1] == NULL) {
					printf("%s : Already existing Directory\n",dir_level[i]);
					if(operation == CREATE_FILE) {
						printf("Info: File & Directory name matches\n");
					}
					goto fail;
					break;
				}
			}
			else {
				printf("unhandled retdent->attr\n");
			}

			/*get the clusterno */
			start_cluster = START(retdent);

			/*get the sector no from cluster*/
			startsect = fs->data_begin + start_cluster * fs->clust_size;

			parent_cluster = start_cluster;
		}
		else
		{
			/*directory entry not  found in start_cluster/startsect*/
			if(dir_level[i+1] != NULL) {
				printf("%s Invalid path:Path Does not found\n",dir_level[i]);
				goto fail;
				break;
			}
			if(dir_level[i+1] == NULL) {
				//printf("Valid Path:Ready to add entry\n");
				break;
			}
		}
	}

	/*Identifying if user supplied is root directory*/
	if((dir_level[1] != NULL) && (dir_level[2] == NULL)) {
		create_in_rootdir = 1;
		parent_cluster = root_dir_cluster;
	}

	*isit_rootdir = create_in_rootdir;
	*parent_clus = parent_cluster;
	*direntry = (struct dir_entry *)retdent;
	memcpy(filename,l_filename,strlen(l_filename));
	return 0; /*success*/

	fail:
		return -1; /*error*/
}


int
validate_path(char *absolute_path)
{
	if(absolute_path == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}
	if(*absolute_path == '/' && strlen(absolute_path) == 1) {
		printf("Info: Please supply Name\n");
		return -1;
	}
	/*TODO: input validation according to FAT32 Standards*/
	return 0;
}

int
_create_FAT32_directory( fsdata *fs,int under_root ,
					char *l_filename,	__u32 parent_cluster)
{
	__u32 start_cluster;
	dir_slot *slotptr;
	char *initial_dir_buff;
	unsigned long size;
	int ret;
	char attr = 0;
	attr = attr | ATTR_DIR;

	if(fs == NULL || l_filename == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	/*create & populate directory entry*/
	if(empty_dentptr == NULL) 	{
		return -1;
	}

	/*getting size of one cluster for new directory*/
	size = fs->clust_size*SECTOR_SIZE;
	slotptr = (dir_slot *)empty_dentptr;

	/* Set short name to set alias checksum field in dir_slot */
	set_name(empty_dentptr, l_filename);
	fill_dir_slot(fs, &empty_dentptr, l_filename);

	/*allocate empy cluster for directory entry*/
	ret = start_cluster = find_empty_cluster(fs);
	if (ret < 0) {
		printf("error: finding empty cluster\n");
		return -1;
	}

	fill_dentry(fs, empty_dentptr, l_filename,
			start_cluster, 0, attr);

	if(under_root == 1)
		parent_cluster = 0;

	/*create . &  .. entry in newly allocated cluster */
	initial_dir_buff=(char*) xzalloc(size);
	init_directory_entry_cluster(fs,initial_dir_buff,
			start_cluster,parent_cluster);
	ret = set_contents(fs, empty_dentptr, initial_dir_buff,size);
	if (ret < 0) {
		printf("error: writing contents\n");
		return -1;
	}

	 /* dump . & .. entry to disk*/
	set_cluster(fs,start_cluster,initial_dir_buff,size);
	update_free_clusters_count(fs,NULL,size);

	/* Flush fat buffer */
	if (flush_fat_buffer(fs) < 0)
		return -1;

	/* Write parent directory entry cluster
	"get_dentfromdir_block" populated by
	"find_directory_entry"*/
	if (set_cluster(fs, dir_curclust,
	get_dentfromdir_block,
	fs->clust_size * SECTOR_SIZE) != 0) {
		printf("error: writing directory entry\n");
		return -1;
	}

	/*freeing*/
	if(initial_dir_buff)
		free(initial_dir_buff);

	return 0; /*success*/
}


int
create_FAT32_directory(fsdata *fs)
{
	dir_entry *dentptr = NULL;
	__u32 parent_cluster = 0;
	char l_filename[256];
	int create_in_rootdir = 0;
	memset(l_filename,'\0',256);

	if(fs == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	if(get_parentdir_info(fs,&create_in_rootdir,
		l_filename,&parent_cluster,&dentptr,CREATE_DIR) != 0) {
		return -1;
	}

	if(_create_FAT32_directory(fs,create_in_rootdir,
		l_filename,parent_cluster) != 0) {
		return -1;
	}
	return 0; /*success*/
}


 int
 FAT32_mkdir (const char *absolute_path )
{
	fsdata fs;
	memset(&fs,'\0',sizeof(fsdata));

	if(absolute_path == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	/*validate the supplied absolute path (complete path)*/
	if (validate_path(absolute_path) != 0) {
		printf("Error: Invalid path\n");
		return -1;
	}

	/*intialize filesystem*/
	if(init_FAT32fs(&fs) != 0) {
		printf("Error in FAT32 fs initialization\n");
		return -1;
	}

	/*parse the absoulte path (complete path*)*/
	if(parse_FAT32_path(absolute_path) != 0) {
		printf("Error: Invalid path\n");
		return -1;
	}

	/*create a FAT32 directory*/
	if (create_FAT32_directory(&fs) != 0 ) {
		return -1;
	}

	/*de-initialze the file system*/
	if(deinit_FAT32fs(&fs)!= 0) {
		return -1;
	}

	return 0;
}


int
do_fat32_mkdir (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *dirname = NULL;
	long size;
	char buf [12];
	block_dev_desc_t *dev_desc = NULL;
	int dev = 0;
	int part = 1;
	char *ep;

	if (argc < 4)
		return cmd_usage(cmdtp);

	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if (fat_register_device(dev_desc,part) != 0) {
		printf("\n** Unable to use %s %d:%d for fatload **\n",
			argv[1], dev, part);
		return 1;
	}

	if (argc == 4)
		dirname = argv[3];

	size = FAT32_mkdir(argv[3]);

	if(size == -1) {
		printf("Directory creation failed..!!!\n");
		return 1;
	}
	else {
		printf("Success!!!\n");
	}

	sprintf(buf, "%lX", size);
	setenv("filesize", buf);

	return 0;
}


U_BOOT_CMD(
	fat32mkdir,	4, 0,	do_fat32_mkdir,
	"Create a directory in FAT32 filesystem",
	"<interface> <dev[:part]>  [Absolute Directory Path] \n"
	"Create a directory in DOS filesystem"
);


int
_create_FAT32_file( fsdata *fs,int under_root ,char *l_filename,
				__u32 parent_cluster,dir_entry *retdent,char *buffer
				,unsigned long size)
{
	__u32 start_cluster = 0;
	dir_slot *slotptr = NULL;
	int ret = -1;

	if(fs == NULL || l_filename == NULL || buffer == NULL) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	if (retdent) {
		update_free_clusters_count(fs,retdent,size);
		/* Update file size and start_cluster in a directory entry */
		retdent->size = cpu_to_le32(size);
		start_cluster = FAT2CPU16(retdent->start);
		if (fs->fatsize == 32)
			start_cluster |=
				(FAT2CPU16(retdent->starthi) << 16);

		ret = check_overflow(fs, start_cluster, size);
		if (ret) {
			printf("error: %ld overflow\n", size);
			return -1;
		}

		ret = clear_fatent(fs, start_cluster);
		if (ret < 0) {
			printf("error: clearing FAT entries\n");
			return -1;
		}

		ret = set_contents(fs, retdent, buffer, size);
		if (ret < 0) {
			printf("error: writing contents\n");
			return -1;
		}

		/* Flush fat buffer */
		if (flush_fat_buffer(fs) < 0)
			return -1;

#if 0
		if (fragmented_count > 0) {
			defragment_file(mydata, retdent);
			retdent->size = cpu_to_le32(size);
		}
#endif

		/* Write directory table to device */
		if (set_cluster(fs, dir_curclust,
			    get_dentfromdir_block,
			    fs->clust_size * SECTOR_SIZE) != 0) {
			printf("error: wrinting directory entry\n");
			return -1;
		}
	} else {
		slotptr = (dir_slot *)empty_dentptr;

		/* Set short name to set alias checksum field in dir_slot */
		set_name(empty_dentptr, l_filename);
		fill_dir_slot(fs, &empty_dentptr, l_filename);

		ret = start_cluster = find_empty_cluster(fs);
		if (ret < 0) {
			printf("error: finding empty cluster\n");
			return -1;
		}

		ret = check_overflow(fs, start_cluster, size);
		if (ret) {
			printf("error: %ld overflow\n", size);
			return -1;
		}

		/* Set attribute as archieve for regular file */
		fill_dentry(fs, empty_dentptr, l_filename,
			start_cluster, size, 0x20);

		ret = set_contents(fs, empty_dentptr, buffer, size);
		if (ret < 0) {
			printf("error: writing contents\n");
			return -1;
		}

		/* Flush fat buffer */
		if (flush_fat_buffer(fs) < 0)
			return -1;

		/* Write directory table to device */
		if (set_cluster(fs, dir_curclust,
			    get_dentfromdir_block,
			    fs->clust_size * SECTOR_SIZE) != 0) {
			printf("error: writing directory entry\n");
			return -1;
		}
		update_free_clusters_count(fs,NULL,size);
#if 0
		if (fragmented_count > 0) {
			defragment_file(mydata, empty_dentptr);
			empty_dentptr->size = cpu_to_le32(size);

			/* Write directory table to device */
			if (set_cluster(mydata, dir_curclust,
				    get_dentfromdir_block,
				    mydata->clust_size * SECTOR_SIZE) != 0) {
				printf("error: writing directory entry\n");
				return -1;
			}
		}
#endif
	}

	return 0;
}


int
create_FAT32_file(fsdata *fs,char *buffer,unsigned long size )
{
	dir_entry *retdent = NULL;
	__u32 parent_cluster = 0;
	char l_filename[256];
	int create_in_rootdir = 0;
	memset(l_filename,'\0',256);
	int ret;
	int operation = CREATE_DIR;

	if(fs == NULL || buffer == NULL || size < 0) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	if(get_parentdir_info(fs,&create_in_rootdir,
		l_filename,&parent_cluster,&retdent,CREATE_FILE) != 0) {
		return -1;
	}

	if(_create_FAT32_file(fs,create_in_rootdir,
		l_filename,parent_cluster,retdent,buffer,size) != 0) {
		return -1;
	}

	return 0; /*success*/
}


 int
 FAT32_write (const char *absolute_path,void *buffer,unsigned long size )
{
	fsdata fs;
	memset(&fs,'\0',sizeof(fsdata));

	if(buffer == NULL || size < 0 || absolute_path == NULL ) {
		printf("Error:Invalid Input arguments: %s\n",__FUNCTION__);
		return -1;
	}

	/*validate the supplied absolute path (complete path)*/
	if (validate_path(absolute_path) != 0) {
		printf("Error: Invalid path\n");
		return -1;
	}

	/*intialize filesystem*/
	if(init_FAT32fs(&fs) != 0) {
		printf("Error in FAT32 fs initialization\n");
		return -1;
	}

	/*parse the absoulte path (complete path*)*/
	if(parse_FAT32_path(absolute_path) != 0) {
		printf("Error: Invalid path\n");
		return -1;
	}

	/*create a FAT32 file*/
	if (create_FAT32_file(&fs,buffer,size) != 0 ) {
		return -1;
	}

	/*de-initialze the file system*/
	if(deinit_FAT32fs(&fs) != 0) {
		return -1;
	}

	return 0;
}


int
do_fat32_write (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	long size;
	unsigned long addr;
	unsigned long offset;
	unsigned long count;
	char buf [12];
	block_dev_desc_t *dev_desc = NULL;
	int dev = 0;
	int part = 1;
	char *ep;

	if (argc < 6) {
		printf( "usage: fatwrite <interface> <dev[:part]> "
			"<addr> <filename> <bytes>\n");
		return 1;
	}

	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if (fat_register_device(dev_desc,part) != 0) {
		printf("\n** Unable to use %s %d:%d for fatload **\n",
			argv[1], dev, part);
		return 1;
	}
	addr = simple_strtoul(argv[3], NULL, 16);
	count = simple_strtoul(argv[5], NULL, NULL);

	size = FAT32_write(argv[4], (unsigned char *)addr, count);

	if(size == -1) {
		printf("File creation failed..!!!\n");
		return 1;
	}
	else {
		printf("Success!!!\n");
	}

	sprintf(buf, "%lX", size);
	setenv("filesize", buf);

	return 0;
}


U_BOOT_CMD(
	fat32write,	7,	0,	do_fat32_write,
	"Create a file in FAT32 filesystem",
	"<interface> <dev[:part]>  <addr> <filename> <bytes>\n"
	"    - write binary file 'filename' from the address 'addr'\n"
	"      to FAT32 filesystem 'dev' on 'interface'"
);

#endif

void file_fat_table (void)
{
	dir_entry *dentptr;
	__u32 startsect;
	boot_sector bs;
	volume_info volinfo;
	fsdata datablock;
	fsdata *mydata = &datablock;
	int cursect;

	if (read_bootsectandvi(&bs, &volinfo, &mydata->fatsize)) {
		debug("error: reading boot sector\n");
	}

	total_sector = bs.total_sect;
	if (total_sector == 0) {
		total_sector = part_size;
	}

	if (mydata->fatsize == 32)
		mydata->fatlength = bs.fat32_length;
	else
		mydata->fatlength = bs.fat_length;

	mydata->fat_sect = bs.reserved;

	cursect = mydata->rootdir_sect
		= mydata->fat_sect + mydata->fatlength * bs.fats;

	mydata->clust_size = bs.cluster_size;

	if (mydata->fatsize == 32) {
		mydata->data_begin = mydata->rootdir_sect -
					(mydata->clust_size * 2);
	} else {
		int rootdir_size;

		rootdir_size = ((bs.dir_entries[1]  * (int)256 +
				 bs.dir_entries[0]) *
				 sizeof(dir_entry)) /
				 SECTOR_SIZE;
		mydata->data_begin = mydata->rootdir_sect +
					rootdir_size -
					(mydata->clust_size * 2);
	}

	mydata->fatbufnum = -1;
	startsect = mydata->rootdir_sect;

	if (disk_read(cursect, mydata->clust_size, do_fat_read_block) < 0) {
		printf("error: reading rootdir block\n");
	}
	dentptr = (dir_entry *) do_fat_read_block;

	find_directory_entry(mydata, startsect,
				"", dentptr, 0, 1, 1);
}

int do_fat_table (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int dev = 0;
	int part = 1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;

	if (argc < 2) {
		printf("usage: fattable <interface> <dev[:part]>\n");
		return 0;
	}
	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1], dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if (fat_register_device(dev_desc,part)!=0) {
		printf("\n** Unable to use %s %d:%d for fattable **\n",
			argv[1], dev, part);
		return 1;
	}

	file_fat_table();

	return 0;
}

U_BOOT_CMD(
	fattable,	3,	1,	do_fat_table,
	"print fat range of files ",
	"<interface> <dev[:part]>\n"
	"    - list files from 'dev' on 'interface' in a 'directory'"
);

#define MAX_CLUST_32    0x0FFFFFF0
#define NUM_FATS        2

void unsupported_field_size(void);
#define STORE_LE(field, value) \
do { \
	if (sizeof(field) == 4) \
		field = cpu_to_le32(value); \
	else if (sizeof(field) == 2) \
		field = cpu_to_le16(value); \
	else if (sizeof(field) == 1) \
		field = (value); \
	else \
		unsupported_field_size(); \
} while (0)

#define BOOT_SIGN 0xAA55

#define FAT_FSINFO_SIG1 0x41615252
#define FAT_FSINFO_SIG2 0x61417272

/* 
 * mkfs_vfat function derives from mkfs_vfat.c in busybox
 * file systems type is FAT32
 */
int mkfs_vfat (block_dev_desc_t *dev_desc, int part_no)
{
	disk_partition_t info;
	boot_sector *bs;
	volume_info *volinfo;
	void *fsinfo;
	const char *volume_label = "";
	__u8 *buf;
	__u64 volume_size_bytes;
	__u32 volume_size_sect;
	__u32 total_clust;
	__u32 volume_id;
	__u32 bytes_per_sect;
	__u32 sect_per_fat;
	__u32 temp;
	__u16 sect_per_track;
	__u8 media_byte;
	__u8 sect_per_clust;
	__u8 heads;
	__u32 reserved_sect = 6;
	__u32 info_sector_number = 1;
	__u32 backup_boot_sector = 3;
	__u32 start_sect;
	unsigned i, j, remainder;
	unsigned char *fat;

	/* volume_id is fixed */
	volume_id = 0x386d43bf;

	/* Get image size and sector size */
	bytes_per_sect = SECTOR_SIZE;

	if (!get_partition_info(dev_desc, part_no, &info)) {
		volume_size_bytes = info.size * info.blksz;
		volume_size_sect = info.size;

		total_sector = volume_size_sect;
	} else {
		printf("error : get partition info\n");
		return 1;
	}

	/* 
	 * FIXME heads and sect_per_track should be set
	 * according to each block device
	 */
	media_byte = 0xf8;
	heads = 4;
	sect_per_track = 16;
	sect_per_clust = 1;

	/* clust_size <= 260M: 512 bytes
	 * clust_size <=   8G: 4k  bytes
	 * clust_size <=  16G: 8k  bytes
	 * clust_size >   16G: 16k bytes
	 */
	sect_per_clust = 1;

	if (volume_size_bytes >= 260 * 1024 * 1024) {
		sect_per_clust = 8;

		temp = (volume_size_bytes >> 32);
		if (temp >= 2)
			sect_per_clust = 16;
		if (temp >= 4)
			sect_per_clust = 32;
	}

	/*
	 * Calculate number of clusters, sectors/cluster, sectors/FAT
	 * minimum: 2 sectors for FATs and 2 data sectors
	 */
	if ((volume_size_sect - reserved_sect) < 4) {
		printf("the image is too small for FAT32\n");
		return 1;
	}

	sect_per_fat = 1;
	while (1) {
		while (1) {
			int spf_adj;
			__u32 tcl = (volume_size_sect - reserved_sect -
				NUM_FATS * sect_per_fat) / sect_per_clust;
			/* tcl may be > MAX_CLUST_32 here, but it may be
			 * because sect_per_fat is underestimated,
			 * and with increased sect_per_fat it still may become
			 * <= MAX_CLUST_32. Therefore, we do not check
			 * against MAX_CLUST_32, but against a bigger const */
			if (tcl > 0x80ffffff)
				goto next;
			total_clust = tcl; /* fits in __u32 */
			/* Every cluster needs 4 bytes in FAT. +2 entries since
			 * FAT has space for non-existent clusters 0 and 1.
			 * Let's see how many sectors that needs.
			 * May overflow at "*4": 
			 * spf_adj = ((total_clust+2) * 4 + bytes_per_sect-1) /
			 * bytes_per_sect - sect_per_fat;
			 * Same in the more obscure, non-overflowing form: */
			spf_adj = ((total_clust+2) + (bytes_per_sect/4)-1) /
				(bytes_per_sect/4) - sect_per_fat;
			if (spf_adj <= 0) {
				/* do not need to adjust sect_per_fat.
				 * so, was total_clust too big after all? */
				if (total_clust <= MAX_CLUST_32)
					goto found_total_clust; /* no */
				/* yes, total_clust is _a bit_ too big */
				goto next;
			}
			/* adjust sect_per_fat, go back and recalc total_clust
			 * (note: just "sect_per_fat += spf_adj" isn't ok) */
			sect_per_fat += ((__u32)spf_adj / 2) | 1;
		}
next:
		if (sect_per_clust == 128) {
			printf("can't make FAT32 with >128 sectors/cluster");
			return 1;
		}
		sect_per_clust *= 2;
		sect_per_fat = (sect_per_fat / 2) | 1;
	}
found_total_clust:
	debug("heads:%u, sectors/track:%u, bytes/sector:%u\n"
		"media descriptor:%02x\n"
		"total sectors:%u, clusters:%u, sectors/cluster:%u\n"
		"FATs:2, sectors/FAT:%u\n"
		"volumeID:%08x, label:'%s'\n",
		heads, sect_per_track, bytes_per_sect,
		(int)media_byte,
		volume_size_sect, (int)total_clust, (int)sect_per_clust,
		sect_per_fat,
		(int)volume_id, volume_label
	);

	/*
	 * Write filesystem image
	 */
	buf = do_fat_read_block;
	memset(buf, 0, sizeof(do_fat_read_block));

	/* boot and fsinfo sectors, and their copies */
	bs = (void*)buf;
	volinfo = (void*)(buf + 0x40);
	fsinfo = (void*)(buf + bytes_per_sect);

	bs->ignored[0] = 0xeb;
	bs->ignored[1] = 0x58;
	bs->ignored[2] = 0x90;
	strncpy(bs->system_id, "mkdosfs", 8);

	bs->sector_size[0] = cpu_to_le16(bytes_per_sect) & 0xff;
	bs->sector_size[1] = (cpu_to_le16(bytes_per_sect) & 0xff00) >> 8;

	STORE_LE(bs->cluster_size, sect_per_clust);
	STORE_LE(bs->reserved, (__u16)reserved_sect);
	STORE_LE(bs->fats, 2);

	if (volume_size_sect <= 0xffff) {
		bs->sectors[0] = cpu_to_le16(volume_size_sect) & 0xff;
		bs->sectors[1] = (cpu_to_le16(volume_size_sect) & 0xff00) >> 8;
	}

	STORE_LE(bs->media, media_byte);
	STORE_LE(bs->secs_track, sect_per_track);
	STORE_LE(bs->heads, heads);

	STORE_LE(bs->total_sect, volume_size_sect);
	STORE_LE(bs->fat32_length, sect_per_fat);

	STORE_LE(bs->root_cluster, 2);
	STORE_LE(bs->info_sector, info_sector_number);
	STORE_LE(bs->backup_boot, backup_boot_sector);

	STORE_LE(volinfo->ext_boot_sign, 0x29);
	volinfo->volume_id[0] = cpu_to_le32(volume_id) & 0xff;
	volinfo->volume_id[1] = (cpu_to_le32(volume_id) & 0xff00) >> 8;
	volinfo->volume_id[2] = (cpu_to_le32(volume_id) & 0xff0000) >> 16;
	volinfo->volume_id[3] = (cpu_to_le32(volume_id) & 0xff000000) >> 24;

	strncpy(volinfo->fs_type, "FAT32   ", sizeof(volinfo->fs_type));
	strncpy(volinfo->volume_label, volume_label,
		sizeof(volinfo->volume_label));
	memset(buf + 0x1fe, (cpu_to_le16(BOOT_SIGN) & 0xff), 1);
	memset(buf + 0x1ff, (cpu_to_le16(BOOT_SIGN) & 0xff00) >> 8, 1);

	memset(fsinfo, (cpu_to_le32(FAT_FSINFO_SIG1) & 0xff), 1);

	memset(fsinfo + 0x1, (cpu_to_le32(FAT_FSINFO_SIG1) & 0xff00) >> 8, 1);
	memset(fsinfo + 0x2,
		(cpu_to_le32(FAT_FSINFO_SIG1) & 0xff0000) >> 16, 1);
	memset(fsinfo + 0x3,
		(cpu_to_le32(FAT_FSINFO_SIG1) & 0xff000000) >> 24, 1);

	memset(fsinfo + 0x1e4, (cpu_to_le32(FAT_FSINFO_SIG2) & 0xff), 1);
	memset(fsinfo + 0x1e5,
		(cpu_to_le32(FAT_FSINFO_SIG2) & 0xff00) >> 8, 1);
	memset(fsinfo + 0x1e6,
		(cpu_to_le32(FAT_FSINFO_SIG2) & 0xff0000) >> 16, 1);
	memset(fsinfo + 0x1e7,
		(cpu_to_le32(FAT_FSINFO_SIG2) & 0xff000000) >> 24, 1);

	memset(fsinfo + 0x1e8, cpu_to_le32(total_clust - 1) & 0xff, 1);
	memset(fsinfo + 0x1e9,
		(cpu_to_le32(total_clust - 1) & 0xff00) >> 8, 1);
	memset(fsinfo + 0x1ea,
		(cpu_to_le32(total_clust - 1) & 0xff0000) >> 16, 1);
	memset(fsinfo + 0x1eb,
		(cpu_to_le32(total_clust - 1) & 0xff000000) >> 24, 1);

	memset(fsinfo + 0x1ec, cpu_to_le32(2) & 0xff, 1);

	memset(fsinfo + 0x1fe, cpu_to_le16(BOOT_SIGN) & 0xff, 1);
	memset(fsinfo + 0x1ff, (cpu_to_le16(BOOT_SIGN) & 0xff00) >> 8, 1);

	/* Write boot and fsinfo sectors */
	if (disk_write(0, backup_boot_sector, buf) < 0)
		return 1;
	if (disk_write(3, backup_boot_sector, buf) < 0)
		return 1;

	/* file allocation tables */
	fat = buf;
	memset(buf, 0, bytes_per_sect * 2);
	/* initial FAT entries */
	((__u32 *)fat)[0] = cpu_to_le32(0x0fffff00 | media_byte);
	((__u32 *)fat)[1] = cpu_to_le32(0x0fffffff);
	/* mark cluster 2 as EOF (used for root dir) */
	((__u32 *)fat)[2] = cpu_to_le32(0x0ffffff8);

	memset(get_vfatname_block, 0, MAX_CLUSTSIZE);
	start_sect = reserved_sect;
	for (i = 0; i < NUM_FATS; i++) {
		temp = sect_per_fat / (MAX_CLUSTSIZE / SECTOR_SIZE);
		remainder = sect_per_fat % (MAX_CLUSTSIZE / SECTOR_SIZE);

		for (j = 0; j < temp; j++) {
			if (disk_write(start_sect, MAX_CLUSTSIZE / SECTOR_SIZE,
				get_vfatname_block) < 0)
				return 1;
			start_sect += MAX_CLUSTSIZE / SECTOR_SIZE;
		}
		if (remainder) {
			if (disk_write(start_sect, remainder,
				get_vfatname_block) < 0)
				return 1;
			start_sect += remainder;
		}
	}

	if (disk_write(reserved_sect, 1, buf) < 0)
		return 1;
	if (disk_write(reserved_sect + sect_per_fat, 1, buf) < 0)
		return 1;

	/* root directory */
	/* empty directory is just a set of zero bytes */
	if (disk_write(start_sect, sect_per_clust, get_vfatname_block) < 0)
		return 1;

	return 0;
}

int do_fat_format (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int dev = 0;
	int part = 1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;

	if (argc < 2) {
		printf("usage: fatformat <interface> <dev[:part]>\n");
		return 0;
	}
	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1], dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if (fat_register_device(dev_desc,part)!=0) {
		printf("\n** Unable to use %s %d:%d for fattable **\n",
			argv[1], dev, part);
		return 1;
	}

	printf("formatting\n");
	if (mkfs_vfat(dev_desc, part))
		return 1;

	return 0;
}

U_BOOT_CMD(
	fatformat,	3,	1,	do_fat_format,
	"format device as FAT32 filesystem",
	"<interface> <dev[:part]>\n"
	"    - format device as FAT32 filesystem on 'dev'"
);

