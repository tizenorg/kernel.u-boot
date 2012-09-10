/*
 * drivers/video/s5pcfb.h
 *
 * Copyright (C) 2008 Jinsung Yang <jsgood.yang@samsung.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	S3C Frame Buffer Driver
 *	based on skeletonfb.c, sa1100fb.h, s3c2410fb.c
 */

#ifndef _S5PCFB_H_
#define _S5PCFB_H_

#define ON	1
#define OFF	0

#define DEBUG
#undef DEBUG
#ifdef DEBUG
#define udebug(args...)		printf(args)
#else
#define udebug(args...)		do { } while (0)
#endif

typedef struct {
	int width;
	int height;
	int bpp;
	int offset;
	int v_width;
	int v_height;
} s5pcfb_vs_info_t;

typedef struct {
	int direction;
	unsigned int compkey_red;
	unsigned int compkey_green;
	unsigned int compkey_blue;
} s5pcfb_color_key_info_t;

typedef struct {
	unsigned int colval_red;
	unsigned int colval_green;
	unsigned int colval_blue;
} s5pcfb_color_val_info_t;

typedef struct {
	/* Screen size */
	int width;
	int height;

	/* Screen info */
	int xres;
	int yres;

	/* Virtual Screen info */
	int xres_virtual;
	int yres_virtual;
	int xoffset;
	int yoffset;

	/* OSD Screen size */
	int osd_width;
	int osd_height;

	/* OSD Screen info */
	int osd_xres;
	int osd_yres;

	/* OSD Screen info */
	int osd_xres_virtual;
	int osd_yres_virtual;

	int bpp;
	int bytes_per_pixel;
	unsigned long pixclock;

	int hsync_len;
	int left_margin;
	int right_margin;
	int vsync_len;
	int upper_margin;
	int lower_margin;
	int sync;

	int cmap_grayscale:1;
	int cmap_inverse:1;
	int cmap_static:1;
	int unused:29;

	/* backlight info */
	int backlight_min;
	int backlight_max;
	int backlight_default;

	int vs_offset;
	int brightness;
	int palette_win;
	int backlight_level;
	int backlight_power;
	int lcd_power;

	unsigned long palette;
	unsigned long screen;
	unsigned long palette_size;

	/* lcd configuration registers */
	unsigned long lcdcon1;
	unsigned long lcdcon2;

    unsigned long lcdcon3;
	unsigned long lcdcon4;
	unsigned long lcdcon5;

	/* GPIOs */
	unsigned long gpcup;
	unsigned long gpcup_mask;
	unsigned long gpccon;
	unsigned long gpccon_mask;
	unsigned long gpdup;
	unsigned long gpdup_mask;
	unsigned long gpdcon;
	unsigned long gpdcon_mask;

	/* lpc3600 control register */
	unsigned long lpcsel;
	unsigned long lcdtcon1;
	unsigned long lcdtcon2;
	unsigned long lcdtcon3;
	unsigned long lcdosd1;
	unsigned long lcdosd2;
	unsigned long lcdosd3;
	unsigned long lcdsaddrb1;
	unsigned long lcdsaddrb2;
	unsigned long lcdsaddrf1;
	unsigned long lcdsaddrf2;
	unsigned long lcdeaddrb1;
	unsigned long lcdeaddrb2;
	unsigned long lcdeaddrf1;
	unsigned long lcdeaddrf2;
	unsigned long lcdvscrb1;
	unsigned long lcdvscrb2;
	unsigned long lcdvscrf1;
	unsigned long lcdvscrf2;
	unsigned long lcdintcon;
	unsigned long lcdkeycon;
	unsigned long lcdkeyval;
	unsigned long lcdbgcon;
	unsigned long lcdfgcon;
	unsigned long lcddithcon;

	unsigned long vidcon0;
	unsigned long vidcon1;
	unsigned long vidtcon0;
	unsigned long vidtcon1;
	unsigned long vidtcon2;
	unsigned long vidtcon3;
	unsigned long wincon0;
	unsigned long wincon2;
	unsigned long wincon1;
	unsigned long wincon3;
	unsigned long wincon4;

	unsigned long vidosd0a;
	unsigned long vidosd0b;
	unsigned long vidosd0c;
	unsigned long vidosd1a;
	unsigned long vidosd1b;
	unsigned long vidosd1c;
	unsigned long vidosd1d;
	unsigned long vidosd2a;
	unsigned long vidosd2b;
	unsigned long vidosd2c;
	unsigned long vidosd2d;
	unsigned long vidosd3a;
	unsigned long vidosd3b;
	unsigned long vidosd3c;
	unsigned long vidosd4a;
	unsigned long vidosd4b;
	unsigned long vidosd4c;

	unsigned long vidw00add0b0;
	unsigned long vidw00add0b1;
	unsigned long vidw01add0;
	unsigned long vidw01add0b0;
	unsigned long vidw01add0b1;

	unsigned long vidw00add1b0;
	unsigned long vidw00add1b1;
	unsigned long vidw01add1;
	unsigned long vidw01add1b0;
	unsigned long vidw01add1b1;

	unsigned long vidw00add2b0;
	unsigned long vidw00add2b1;

	unsigned long vidw02add0;
	unsigned long vidw03add0;
	unsigned long vidw04add0;

	unsigned long vidw02add1;
	unsigned long vidw03add1;
	unsigned long vidw04add1;
	unsigned long vidw00add2;
	unsigned long vidw01add2;
	unsigned long vidw02add2;
	unsigned long vidw03add2;
	unsigned long vidw04add2;

	unsigned long vidintcon;
	unsigned long vidintcon0;
	unsigned long vidintcon1;
	unsigned long w1keycon0;
	unsigned long w1keycon1;
	unsigned long w2keycon0;
	unsigned long w2keycon1;
	unsigned long w3keycon0;
	unsigned long w3keycon1;
	unsigned long w4keycon0;
	unsigned long w4keycon1;

	unsigned long win0map;
	unsigned long win1map;
	unsigned long win2map;
	unsigned long win3map;
	unsigned long win4map;

	unsigned long wpalcon;
	unsigned long dithmode;
	unsigned long intclr0;
	unsigned long intclr1;
	unsigned long intclr2;

	unsigned long win0pal;
	unsigned long win1pal;

}s5pcfb_fimd_info_t;

enum s5pcfb_rgb_mode_t {
	MODE_RGB_P = 0,
	MODE_BGR_P = 1,
	MODE_RGB_S = 2,
	MODE_BGR_S = 3,
};

enum s3cfb_cpu_auto_cmd_rate {
	DISABLE_AUTO_FRM,
	PER_TWO_FRM,
	PER_FOUR_FRM,
	PER_SIX_FRM,
	PER_EIGHT_FRM,
	PER_TEN_FRM,
	PER_TWELVE_FRM,
	PER_FOURTEEN_FRM,
	PER_SIXTEEN_FRM,
	PER_EIGHTEEN_FRM,
	PER_TWENTY_FRM,
	PER_TWENTY_TWO_FRM,
	PER_TWENTY_FOUR_FRM,
	PER_TWENTY_SIX_FRM,
	PER_TWENTY_EIGHT_FRM,
	PER_THIRTY_FRM,
};

void s5pc_fimd_lcd_init_mem(unsigned long screen_base, unsigned long fb_size,
	unsigned long palette_size);
void s5pc_fimd_lcd_init(vidinfo_t *vid);
unsigned long s5pc_fimd_calc_fbsize(void);
void s5pc_c100_gpio_setup(void);
void s5pc_c110_gpio_setup(void);

extern void s6e63m0_lcd_panel_init(void);
extern void s6e63m0_lcd_panel_power_on(void);
extern void s6e63m0_lcd_panel_enable(void);

extern void s6d16a0x_lcd_panel_init(void);
extern void s6d16a0x_lcd_panel_power_on(void);
extern void s6d16a0x_lcd_panel_enable(void);

extern void ams701ka_lcd_panel_init(void);
extern void ams701ka_lcd_panel_power_on(void);
extern void ams701ka_lcd_panel_enable(void);

#endif

