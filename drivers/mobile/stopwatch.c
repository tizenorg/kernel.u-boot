/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/pwm.h>
#include <asm/arch/clk.h>
#include <mobile/stopwatch.h>

#define PRESCALER_1		(16 - 1)	/* prescaler of timer 2, 3, 4 */

int stopwatch_init(void)
{
	struct s5p_timer *const timer =
	    (struct s5p_timer *)samsung_get_base_timer();
	u32 val;
	u32 count_value;
	int ch = 3;		/* use timer3 */

	/* set divider : 1 */
	val = readl(&timer->tcfg1);
	val &= ~(0xf << (4 * ch));
	writel(val, &timer->tcfg1);

	/* init count value */
	count_value = -1;

	val = readl(&timer->tcntb3);
	if (val == count_value)
		return 1;

	/* set count value */
	writel(count_value, &timer->tcntb3);

	/* start timer */
	val = readl(&timer->tcon);
	writel(val | TCON3_UPDATE, &timer->tcon);
	writel(val | TCON3_START, &timer->tcon);

	return 0;
}

static void print_float(u32 val, u32 div)
{
	int i, j;
	u32 remain;

	remain = val % div;
	printf("%d.", val / div);
	for (i = 1, j = 1; i < 4; i++) {
		j = j * 10;
		printf("%d", remain / (div / j));
		remain %= (div / j);
	}
}

void stopwatch_print(void)
{
	u32 val = 0;
	u32 timer_clk = 1;
	struct s5p_timer *const timer =
	    (struct s5p_timer *)samsung_get_base_timer();

	val = ~readl(&timer->tcnto3);
	timer_clk = get_pwm_clk() / ((PRESCALER_1 + 1) * (0 + 1));

	printf("\nElapsed time: ");
	print_float(val, timer_clk);
	printf(" sec (0x%x, %dHz)\n", val, timer_clk);
#ifdef CONFIG_SYS_STOPWATCH_ADDR
	stopwatch_tick_print();
#endif
}

inline u32 stopwatch_tick_count(void)
{
	struct s5p_timer *const timer =
	    (struct s5p_timer *)samsung_get_base_timer();

	return ~readl(&timer->tcnto3);
}

inline u32 stopwatch_tick_clock(void)
{
	return (get_pwm_clk() / ((PRESCALER_1 + 1) * (0 + 1)));
}

#ifdef CONFIG_SYS_STOPWATCH_ADDR
struct stopwatch_header {
	u32 magic;
	u32 count;
	u32 reserved[2];
};

struct stopwatch_tick {
	char tag[28];
	u32 time;
};

#define STOPWATCH_MAGIC	0x53545743	/* STWC */
#define GET_HEADER_ADDR	((struct stopwatch_header *)CONFIG_SYS_STOPWATCH_ADDR)
#define GET_TICK_ADDR	((struct stopwatch_tick *)(CONFIG_SYS_STOPWATCH_ADDR + \
			sizeof(struct stopwatch_header)))

void stopwatch_tick_init(void)
{
	/* Note: use variable in stack, not heap. cause relocation */
	struct stopwatch_header *header = GET_HEADER_ADDR;

	if (header->magic != STOPWATCH_MAGIC) {
		header->magic = STOPWATCH_MAGIC;
		header->count = 0;
	}
}

void stopwatch_tick_mark(char *tag)
{
	u32 val = 0;
	struct stopwatch_header *header = GET_HEADER_ADDR;
	struct stopwatch_tick *tick = GET_TICK_ADDR;
	struct s5p_timer *const timer =
	    (struct s5p_timer *)samsung_get_base_timer();

	val = ~readl(&timer->tcnto3);
	tick += header->count;

	strncpy(tick->tag, tag, 28);
	tick->time = val;
	header->count++;
}

void stopwatch_tick_print(void)
{
	int i;
	u32 val = 0, val_bak = 0;
	u32 timer_clk = 1;
	u32 millisec;
	struct stopwatch_header *header = GET_HEADER_ADDR;
	struct stopwatch_tick *tick = GET_TICK_ADDR;

	timer_clk = get_pwm_clk() / ((PRESCALER_1 + 1) * (0 + 1));
	tick = (struct stopwatch_tick *)(CONFIG_SYS_STOPWATCH_ADDR +
					 sizeof(struct stopwatch_header));

	printf("\n");
	for (i = 0; i < header->count; i++) {
		val = tick->time;
		millisec = val % timer_clk;

		printf("(%2d) %-28s ", i + 1, tick->tag);
		print_float(val, timer_clk);
		printf(" sec (0x%06x) ", val);
		printf("delta ");
		print_float(val - val_bak, timer_clk);
		printf("\n");

		val_bak = val;
		tick++;
	}

	/* clear for soft reset */
	header->count = 0;
}
#endif
