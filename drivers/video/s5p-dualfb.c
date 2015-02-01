/*
 * S5PC100 and S5PC110 LCD Controller driver.
 *
 * Author: InKi Dae <inki.dae@samsung.com>
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

#include "s5p-fb.h"
#include "opening_wvga_32.h"

#define PANEL_WIDTH		480
#define PANEL_HEIGHT		800
#define S5P_LCD_BPP		32

/* for DUAL LCD */
#define SCREEN_WIDTH		(480 * 2)
#define SCREEN_HEIGHT		800

extern void tl2796_panel_power_on(void);
extern void tl2796_panel_enable(void);
extern void tl2796_panel_init(void);

int lcd_line_length;
int lcd_color_fg;
int lcd_color_bg;

int dual_lcd = 1;

void *lcd_base;
void *lcd_console_address;

short console_col;
short console_row;

static unsigned short makepixel565(char r, char g, char b)
{
    return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static unsigned int makepixel8888(char a, char r, char g, char b)
{
	return (unsigned int)((a << 24) | (r << 16) | (g << 8)  | b);
}

static void read_image16(char* pImg, int x1pos, int y1pos, int x2pos,
		int y2pos, unsigned short pixel)
{
	unsigned short *pDst = (unsigned short *)pImg;
	unsigned int offset_s;
	int i, j;

	for(i = y1pos; i < y2pos; i++) {
		for(j = x1pos; j < x2pos; j++) {
			offset_s = i * SCREEN_WIDTH + j;
			*(pDst + offset_s) = pixel;
		}
	}
}

static void read_image32(char* pImg, int x1pos, int y1pos, int x2pos,
		int y2pos, unsigned int pixel)
{
	unsigned int *pDst = (unsigned int *)pImg;
	unsigned int offset_s;
	int i, j;

	for(i = y1pos; i < y2pos; i++) {
		for(j = x1pos; j < x2pos; j++) {
			offset_s = i * SCREEN_WIDTH + j;
			*(pDst+offset_s) = pixel;
		}
	}
}

/* LCD Panel data */
vidinfo_t panel_info = {
		.vl_col		= SCREEN_WIDTH,
		.vl_row		= SCREEN_HEIGHT,
		.vl_width	= PANEL_WIDTH,
		.vl_height	= PANEL_HEIGHT,
		.vl_clkp	= CONFIG_SYS_HIGH,
		.vl_hsp		= CONFIG_SYS_LOW,
		.vl_vsp		= CONFIG_SYS_LOW,
		.vl_dp		= CONFIG_SYS_HIGH,
		.vl_bpix	= S5P_LCD_BPP,
		.vl_lbw		= 0,
		.vl_splt	= 0,
		.vl_clor	= 1,
		.vl_tft		= 1,

		.vl_hpw		= 4,
		.vl_blw		= 8,
		.vl_elw		= 8,

		.vl_vpw		= 4,
		.vl_bfw		= 8,
		.vl_efw		= 8,
};

static void s5pc_lcd_init_mem(void *lcdbase, vidinfo_t *vid)
{
	unsigned long palette_size, palette_mem_size;
	unsigned int fb_size;

	fb_size = vid->vl_row * vid->vl_col * (vid->vl_bpix / 8);

	lcd_base = lcdbase;

	palette_size = NBITS(vid->vl_bpix) == 8 ? 256 : 16;
	palette_mem_size = palette_size * sizeof(u32);

	s5pc_fimd_lcd_init_mem((unsigned long)lcd_base, (unsigned long)fb_size, palette_size);

	udebug("fb_size=%d, screen_base=%x, palette_size=%d, palettle_mem_size=%d\n",
			fb_size, (unsigned int)lcd_base, (int)palette_size, (int)palette_mem_size);
}

static void s5pc_gpio_setup(void)
{
	s5pc_c110_gpio_setup();
}

static void s5pc_lcd_init(vidinfo_t *vid)
{
	s5pc_fimd_lcd_init(vid);
}

static void dual_lcd_test(void)
{
	int x1 = 100, y1 = 50, x2 = 100, y2 = 50, i;
	/* red */
	read_image32((char *)lcd_base, 0+x1, 0+y1, 960-x2, 200,
			makepixel8888(0, 255, 0, 0));
	/* green */
	read_image32((char *)lcd_base, 0+x1, 200, 960-x2, 400,
			makepixel8888(0, 0, 255, 0));
	/* blue */
	read_image32((char *)lcd_base, 0+x1, 400, 960-x2, 600,
			makepixel8888(0, 0, 0, 255));
	/* write */
	read_image32((char *)lcd_base, 0+x1, 600, 960-x2, 800-y2,
			makepixel8888(0, 255, 255, 255));
}

static void repeat_test(void)
{
	int i;

	for (i = 100; i < 200; i++) {
		read_image32((char *)lcd_base, 0, 0, 960, 800,
				makepixel8888(0, (0x1234 * i) & 255, (0x5421 * i) & 255, (0x6789 * i) & 255));
	}
}

void draw_bitmap(void *lcdbase, int x, int y, int w, int h, unsigned long *bmp)
{
	int i, j, k = 0;
	unsigned long *fb = (unsigned  long*)lcdbase;

	for (j = y; j < (y + h); j++) {
		for (i = x; i < (w + x); i++) {
			*(fb + (j * SCREEN_WIDTH) + i) =
			    *(unsigned long *)(bmp + k);
			k++;
		}
	}
}

static void draw_samsung_logo(void* lcdbase)
{
	int x, y;

	x = (SCREEN_WIDTH - 138) / 2;
	y = (SCREEN_HEIGHT - 28) / 2 - 5;

	draw_bitmap(lcdbase, x, y, 138, 28, (unsigned long *)opening_32);
}

static void lcd_panel_on(void)
{
	tl2796_c110_panel_init();
	tl2796_c110_panel_power_on();

	tl2796_panel_enable();
}

void lcd_ctrl_init(void *lcdbase)
{
	s5pc_lcd_init_mem(lcdbase, &panel_info);

	memset(lcd_base, 0, panel_info.vl_col *	panel_info.vl_row * S5P_LCD_BPP >> 3);

	s5pc_gpio_setup();

	s5pc_lcd_init(&panel_info);
}

void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blud)
{
	return;
}

void lcd_enable(void)
{
	char *option;

	lcd_panel_on();

	option = getenv("lcd");
	if (strcmp(option, "test") == 0)
		dual_lcd_test();
	else if (strcmp(option, "on") == 0)
		draw_samsung_logo(lcd_base);
	else if (strcmp(option, "repeat") == 0)
		repeat_test();
	else
		draw_samsung_logo(lcd_base);
}

ulong calc_fbsize(void)
{
	return s5pc_fimd_calc_fbsize();
}

void s5pc1xxfb_test(void *lcdbase)
{
	lcd_ctrl_init(lcdbase);
	lcd_enable();
}
