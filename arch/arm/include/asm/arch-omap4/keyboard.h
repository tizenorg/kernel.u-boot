/*
 * Copyright (C) 2010 Samsung Electronics
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
 *
 */

#ifndef __ASM_ARCH_KBD_H_
#define __ASM_ARCH_KBD_H_

#define OMAP4_KBD_BASE			0x4A31C000

#define OMAP4_KBD_SYSCONFIG_OFFSET	0x10
#define OMAP4_KBD_CTL			0x28

#define OMAP4_KBD_ROWINPUTS		0x3C
#define OMAP4_KBD_COLUMNOUTPUTS		0x40

#define OMAP4_KBD_FULLCODE31_0		0x44
#define OMAP4_KBD_FULLCODE63_32		0x48
#define OMAP4_KBD_FULLCODE17_0		0x4C
#define OMAP4_KBD_FULLCODE35_18		0x50
#define OMAP4_KBD_FULLCODE53_36		0x54
#define OMAP4_KBD_FULLCODE71_54		0x58
#define OMAP4_KBD_FULLCODE80_72		0x5C

#endif
