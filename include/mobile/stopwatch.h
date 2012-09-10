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

#ifndef _STOPWATCH_H_
#define _STOPWATCH_H_

int stopwatch_init(void);
void stopwatch_print(void);
u32 stopwatch_tick_count(void);
u32 stopwatch_tick_clock(void);
#ifdef CONFIG_SYS_STOPWATCH_ADDR
void stopwatch_tick_init(void);
void stopwatch_tick_mark(char *tag);
void stopwatch_tick_print(void);
#endif

#endif
