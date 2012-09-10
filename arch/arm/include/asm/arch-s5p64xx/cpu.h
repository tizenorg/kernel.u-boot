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

#ifndef _S5P64XX_CPU_H
#define _S5P64XX_CPU_H

#define S5P64XX_ADDR_BASE	0xE0000000

#define S5P64XX_CLOCK_BASE	0xE0100000

/* S5P6442 */
#define S5P6442_GPIO_BASE	0xE0200000
#define S5P6442_SROMC_BASE	0xE7000000
#define S5P6442_DMC_BASE	0xE6000000
#define S5P6442_VIC0_BASE	0xE4000000
#define S5P6442_VIC1_BASE	0xE4100000
#define S5P6442_VIC2_BASE	0xE4200000
#define S5P6442_PWMTIMER_BASE	0xEA000000
#define S5P6442_WATCHDOG_BASE	0xEA200000
#define S5P6442_UART_BASE	0xEC000000

/* Chip ID */
#define S5P64XX_PRO_ID		0xE0000000

#ifndef __ASSEMBLY__
/* CPU detection macros */
extern unsigned int s5p64xx_cpu_id;

#define IS_SAMSUNG_TYPE(type, id)			\
static inline int cpu_is_##type(void)			\
{							\
	return s5p64xx_cpu_id == id ? 1 : 0;		\
}

IS_SAMSUNG_TYPE(s5p6442, 0x6442)

#endif

#endif	/* _S5P6442_CPU_H */
