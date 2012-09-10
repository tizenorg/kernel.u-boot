/*
 * OMAP4430 Dss & display Controller driver.
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
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <lcd.h>
#include <malloc.h>

#include <mobile/logo_rgb16_wvga_portrait.h>
#include "omap-fb.h"

int lcd_line_length;
int lcd_color_fg;
int lcd_color_bg;

void *lcd_base;
void *lcd_console_address;

short console_col;
short console_row;

static unsigned int panel_width, panel_height;

extern void init_panel_info(vidinfo_t * vid);
/* LCD Panel data */
vidinfo_t panel_info;

static void omap4_lcd_init_mem(void *lcdbase, vidinfo_t * vid)
{
	unsigned long palette_size, palette_mem_size;
	unsigned int fb_size;

	fb_size = vid->vl_row * vid->vl_col * (vid->vl_bpix >> 3);

	lcd_base = lcdbase;

	palette_size = NBITS(vid->vl_bpix) == 8 ? 256 : 16;
	palette_mem_size = palette_size * sizeof(u32);

	udebug
	    ("fb_size=%d, screen_base=%x, palette_size=%d, palettle_mem_size=%d\n",
	     fb_size, (unsigned int)lcd_base, (int)palette_size,
	     (int)palette_mem_size);
}

int conv_rgb565_to_rgb888(unsigned short rgb565, unsigned int sw)
{
	char red, green, blue;
	unsigned int threshold = 150;

	red = (rgb565 & 0xF800) >> 11;
	green = (rgb565 & 0x7E0) >> 5;
	blue = (rgb565 & 0x1F);

	red = red << 3;
	green = green << 2;
	blue = blue << 3;

	/* correct error pixels of samsung logo. */
	if (sw) {
		if (red > threshold)
			red = 255;
		if (green > threshold)
			green = 255;
		if (blue > threshold)
			blue = 255;
	}

	return (red << 16 | green << 8 | blue);
}

void _draw_samsung_logo(void *lcdbase, int x, int y, int w, int h,
			unsigned short *bmp)
{
	int i, j, error_range = 40;
	short k = 0;
	unsigned int pixel;
	unsigned long *fb = (unsigned long *)lcdbase;

	for (j = y; j < (y + h); j++) {
		for (i = x; i < (x + w); i++) {
			pixel = (*(bmp + k++));

			/* 40 lines under samsung logo image are error range. */
			if (j > h + y - error_range)
				*(fb + (j * panel_width) + i) =
				    conv_rgb565_to_rgb888(pixel, 1);
			else
				*(fb + (j * panel_width) + i) =
				    conv_rgb565_to_rgb888(pixel, 0);
		}
	}
}

void _draw_samsung_logo_rgb32(void *lcdbase, int x, int y, int w, int h,
			      unsigned short *bmp)
{
	int i, j;
	u32 *fb = (u32 *) lcdbase;
	u32 *bmap = (u32 *) bmp;

	fb += (panel_width * (y + h)) + x;

	for (i = y; i < (y + h); i++) {
		for (j = x; j < (x + w); j++)
			*(fb++) = *(bmap++);
		fb -= (panel_width + w);
	}
}

static void draw_samsung_logo(void *lcdbase)
{
	int x, y;
	unsigned int in_len, width, height;
	unsigned long out_len = ARRAY_SIZE(logo) * sizeof(*logo);
	void *dst = NULL;

	/* top image */
	width = 380;
	height = 50;
	x = 50 + 1;
	y = 180 + 5;

	in_len = width * height * 4;
	dst = malloc(in_len);
	if (dst == NULL) {
		printf("Error: malloc in gunzip failed!\n");
		return;
	}
	if (gunzip(dst, in_len, (uchar *) logo, &out_len) != 0) {
		free(dst);
		printf("Error: gunzip failed!\n");
		return;
	}
	if (out_len == CONFIG_SYS_VIDEO_LOGO_MAX_SIZE)
		printf("Image could be truncated"
		       " (increase CONFIG_SYS_VIDEO_LOGO_MAX_SIZE)!\n");

	_draw_samsung_logo(lcdbase, x, y, width, height, (unsigned short *)dst);

	free(dst);

	/* bottom image */
#ifdef CONFIG_CMD_BMP
	x = 140 + 1;
	y = 640;

	dst = gunzip_bmp((ulong) logo_bottom, &out_len);
	lcd_display_bitmap((ulong) dst, x, y);
	free(dst);
#endif
}

static void lcd_panel_on(vidinfo_t * vid)
{
	if (vid->mipi_power)
		vid->mipi_power();

	udelay(vid->init_delay);

	if (vid->cfg_gpio)
		vid->cfg_gpio();

	if (vid->lcd_power_on)
		vid->lcd_power_on(1);

	udelay(vid->power_on_delay);

	if (vid->reset_lcd)
		vid->reset_lcd();

	udelay(vid->reset_delay);

	if (vid->backlight_on)
		vid->backlight_on(1);

	if (vid->cfg_ldo)
		vid->cfg_ldo();

	if (vid->enable_ldo)
		vid->enable_ldo(1);
}

static void omap4_lcd_init(vidinfo_t * vid, void *lcdbase)
{
	//turn on dss_pd & dss_dlk
	init_dss_prcm();

	//initialize all of display subsystem
	omap_dss_init();

	//setup plane and window on
	omap_planes_init(vid, lcdbase);
}

void lcd_ctrl_init(void *lcdbase)
{
	char *option;

	/* initialize parameters which is specific to panel. */
	init_panel_info(&panel_info);
	panel_info.draw_logo = draw_samsung_logo;

	panel_width = panel_info.vl_width;
	panel_height = panel_info.vl_height;

	omap4_lcd_init_mem(lcdbase, &panel_info);

	option = getenv("lcd");

	memset(lcdbase, 0, panel_width * panel_height * (32 >> 3));

	omap4_lcd_init(&panel_info, lcdbase);

}

void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blud)
{
	return;
}

void lcd_enable(void)
{
	lcd_panel_on(&panel_info);
}
