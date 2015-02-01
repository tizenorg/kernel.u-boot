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
#include <asm/arch/mipi_dsim.h>
#include <lcd.h>
#include <malloc.h>

#include "s5p-fb.h"
#include "logo.h"
/*
#include "logo_rgb24_wvga_portrait.h"
#include "opening_logo_rgb24_143_44.h"
*/

extern void tl2796_panel_power_on(void);
extern void tl2796_panel_enable(void);
extern void tl2796_panel_init(void);

int lcd_line_length;
int lcd_color_fg;
int lcd_color_bg;

void *lcd_base;
void *lcd_console_address;

short console_col;
short console_row;

static unsigned int panel_width, panel_height;

#ifdef USE_LCD_TEST
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
			offset_s = i * panel_width + j;
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

	for (i = y1pos; i < y2pos; i++) {
		for (j = x1pos; j < x2pos; j++) {
			offset_s = i * panel_width + j;
			*(pDst+offset_s) = pixel;
		}
	}
}
#endif

/* LCD Panel data */
vidinfo_t panel_info;

static void s5pc_lcd_init_mem(void *lcdbase, vidinfo_t *vid)
{
	unsigned long palette_size, palette_mem_size;
	unsigned int fb_size;

	fb_size = vid->vl_row * vid->vl_col * (vid->vl_bpix >> 3);

	lcd_base = lcdbase;

	palette_size = NBITS(vid->vl_bpix) == 8 ? 256 : 16;
	palette_mem_size = palette_size * sizeof(u32);

	s5pc_fimd_lcd_init_mem((unsigned long)lcd_base, (unsigned long)fb_size, palette_size);

	udebug("fb_size=%d, screen_base=%x, palette_size=%d, palettle_mem_size=%d\n",
			fb_size, (unsigned int)lcd_base, (int)palette_size, (int)palette_mem_size);
}

static void s5pc_lcd_init(vidinfo_t *vid)
{
	s5pc_fimd_lcd_init(vid);
}

#ifdef USE_LCD_TEST
static void lcd_test(unsigned int width, unsigned int height)
{
	unsigned int height_level = height / 3;

	/* red */
	read_image32((char *)lcd_base, 0, 0, width, height_level,
			makepixel8888(0, 255, 0, 0));
	/* green */
	read_image32((char *)lcd_base, 0, height_level, width,
		height_level * 2, makepixel8888(0, 0, 255, 0));
	/* blue */
	read_image32((char *)lcd_base, 0, height_level * 2, width, height,
			makepixel8888(0, 0, 0, 255));
}
#endif

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

#if 0
void draw_bitmap(void *lcdbase, int x, int y, int w, int h, unsigned long *bmp)
{
	int i, j;
	short k = 0;
	unsigned long *fb = (unsigned  long*)lcdbase;

	for (j = y; j < (y + h); j++) {
		for (i = x; i < (x + w); i++)
			*(fb + (j * panel_width) + i) = *(bmp + k++);
	}
}
#endif

void _draw_samsung_logo(void *lcdbase, int x, int y, int w, int h, unsigned short *bmp)
{
	int i, j, error_range = 40;
	short k = 0;
	unsigned int pixel;
	unsigned long *fb = (unsigned  long*)lcdbase;

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

void rotate_samsung_logo(void *lcdbase, int x, int y, int w, int h, unsigned short *bmp)
{
	int i, j, error_range = 40;
	short k = 0;
	unsigned int pixel;
	unsigned long *fb = (unsigned  long*)lcdbase;

	for (j = (y + h); j > y; j--) {
		for (i = x; i < (x + w); i++) {
			pixel = (*(bmp + k++));

			/* 40 lines under samsung logo image are error range. */
			if (j > h + y - error_range)
				*(fb + (i * panel_width) + j) =
					conv_rgb565_to_rgb888(pixel, 1);
			else
				*(fb + (i * panel_width) + j) =
					conv_rgb565_to_rgb888(pixel, 0);
		}
	}
}

static void draw_samsung_logo(void* lcdbase)
{
	int x, y;
	unsigned int in_len, width, height;
	unsigned long out_len = ARRAY_SIZE(logo) * sizeof(*logo);
	void *dst = NULL;

	width = 298;
	height = 78;
	x = ((panel_width - width) >> 1);
	y = ((panel_height - height) >> 1) - 5;

	in_len = width * height * 4;
	dst = malloc(in_len);
	if (dst == NULL) {
		printf("Error: malloc in gunzip failed!\n");
		return;
	}
	if (gunzip(dst, in_len, (uchar *)logo, &out_len) != 0) {
		free(dst);
		return;
	}
	if (out_len == CONFIG_SYS_VIDEO_LOGO_MAX_SIZE)
		printf("Image could be truncated"
				" (increase CONFIG_SYS_VIDEO_LOGO_MAX_SIZE)!\n");

	if (panel_info.lcd_rotate == SCREEN_NORMAL)
		_draw_samsung_logo(lcdbase, x, y, width, height, (unsigned short *) dst);
	else {
		x = ((panel_height- width) >> 1);
		y = ((panel_width - height) >> 1);
		rotate_samsung_logo(lcdbase, x, y, width, height, (unsigned short *) dst);
	}

	free(dst);
}

static void lcd_panel_on(vidinfo_t *vid)
{
	if (vid->mipi_power)
		vid->mipi_power();

	udelay(vid->init_delay);

	if (vid->backlight_reset)
		vid->backlight_reset();

	if (vid->cfg_gpio)
		vid->cfg_gpio();

	if (vid->lcd_power_on)
		vid->lcd_power_on(1);

	udelay(vid->power_on_delay);

	if (vid->reset_lcd) {
		vid->reset_lcd();
		udelay(vid->reset_delay);
	}

	if (vid->backlight_on)
		vid->backlight_on(1);

#ifdef CONFIG_DSIM
	s5p_mipi_dsi_init();
#endif

	if (vid->cfg_ldo)
		vid->cfg_ldo();

	if (vid->enable_ldo)
		vid->enable_ldo(1);
}

/* extern void init_onenand_ext2(void); */
extern void init_panel_info(vidinfo_t *vid);
extern int s5p_no_lcd_support(void);
extern int fimd_clk_set(void);

void lcd_ctrl_init(void *lcdbase)
{
	char *option;

	if (s5p_no_lcd_support())
		return;

	fimd_clk_set();

	/* initialize parameters which is specific to panel. */
	init_panel_info(&panel_info);

	panel_width = panel_info.vl_width;
	panel_height = panel_info.vl_height;

	s5pc_lcd_init_mem(lcdbase, &panel_info);

	/*
	option = getenv("lcd");

	if (strcmp(option, "test") == 0) {
		memset(lcdbase, 0, PANEL_WIDTH*PANEL_HEIGHT*S5P_LCD_BPP >> 3);
#ifdef USE_LCD_TEST
		lcd_test(panel_width, panel_height);
#endif
	} else if (strcmp(option, "image") == 0)
		memcpy(lcdbase, LOGO_RGB24, PANEL_WIDTH*PANEL_HEIGHT*S5P_LCD_BPP >> 3);
	else {
		memset(lcdbase, 0, PANEL_WIDTH*PANEL_HEIGHT*S5P_LCD_BPP >> 3);
		draw_samsung_logo(lcdbase);
	}
	*/

	memset(lcdbase, 0, panel_width * panel_height * (32 >> 3));

	if (!panel_info.board_logo)
		draw_samsung_logo(lcdbase);

	s5pc_lcd_init(&panel_info);

	/* font test */
	/*
	init_font();
	set_font_color(FONT_WHITE);
	fb_printf("Test\n");
	exit_font();
	*/

	/* init_onenand_ext2(); */
}


void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blud)
{
	return;
}

void lcd_enable(void)
{
	lcd_panel_on(&panel_info);
}

ulong calc_fbsize(void)
{
	return s5pc_fimd_calc_fbsize();
}
