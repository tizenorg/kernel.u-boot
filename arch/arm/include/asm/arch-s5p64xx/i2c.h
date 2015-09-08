/*
 * Copyright (C) 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Kyungnin Park <kyungmin.park@samsung.com>
 *
 * based on s3c24x0_i2c.c
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

#ifndef __ASM_ARCH_I2C_H_
#define __ASM_ARCH_I2C_H_

/* I2C */
#define S5PC100_I2C0_BASE	0xEC100000
#define S5PC100_I2C1_BASE	0xEC200000
#define S5PC110_I2C0_BASE	0xE1800000
#define S5PC110_I2C2_BASE	0xE1A00000

#ifndef __ASSEMBLY__
typedef struct s5pc1xx_i2c {
	volatile unsigned long	IICCON;
	volatile unsigned long	IICSTAT;
	volatile unsigned long	IICADD;
	volatile unsigned long	IICDS;
	volatile unsigned long	IICLC;
} s5pc1xx_i2c_t;
#endif

#endif
