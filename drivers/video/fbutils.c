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

static unsigned colormap [256];
extern vidinfo_t panel_info;
static gd_t *g_gd;

static char red_length = 8, red_offset = 16;
static char green_length = 8, green_offset = 8;
static char blue_length = 8, blue_offset = 0;

static unsigned int color_table_8[MAX_INDEX_TABLE] ={
	0x000000,	/* BLACK */
	0xff0000,	/* RED */
	0x00ff00,	/* GREEN */
	0x0000ff,	/* BLUE */
	0xffff00,	/* YELLOW */
	0xff00ff,	/* MAGENTA */
	0x00ffff,	/* AQUA */
	0xffffff,	/* WHITE */
	0x80000000	/* XORMODE */
};

static unsigned int g_x, g_y;

void set_font_color(unsigned char index)
{
	if (index < 0 || index > 8) {
		printf("index is invalid.\n");
		return;
	}

	color_index = index;
}

static void set_color_table(unsigned colidx, unsigned value)
{
	unsigned res;
	unsigned short red, green, blue;

	switch (bytes_per_pixel) {
	case 2:
	case 4:
		red = (value >> 16) & 0xff;
		green = (value >> 8) & 0xff;
		blue = value & 0xff;
		res = ((red >> (8 - red_length)) << red_offset) |
                      ((green >> (8 - green_length)) << green_offset) |
                      ((blue >> (8 - blue_length)) << blue_offset);
		break;
	default:
		printf("bpp %d is invalid.\n", bytes_per_pixel);
		break;
	}

        colormap [colidx] = res;
}

static void make_color_table(void)
{
	int i;

	for(i = 0; i < MAX_INDEX_TABLE; i++)
		set_color_table(i, color_table_8[i]);
}

void set_font_xy(unsigned int x, unsigned int y)
{
	g_default_x = x;
	g_default_y = y;

	/* it needs handling for exception */

	g_x = x;
	g_y = y;
}

void get_font_xy(unsigned int *x, unsigned int *y)
{
	*x = g_x;
	*y = g_y;
}

static inline void __setpixel (union multiptr loc, unsigned xormode, unsigned color)
{
	switch(bytes_per_pixel) {
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

static void pixel(int x, int y)
{
	unsigned int xormode;
	union multiptr loc;

	if ((x < 0) || ((__u32)x >= panel_info.vl_width) ||
	    (y < 0) || ((__u32)y >= panel_info.vl_height))
		return;

	xormode = (unsigned int)(color_index & XORMODE);
	color_index &= (unsigned int)~XORMODE;

	loc.p8 = line_addr [y] + x * bytes_per_pixel;
	__setpixel (loc, xormode, colormap[color_index]);
}

static void put_char(int c)
{
	int i,j,bits;

	if (c == CARRIAGE_RETURN) {
		g_y += font_vga_8x16.height;
		g_x = g_default_x - font_vga_8x16.width;
		return;
	}

	for (i = 0; i < font_vga_8x16.height; i++) {
		bits = font_vga_8x16.data [font_vga_8x16.height * c + i];
		for (j = 0; j < font_vga_8x16.width; j++, bits <<= 1) {
				if (bits & 0x80)
					pixel (g_x + j, g_y + i);
		}
	}
}

void fb_printf(char *s)
{
	int i;

	for (i = 0; *s; i++, g_x += font_vga_8x16.width, s++)
		put_char (*s);
}

void init_font(void)
{
	int addr = 0, y;
	int line_length;

	line_length = panel_info.vl_width * 4;
	bytes_per_pixel = panel_info.vl_bpix / 8;

	g_gd = (gd_t *)(_armboot_start - CONFIG_SYS_MALLOC_LEN - sizeof(gd_t));

	line_addr = (unsigned char **)malloc(sizeof(__u32) * panel_info.vl_height);

	for (y = 0; y < panel_info.vl_height; y++, addr += line_length) {
		line_addr[y] = (unsigned char *)(g_gd->fb_base + addr);
	}

	make_color_table();
}

void exit_font(void)
{
	if (line_addr)
		free(line_addr);

	g_default_x = g_default_y = 0;
	g_x = g_y = 0;
}
