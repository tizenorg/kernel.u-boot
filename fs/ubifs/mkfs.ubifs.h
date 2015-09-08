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
 * Authors: Artem Bityutskiy
 *          Adrian Hunter
 *          Zoltan Sogor
 */

#ifndef __MKFS_UBIFS_H__
#define __MKFS_UBIFS_H__

#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE

#include <linux/types.h>
#include <linux/lzo.h>
#include <malloc.h>
#include "crc32.h"
#include "ubifs_qsort.h"
#include "crc16.h"
#include "ubifs.h"

#define err_msg(fmt, ...) ({                                \
	fprintf(stderr, "Error: " fmt "\n", ##__VA_ARGS__); \
	-1;                                                 \
})

#define sys_err_msg(fmt, ...) ({                                         \
	int err_ = errno;                                                \
	fprintf(stderr, "Error: " fmt "\n", ##__VA_ARGS__);              \
	fprintf(stderr, "       %s (error %d)\n", strerror(err_), err_); \
	-1;                                                              \
})

#endif
