/*
 * Copyright (C) 2008 Nokia Corporation
 * Copyright (c) International Business Machines Corp., 2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Generate UBI images.
 *
 * Authors: Artem Bityutskiy
 *          Oliver Lohmann
 */

#include "ubi-media.h"
#include "libubigen.h"
#include <linux/crc32.h>
#include <linux/string.h>
#include <malloc.h>

static int set_volume_info(const struct ubigen_info *ui, struct ubigen_vol_info *vi,
			int id, int type, long long bytes,
			const char *name, int alignment)
{
	vi->id = id;
	vi->type = type;
	vi->bytes = bytes;
	vi->name = name;
	vi->name_len = strlen(name);
	vi->alignment = alignment;

	/* Initialize the rest of the volume information */
	vi->data_pad = ui->leb_size % vi->alignment;
	vi->usable_leb_size = ui->leb_size - vi->data_pad;
	vi->used_ebs = (vi->bytes + vi->usable_leb_size - 1) / vi->usable_leb_size;
	vi->compat = 0;
	return 0;
}

/**
 * ubigen_create_empty_vtbl - creates empty volume table.
 *
 * This function creates an empty volume table and returns a pointer to it in
 * case of success and %NULL in case of failure. The returned object has to be
 * freed with 'free()' call.
 */
static struct ubi_vtbl_record *ubigen_create_empty_vtbl(const struct ubigen_info *ui)
{
	struct ubi_vtbl_record *vtbl;
	int i;

	vtbl = calloc(1, ui->vtbl_size);
	if (!vtbl)
		return NULL;

	for (i = 0; i < ui->max_volumes; i++) {
		uint32_t crc = crc32(UBI_CRC32_INIT, &vtbl[i],
				     UBI_VTBL_RECORD_SIZE_CRC);
		vtbl[i].crc = cpu_to_be32(crc);
	}

	return vtbl;
}

/**
 * ubigen_info_init - initialize libubigen.
 * @ui: libubigen information
 * @peb_size: flash physical eraseblock size
 * @min_io_size: flash minimum input/output unit size
 * @subpage_size: flash sub-page, if present (has to be equivalent to
 *                @min_io_size if does not exist)
 * @vid_hdr_offs: offset of the VID header
 * @ubi_ver: UBI version
 * @image_seq: UBI image sequence number
 */
static void ubigen_info_init(struct ubigen_info *ui, int peb_size, int min_io_size,
		      int subpage_size, int vid_hdr_offs, int ubi_ver,
		      uint32_t image_seq)
{
	if (!vid_hdr_offs) {
		vid_hdr_offs = UBI_EC_HDR_SIZE + subpage_size - 1;
		vid_hdr_offs /= subpage_size;
		vid_hdr_offs *= subpage_size;
	}

	ui->peb_size = peb_size;
	ui->min_io_size = min_io_size;
	ui->vid_hdr_offs = vid_hdr_offs;
	ui->data_offs = vid_hdr_offs + UBI_VID_HDR_SIZE + min_io_size - 1;
	ui->data_offs /= min_io_size;
	ui->data_offs *= min_io_size;
	ui->leb_size = peb_size - ui->data_offs;
	ui->ubi_ver = ubi_ver;
	ui->image_seq = image_seq;

	ui->max_volumes = ui->leb_size / UBI_VTBL_RECORD_SIZE;
	if (ui->max_volumes > UBI_MAX_VOLUMES)
		ui->max_volumes = UBI_MAX_VOLUMES;
	ui->vtbl_size = ui->max_volumes * UBI_VTBL_RECORD_SIZE;
}

/**
 * ubigen_add_volume - add a volume to the volume table.
 * @ui: libubigen information
 * @vi: volume information
 * @vtbl: volume table to add to
 *
 * This function adds volume described by input parameters to the volume table
 * @vtbl.
 */
static int ubigen_add_volume(const struct ubigen_info *ui,
		      const struct ubigen_vol_info *vi,
		      struct ubi_vtbl_record *vtbl)
{
	struct ubi_vtbl_record *vtbl_rec = &vtbl[vi->id];
	uint32_t tmp;

	if (vi->id >= ui->max_volumes)
		return -1;

	if (vi->alignment >= ui->leb_size)
		return -1;

	memset(vtbl_rec, 0, sizeof(struct ubi_vtbl_record));
	tmp = (vi->bytes + ui->leb_size - 1) / ui->leb_size;
	vtbl_rec->reserved_pebs = cpu_to_be32(tmp);
	vtbl_rec->alignment = cpu_to_be32(vi->alignment);
	vtbl_rec->vol_type = vi->type;
	tmp = ui->leb_size % vi->alignment;
	vtbl_rec->data_pad = cpu_to_be32(tmp);
	vtbl_rec->flags = vi->flags;

	memcpy(vtbl_rec->name, vi->name, vi->name_len);
	vtbl_rec->name[vi->name_len] = '\0';
	vtbl_rec->name_len = cpu_to_be16(vi->name_len);

	tmp = crc32(UBI_CRC32_INIT, vtbl_rec, UBI_VTBL_RECORD_SIZE_CRC);
	vtbl_rec->crc =	 cpu_to_be32(tmp);
	return 0;
}

/**
 * ubigen_init_ec_hdr - initialize EC header.
 * @ui: libubigen information
 * @hdr: the EC header to initialize
 * @ec: erase counter value
 */
static void ubigen_init_ec_hdr(const struct ubigen_info *ui,
		        struct ubi_ec_hdr *hdr, long long ec)
{
	uint32_t crc;

	memset(hdr, 0, sizeof(struct ubi_ec_hdr));

	hdr->magic = cpu_to_be32(UBI_EC_HDR_MAGIC);
	hdr->version = ui->ubi_ver;
	hdr->ec = cpu_to_be64(ec);
	hdr->vid_hdr_offset = cpu_to_be32(ui->vid_hdr_offs);
	hdr->data_offset = cpu_to_be32(ui->data_offs);
	hdr->image_seq = cpu_to_be32(ui->image_seq);

	crc = crc32(UBI_CRC32_INIT, hdr, UBI_EC_HDR_SIZE_CRC);
	hdr->hdr_crc = cpu_to_be32(crc);
}

/**
 * init_vid_hdr - initialize VID header.
 * @ui: libubigen information
 * @vi: volume information
 * @hdr: the VID header to initialize
 * @lnum: logical eraseblock number
 * @data: the contents of the LEB (static volumes only)
 * @data_size: amount of data in this LEB (static volumes only)
 *
 * Note, @used_ebs, @data and @data_size are ignored in case of dynamic
 * volumes.
 */
static void init_vid_hdr(const struct ubigen_info *ui,
			 const struct ubigen_vol_info *vi,
			 struct ubi_vid_hdr *hdr, int lnum,
			 const void *data, int data_size)
{
	uint32_t crc;

	memset(hdr, 0, sizeof(struct ubi_vid_hdr));

	hdr->magic = cpu_to_be32(UBI_VID_HDR_MAGIC);
	hdr->version = ui->ubi_ver;
	hdr->vol_type = vi->type;
	hdr->vol_id = cpu_to_be32(vi->id);
	hdr->lnum = cpu_to_be32(lnum);
	hdr->data_pad = cpu_to_be32(vi->data_pad);
	hdr->compat = vi->compat;

	if (vi->type == UBI_VID_STATIC) {
		hdr->data_size = cpu_to_be32(data_size);
		hdr->used_ebs = cpu_to_be32(vi->used_ebs);
		crc = crc32(UBI_CRC32_INIT, data, data_size);
		hdr->data_crc = cpu_to_be32(crc);
	}

	crc = crc32(UBI_CRC32_INIT, hdr, UBI_VID_HDR_SIZE_CRC);
	hdr->hdr_crc = cpu_to_be32(crc);
}

/**
 * ubigen_write_volume - write UBI volume.
 * @ui: libubigen information
 * @vi: volume information
 * @ec: erase coutner value to put to EC headers
 * @bytes: volume size in bytes
 * @in: input file descriptor (has to be properly seeked)
 * @out: output file descriptor
 *
 * This function reads the contents of the volume from the input file @in and
 * writes the UBI volume to the output file @out. Returns zero on success and
 * %-1 on failure.
 */
static int ubigen_write_volume(const struct ubigen_info *ui,
			const struct ubigen_vol_info *vi, long long ec,
			long long bytes, void *in, void *out,
			unsigned long *dest_size)
{
	int len = vi->usable_leb_size, lnum = 0;
	char inbuf[ui->leb_size], outbuf[ui->peb_size];
	unsigned long offset_in = 0, offset_out;

	/*
	 * Skip 2 PEBs at the beginning of the file for the volume table which
	 * will be written later.
	 */
	offset_out = ui->peb_size * 2;

	if (vi->id >= ui->max_volumes)
		return -1;

	if (vi->alignment >= ui->leb_size)
		return -1;

	memset(outbuf, 0xFF, ui->data_offs);
	ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)outbuf, ec);

	while (bytes) {
		struct ubi_vid_hdr *vid_hdr;

		if (bytes < len)
			len = bytes;
		bytes -= len;

		memcpy(inbuf, in + offset_in, len);
		offset_in += len;

		vid_hdr = (struct ubi_vid_hdr *)(&outbuf[ui->vid_hdr_offs]);
		init_vid_hdr(ui, vi, vid_hdr, lnum, inbuf, len);

		memcpy(outbuf + ui->data_offs, inbuf, len);
		memset(outbuf + ui->data_offs + len, 0xFF,
		       ui->peb_size - ui->data_offs - len);

		memcpy(out + offset_out, outbuf, ui->peb_size);
		offset_out += ui->peb_size;

		lnum += 1;
	}

	*dest_size = offset_out;

	return 0;
}

/**
 * ubigen_write_layout_vol - write UBI layout volume
 * @ui: libubigen information
 * @peb1: physical eraseblock number to write the first volume table copy
 * @peb2: physical eraseblock number to write the second volume table copy
 * @ec1: erase counter value for @peb1
 * @ec2: erase counter value for @peb1
 * @vtbl: volume table
 * @fd: output file descriptor seeked to the proper position
 *
 * This function creates the UBI layout volume which contains 2 copies of the
 * volume table. Returns zero in case of success and %-1 in case of failure.
 */
static int ubigen_write_layout_vol(const struct ubigen_info *ui, int peb1, int peb2,
			    long long ec1, long long ec2,
			    struct ubi_vtbl_record *vtbl, void *out)
{
	struct ubigen_vol_info vi;
	char *outbuf;
	struct ubi_vid_hdr *vid_hdr;
	unsigned long seek;

	vi.bytes = ui->leb_size * UBI_LAYOUT_VOLUME_EBS;
	vi.id = UBI_LAYOUT_VOLUME_ID;
	vi.alignment = UBI_LAYOUT_VOLUME_ALIGN;
	vi.data_pad = ui->leb_size % UBI_LAYOUT_VOLUME_ALIGN;
	vi.usable_leb_size = ui->leb_size - vi.data_pad;
	vi.data_pad = ui->leb_size - vi.usable_leb_size;
	vi.type = UBI_LAYOUT_VOLUME_TYPE;
	vi.name = UBI_LAYOUT_VOLUME_NAME;
	vi.name_len = strlen(UBI_LAYOUT_VOLUME_NAME);
	vi.compat = UBI_LAYOUT_VOLUME_COMPAT;

	outbuf = malloc(ui->peb_size);
	if (!outbuf)
		return -1;

	memset(outbuf, 0xFF, ui->data_offs);
	vid_hdr = (struct ubi_vid_hdr *)(&outbuf[ui->vid_hdr_offs]);
	memcpy(outbuf + ui->data_offs, vtbl, ui->vtbl_size);
	memset(outbuf + ui->data_offs + ui->vtbl_size, 0xFF,
	       ui->peb_size - ui->data_offs - ui->vtbl_size);

	seek = peb1 * ui->peb_size;
	ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)outbuf, ec1);
	init_vid_hdr(ui, &vi, vid_hdr, 0, NULL, 0);
	memcpy(out + seek, outbuf, ui->peb_size);

	seek = peb2 * ui->peb_size;
	ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)outbuf, ec2);
	init_vid_hdr(ui, &vi, vid_hdr, 1, NULL, 0);
	memcpy(out + seek, outbuf, ui->peb_size);

	free(outbuf);
	return 0;
}

int ubinize(void *src_addr, unsigned long src_size,
	    void *dest_addr, unsigned long *dest_size,
	    int peb_size, int min_io_size,
	    int subpage_size, int vid_hdr_offs)
{
	int err = -1;
	struct ubigen_info ui;
	struct ubi_vtbl_record *vtbl;
	struct ubigen_vol_info *vi;

	ubigen_info_init(&ui, peb_size, min_io_size, subpage_size, vid_hdr_offs, 1, 2);

	vtbl = ubigen_create_empty_vtbl(&ui);
	if (!vtbl)
		goto out;

	vi = calloc(sizeof(struct ubigen_vol_info), 1);
	if (!vi)
		goto out_free;

	err = set_volume_info(&ui, vi, 1, UBI_VID_DYNAMIC, src_size, "modem", 1);
	if (err == -1)
		goto out_free;

	err = ubigen_add_volume(&ui, vi, vtbl);
	if (err)
		goto out_free;

	err = ubigen_write_volume(&ui, vi, 0, src_size, src_addr, dest_addr, dest_size);
	if (err)
		goto out_free;

	err = ubigen_write_layout_vol(&ui, 0, 1, 0, 0, vtbl, dest_addr);
	if (err)
		goto out_free;

	free(vi);
	free(vtbl);
	return 0;

out_free:
	free(vi);
	free(vtbl);
out:
	return err;
}
