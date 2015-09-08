/*
 * OMAP4430 Dss and Display controller Specific driver.
 *
 * Author: Min oh<min01.ohe@samsung.com>
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
#include <stdarg.h>
#include <linux/types.h>
#include <asm/io.h>
#include <lcd.h>
#include <div64.h>
#include <asm/arch/regs-fb.h>
#include "omap-fb.h"

#define mdelay(n) ({unsigned long msec = (n); while (msec--) udelay(1000); })

#if 1
#define DSS_BASE              0x48040000
#define DISPC_BASE            0x48041000
#else
#define DSS_BASE              0x58000000
#define DSS_BASE_SYSCONFIG    0x58000010
#define DISPC_BASE            0x58001000

#endif

#define DISPC_SZ_REGS           SZ_1K

struct dss_reg {
	u16 idx;
};

struct dispc_reg {
	u16 idx;
};

#define DISPC_REG(idx)                  ((const struct dispc_reg) { idx })
#define DSS_REG(idx)                    ((const struct dss_reg) { idx })

#define DSS_REVISION                    DSS_REG(0x0000)
#define DSS_SYSCONFIG                   DSS_REG(0x0010)
#define DSS_SYSSTATUS                   DSS_REG(0x0014)
#define DSS_IRQSTATUS                   DSS_REG(0x0018)
#define DSS_CONTROL                     DSS_REG(0x0040)
#define DSS_STATUS                      DSS_REG(0x005C)

#define DISPC_VID_REG(n, idx)           DISPC_REG(0x00BC + (n)*0x90 + idx)
#define DISPC_VID_BA0(n)                DISPC_VID_REG(n, 0x0000)
#define DISPC_VID_BA1(n)                DISPC_VID_REG(n, 0x0004)
#define DISPC_VID_POSITION(n)           DISPC_VID_REG(n, 0x0008)
#define DISPC_VID_SIZE(n)               DISPC_VID_REG(n, 0x000C)
#define DISPC_VID_ATTRIBUTES(n)         DISPC_VID_REG(n, 0x0010)
#define DISPC_VID_FIFO_THRESHOLD(n)     DISPC_VID_REG(n, 0x0014)
#define DISPC_VID_FIFO_SIZE_STATUS(n)   DISPC_VID_REG(n, 0x0018)
#define DISPC_VID_ROW_INC(n)            DISPC_VID_REG(n, 0x001C)
#define DISPC_VID_PIXEL_INC(n)          DISPC_VID_REG(n, 0x0020)
#define DISPC_VID_FIR(n)                DISPC_VID_REG(n, 0x0024)
#define DISPC_VID_PICTURE_SIZE(n)       DISPC_VID_REG(n, 0x0028)
#define DISPC_VID_ACCU0(n)              DISPC_VID_REG(n, 0x002C)
#define DISPC_VID_ACCU1(n)              DISPC_VID_REG(n, 0x0030)

struct omap_dss_device dss_device;

void dss_write_reg(const struct dss_reg idx, u32 val)
{
	writel(val, dss_device.dss_base + idx.idx);
}

u32 dss_read_reg(const struct dss_reg idx)
{
	return readl(dss_device.dss_base + idx.idx);
}

void dispc_write_reg(const struct dispc_reg idx, u32 val)
{
	writel(val, dss_device.dispc_base + idx.idx);
}

u32 dispc_read_reg(const struct dispc_reg idx)
{
	return readl(dss_device.dispc_base + idx.idx);
}

#define DISPC_REG_GET(idx, start, end) \
        FLD_GET(dispc_read_reg(idx), start, end)

#define DISPC_REG_FLD_MOD(idx, val, start, end)                               \
        dispc_write_reg(idx, FLD_MOD(dispc_read_reg(idx), val, start, end))

#define DSS_REG_GET(idx, start, end) \
        FLD_GET(dss_read_reg(idx), start, end)

#define DSS_REG_FLD_MOD(idx, val, start, end) \
        dss_write_reg(idx, FLD_MOD(dss_read_reg(idx), val, start, end))

/*
 * enables mainclk (DSS clocks on OMAP4 if any device is enabled.
 * Returns 0 on success.
 */

void dispc_set_parallel_interface_mode(enum omap_channel channel,
				  enum omap_parallel_interface_mode mode)
{
	u32 l;
	int stallmode;
	int gpout0 = 1;
	int gpout1;

	switch (mode) {
	case OMAP_DSS_PARALLELMODE_BYPASS:
		stallmode = 0;
		gpout1 = 1;
		break;

	case OMAP_DSS_PARALLELMODE_RFBI:
		stallmode = 1;
		gpout1 = 0;
		break;

	case OMAP_DSS_PARALLELMODE_DSI:
		stallmode = 1;
		gpout1 = 1;
		break;

	default:
		BUG();
		return;
	}

	if (OMAP_DSS_CHANNEL_LCD2 == channel) {
		l = dispc_read_reg(DISPC_CONTROL2);
		l = FLD_MOD(l, stallmode, 11, 11);

		dispc_write_reg(DISPC_CONTROL2, l);
	} else {
		l = dispc_read_reg(DISPC_CONTROL);
		l = FLD_MOD(l, stallmode, 11, 11);
		l = FLD_MOD(l, gpout0, 15, 15);
		l = FLD_MOD(l, gpout1, 16, 16);

		dispc_write_reg(DISPC_CONTROL, l);
	}

}

void dispc_set_lcd_display_type(enum omap_channel channel,
			   enum omap_lcd_display_type type)
{
	int mode;

	switch (type) {
	case OMAP_DSS_LCD_DISPLAY_STN:
		mode = 0;
		break;

	case OMAP_DSS_LCD_DISPLAY_TFT:
		mode = 1;
		break;

	default:
		return;
	}
	if (OMAP_DSS_CHANNEL_LCD2 == channel)
		DISPC_REG_FLD_MOD(DISPC_CONTROL2, mode, 3, 3);
	else
		DISPC_REG_FLD_MOD(DISPC_CONTROL, mode, 3, 3);

}

void dispc_set_tft_data_lines(enum omap_channel channel, u8 data_lines)
{
	int code;

	switch (data_lines) {
	case 12:
		code = 0;
		break;
	case 16:
		code = 1;
		break;
	case 18:
		code = 2;
		break;
	case 24:
		code = 3;
		break;
	default:
		return;
	}
	if (channel == OMAP_DSS_CHANNEL_LCD2)
		DISPC_REG_FLD_MOD(DISPC_CONTROL2, code, 9, 8);
	else
		DISPC_REG_FLD_MOD(DISPC_CONTROL, code, 9, 8);
}

static int dpi_basic_init(struct omap_dss_device *dssdev)
{
	int is_tft;

	is_tft = (dssdev->panel.config & OMAP_DSS_LCD_TFT) != 0;

	dispc_set_parallel_interface_mode(dssdev->channel,
					  OMAP_DSS_PARALLELMODE_BYPASS);
	dispc_set_lcd_display_type(dssdev->channel, is_tft ?
				   OMAP_DSS_LCD_DISPLAY_TFT :
				   OMAP_DSS_LCD_DISPLAY_STN);
	dispc_set_tft_data_lines(dssdev->channel, dssdev->data_lines);

	return 0;
}

static void _dispc_set_lcd_timings(enum omap_channel channel, int hsw,
		       int hfp, int hbp, int vsw, int vfp, int vbp)
{
	u32 timing_h, timing_v;

	timing_h = FLD_VAL(hsw - 1, 7, 0) | FLD_VAL(hfp - 1, 19, 8) |
	    FLD_VAL(hbp - 1, 31, 20);

	timing_v = FLD_VAL(vsw - 1, 7, 0) | FLD_VAL(vfp, 19, 8) |
	    FLD_VAL(vbp, 31, 20);

	if (OMAP_DSS_CHANNEL_LCD2 == channel) {
		dispc_write_reg(DISPC_TIMING_H2, timing_h);
		dispc_write_reg(DISPC_TIMING_V2, timing_v);
	} else {
		dispc_write_reg(DISPC_TIMING_H, timing_h);
		dispc_write_reg(DISPC_TIMING_V, timing_v);
	}

}

void dispc_set_lcd_size(enum omap_channel channel, u16 width, u16 height)
{
	u32 val;

	val = FLD_VAL(height - 1, 26, 16) | FLD_VAL(width - 1, 10, 0);

	if (OMAP_DSS_CHANNEL_LCD2 == channel)
		dispc_write_reg(DISPC_SIZE_LCD2, val);
	else
		dispc_write_reg(DISPC_SIZE_LCD, val);
}

/* change name to mode? */
void dispc_set_lcd_timings(enum omap_channel channel,
		      struct omap_video_timings *timings)
{
	/* config timing for hsync & vsync */
	_dispc_set_lcd_timings(channel, timings->hsw, timings->hfp,
			       timings->hbp, timings->vsw, timings->vfp,
			       timings->vbp);
	/* configure lcd size */
	dispc_set_lcd_size(channel, timings->x_res, timings->y_res);

	debug("channel %u xres %u yres %u\n", channel, timings->x_res,
	      timings->y_res);
	debug("hsw %d hfp %d hbp %d vsw %d vfp %d vbp %d\n",
	      timings->hsw, timings->hfp, timings->hbp,
	      timings->vsw, timings->vfp, timings->vbp);

}

static int dpi_set_dispc_clk(enum omap_channel channel,
		  unsigned long pck_req, unsigned long *fck, int *lck_div,
		  int *pck_div)
{
	struct dss_clock_info dss_cinfo;
	struct dispc_clock_info dispc_cinfo;

	/* h/w coding temporarily */
	dispc_cinfo.lck_div = 0x1;
	dispc_cinfo.pck_div = 0x7;

	if (OMAP_DSS_CHANNEL_LCD2 == channel)
		dispc_write_reg(DISPC_DIVISOR2,
				FLD_VAL(dispc_cinfo.lck_div, 23,
					16) | FLD_VAL(dispc_cinfo.pck_div, 7,
						      0));
	else
		dispc_write_reg(DISPC_DIVISOR1,
				FLD_VAL(dispc_cinfo.lck_div, 23,
					16) | FLD_VAL(dispc_cinfo.pck_div, 7,
						      0));

	return 0;
}

void dispc_set_pol_freq(enum omap_channel channel, enum omap_panel_config config,
		   u8 acbi, u8 acb)
{

	u32 l = 0;

	l |= FLD_VAL((config & OMAP_DSS_LCD_ONOFF) != 0, 17, 17);
	l |= FLD_VAL((config & OMAP_DSS_LCD_RF) != 0, 16, 16);
	/* l |= FLD_VAL((config & OMAP_DSS_LCD_IEO) != 0, 15, 15); */
	/* l |= FLD_VAL((config & OMAP_DSS_LCD_IPC) != 0, 14, 14); */
	/* l |= FLD_VAL((config & OMAP_DSS_LCD_IHS) != 0, 13, 13); */
	/* l |= FLD_VAL((config & OMAP_DSS_LCD_IVS) != 0, 12, 12); */

	/* this setting value is very important to turn on DISP CONTROL */
	l |= FLD_VAL(1, 15, 15);
	l |= FLD_VAL(1, 14, 14);
	l |= FLD_VAL(1, 13, 13);
	l |= FLD_VAL(1, 12, 12);
	l |= FLD_VAL(acbi, 11, 8);
	l |= FLD_VAL(acb, 7, 0);

	if (OMAP_DSS_CHANNEL_LCD2 == channel)
		dispc_write_reg(DISPC_POL_FREQ2, l);
	else
		dispc_write_reg(DISPC_POL_FREQ, l);

}

static int dpi_set_clk_timing(struct omap_dss_device *dssdev)
{

	struct omap_video_timings *t = &dssdev->panel.timings;
	int lck_div = 0, pck_div = 0;
	unsigned long fck = 0;

	dispc_set_pol_freq(dssdev->channel, dssdev->panel.config,
			   dssdev->panel.acbi, dssdev->panel.acb);

	dpi_set_dispc_clk(dssdev->channel,
			  t->pixel_clock * 1000, &fck, &lck_div, &pck_div);

	dispc_set_lcd_timings(dssdev->channel, t);

	return 0;

}

int dpi_setup_plane(struct omap_dss_device *dessdev, void *lcdbase)
{
	u32 val = (u32) lcdbase;

	dispc_write_reg(DISPC_GFX_BA0, val);
	dispc_write_reg(DISPC_GFX_BA1, val);

	val = 0;
	/* configure the position of graphics window */
	val = ((0x00 << 16) |	/* Y position */
	       (0x00 << 0));	/* X position */

	dispc_write_reg(DISPC_GFX_POSITION, val);

	val = 0;
	/* size of graphics window */
	val = (((dessdev->panel.timings.y_res - 1) << 16) |
	       ((dessdev->panel.timings.x_res - 1) << 0));
	dispc_write_reg(DISPC_GFX_SIZE, val);

	val = 0;
	val = ((0x1 << 30) |
	       (0x1 << 25) | (0x1 << 7) | (0x1 << 4) | (0x1 << 3) | (0x1 << 0));

	dispc_write_reg(DISPC_GFX_ATTRIBUTES, val);

	val = 0;
	val = ((0x4FF << 16) |	/* graphics FIFO high threshold */
	       (0x4f8 << 0));	/* graphics FIFO low threshold */

	dispc_write_reg(DISPC_GFX_FIFO_THRESHOLD, val);

	return 0;
}

void dss_reg_dump(void)
{
	int i = 0;
	u32 val;
	u32 reg = DISPC_BASE;

	while (1) {
		val = readl(reg);
		debug("dispc_controller:[reg:0x%x=0x%x]\n", reg, val);

		reg = reg + 4;
		i++;

		if (i > 516)
			return;

	}
}

int dpi_window_on(enum omap_channel channel)
{
	u32 val;

	if (OMAP_DSS_CHANNEL_LCD2 == channel) {
		/* LCD output enabled */
		val = dispc_read_reg(DISPC_CONTROL2) | (0x1 << 0);
		dispc_write_reg(DISPC_CONTROL2, val);
		/* LCD GO command */
		val = dispc_read_reg(DISPC_CONTROL2) | (0x1 << 5);
		dispc_write_reg(DISPC_CONTROL2, val);
	} else {
		/* LCD output enabled */
		val = dispc_read_reg(DISPC_CONTROL) | (0x1 << 0);
		dispc_write_reg(DISPC_CONTROL, val);
		/* LCD GO command */
		val = dispc_read_reg(DISPC_CONTROL) | (0x1 << 5);
		dispc_write_reg(DISPC_CONTROL, val);
	}

	val = 0;
	val = 0xd640;
	dispc_write_reg(DISPC_IRQENABLE, val);

	return 0;
}

void omap_lcd_get_timing_value(vidinfo_t * vid)
{

	dss_device.panel.timings.x_res = vid->vl_width;
	dss_device.panel.timings.y_res = vid->vl_height;
	dss_device.panel.timings.hsw = vid->vl_hspw;
	dss_device.panel.timings.hfp = vid->vl_hfpd;
	dss_device.panel.timings.hbp = vid->vl_hbpd;
	dss_device.panel.timings.vsw = vid->vl_vspw;
	dss_device.panel.timings.vfp = vid->vl_vfpd;
	dss_device.panel.timings.vbp = vid->vl_vbpd;
	dss_device.panel.acbi = 0;
	dss_device.panel.acb = 0;

}

int omap_dispc_init(void)
{
	int rev;
	u32 l;

	dss_device.dispc_base = DISPC_BASE;
	/* sw reset of dispc */
	DISPC_REG_FLD_MOD(DISPC_SYSCONFIG, 0x1, 1, 1);
	/* check sw reset done */
	while (!DISPC_REG_GET(DISPC_SYSSTATUS, 0, 0)) {
		debug("wait unitl the dispc_sw_reset is done!\n");
	}

	/* dispc_init_config  */
	l = dispc_read_reg(DISPC_SYSCONFIG);
	l = FLD_MOD(l, 2, 13, 12);	/* MIDLEMODE: smart standby */
	l = FLD_MOD(l, 2, 4, 3);	/* SIDLEMODE: smart idle */
	l = FLD_MOD(l, 1, 2, 2);	/* ENWAKEUP */
	l = FLD_MOD(l, 1, 0, 0);	/* AUTOIDLE */
	dispc_write_reg(DISPC_SYSCONFIG, l);

	/* disable all the interrupts */
	DISPC_REG_FLD_MOD(DISPC_IRQENABLE, 0x00, 7, 0);
	DISPC_REG_FLD_MOD(DISPC_IRQENABLE, 0x00, 15, 8);
	DISPC_REG_FLD_MOD(DISPC_IRQENABLE, 0x00, 23, 16);

	DISPC_REG_FLD_MOD(DISPC_CONFIG, OMAP_DSS_LOAD_FRAME_ONLY, 2, 1);

	/* generating core function clk */
	DISPC_REG_FLD_MOD(DISPC_DIVISOR, 1, 0, 0);
	DISPC_REG_FLD_MOD(DISPC_DIVISOR, 1, 23, 16);

	rev = dispc_read_reg(DISPC_REVISION);
	debug("OMAP DISPC rev %d.%d\n", FLD_GET(rev, 7, 4), FLD_GET(rev, 3, 0));

	return 0;
}

int omap_planes_init(vidinfo_t * vid, void *lcdbase)
{
	int ret = 0;
	struct omap_dss_device *dssdev = &dss_device;

	dss_device.panel.config =
	    OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_ONOFF;
	dss_device.channel = OMAP_DSS_CHANNEL_LCD2;	/* Parallel RGB I/F use LCD2  */
	dss_device.data_lines = vid->vl_dline;	/* vid->vl_bpix;  */
	dssdev = &dss_device;

	omap_lcd_get_timing_value(vid);

	ret = dpi_basic_init(dssdev);
	/* clock setting */
	ret = dpi_set_clk_timing(dssdev);

	udelay(10);

	/* set gfx layer */
	ret = dpi_setup_plane(dssdev, lcdbase);
	/* window_on */
	ret = dpi_window_on(dss_device.channel);

	return ret;
}

int omap_dss_init(void)
{
	int rev = 0;
	u32 val;

	dss_device.dss_base = DSS_BASE;

	dss_write_reg(DSS_CONTROL, 0x0);

	val = dss_read_reg(DSS_SYSSTATUS);
	debug("DSS_SYSSTATUS:0x%x\n", val);

	val = dss_read_reg(DSS_CONTROL);
	debug("DSS_CONTROL:0x%x\n", val);

	val = dss_read_reg(DSS_STATUS);
	debug("DSS_STATUS:0x%x\n", val);

	rev = dss_read_reg(DSS_REVISION);
	debug("OMAP DSS rev %d.%d\n", FLD_GET(rev, 7, 4), FLD_GET(rev, 3, 0));

	omap_dispc_init();

	return 0;
}

void init_dss_prcm(void)
{
	u32 reg, val;

	/* PM_DSS */
	reg = PM_DSS_PWRSTCTRL;
	writel(0x3, reg);	/* power ON */

	/* CM_DSS */
	reg = CM_DSS_CLKSTCTRL;
	writel(0x2, reg);	/* software wake-up */

	/* iclk_dss */
	reg = CM_DSS_DSS_CLKCTRL;
	writel(0xf02, reg);

	reg = CM_DSS_CLKSTCTRL;
	while ((readl(reg) & 0xF00) != 0xF00) ;

	reg = CM_DSS_CLKSTCTRL;
	writel(0x3, reg);

	mdelay(40);
}
