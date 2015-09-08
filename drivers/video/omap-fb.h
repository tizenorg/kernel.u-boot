/*
 * drivers/video/omap-fb.h
 *
 * Copyright (C) 2011 min01.oh <min01.oh@samsung.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	S3C Frame Buffer Driver
 */

#ifndef _OMAPFB_H_
#define _OMAPFB_H_

#define ON	1
#define OFF	0

#define DEBUG
#undef DEBUG
#ifdef DEBUG
#define udebug(args...)		printf(args)
#else
#define udebug(args...)		do { } while (0)
#endif

/* OMAP TRM gives bitfields as start:end, where start is the higher bit
   number. For example 7:0 */
#define FLD_MASK(start, end)    (((1 << ((start) - (end) + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val) << (end)) & FLD_MASK(start, end))
#define FLD_GET(val, start, end) (((val) & FLD_MASK(start, end)) >> (end))
#define FLD_MOD(orig, val, start, end) \
        (((orig) & ~FLD_MASK(start, end)) | FLD_VAL(val, start, end))

enum dss_clock {
	DSS_CLK_ICK = 1 << 0,
	DSS_CLK_FCK1 = 1 << 1,
	DSS_CLK_FCK2 = 1 << 2,
	DSS_CLK_54M = 1 << 3,
	DSS_CLK_96M = 1 << 4,
};
enum omap_dss_load_mode {
	OMAP_DSS_LOAD_CLUT_AND_FRAME = 0,
	OMAP_DSS_LOAD_CLUT_ONLY = 1,
	OMAP_DSS_LOAD_FRAME_ONLY = 2,
	OMAP_DSS_LOAD_CLUT_ONCE_FRAME = 3,
};
enum omap_parallel_interface_mode {
	OMAP_DSS_PARALLELMODE_BYPASS,	/* MIPI DPI */
	OMAP_DSS_PARALLELMODE_RFBI,	/* MIPI DBI */
	OMAP_DSS_PARALLELMODE_DSI,
};

enum omap_lcd_display_type {
	OMAP_DSS_LCD_DISPLAY_STN,
	OMAP_DSS_LCD_DISPLAY_TFT,
};

enum omap_panel_config {
	OMAP_DSS_LCD_IVS = 1 << 0,
	OMAP_DSS_LCD_IHS = 1 << 1,
	OMAP_DSS_LCD_IPC = 1 << 2,
	OMAP_DSS_LCD_IEO = 1 << 3,
	OMAP_DSS_LCD_RF = 1 << 4,
	OMAP_DSS_LCD_ONOFF = 1 << 5,
	OMAP_DSS_LCD_TFT = 1 << 20,
};

enum omap_channel {
	OMAP_DSS_CHANNEL_LCD = 0,
	OMAP_DSS_CHANNEL_DIGIT = 1,
	OMAP_DSS_CHANNEL_LCD2 = 2,
};

/* Stereoscopic Panel types
 * row, column, overunder, sidebyside options
 * are with respect to native scan order
*/
enum s3d_disp_type {
	S3D_DISP_NONE = 0,
	S3D_DISP_FRAME_SEQ,
	S3D_DISP_ROW_IL,
	S3D_DISP_COL_IL,
	S3D_DISP_PIX_IL,
	S3D_DISP_CHECKB,
	S3D_DISP_OVERUNDER,
	S3D_DISP_SIDEBYSIDE,
};

/* Subsampling direction is based on native panel scan order.
*/
enum s3d_disp_sub_sampling {
	S3D_DISP_SUB_SAMPLE_NONE = 0,
	S3D_DISP_SUB_SAMPLE_V,
	S3D_DISP_SUB_SAMPLE_H,
};

enum dss_clk_source {
	/* OMAP 3 */
	DSS_SRC_DSI1_PLL_FCLK,
	DSS_SRC_DSI2_PLL_FCLK,

	/* common */
	DSS_SRC_DSS1_ALWON_FCLK,

	/* OMAP 4 */
	DSS_SRC_PLL1_CLK1,
	DSS_SRC_PLL2_CLK1,
	DSS_SRC_PLL3_CLK1,
	DSS_SRC_PLL1_CLK2,
	DSS_SRC_PLL2_CLK2,
	DSS_SRC_PLL1_CLK4,
};

struct omap_video_timings {
	/* Unit: pixels */
	u16 x_res;
	/* Unit: pixels */
	u16 y_res;
	/* Unit: KHz */
	u32 pixel_clock;
	/* Unit: pixel clocks */
	u16 hsw;		/* Horizontal synchronization pulse width */
	/* Unit: pixel clocks */
	u16 hfp;		/* Horizontal front porch */
	/* Unit: pixel clocks */
	u16 hbp;		/* Horizontal back porch */
	/* Unit: line clocks */
	u16 vsw;		/* Vertical synchronization pulse width */
	/* Unit: line clocks */
	u16 vfp;		/* Vertical front porch */
	/* Unit: line clocks */
	u16 vbp;		/* Vertical back porch */
};

struct s3d_disp_info {
	enum s3d_disp_type type;
	enum s3d_disp_sub_sampling sub_samp;
	/* enum s3d_disp_order order; */
	/* Gap between left and right views
	 * For over/under units are lines
	 * For sidebyside units are pixels
	 * For other types ignored*/
	unsigned int gap;
};

struct dss_clock_info {
	/* rates that we get with dividers below */
	unsigned long fck;

	/* dividers */
	u16 fck_div;
};

struct dispc_clock_info {
	/* rates that we get with dividers below */
	unsigned long lck;
	unsigned long pck;

	/* dividers */
	u16 lck_div;
	u16 pck_div;
};

struct omap_dss_device {
	unsigned int dss_base;
	unsigned int dispc_base;
	int data_lines;
	int state;
	enum omap_channel channel;
	struct {
		struct omap_video_timings timings;

		int acbi;	/* ac-bias pin transitions per interrupt */
		/* Unit: line clocks */
		int acb;	/* ac-bias pin frequency */

		enum omap_panel_config config;
		struct s3d_disp_info s3d_info;
	} panel;
};

extern int omap_planes_init(vidinfo_t * vid, void *lcdbase);
extern int omap_dss_init(void);
extern void init_dss_prcm(void);

#endif
