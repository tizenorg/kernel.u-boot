/* 
 * Copied and modified from linux/arch/arm/plat-s5pc11x/pm.c
 * Copyright (c) 2004,2009 Simtec Electronics
 *	boyko.lee <boyko.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Parts based on arch/arm/mach-pxa/pm.c
 *
 * Thanks to Dimitry Andric for debugging
*/

#ifndef __S5PC110_SLEEP_H
#define __S5PC110_SLEEP_H

#include <common.h>
#include <asm/ptrace.h>

/*
 * PSR bits
 */
/* #define USR26_MODE      0x00000000 */
/* #define FIQ26_MODE      0x00000001 */
/* #define IRQ26_MODE      0x00000002 */
/* #define SVC26_MODE      0x00000003 */
/* #define USR_MODE        0x00000010 */
/* #define FIQ_MODE        0x00000011 */
/* #define IRQ_MODE        0x00000012 */
/* #define SVC_MODE        0x00000013 */
/* #define ABT_MODE        0x00000017 */
/* #define UND_MODE        0x0000001b */
/* #define SYSTEM_MODE     0x0000001f */
#define MODE32_BIT      0x00000010
/* #define MODE_MASK       0x0000001f */
#define PSR_T_BIT       0x00000020
#define PSR_F_BIT       0x00000040
#define PSR_I_BIT       0x00000080
#define PSR_A_BIT       0x00000100
#define PSR_J_BIT       0x01000000
#define PSR_Q_BIT       0x08000000
#define PSR_V_BIT       0x10000000
#define PSR_C_BIT       0x20000000
#define PSR_Z_BIT       0x40000000
#define PSR_N_BIT       0x80000000

#ifndef __ASSEMBLY__
extern void board_sleep_resume(void);
extern int s5pc110_cpu_save(unsigned long *saveblk);
extern void s5pc110_cpu_resume(void);
extern unsigned int s5pc110_sleep_return_addr;
extern unsigned int s5pc110_sleep_save_phys;

#endif

#endif
