/*
 * Copyright (C) 2008 Nokia Corporation.
 * Copyright (C) 2008 University of Szeged, Hungary
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Authors: Adrian Hunter
 *          Artem Bityutskiy
 *          Zoltan Sogor
 */

#include "mkfs.ubifs.h"

#define PROGRAM_VERSION "1.3"

/* The node buffer must allow for worst case compression */
#define NODE_BUFFER_SIZE (UBIFS_DATA_NODE_SZ + \
			  UBIFS_BLOCK_SIZE * WORST_COMPR_FACTOR)

/* Default time granularity in nanoseconds */
#define DEFAULT_TIME_GRAN 1000000000

#define MKFS_UBIFS_COMPR_NONE	0
#define MKFS_UBIFS_COMPR_LZO	1

#define MODEM_IMAGE_SIZE	0xc00000

#define DEFAULT_ROOTDIR_SQNUM	1
#define DEFAULT_ROOTDIR_NLINK	2
#define DEFAULT_FILE_SQNUM	2
#define DEFAULT_FILE_NLINK	1
#define DEFAULT_TIME_SEC	0x4ca2dfd9
#define DEFAULT_ROOTDIR_MODE	0x41ed
#define DEFAULT_FILE_MODE	0x81c0
#define DEFAULT_UID		1000
#define DEFAULT_GID		513

/**
 * struct idx_entry - index entry.
 * @next: next index entry (NULL at end of list)
 * @prev: previous index entry (NULL at beginning of list)
 * @key: key
 * @name: directory entry name used for sorting colliding keys by name
 * @lnum: LEB number
 * @offs: offset
 * @len: length
 *
 * The index is recorded as a linked list which is sorted and used to create
 * the bottom level of the on-flash index tree. The remaining levels of the
 * index tree are each built from the level below.
 */
struct idx_entry {
	struct idx_entry *next;
	struct idx_entry *prev;
	union ubifs_key key;
	char *name;
	int lnum;
	int offs;
	int len;
};

/*
 * Because we copy functions from the kernel, we use a subset of the UBIFS
 * file-system description object struct ubifs_info.
 */
struct ubifs_info info_;
static struct ubifs_info *c = &info_;

/* Debug levels are: 0 (none), 1 (statistics), 2 (files) ,3 (more details) */
int debug_level;
int verbose;

/* The 'head' (position) which nodes are written */
static int head_lnum;
static int head_offs;
static int head_flags;

/* The index list */
static struct idx_entry *idx_list_first;
static struct idx_entry *idx_list_last;
static size_t idx_cnt;

/* Global buffers */
static void *leb_buf;
static void *node_buf;
static void *block_buf;
static void *lzo_mem;

static void *modem_buf;

/* Inode creation sequence number */
static unsigned long long creat_sqnum;

/**
 * calc_min_log_lebs - calculate the minimum number of log LEBs needed.
 * @max_bud_bytes: journal size (buds only)
 */
static int calc_min_log_lebs(unsigned long long max_bud_bytes)
{
	int buds, log_lebs;
	unsigned long long log_size;

	buds = (max_bud_bytes + c->leb_size - 1) / c->leb_size;
	log_size = ALIGN(UBIFS_REF_NODE_SZ, c->min_io_size);
	log_size *= buds;
	log_size += ALIGN(UBIFS_CS_NODE_SZ +
			  UBIFS_REF_NODE_SZ * (c->jhead_cnt + 2),
			  c->min_io_size);
	log_lebs = (log_size + c->leb_size - 1) / c->leb_size;
	log_lebs += 1;
	return log_lebs;
}

static int set_options(int min_io_size, int leb_size, int max_leb_cnt)
{
	int lebs;

	c->fanout = 8;
	c->orph_lebs = 1;
	c->key_hash = key_r5_hash;
	c->key_len = UBIFS_SK_LEN;
	c->default_compr = UBIFS_COMPR_LZO;
	c->lsave_cnt = 256;

	c->min_io_size = min_io_size;
	c->leb_size = leb_size;
	c->max_leb_cnt = max_leb_cnt;

	/* calcuate max_bud_bytes */
	lebs = c->max_leb_cnt - UBIFS_SB_LEBS - UBIFS_MST_LEBS;
	lebs -= c->orph_lebs;
	lebs -= UBIFS_MIN_LOG_LEBS;
	/*
	 * We do not know lprops geometry so far, so assume minimum
	 * count of lprops LEBs.
	 */
	lebs -= UBIFS_MIN_LPT_LEBS;
	/* Make the journal about 12.5% of main area lebs */
	c->max_bud_bytes = (lebs / 8) * (long long)c->leb_size;
	/* Make the max journal size 8MiB */
	if (c->max_bud_bytes > 8 * 1024 * 1024)
		c->max_bud_bytes = 8 * 1024 * 1024;
	if (c->max_bud_bytes < 4 * c->leb_size)
		c->max_bud_bytes = 4 * c->leb_size;

	/* calculate log_lebs */
	c->log_lebs = calc_min_log_lebs(c->max_bud_bytes);
	c->log_lebs += 2;

	return 0;
}

/**
 * prepare_node - fill in the common header.
 * @node: node
 * @len: node length
 */
static void prepare_node(void *node, int len)
{
	uint32_t crc;
	struct ubifs_ch *ch = node;

	ch->magic = cpu_to_le32(UBIFS_NODE_MAGIC);
	ch->len = cpu_to_le32(len);
	ch->group_type = UBIFS_NO_NODE_GROUP;
	ch->sqnum = cpu_to_le64(++c->max_sqnum);
	ch->padding[0] = ch->padding[1] = 0;
	crc = ubifs_crc32(UBIFS_CRC32_INIT, node + 8, len - 8);
	ch->crc = cpu_to_le32(crc);
}

/**
 * write_leb - copy the image of a LEB to the output target.
 * @lnum: LEB number
 * @len: length of data in the buffer
 * @buf: buffer (must be at least c->leb_size bytes)
 * @dtype: expected data type
 */
int write_leb(int lnum, int len, void *buf, int dtype)
{
	unsigned long pos = (unsigned long)lnum * c->leb_size;

	dbg_msg("LEB %d len %d", lnum, len);
	memset(buf + len, 0xff, c->leb_size - len);

	memcpy(modem_buf + pos, buf, c->leb_size);

	return 0;
}

/**
 * write_empty_leb - copy the image of an empty LEB to the output target.
 * @lnum: LEB number
 * @dtype: expected data type
 */
static int write_empty_leb(int lnum, int dtype)
{
	return write_leb(lnum, 0, leb_buf, dtype);
}

/**
 * do_pad - pad a buffer to the minimum I/O size.
 * @buf: buffer
 * @len: buffer length
 */
static int do_pad(void *buf, int len)
{
	int pad_len, alen = ALIGN(len, 8), wlen = ALIGN(alen, c->min_io_size);
	uint32_t crc;

	memset(buf + len, 0xff, alen - len);
	pad_len = wlen - alen;
	dbg_msg("len %d pad_len %d", len, pad_len);
	buf += alen;
	if (pad_len >= (int)UBIFS_PAD_NODE_SZ) {
		struct ubifs_ch *ch = buf;
		struct ubifs_pad_node *pad_node = buf;

		ch->magic      = cpu_to_le32(UBIFS_NODE_MAGIC);
		ch->node_type  = UBIFS_PAD_NODE;
		ch->group_type = UBIFS_NO_NODE_GROUP;
		ch->padding[0] = ch->padding[1] = 0;
		ch->sqnum      = cpu_to_le64(0);
		ch->len        = cpu_to_le32(UBIFS_PAD_NODE_SZ);

		pad_len -= UBIFS_PAD_NODE_SZ;
		pad_node->pad_len = cpu_to_le32(pad_len);

		crc = ubifs_crc32(UBIFS_CRC32_INIT, buf + 8,
				  UBIFS_PAD_NODE_SZ - 8);
		ch->crc = cpu_to_le32(crc);

		memset(buf + UBIFS_PAD_NODE_SZ, 0, pad_len);
	} else if (pad_len > 0)
		memset(buf, UBIFS_PADDING_BYTE, pad_len);

	return wlen;
}

/**
 * write_node - write a node to a LEB.
 * @node: node
 * @len: node length
 * @lnum: LEB number
 * @dtype: expected data type
 */
static int write_node(void *node, int len, int lnum, int dtype)
{
	prepare_node(node, len);

	memcpy(leb_buf, node, len);

	len = do_pad(leb_buf, len);

	return write_leb(lnum, len, leb_buf, dtype);
}

/**
 * calc_dark - calculate LEB dark space size.
 * @c: the UBIFS file-system description object
 * @spc: amount of free and dirty space in the LEB
 *
 * This function calculates amount of dark space in an LEB which has @spc bytes
 * of free and dirty space. Returns the calculations result.
 *
 * Dark space is the space which is not always usable - it depends on which
 * nodes are written in which order. E.g., if an LEB has only 512 free bytes,
 * it is dark space, because it cannot fit a large data node. So UBIFS cannot
 * count on this LEB and treat these 512 bytes as usable because it is not true
 * if, for example, only big chunks of uncompressible data will be written to
 * the FS.
 */
static int calc_dark(struct ubifs_info *c, int spc)
{
	if (spc < c->dark_wm)
		return spc;

	/*
	 * If we have slightly more space then the dark space watermark, we can
	 * anyway safely assume it we'll be able to write a node of the
	 * smallest size there.
	 */
	if (spc - c->dark_wm < (int)MIN_WRITE_SZ)
		return spc - MIN_WRITE_SZ;

	return c->dark_wm;
}

/**
 * set_lprops - set the LEB property values for a LEB.
 * @lnum: LEB number
 * @offs: end offset of data in the LEB
 * @flags: LEB property flags
 */
static void set_lprops(int lnum, int offs, int flags)
{
	int i = lnum - c->main_first, free, dirty;
	int a = max_t(int, c->min_io_size, 8);

	free = c->leb_size - ALIGN(offs, a);
	dirty = c->leb_size - free - ALIGN(offs, 8);
	dbg_msg("LEB %d free %d dirty %d flags %d", lnum, free, dirty,
		flags);
	c->lpt[i].free = free;
	c->lpt[i].dirty = dirty;
	c->lpt[i].flags = flags;
	c->lst.total_free += free;
	c->lst.total_dirty += dirty;
	if (flags & LPROPS_INDEX)
		c->lst.idx_lebs += 1;
	else {
		int spc;

		spc = free + dirty;
		if (spc < c->dead_wm)
			c->lst.total_dead += spc;
		else
			c->lst.total_dark += calc_dark(c, spc);
		c->lst.total_used += c->leb_size - spc;
	}
}

/**
 * add_to_index - add a node key and position to the index.
 * @key: node key
 * @lnum: node LEB number
 * @offs: node offset
 * @len: node length
 */
static int add_to_index(union ubifs_key *key, char *name, int lnum, int offs,
			int len)
{
	struct idx_entry *e;

	dbg_msg("LEB %d offs %d len %d", lnum, offs, len);
	e = malloc(sizeof(struct idx_entry));
	if (!e)
		return err_msg("out of memory");

	e->next = NULL;
	e->prev = idx_list_last;
	e->key = *key;
	e->name = name;
	e->lnum = lnum;
	e->offs = offs;
	e->len = len;
	if (!idx_list_first)
		idx_list_first = e;
	if (idx_list_last)
		idx_list_last->next = e;
	idx_list_last = e;
	idx_cnt += 1;
	return 0;
}

/**
 * flush_nodes - write the current head and move the head to the next LEB.
 */
static int flush_nodes(void)
{
	int len, err;

	if (!head_offs)
		return 0;
	len = do_pad(leb_buf, head_offs);
	err = write_leb(head_lnum, len, leb_buf, UBI_UNKNOWN);
	if (err)
		return err;
	set_lprops(head_lnum, head_offs, head_flags);
	head_lnum += 1;
	head_offs = 0;

	return 0;
}

/**
 * reserve_space - reserve space for a node on the head.
 * @len: node length
 * @lnum: LEB number is returned here
 * @offs: offset is returned here
 */
static int reserve_space(int len, int *lnum, int *offs)
{
	int err;

	if (len > c->leb_size - head_offs) {
		err = flush_nodes();
		if (err)
			return err;
	}
	*lnum = head_lnum;
	*offs = head_offs;
	head_offs += ALIGN(len, 8);
	return 0;
}

/**
 * add_node - write a node to the head.
 * @key: node key
 * @node: node
 * @len: node length
 */
static int add_node(union ubifs_key *key, char *name, void *node, int len)
{
	int err, lnum, offs;

	prepare_node(node, len);

	err = reserve_space(len, &lnum, &offs);
	if (err)
		return err;

	memcpy(leb_buf + offs, node, len);
	memset(leb_buf + offs + len, 0xff, ALIGN(len, 8) - len);

	add_to_index(key, name, lnum, offs, len);

	return 0;
}

/**
 * add_inode_with_data - write an inode.
 */
static int add_inode_with_data(unsigned long file_size, ino_t inum, void *data,
			       unsigned int data_len)
{
	struct ubifs_ino_node *ino = node_buf;
	union ubifs_key key;
	int len, use_flags = 0;

	creat_sqnum = c->max_sqnum;

	use_flags |= UBIFS_COMPR_FL;

	memset(ino, 0, UBIFS_INO_NODE_SZ);

	ino_key_init(c, &key, inum);
	ino->ch.node_type = UBIFS_INO_NODE;
	key_write(c, &key, &ino->key);
	ino->size       = cpu_to_le64(file_size);

	if (inum == UBIFS_ROOT_INO) {
		ino->creat_sqnum	= cpu_to_le64(DEFAULT_ROOTDIR_SQNUM);
		ino->nlink		= cpu_to_le32(DEFAULT_ROOTDIR_NLINK);
		ino->atime_sec		= cpu_to_le64(DEFAULT_TIME_SEC);
		ino->ctime_sec		= cpu_to_le64(DEFAULT_TIME_SEC);
		ino->mtime_sec		= cpu_to_le64(DEFAULT_TIME_SEC);
		ino->mode		= cpu_to_le32(DEFAULT_ROOTDIR_MODE);
	} else {
		ino->creat_sqnum	= cpu_to_le64(DEFAULT_FILE_SQNUM);
		ino->nlink		= cpu_to_le32(DEFAULT_FILE_NLINK);
		ino->atime_sec		= cpu_to_le64(DEFAULT_TIME_SEC);
		ino->ctime_sec		= cpu_to_le64(DEFAULT_TIME_SEC);
		ino->mtime_sec		= cpu_to_le64(DEFAULT_TIME_SEC);
		ino->uid		= cpu_to_le32(DEFAULT_UID);
		ino->gid		= cpu_to_le32(DEFAULT_GID);
		ino->mode		= cpu_to_le32(DEFAULT_FILE_MODE);
	}

	ino->flags      = cpu_to_le32(use_flags);
	ino->data_len   = cpu_to_le32(data_len);
	ino->compr_type = cpu_to_le16(c->default_compr);
	if (data_len)
		memcpy(&ino->data, data, data_len);

	len = UBIFS_INO_NODE_SZ + data_len;

	return add_node(&key, NULL, ino, len);
}

/**
 * add_inode - write an inode.
 */
static int add_inode(unsigned long file_size, ino_t inum)
{
	return add_inode_with_data(file_size, inum, NULL, 0);
}

/**
 * add_dent_node - write a directory entry node.
 * @dir_inum: target inode number of directory
 * @name: directory entry name
 * @inum: target inode number of the directory entry
 * @type: type of the target inode
 */
static int add_dent_node(ino_t dir_inum, const char *name, ino_t inum,
			 unsigned char type)
{
	struct ubifs_dent_node *dent = node_buf;
	union ubifs_key key;
	struct qstr dname;
	char *kname;
	int len;

	dbg_msg("%s ino %lu type %u dir ino %lu", name, inum,
		(unsigned)type, dir_inum);
	memset(dent, 0, UBIFS_DENT_NODE_SZ);

	dname.name = (void *)name;
	dname.len = strlen(name);

	dent->ch.node_type = UBIFS_DENT_NODE;

	dent_key_init(c, &key, dir_inum, &dname);
	key_write(c, &key, dent->key);
	dent->inum = cpu_to_le64(inum);
	dent->padding1 = 0;
	dent->type = type;
	dent->nlen = cpu_to_le16(dname.len);
	memcpy(dent->name, dname.name, dname.len);
	dent->name[dname.len] = '\0';

	len = UBIFS_DENT_NODE_SZ + dname.len + 1;

	kname = strdup(name);
	if (!kname)
		return err_msg("cannot allocate memory");

	return add_node(&key, kname, dent, len);
}

/**
 * add_data_node - write the data of a file to the output buffer.
 */
static int add_data_node(unsigned long file_size, void *src_addr, ino_t inum)
{
	struct ubifs_data_node *dn = node_buf;
	void *buf = block_buf;
	ssize_t bytes_read, offset;
	union ubifs_key key;
	int dn_len, err;
	unsigned int block_no = 0;
	size_t out_len;

	creat_sqnum = c->max_sqnum++;

	bytes_read = 0;
	offset = 0;

	do {
		memset(buf, 0, UBIFS_BLOCK_SIZE);
		if (offset + UBIFS_BLOCK_SIZE > file_size) {
			memcpy(buf, src_addr + offset, file_size - offset);
			bytes_read = file_size - offset;
		} else {
			memcpy(buf, src_addr + offset, UBIFS_BLOCK_SIZE);
			bytes_read = UBIFS_BLOCK_SIZE;
		}

		/* Make data node */
		memset(dn, 0, UBIFS_DATA_NODE_SZ);
		data_key_init(c, &key, inum, block_no++);
		dn->ch.node_type = UBIFS_DATA_NODE;
		key_write(c, &key, &dn->key);
		dn->size = cpu_to_le32(bytes_read);
		out_len = bytes_read;

		err = lzo1x_1_compress(buf, bytes_read, dn->data,
				       &out_len, lzo_mem);
		if (err != LZO_E_OK)
			return err;
		dn->compr_type = cpu_to_le16(MKFS_UBIFS_COMPR_LZO);

		/* No compression */
		if (out_len > UBIFS_BLOCK_SIZE) {
			memcpy(dn->data, buf, bytes_read);
			out_len = bytes_read;
			dn->compr_type = cpu_to_le16(MKFS_UBIFS_COMPR_NONE);
		}

		dn_len = UBIFS_DATA_NODE_SZ + out_len;

		/* Add data node to file system */
		err = add_node(&key, NULL, dn, dn_len);
		if (err)
			return err;

		offset += bytes_read;
	} while (offset < file_size);

	return err;
}

static int namecmp(const char *name1, const char *name2)
{
	size_t len1 = strlen(name1), len2 = strlen(name2);
	size_t clen = (len1 < len2) ? len1 : len2;
	int cmp;

	cmp = memcmp(name1, name2, clen);
	if (cmp)
		return cmp;
	return (len1 < len2) ? -1 : 1;
}

static int cmp_idx(const void *a, const void *b)
{
	const struct idx_entry *e1 = *(const struct idx_entry **)a;
	const struct idx_entry *e2 = *(const struct idx_entry **)b;
	int cmp;

	cmp = keys_cmp(c, &e1->key, &e2->key);
	if (cmp)
		return cmp;
	return namecmp(e1->name, e2->name);
}

/**
 * add_idx_node - write an index node to the head.
 * @node: index node
 * @child_cnt: number of children of this index node
 */
static int add_idx_node(void *node, int child_cnt)
{
	int err, lnum, offs, len;

	len = ubifs_idx_node_sz(c, child_cnt);

	prepare_node(node, len);

	err = reserve_space(len, &lnum, &offs);
	if (err)
		return err;

	memcpy(leb_buf + offs, node, len);
	memset(leb_buf + offs + len, 0xff, ALIGN(len, 8) - len);

	c->old_idx_sz += ALIGN(len, 8);

	dbg_msg("at %d:%d len %d index size %llu", lnum, offs, len,
		c->old_idx_sz);

	/* The last index node written will be the root */
	c->zroot.lnum = lnum;
	c->zroot.offs = offs;
	c->zroot.len = len;

	return 0;
}

/**
 * write_index - write out the index.
 */
static int write_index(void)
{
	size_t sz, i, cnt, idx_sz, pstep, bcnt;
	struct idx_entry **idx_ptr, **p;
	struct ubifs_idx_node *idx;
	struct ubifs_branch *br;
	int child_cnt, j, level, blnum, boffs, blen, blast_len, err;

	dbg_msg("leaf node count: %zd", idx_cnt);

	/* Reset the head for the index */
	head_flags = LPROPS_INDEX;
	/* Allocate index node */
	idx_sz = ubifs_idx_node_sz(c, c->fanout);
	idx = malloc(idx_sz);
	if (!idx)
		return err_msg("out of memory");

	/* Make an array of pointers to sort the index list */
	sz = idx_cnt * sizeof(struct idx_entry *);
	if (sz / sizeof(struct idx_entry *) != idx_cnt) {
		free(idx);
		return err_msg("index is too big (%zu entries)", idx_cnt);
	}
	idx_ptr = malloc(sz);
	if (!idx_ptr) {
		free(idx);
		return err_msg("out of memory - needed %zu bytes for index",
			       sz);
	}
	idx_ptr[0] = idx_list_first;
	for (i = 1; i < idx_cnt; i++)
		idx_ptr[i] = idx_ptr[i - 1]->next;
	ubifs_qsort(idx_ptr, idx_cnt, sizeof(struct idx_entry *), cmp_idx);
	/* Write level 0 index nodes */
	cnt = idx_cnt / c->fanout;
	if (idx_cnt % c->fanout)
		cnt += 1;
	p = idx_ptr;
	blnum = head_lnum;
	boffs = head_offs;
	for (i = 0; i < cnt; i++) {
		/*
		 * Calculate the child count. All index nodes are created full
		 * except for the last index node on each row.
		 */
		if (i == cnt - 1) {
			child_cnt = idx_cnt % c->fanout;
			if (child_cnt == 0)
				child_cnt = c->fanout;
		} else
			child_cnt = c->fanout;
		memset(idx, 0, idx_sz);
		idx->ch.node_type = UBIFS_IDX_NODE;
		idx->child_cnt = cpu_to_le16(child_cnt);
		idx->level = cpu_to_le16(0);
		for (j = 0; j < child_cnt; j++, p++) {
			br = ubifs_idx_branch(c, idx, j);
			key_write_idx(c, &(*p)->key, &br->key);
			br->lnum = cpu_to_le32((*p)->lnum);
			br->offs = cpu_to_le32((*p)->offs);
			br->len = cpu_to_le32((*p)->len);
		}
		add_idx_node(idx, child_cnt);
	}
	/* Write level 1 index nodes and above */
	level = 0;
	pstep = 1;
	while (cnt > 1) {
		/*
		 * 'blast_len' is the length of the last index node in the level
		 * below.
		 */
		blast_len = ubifs_idx_node_sz(c, child_cnt);
		/* 'bcnt' is the number of index nodes in the level below */
		bcnt = cnt;
		/* 'cnt' is the number of index nodes in this level */
		cnt = (cnt + c->fanout - 1) / c->fanout;
		if (cnt == 0)
			cnt = 1;
		level += 1;
		/*
		 * The key of an index node is the same as the key of its first
		 * child. Thus we can get the key by stepping along the bottom
		 * level 'p' with an increasing large step 'pstep'.
		 */
		p = idx_ptr;
		pstep *= c->fanout;
		for (i = 0; i < cnt; i++) {
			/*
			 * Calculate the child count. All index nodes are
			 * created full except for the last index node on each
			 * row.
			 */
			if (i == cnt - 1) {
				child_cnt = bcnt % c->fanout;
				if (child_cnt == 0)
					child_cnt = c->fanout;
			} else
				child_cnt = c->fanout;
			memset(idx, 0, idx_sz);
			idx->ch.node_type = UBIFS_IDX_NODE;
			idx->child_cnt = cpu_to_le16(child_cnt);
			idx->level = cpu_to_le16(level);
			for (j = 0; j < child_cnt; j++) {
				size_t bn = i * c->fanout + j;

				/*
				 * The length of the index node in the level
				 * below is 'idx_sz' except when it is the last
				 * node on the row. i.e. all the others on the
				 * row are full.
				 */
				if (bn == bcnt - 1)
					blen = blast_len;
				else
					blen = idx_sz;
				/*
				 * 'blnum' and 'boffs' hold the position of the
				 * index node on the level below.
				 */
				if (boffs + blen > c->leb_size) {
					blnum += 1;
					boffs = 0;
				}
				/*
				 * Fill in the branch with the key and position
				 * of the index node from the level below.
				 */
				br = ubifs_idx_branch(c, idx, j);
				key_write_idx(c, &(*p)->key, &br->key);
				br->lnum = cpu_to_le32(blnum);
				br->offs = cpu_to_le32(boffs);
				br->len = cpu_to_le32(blen);
				/*
				 * Step to the next index node on the level
				 * below.
				 */
				boffs += ALIGN(blen, 8);
				p += pstep;
			}
			add_idx_node(idx, child_cnt);
		}
	}

	/* Free stuff */
	for (i = 0; i < idx_cnt; i++)
		free(idx_ptr[i]);
	free(idx_ptr);
	free(idx);

	dbg_msg("zroot is at %d:%d len %d", c->zroot.lnum, c->zroot.offs,
		c->zroot.len);

	/* Set the index head */
	c->ihead_lnum = head_lnum;
	c->ihead_offs = ALIGN(head_offs, c->min_io_size);
	dbg_msg("ihead is at %d:%d", c->ihead_lnum, c->ihead_offs);

	/* Flush the last index LEB */
	err = flush_nodes();
	if (err)
		return err;

	return 0;
}

/**
 * set_gc_lnum - set the LEB number reserved for the garbage collector.
 */
static int set_gc_lnum(void)
{
	int err;

	c->gc_lnum = head_lnum++;

	err = write_empty_leb(c->gc_lnum, UBI_LONGTERM);
	if (err)
		return err;
	set_lprops(c->gc_lnum, 0, 0);
	c->lst.empty_lebs += 1;
	return 0;
}

/**
 * finalize_leb_cnt - now that we know how many LEBs we used.
 */
static int finalize_leb_cnt(void)
{
	c->leb_cnt = head_lnum;
	if (c->leb_cnt > c->max_leb_cnt)
		/* TODO: in this case it segfaults because buffer overruns - we
		 * somewhere allocate smaller buffers - fix */
		return err_msg("max_leb_cnt too low (%d needed)", c->leb_cnt);
	c->main_lebs = c->leb_cnt - c->main_first;

	c->max_bud_bytes = c->leb_size * (c->main_lebs - 1);

	return 0;
}

/**
 * write_super - write the super block.
 */
static int write_super(void)
{
	struct ubifs_sb_node sup;
	int i;

	memset(&sup, 0, UBIFS_SB_NODE_SZ);

	sup.ch.node_type  = UBIFS_SB_NODE;
	sup.key_hash      = c->key_hash_type;
	sup.min_io_size   = cpu_to_le32(c->min_io_size);
	sup.leb_size      = cpu_to_le32(c->leb_size);
	sup.leb_cnt       = cpu_to_le32(c->leb_cnt);
	sup.max_leb_cnt   = cpu_to_le32(c->max_leb_cnt);
	sup.max_bud_bytes = cpu_to_le64(c->max_bud_bytes);
	sup.log_lebs      = cpu_to_le32(c->log_lebs);
	sup.lpt_lebs      = cpu_to_le32(c->lpt_lebs);
	sup.orph_lebs     = cpu_to_le32(c->orph_lebs);
	sup.jhead_cnt     = cpu_to_le32(c->jhead_cnt);
	sup.fanout        = cpu_to_le32(c->fanout);
	sup.lsave_cnt     = cpu_to_le32(c->lsave_cnt);
	sup.fmt_version   = cpu_to_le32(UBIFS_FORMAT_VERSION);
	sup.default_compr = cpu_to_le16(c->default_compr);
	sup.rp_size       = cpu_to_le64(c->rp_size);
	sup.time_gran     = cpu_to_le32(DEFAULT_TIME_GRAN);

	for (i = 0; i < 16; i++)
		sup.uuid[i]	= 1;

	if (c->big_lpt)
		sup.flags |= cpu_to_le32(UBIFS_FLG_BIGLPT);

	return write_node(&sup, UBIFS_SB_NODE_SZ, UBIFS_SB_LNUM, UBI_LONGTERM);
}

/**
 * write_master - write the master node.
 */
static int write_master(void)
{
	struct ubifs_mst_node mst;
	int err;

	memset(&mst, 0, UBIFS_MST_NODE_SZ);

	mst.ch.node_type = UBIFS_MST_NODE;
	mst.log_lnum     = cpu_to_le32(UBIFS_LOG_LNUM);
	mst.highest_inum = cpu_to_le64(c->highest_inum);
	mst.cmt_no       = cpu_to_le64(0);
	mst.flags        = cpu_to_le32(UBIFS_MST_NO_ORPHS);
	mst.root_lnum    = cpu_to_le32(c->zroot.lnum);
	mst.root_offs    = cpu_to_le32(c->zroot.offs);
	mst.root_len     = cpu_to_le32(c->zroot.len);
	mst.gc_lnum      = cpu_to_le32(c->gc_lnum);
	mst.ihead_lnum   = cpu_to_le32(c->ihead_lnum);
	mst.ihead_offs   = cpu_to_le32(c->ihead_offs);
	mst.index_size   = cpu_to_le64(c->old_idx_sz);
	mst.lpt_lnum     = cpu_to_le32(c->lpt_lnum);
	mst.lpt_offs     = cpu_to_le32(c->lpt_offs);
	mst.nhead_lnum   = cpu_to_le32(c->nhead_lnum);
	mst.nhead_offs   = cpu_to_le32(c->nhead_offs);
	mst.ltab_lnum    = cpu_to_le32(c->ltab_lnum);
	mst.ltab_offs    = cpu_to_le32(c->ltab_offs);
	mst.lsave_lnum   = cpu_to_le32(c->lsave_lnum);
	mst.lsave_offs   = cpu_to_le32(c->lsave_offs);
	mst.lscan_lnum   = cpu_to_le32(c->lscan_lnum);
	mst.empty_lebs   = cpu_to_le32(c->lst.empty_lebs);
	mst.idx_lebs     = cpu_to_le32(c->lst.idx_lebs);
	mst.total_free   = cpu_to_le64(c->lst.total_free);
	mst.total_dirty  = cpu_to_le64(c->lst.total_dirty);
	mst.total_used   = cpu_to_le64(c->lst.total_used);
	mst.total_dead   = cpu_to_le64(c->lst.total_dead);
	mst.total_dark   = cpu_to_le64(c->lst.total_dark);
	mst.leb_cnt      = cpu_to_le32(c->leb_cnt);

	err = write_node(&mst, UBIFS_MST_NODE_SZ, UBIFS_MST_LNUM,
			 UBI_SHORTTERM);
	if (err)
		return err;

	err = write_node(&mst, UBIFS_MST_NODE_SZ, UBIFS_MST_LNUM + 1,
			 UBI_SHORTTERM);
	if (err)
		return err;

	return 0;
}

/**
 * write_log - write an empty log.
 */
static int write_log(void)
{
	struct ubifs_cs_node cs;
	int err, i, lnum;

	lnum = UBIFS_LOG_LNUM;

	cs.ch.node_type = UBIFS_CS_NODE;
	cs.cmt_no = cpu_to_le64(0);

	err = write_node(&cs, UBIFS_CS_NODE_SZ, lnum, UBI_UNKNOWN);
	if (err)
		return err;

	lnum += 1;

	for (i = 1; i < c->log_lebs; i++, lnum++) {
		err = write_empty_leb(lnum, UBI_UNKNOWN);
		if (err)
			return err;
	}

	return 0;
}


/**
 * pack_bits - pack bit fields end-to-end.
 * @addr: address at which to pack (passed and next address returned)
 * @pos: bit position at which to pack (passed and next position returned)
 * @val: value to pack
 * @nrbits: number of bits of value to pack (1-32)
 */
static void pack_bits(uint8_t **addr, int *pos, uint32_t val, int nrbits)
{
	uint8_t *p = *addr;
	int b = *pos;

	if (b) {
		*p |= ((uint8_t)val) << b;
		nrbits += b;
		if (nrbits > 8) {
			*++p = (uint8_t)(val >>= (8 - b));
			if (nrbits > 16) {
				*++p = (uint8_t)(val >>= 8);
				if (nrbits > 24) {
					*++p = (uint8_t)(val >>= 8);
					if (nrbits > 32)
						*++p = (uint8_t)(val >>= 8);
				}
			}
		}
	} else {
		*p = (uint8_t)val;
		if (nrbits > 8) {
			*++p = (uint8_t)(val >>= 8);
			if (nrbits > 16) {
				*++p = (uint8_t)(val >>= 8);
				if (nrbits > 24)
					*++p = (uint8_t)(val >>= 8);
			}
		}
	}
	b = nrbits & 7;
	if (b == 0)
		p++;
	*addr = p;
	*pos = b;
}

/**
 * pack_pnode - pack all the bit fields of a pnode.
 * @c: UBIFS file-system description object
 * @buf: buffer into which to pack
 * @pnode: pnode to pack
 */
static void pack_pnode(struct ubifs_info *c, void *buf,
		       struct ubifs_pnode *pnode)
{
	uint8_t *addr = buf + UBIFS_LPT_CRC_BYTES;
	int i, pos = 0;
	uint16_t crc;

	pack_bits(&addr, &pos, UBIFS_LPT_PNODE, UBIFS_LPT_TYPE_BITS);
	if (c->big_lpt)
		pack_bits(&addr, &pos, pnode->num, c->pcnt_bits);
	for (i = 0; i < UBIFS_LPT_FANOUT; i++) {
		pack_bits(&addr, &pos, pnode->lprops[i].free >> 3,
			  c->space_bits);
		pack_bits(&addr, &pos, pnode->lprops[i].dirty >> 3,
			  c->space_bits);
		if (pnode->lprops[i].flags & LPROPS_INDEX)
			pack_bits(&addr, &pos, 1, 1);
		else
			pack_bits(&addr, &pos, 0, 1);
	}
	crc = crc16(-1, buf + UBIFS_LPT_CRC_BYTES,
		    c->pnode_sz - UBIFS_LPT_CRC_BYTES);
	addr = buf;
	pos = 0;
	pack_bits(&addr, &pos, crc, UBIFS_LPT_CRC_BITS);
}

/**
 * calc_nnode_num - calculate nnode number.
 * @row: the row in the tree (root is zero)
 * @col: the column in the row (leftmost is zero)
 *
 * The nnode number is a number that uniquely identifies a nnode and can be used
 * easily to traverse the tree from the root to that nnode.
 *
 * This function calculates and returns the nnode number for the nnode at @row
 * and @col.
 */
static int calc_nnode_num(int row, int col)
{
	int num, bits;

	num = 1;
	while (row--) {
		bits = (col & (UBIFS_LPT_FANOUT - 1));
		col >>= UBIFS_LPT_FANOUT_SHIFT;
		num <<= UBIFS_LPT_FANOUT_SHIFT;
		num |= bits;
	}
	return num;
}

/**
 * pack_nnode - pack all the bit fields of a nnode.
 * @c: UBIFS file-system description object
 * @buf: buffer into which to pack
 * @nnode: nnode to pack
 */
static void pack_nnode(struct ubifs_info *c, void *buf,
		       struct ubifs_nnode *nnode)
{
	uint8_t *addr = buf + UBIFS_LPT_CRC_BYTES;
	int i, pos = 0;
	uint16_t crc;

	pack_bits(&addr, &pos, UBIFS_LPT_NNODE, UBIFS_LPT_TYPE_BITS);
	if (c->big_lpt)
		pack_bits(&addr, &pos, nnode->num, c->pcnt_bits);
	for (i = 0; i < UBIFS_LPT_FANOUT; i++) {
		int lnum = nnode->nbranch[i].lnum;

		if (lnum == 0)
			lnum = c->lpt_last + 1;
		pack_bits(&addr, &pos, lnum - c->lpt_first, c->lpt_lnum_bits);
		pack_bits(&addr, &pos, nnode->nbranch[i].offs,
			  c->lpt_offs_bits);
	}
	crc = crc16(-1, buf + UBIFS_LPT_CRC_BYTES,
		    c->nnode_sz - UBIFS_LPT_CRC_BYTES);
	addr = buf;
	pos = 0;
	pack_bits(&addr, &pos, crc, UBIFS_LPT_CRC_BITS);
}

/**
 * pack_lsave - pack the LPT's save table.
 * @c: UBIFS file-system description object
 * @buf: buffer into which to pack
 * @lsave: LPT's save table to pack
 */
static void pack_lsave(struct ubifs_info *c, void *buf, int *lsave)
{
	uint8_t *addr = buf + UBIFS_LPT_CRC_BYTES;
	int i, pos = 0;
	uint16_t crc;

	pack_bits(&addr, &pos, UBIFS_LPT_LSAVE, UBIFS_LPT_TYPE_BITS);
	for (i = 0; i < c->lsave_cnt; i++)
		pack_bits(&addr, &pos, lsave[i], c->lnum_bits);
	crc = crc16(-1, buf + UBIFS_LPT_CRC_BYTES,
		    c->lsave_sz - UBIFS_LPT_CRC_BYTES);
	addr = buf;
	pos = 0;
	pack_bits(&addr, &pos, crc, UBIFS_LPT_CRC_BITS);
}

/**
 * set_ltab - set LPT LEB properties.
 * @c: UBIFS file-system description object
 * @lnum: LEB number
 * @free: amount of free space
 * @dirty: amount of dirty space
 */
static void set_ltab(struct ubifs_info *c, int lnum, int free, int dirty)
{
	dbg_msg("LEB %d free %d dirty %d to %d %d",
		lnum, c->ltab[lnum - c->lpt_first].free,
		c->ltab[lnum - c->lpt_first].dirty, free, dirty);
	c->ltab[lnum - c->lpt_first].free = free;
	c->ltab[lnum - c->lpt_first].dirty = dirty;
}

/**
 * pack_ltab - pack the LPT's own lprops table.
 * @c: UBIFS file-system description object
 * @buf: buffer into which to pack
 * @ltab: LPT's own lprops table to pack
 */
static void pack_ltab(struct ubifs_info *c, void *buf,
			 struct ubifs_lpt_lprops *ltab)
{
	uint8_t *addr = buf + UBIFS_LPT_CRC_BYTES;
	int i, pos = 0;
	uint16_t crc;

	pack_bits(&addr, &pos, UBIFS_LPT_LTAB, UBIFS_LPT_TYPE_BITS);
	for (i = 0; i < c->lpt_lebs; i++) {
		pack_bits(&addr, &pos, ltab[i].free, c->lpt_spc_bits);
		pack_bits(&addr, &pos, ltab[i].dirty, c->lpt_spc_bits);
	}
	crc = crc16(-1, buf + UBIFS_LPT_CRC_BYTES,
		    c->ltab_sz - UBIFS_LPT_CRC_BYTES);
	addr = buf;
	pos = 0;
	pack_bits(&addr, &pos, crc, UBIFS_LPT_CRC_BITS);
}

/**
 * create_lpt - create LPT.
 * @c: UBIFS file-system description object
 *
 * This function returns %0 on success and a negative error code on failure.
 */
int create_lpt(struct ubifs_info *c)
{
	int lnum, err = 0, i, j, cnt, len, alen, row;
	int blnum, boffs, bsz, bcnt;
	struct ubifs_pnode *pnode = NULL;
	struct ubifs_nnode *nnode = NULL;
	void *buf = NULL, *p;
	int *lsave = NULL;

	pnode = malloc(sizeof(struct ubifs_pnode));
	nnode = malloc(sizeof(struct ubifs_nnode));
	buf = malloc(c->leb_size);
	lsave = malloc(sizeof(int) * c->lsave_cnt);
	if (!pnode || !nnode || !buf || !lsave) {
		err = -ENOMEM;
		goto out;
	}
	memset(pnode, 0 , sizeof(struct ubifs_pnode));
	memset(nnode, 0 , sizeof(struct ubifs_pnode));

	c->lscan_lnum = c->main_first;

	lnum = c->lpt_first;
	p = buf;
	len = 0;
	/* Number of leaf nodes (pnodes) */
	cnt = (c->main_lebs + UBIFS_LPT_FANOUT - 1) >> UBIFS_LPT_FANOUT_SHIFT;
	//printf("pnode_cnt=%d\n",cnt);

	/*
	 * To calculate the internal node branches, we keep information about
	 * the level below.
	 */
	blnum = lnum; /* LEB number of level below */
	boffs = 0; /* Offset of level below */
	bcnt = cnt; /* Number of nodes in level below */
	bsz = c->pnode_sz; /* Size of nodes in level below */

	/* Add pnodes */
	for (i = 0; i < cnt; i++) {
		if (len + c->pnode_sz > c->leb_size) {
			alen = ALIGN(len, c->min_io_size);
			set_ltab(c, lnum, c->leb_size - alen, alen - len);
			memset(p, 0xff, alen - len);
			err = write_leb(lnum++, alen, buf, UBI_SHORTTERM);
			if (err)
				goto out;
			p = buf;
			len = 0;
		}
		/* Fill in the pnode */
		for (j = 0; j < UBIFS_LPT_FANOUT; j++) {
			int k = (i << UBIFS_LPT_FANOUT_SHIFT) + j;

			if (k < c->main_lebs)
				pnode->lprops[j] = c->lpt[k];
			else {
				pnode->lprops[j].free = c->leb_size;
				pnode->lprops[j].dirty = 0;
				pnode->lprops[j].flags = 0;
			}
		}
		pack_pnode(c, p, pnode);
		p += c->pnode_sz;
		len += c->pnode_sz;
		/*
		 * pnodes are simply numbered left to right starting at zero,
		 * which means the pnode number can be used easily to traverse
		 * down the tree to the corresponding pnode.
		 */
		pnode->num += 1;
	}

	row = c->lpt_hght - 1;
	/* Add all nnodes, one level at a time */
	while (1) {
		/* Number of internal nodes (nnodes) at next level */
		cnt = (cnt + UBIFS_LPT_FANOUT - 1) / UBIFS_LPT_FANOUT;
		if (cnt == 0)
			cnt = 1;
		for (i = 0; i < cnt; i++) {
			if (len + c->nnode_sz > c->leb_size) {
				alen = ALIGN(len, c->min_io_size);
				set_ltab(c, lnum, c->leb_size - alen,
					    alen - len);
				memset(p, 0xff, alen - len);
				err = write_leb(lnum++, alen, buf, UBI_SHORTTERM);
				if (err)
					goto out;
				p = buf;
				len = 0;
			}
			/* The root is on row zero */
			if (row == 0) {
				c->lpt_lnum = lnum;
				c->lpt_offs = len;
			}
			/* Set branches to the level below */
			for (j = 0; j < UBIFS_LPT_FANOUT; j++) {
				if (bcnt) {
					if (boffs + bsz > c->leb_size) {
						blnum += 1;
						boffs = 0;
					}
					nnode->nbranch[j].lnum = blnum;
					nnode->nbranch[j].offs = boffs;
					boffs += bsz;
					bcnt--;
				} else {
					nnode->nbranch[j].lnum = 0;
					nnode->nbranch[j].offs = 0;
				}
			}
			nnode->num = calc_nnode_num(row, i);
			pack_nnode(c, p, nnode);
			p += c->nnode_sz;
			len += c->nnode_sz;
		}
		/* Row zero  is the top row */
		if (row == 0)
			break;
		/* Update the information about the level below */
		bcnt = cnt;
		bsz = c->nnode_sz;
		row -= 1;
	}

	if (c->big_lpt) {
		/* Need to add LPT's save table */
		if (len + c->lsave_sz > c->leb_size) {
			alen = ALIGN(len, c->min_io_size);
			set_ltab(c, lnum, c->leb_size - alen, alen - len);
			memset(p, 0xff, alen - len);
			err = write_leb(lnum++, alen, buf, UBI_SHORTTERM);
			if (err)
				goto out;
			p = buf;
			len = 0;
		}

		c->lsave_lnum = lnum;
		c->lsave_offs = len;

		for (i = 0; i < c->lsave_cnt; i++)
			lsave[i] = c->main_first + i;

		pack_lsave(c, p, lsave);
		p += c->lsave_sz;
		len += c->lsave_sz;
	}

	/* Need to add LPT's own LEB properties table */
	if (len + c->ltab_sz > c->leb_size) {
		alen = ALIGN(len, c->min_io_size);
		set_ltab(c, lnum, c->leb_size - alen, alen - len);
		memset(p, 0xff, alen - len);
		err = write_leb(lnum++, alen, buf, UBI_SHORTTERM);
		if (err)
			goto out;
		p = buf;
		len = 0;
	}

	c->ltab_lnum = lnum;
	c->ltab_offs = len;

	/* Update ltab before packing it */
	len += c->ltab_sz;
	alen = ALIGN(len, c->min_io_size);
	set_ltab(c, lnum, c->leb_size - alen, alen - len);

	pack_ltab(c, p, c->ltab);
	p += c->ltab_sz;

	/* Write remaining buffer */
	memset(p, 0xff, alen - len);
	err = write_leb(lnum, alen, buf, UBI_SHORTTERM);
	if (err)
		goto out;

	c->nhead_lnum = lnum;
	c->nhead_offs = ALIGN(len, c->min_io_size);

out:
	return err;
}

/**
 * write_lpt - write the LEB properties tree.
 */
static int write_lpt(void)
{
	int err, lnum;

	err = create_lpt(c);
	if (err)
		return err;

	lnum = c->nhead_lnum + 1;
	while (lnum <= c->lpt_last) {
		err = write_empty_leb(lnum++, UBI_SHORTTERM);
		if (err)
			return err;
	}

	return 0;
}

/**
 * write_orphan_area - write an empty orphan area.
 */
static int write_orphan_area(void)
{
	int err, i, lnum;

	lnum = UBIFS_LOG_LNUM + c->log_lebs + c->lpt_lebs;
	for (i = 0; i < c->orph_lebs; i++, lnum++) {
		err = write_empty_leb(lnum, UBI_SHORTTERM);
		if (err)
			return err;
	}
	return 0;
}

/**
 * do_calc_lpt_geom - calculate sizes for the LPT area.
 * @c: the UBIFS file-system description object
 *
 * Calculate the sizes of LPT bit fields, nodes, and tree, based on the
 * properties of the flash and whether LPT is "big" (c->big_lpt).
 */
static void do_calc_lpt_geom(struct ubifs_info *c)
{
	int n, bits, per_leb_wastage;
	long long sz, tot_wastage;

	c->pnode_cnt = (c->main_lebs + UBIFS_LPT_FANOUT - 1) / UBIFS_LPT_FANOUT;

	n = (c->pnode_cnt + UBIFS_LPT_FANOUT - 1) / UBIFS_LPT_FANOUT;
	c->nnode_cnt = n;
	while (n > 1) {
		n = (n + UBIFS_LPT_FANOUT - 1) / UBIFS_LPT_FANOUT;
		c->nnode_cnt += n;
	}

	c->lpt_hght = 1;
	n = UBIFS_LPT_FANOUT;
	while (n < c->pnode_cnt) {
		c->lpt_hght += 1;
		n <<= UBIFS_LPT_FANOUT_SHIFT;
	}

	c->space_bits = fls(c->leb_size) - 3;
	c->lpt_lnum_bits = fls(c->lpt_lebs);
	c->lpt_offs_bits = fls(c->leb_size - 1);
	c->lpt_spc_bits = fls(c->leb_size);

	n = (c->max_leb_cnt + UBIFS_LPT_FANOUT - 1) / UBIFS_LPT_FANOUT;
	c->pcnt_bits = fls(n - 1);

	c->lnum_bits = fls(c->max_leb_cnt - 1);

	bits = UBIFS_LPT_CRC_BITS + UBIFS_LPT_TYPE_BITS +
	       (c->big_lpt ? c->pcnt_bits : 0) +
	       (c->space_bits * 2 + 1) * UBIFS_LPT_FANOUT;
	c->pnode_sz = (bits + 7) / 8;

	bits = UBIFS_LPT_CRC_BITS + UBIFS_LPT_TYPE_BITS +
	       (c->big_lpt ? c->pcnt_bits : 0) +
	       (c->lpt_lnum_bits + c->lpt_offs_bits) * UBIFS_LPT_FANOUT;
	c->nnode_sz = (bits + 7) / 8;

	bits = UBIFS_LPT_CRC_BITS + UBIFS_LPT_TYPE_BITS +
	       c->lpt_lebs * c->lpt_spc_bits * 2;
	c->ltab_sz = (bits + 7) / 8;

	bits = UBIFS_LPT_CRC_BITS + UBIFS_LPT_TYPE_BITS +
	       c->lnum_bits * c->lsave_cnt;
	c->lsave_sz = (bits + 7) / 8;

	/* Calculate the minimum LPT size */
	c->lpt_sz = (long long)c->pnode_cnt * c->pnode_sz;
	c->lpt_sz += (long long)c->nnode_cnt * c->nnode_sz;
	c->lpt_sz += c->ltab_sz;
	c->lpt_sz += c->lsave_sz;

	/* Add wastage */
	sz = c->lpt_sz;
	per_leb_wastage = max_t(int, c->pnode_sz, c->nnode_sz);
	sz += per_leb_wastage;
	tot_wastage = per_leb_wastage;
	while (sz > c->leb_size) {
		sz += per_leb_wastage;
		sz -= c->leb_size;
		tot_wastage += per_leb_wastage;
	}
	tot_wastage += ALIGN(sz, c->min_io_size) - sz;
	c->lpt_sz += tot_wastage;
}

/**
 * calc_dflt_lpt_geom - calculate default LPT geometry.
 * @c: the UBIFS file-system description object
 * @main_lebs: number of main area LEBs is passed and returned here
 * @big_lpt: whether the LPT area is "big" is returned here
 *
 * The size of the LPT area depends on parameters that themselves are dependent
 * on the size of the LPT area. This function, successively recalculates the LPT
 * area geometry until the parameters and resultant geometry are consistent.
 *
 * This function returns %0 on success and a negative error code on failure.
 */
int calc_dflt_lpt_geom(struct ubifs_info *c, int *main_lebs, int *big_lpt)
{
	int i, lebs_needed;
	uint64_t sz;

	/* Start by assuming the minimum number of LPT LEBs */
	c->lpt_lebs = UBIFS_MIN_LPT_LEBS;
	c->main_lebs = *main_lebs - c->lpt_lebs;
	if (c->main_lebs <= 0)
		return -EINVAL;

	/* And assume we will use the small LPT model */
	c->big_lpt = 0;

	/*
	 * Calculate the geometry based on assumptions above and then see if it
	 * makes sense
	 */
	do_calc_lpt_geom(c);

	/* Small LPT model must have lpt_sz < leb_size */
	if (c->lpt_sz > c->leb_size) {
		/* Nope, so try again using big LPT model */
		c->big_lpt = 1;
		do_calc_lpt_geom(c);
	}

	/* Now check there are enough LPT LEBs */
	for (i = 0; i < 64 ; i++) {
		sz = c->lpt_sz * 4; /* Allow 4 times the size */
		sz += c->leb_size - 1;
		do_div(sz, c->leb_size);
		lebs_needed = sz;
		if (lebs_needed > c->lpt_lebs) {
			/* Not enough LPT LEBs so try again with more */
			c->lpt_lebs = lebs_needed;
			c->main_lebs = *main_lebs - c->lpt_lebs;
			if (c->main_lebs <= 0)
				return -EINVAL;
			do_calc_lpt_geom(c);
			continue;
		}
		if (c->ltab_sz > c->leb_size) {
			err_msg("LPT ltab too big");
			return -EINVAL;
		}
		*main_lebs = c->main_lebs;
		*big_lpt = c->big_lpt;
		return 0;
	}
	return -EINVAL;
}

int init_compression(void)
{
	lzo_mem = malloc(LZO1X_MEM_COMPRESS);
	if (!lzo_mem)
		return -1;

	return 0;
}

/**
 * init - initialize things.
 */
static int init(void)
{
	int err, i, main_lebs, big_lpt = 0;

	c->highest_inum = UBIFS_FIRST_INO;

	c->jhead_cnt = 1;

	main_lebs = c->max_leb_cnt - UBIFS_SB_LEBS - UBIFS_MST_LEBS;
	main_lebs -= c->log_lebs + c->orph_lebs;

	err = calc_dflt_lpt_geom(c, &main_lebs, &big_lpt);
	if (err)
		return err;

	c->main_first = UBIFS_LOG_LNUM + c->log_lebs + c->lpt_lebs +
			c->orph_lebs;
	head_lnum = c->main_first;
	head_offs = 0;

	c->lpt_first = UBIFS_LOG_LNUM + c->log_lebs;
	c->lpt_last = c->lpt_first + c->lpt_lebs - 1;

	c->lpt = malloc(c->main_lebs * sizeof(struct ubifs_lprops));
	if (!c->lpt)
		return err_msg("unable to allocate LPT");

	c->ltab = malloc(c->lpt_lebs * sizeof(struct ubifs_lprops));
	if (!c->ltab)
		return err_msg("unable to allocate LPT ltab");

	/* Initialize LPT's own lprops */
	for (i = 0; i < c->lpt_lebs; i++) {
		c->ltab[i].free = c->leb_size;
		c->ltab[i].dirty = 0;
	}

	c->dead_wm = ALIGN(MIN_WRITE_SZ, c->min_io_size);
	c->dark_wm = ALIGN(UBIFS_MAX_NODE_SZ, c->min_io_size);
	dbg_msg("dead_wm %d  dark_wm %d", c->dead_wm, c->dark_wm);

	leb_buf = malloc(c->leb_size);
	if (!leb_buf)
		return err_msg("out of memory");

	node_buf = malloc(NODE_BUFFER_SIZE);
	if (!node_buf)
		return err_msg("out of memory");

	block_buf = malloc(UBIFS_BLOCK_SIZE);
	if (!block_buf)
		return err_msg("out of memory");

	err = init_compression();
	if (err)
		return err;

	return 0;
}

/**
 * deinit - deinitialize things.
 */
static void deinit(void)
{
	free(c->lpt);
	free(c->ltab);
	free(leb_buf);
	free(node_buf);
	free(block_buf);
	free(lzo_mem);
}

/**
 * mkfs - make the file system.
 *
 * Each on-flash area has a corresponding function to create it. The order of
 * the functions reflects what information must be known to complete each stage.
 * As a consequence the output file is not written sequentially. No effort has
 * been made to make efficient use of memory or to allow for the possibility of
 * incremental updates to the output file.
 */
int mkfs(void *src_addr, unsigned long src_size,
	 void *dest_addr, unsigned long *dest_size,
	 int min_io_size, int leb_size, int max_leb_cnt)
{
	int err = 0;
	ino_t inum;
	ino_t dir_inum = UBIFS_ROOT_INO;
	unsigned char type = UBIFS_ITYPE_REG;
	char *name = "modem.bin";
	unsigned long root_dir_size = UBIFS_INO_NODE_SZ;

	modem_buf = dest_addr;
	memset(modem_buf, 0, MODEM_IMAGE_SIZE);

	err = set_options(min_io_size, leb_size, max_leb_cnt);
	if (err)
		return err;

	err = init();
	if (err)
		goto out;

	c->highest_inum++;
	inum = c->highest_inum;
	creat_sqnum = c->max_sqnum++;

	err = add_data_node(src_size, src_addr, inum);
	if (err)
		goto out;

	err = add_inode(src_size, inum);
	if (err)
		goto out;

	err = add_dent_node(dir_inum, name, inum, type);
	if (err)
		goto out;

	root_dir_size += ALIGN(UBIFS_DENT_NODE_SZ + strlen(name) + 1, 8);

	err = add_inode(root_dir_size, dir_inum);
	if (err)
		goto out;

	err = flush_nodes();
	if (err)
		goto out;

	err = set_gc_lnum();
	if (err)
		goto out;

	err = write_index();
	if (err)
		goto out;

	err = finalize_leb_cnt();
	if (err)
		goto out;

	err = write_lpt();
	if (err)
		goto out;

	err = write_super();
	if (err)
		goto out;

	err = write_master();
	if (err)
		goto out;

	err = write_log();
	if (err)
		goto out;

	err = write_orphan_area();

	*dest_size = (unsigned long) c->leb_cnt * c->leb_size;

	printf("ubifs image offset : 0x%x\n", (unsigned int) dest_addr);
	printf("ubifs image size : 0x%lx\n", *dest_size);

out:
	deinit();
	return err;
}
