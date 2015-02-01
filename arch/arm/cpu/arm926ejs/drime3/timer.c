/*
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
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

static unsigned long count_value;

/* Internal tick units */
static unsigned long long timestamp;	/* Monotonic incrementing timer */
static unsigned long lastdec;		/* Last decremneter snapshot */

static inline struct drime3_timer *drime3_get_base_timer(void)
{
	return (struct drime3_timer *)DRIME3_TIMER_BASE;
}

int timer_init(void)
{
	struct drime3_timer *const timer = drime3_get_base_timer();

	writel(DRIME3_MCLK / 1000000 - 1, &timer->psr);	/* prescaler */
}

unsigned long get_timer(unsigned long base)
{
	struct drime3_timer *const timer = drime3_get_base_timer();
	return readl(&timer->ldr) & 0xffff;
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
	struct drime3_timer *const timer = drime3_get_base_timer();
	int i;

	for (i = 0; i < usec; i++) {
		writel(0x0, &timer->tcr);	/* disable timer */
		writel(1000, &timer->ldr);
		writel(0x103, &timer->tcr);	/* disable timer */

		while (readl(&timer->isr))
				;
	}
}
