/*
 * (C) Copyright 2009
 * Samsung Electronics, <www.samsung.com/sec>
 * Heungjun Kim <riverful.kim@samsung.com>
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

#ifndef __ASM_ARM_ARCH_MEM_H_
#define __ASM_ARM_ARCH_MEM_H_

/* DRAM Memory Controller */
#define CONCONTROL_OFFSET	0x00
#define MEMCONTROL_OFFSET	0x04
#define MEMCONFIG0_OFFSET	0x08
#define MEMCONFIG1_OFFSET	0x0c
#define DIRECTCMD_OFFSET	0x10
#define PRECHCONFIG_OFFSET	0x14
#define PHYCONTROL0_OFFSET	0x18
#define PHYCONTROL1_OFFSET	0x1c
#define PHYCONTROL2_OFFSET	0x20
#define PWRDNCONFIG_OFFSET	0x28
#define TIMINGAREF_OFFSET	0x30
#define TIMINGROW_OFFSET	0x34
#define TIMINGDATA_OFFSET	0x38
#define TIMINGPOWER_OFFSET	0x3c
#define PHYSTATUS0_OFFSET	0x40
#define PHYSTATUS1_OFFSET	0x44
#define CHIP0STATUS_OFFSET	0x48
#define CHIP1STATUS_OFFSET	0x4c
#define AREFSTATUS_OFFSET	0x50
#define MRSTATUS_OFFSET		0x54
#define PHYTEST0_OFFSET		0x58
#define PHYTEST1_OFFSET		0x5c

#endif
