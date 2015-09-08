/*
 * Copyright(C) 2009 Samsung Electronics
 * InKi Dae <inki.dae@samsung.com>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive
 *  for more details.
 */

#ifndef _FONT_H
#define _FONT_H

#include <linux/types.h>

struct fbcon_font_desc {
    int idx;
    char *name;
    int width, height;
    char *data;
    int pref;
};

/* define 8 index color table */
enum {
	FONT_BLACK	= 0,
	FONT_RED,
	FONT_GREEN,
	FONT_BLUE,
	FONT_YELLOW,
	FONT_MAGENTA,
	FONT_AQUA,
	FONT_WHITE,
	FONT_XOR,
};

#define VGA8x8_IDX	0
#define VGA8x16_IDX	1

/* Max. length for the name of a predefined font */
#define MAX_FONT_NAME	32

/* max index color count */
#define MAX_INDEX_TABLE		(8 + 1)

/* initialize font module and then create color table. */
void init_font(void);

/* exit font module. */
void exit_font(void);

/* set font color */
void set_font_color(unsigned char index);

/* set coordinates for x and y axis */
void set_font_xy(unsigned int x, unsigned int y);

/* get coordinates for x and y axis */
void get_font_xy(unsigned int *x, unsigned int *y);

/* draw string with user-requested color */
void fb_printf(char *s);

/* draw wheel */
void draw_wheel(int x, int y, unsigned char color);

/* draw progress */
void draw_progress(int y, int per, unsigned char color);

/* clear the framebuffer */
void fb_clear(int range);

unsigned int get_chars(void);

#endif /* _FONT_H */
