/*
 * Copyright (C) 2011 Samsung Electronics
 * Chanho Park <chanho61.park@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/timer.h>

static inline struct drime4_timer *drime4_get_base_timer(void)
{
	return (struct drime4_timer *)DRIME4_TIMER_BASE;
}

int timer_init(void)
{
	struct drime4_timer *const timer = drime4_get_base_timer();

	writel(DRIME4_MCLK / 1000000 - 1, &timer->psr);	/* prescaler */
	return 0;
}

unsigned long get_timer(unsigned long base)
{
	struct drime4_timer *const timer = drime4_get_base_timer();
	return readl(&timer->ldr) & 0xffff;
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
	struct drime4_timer *const timer = drime4_get_base_timer();
	int i;
	unsigned long usec_fpga = usec / 15;

	for (i = 0; i < usec_fpga; i++) {
		writel(0x0, &timer->tcr);	/* disable timer */
		writel(1000, &timer->ldr);
		writel(0x103, &timer->tcr);	/* enable timer */

		while (readl(&timer->isr))
				;
	}
}
