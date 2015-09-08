/*
 * (C) Copyright 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
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
 *
 */

#ifndef __ASM_ARM_ARCH_CLOCK_H_
#define __ASM_ARM_ARCH_CLOCK_H_

#define APLL	0
#define MPLL	1
#define EPLL	2
#define HPLL	3
#define VPLL	4

#define S5P6442_APLL_LOCK		0xE0100000
#define S5P6442_MPLL_LOCK		0xE0100008
#define S5P6442_EPLL_LOCK		0xE0100010
#define S5P6442_VPLL_LOCK		0xE0100020
#define S5P6442_APLL_CON		0xE0100100
#define S5P6442_MPLL_CON		0xE0100108
#define S5P6442_EPLL_CON		0xE0100110
#define S5P6442_VPLL_CON		0xE0100120

#ifndef __ASSEMBLY__
struct s5p6442_clock {
	unsigned int	apll_lock;
	unsigned char	res1[0x4];
	unsigned int	mpll_lock;
	unsigned char	res2[0x4];
	unsigned int	epll_lock;
	unsigned char	res3[0xc];
	unsigned int	vpll_lock;
	unsigned char	res4[0xdc];
	unsigned int	apll_con;
	unsigned char	res5[0x4];
	unsigned int	mpll_con;
	unsigned char	res6[0x4];
	unsigned int	epll_con;
	unsigned char	res7[0xc];
	unsigned int	vpll_con;
	unsigned char	res8[0xdc];
	unsigned int	src0;
	unsigned int	src1;
	unsigned int	src2;
	unsigned int	src3;
	unsigned int	src4;
	unsigned int	src5;
	unsigned int	src6;
	unsigned char	res9[0xe4];
	unsigned int	div0;
	unsigned int	div1;
	unsigned int	div2;
	unsigned int	div3;
	unsigned int	div4;
	unsigned int	div5;
	unsigned int	div6;
	unsigned char	res10[0x1e4];
	unsigned int	gate_d00;
	unsigned int	gate_d01;
	unsigned int	gate_d02;
	unsigned char	res11[0x54];
	unsigned int	gate_sclk0;
	unsigned int	gate_sclk1;
};
#endif

#endif
