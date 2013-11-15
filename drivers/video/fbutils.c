/*
 * Font module for Framebuffer.
 *
 * Copyright(C) 2009 Samsung Electronics
 * InKi Dae <inki.dae@samsung.com>
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

#include <config.h>
#include <common.h>
#include <version.h>
#include <stdarg.h>
#include <malloc.h>
#include <linux/types.h>
#include <asm/io.h>
#include <lcd.h>
#include <asm/global_data.h>

#include <fbutils.h>

#define XORMODE			0x80000000
#define CARRIAGE_RETURN		10
#define HORIZONTAL_TAP		9

#define RED_OFFSET	 16
#define GREEN_OFFSET	 8
#define BLUE_OFFSET	 0

extern struct fbcon_font_desc font_vga_8x16;

union multiptr {
	unsigned char *p8;
	unsigned short *p16;
	unsigned long *p32;
};

static int bytes_per_pixel;
static unsigned char **line_addr;

static unsigned int g_default_x, g_default_y;
static unsigned int color_index;

static unsigned int COLOR_TABLE_8[NUM_ELEM_COLOR_TABLE] = {
	0x000000ff,	/* BLACK */
	0xff0000ff,	/* RED */
	0x00ff00ff,	/* GREEN */
	0x0000ffff,	/* BLUE */
	0xffff00ff,	/* YELLOW */
	0xff00ffff,	/* MAGENTA */
	0x00ffffff,	/* AQUA */
	0xffffffff,	/* WHITE */
	0x80000000	/* XORMODE */
};

static unsigned int g_x, g_y;

void set_font_color(unsigned char index)
{
	if (index < 0 || index >= NUM_ELEM_COLOR_TABLE) {
		printf("index is invalid.\n");
		return;
	}

	color_index = index;
}

static void set_pixel(int x, int y)
{
	union multiptr loc;
	unsigned xormode;
	unsigned color;

	if ((x < 0) || ((__u32)x >= panel_info.vl_width) ||
	    (y < 0) || ((__u32)y >= panel_info.vl_height))
		return;

	xormode = color_index & XORMODE;
	color = COLOR_TABLE_8[color_index & (unsigned int)~XORMODE];
	loc.p8 = line_addr[y] + x * bytes_per_pixel;

	switch (bytes_per_pixel) {
	case 1:
	default:
		if (xormode)
			*loc.p8 ^= color;
		else
			*loc.p8 = color;
		break;
	case 2:
		if (xormode)
			*loc.p16 ^= color;
		else
			*loc.p16 = color;
		break;
	case 4:
		if (xormode)
			*loc.p32 ^= color;
		else
			*loc.p32 = color;
		break;
	}
}

static void put_char(int c)
{
	int i, j, bits;

	if (c == CARRIAGE_RETURN) {
		g_y += font_vga_8x16.height;
		g_x = g_default_x - font_vga_8x16.width;
		return;
	}

	if (c == HORIZONTAL_TAP) {
		g_x += 20;
		return;
	}

	/* check x-axis limitation. */
	if (g_x > panel_info.vl_width - font_vga_8x16.width) {
		g_x = g_default_x;
		g_y += font_vga_8x16.height;
	}

	for (i = 0; i < font_vga_8x16.height; i++) {
		bits = font_vga_8x16.data[font_vga_8x16.height * c + i];
		for (j = 0; j < font_vga_8x16.width; j++, bits <<= 1) {
			if (bits & 0x80)
				set_pixel(g_x + j, g_y + i);
		}
	}
}

void fb_printf(char *s)
{
	int i;

	for (i = 0; *s; i++, g_x += font_vga_8x16.width, s++)
		put_char(*s);

	lcd_sync();
}

DECLARE_GLOBAL_DATA_PTR;

void init_font(void)
{
	int addr = 0, y;
	int line_length;

	g_default_x = 0;
	g_default_y = 0;

	g_x = 0;
	g_y = 0;

	bytes_per_pixel = NBITS(panel_info.vl_bpix) / 8;
	line_length = panel_info.vl_width * bytes_per_pixel;

	line_addr = (unsigned char **)malloc(
			line_length * panel_info.vl_height);

	for (y = 0, addr = 0; y < panel_info.vl_height; y++,
			addr += line_length) {
		line_addr[y] = (unsigned char *)(gd->fb_base + addr);
		memset(line_addr[y], 0x0, line_length);
	}
}

void exit_font(void)
{
	if (line_addr)
		free(line_addr);

	g_default_x = 0;
	g_default_y = 0;
	g_x = 0;
	g_y = 0;
}
