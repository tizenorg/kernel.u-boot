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

#ifndef __HEADER_H__
#define __HEADER_H__

#define HDR_BOOT_MAGIC		0x744f6f42	/* BoOt */
#define HDR_KERNEL_MAGIC	0x6e52654b	/* KeRn */

#define HDR_SIZE		512

struct boot_header {
	uint32_t magic;		/* image magic number */
	uint32_t size;		/* image data size */
	uint32_t valid;		/* valid flag */
	char date[12];		/* image creation timestamp - YYMMDDHH */
	char version[24];	/* image version */
	char boardname[16];	/* target board name */
#ifdef CONFIG_SLP_NEW_HEADER
	char blank[448];	/* reserved */
#endif
};

#endif
