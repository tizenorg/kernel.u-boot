/*
 * Copyright (C) 2009 Samsung Electronics
 * Kyungnin Park <kyungmin.park@samsung.com>
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

#ifndef __ASM_ARCH_KEYPAD_H_
#define __ASM_ARCH_KEYPAD_H_

/* I2C */
#define S5PC100_KEYPAD_BASE		0xF3100000
#define S5PC110_KEYPAD_BASE		0xE1600000

#define S5PC1XX_KEYIFCON_OFFSET		(0x00)
#define S5PC1XX_KEYIFSTSCLR_OFFSET	(0x04)
#define S5PC1XX_KEYIFCOL_OFFSET		(0x08)
#define S5PC1XX_KEYIFROW_OFFSET		(0x0C)
#define S5PC1XX_KEYIFFC_OFFSET		(0x10)

#endif
