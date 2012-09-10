/*
 * S5PC100 and S5PC110 LCD Controller Specific driver.
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
#include <stdarg.h>
#include <linux/types.h>
#include <asm/io.h>
#include <lcd.h>
#include <div64.h>

#include <asm/arch/clk.h>
#include <asm/arch/clock.h>
#include <asm/arch/cpu.h>
#include <asm/arch/regs-fb.h>
#include <mipi_ddi.h>
#include <asm/arch/gpio.h>
#include "s5p-fb.h"

static unsigned int ctrl_base;
static unsigned long *lcd_base_addr;
static vidinfo_t *pvid = NULL;

void s5pc_fimd_lcd_init_mem(u_long screen_base, u_long fb_size, u_long palette_size)
{
	lcd_base_addr = (unsigned long *)screen_base;

	udebug("lcd_base_addr(framebuffer memory) = %x\n", lcd_base_addr);

	return;
}

static void s5pc_fimd_set_dualrgb(unsigned int enabled)
{
	unsigned int cfg = 0;

	if (enabled) {
		cfg = S5P_DUALRGB_BYPASS_DUAL | S5P_DUALRGB_LINESPLIT |
			S5P_DUALRGB_VDEN_EN_ENABLE;

		/* in case of Line Split mode, MAIN_CNT doesn't neet to set. */
		cfg |= S5P_DUALRGB_SUB_CNT(pvid->vl_col/2) | S5P_DUALRGB_MAIN_CNT(0);
	} else
		cfg = 0;

	writel(cfg, ctrl_base + S5P_DUALRGB);
}

static void s5pc_fimd_set_par(unsigned int win_id)
{
	unsigned int cfg = 0;

	/* set window control */
	cfg = readl(ctrl_base + S5P_WINCON(win_id));

	cfg &= ~(S5P_WINCON_BITSWP_ENABLE | S5P_WINCON_BYTESWP_ENABLE | \
		S5P_WINCON_HAWSWP_ENABLE | S5P_WINCON_WSWP_ENABLE | \
		S5P_WINCON_BURSTLEN_MASK | S5P_WINCON_BPPMODE_MASK | \
		S5P_WINCON_INRGB_MASK | S5P_WINCON_DATAPATH_MASK);

	/* DATAPATH is DMA */
	cfg |= S5P_WINCON_DATAPATH_DMA;

	/* bpp is 32 */
	cfg |= S5P_WINCON_WSWP_ENABLE;

	/* dma burst is 16 */
	cfg |= S5P_WINCON_BURSTLEN_16WORD;

	/* pixel format is unpacked RGB888 */
	cfg |= S5P_WINCON_BPPMODE_24BPP_888;

	writel(cfg, ctrl_base + S5P_WINCON(win_id));
	udebug("wincon%d = %x\n", win_id, cfg);

	/* set window position to x=0, y=0*/
	cfg = S5P_VIDOSD_LEFT_X(0) | S5P_VIDOSD_TOP_Y(0);
	writel(cfg, ctrl_base + S5P_VIDOSD_A(win_id));
	udebug("window postion left,top = %x\n", cfg);

	cfg = S5P_VIDOSD_RIGHT_X(pvid->vl_col - 1) |
		S5P_VIDOSD_BOTTOM_Y(pvid->vl_row - 1);
	writel(cfg, ctrl_base + S5P_VIDOSD_B(win_id));
	udebug("window postion right,bottom= %x\n", cfg);

	/* set window size for window0*/
	cfg = S5P_VIDOSD_SIZE(pvid->vl_col * pvid->vl_row);
	writel(cfg, ctrl_base + S5P_VIDOSD_C(win_id));
	udebug("vidosd_c%d= %x\n", win_id, cfg);

	return;
}

static void s5pc_fimd_set_buffer_address(unsigned int win_id)
{
	unsigned long start_addr, end_addr;

	start_addr = (unsigned long)lcd_base_addr;
	end_addr = start_addr + ((pvid->vl_col * (pvid->vl_bpix / 8))
		* pvid->vl_row);

	writel(start_addr, ctrl_base + S5P_VIDADDR_START0(win_id));
	writel(end_addr, ctrl_base + S5P_VIDADDR_END0(win_id));

	udebug("start addr = %x, end addr = %x\n", start_addr, end_addr);

	return;
}

static void s5pc_fimd_set_clock(vidinfo_t *pvid)
{
	unsigned int cfg = 0, div = 0, remainder, remainder_div;
	unsigned long pixel_clock, src_clock, max_clock;
	u64 div64;

	max_clock = 86 * 1000000;

	if (pvid->dual_lcd_enabled)
		pixel_clock = pvid->vl_freq * (pvid->vl_hspw + pvid->vl_hfpd +
			pvid->vl_hbpd + pvid->vl_col / 2) * (pvid->vl_vspw +
			    pvid->vl_vfpd + pvid->vl_vbpd + pvid->vl_row);
	else if (pvid->interface_mode == FIMD_CPU_INTERFACE) {
		pixel_clock = pvid->vl_freq * pvid->vl_width * pvid->vl_height *
		    (pvid->cs_setup + pvid->wr_setup + pvid->wr_act +
			pvid->wr_hold + 1);
	} else
		pixel_clock = pvid->vl_freq * (pvid->vl_hspw + pvid->vl_hfpd +
			pvid->vl_hbpd + pvid->vl_col) * (pvid->vl_vspw +
			    pvid->vl_vfpd + pvid->vl_vbpd + pvid->vl_row);

	if (get_pll_clk == NULL) {
		printf("get_pll_clk is null.\n");
		return;
	}
	src_clock = get_lcd_clk();

	cfg = readl(ctrl_base + S5P_VIDCON0);
	cfg &= ~(S5P_VIDCON0_CLKSEL_MASK | S5P_VIDCON0_CLKVALUP_MASK | \
		S5P_VIDCON0_CLKVAL_F(0xFF) |
		S5P_VIDCON0_VCLKEN_MASK | S5P_VIDCON0_CLKDIR_MASK);
	cfg |= (S5P_VIDCON0_CLKSEL_SCLK | S5P_VIDCON0_CLKVALUP_ALWAYS | \
		S5P_VIDCON0_VCLKEN_NORMAL | S5P_VIDCON0_CLKDIR_DIVIDED);

	if (pixel_clock > max_clock)
		pixel_clock = max_clock;

	/**
	 * after it adds pclk_name and sclk_div for c110 to board file.
	 * cancel comment out below.
	 */
	/* set_lcd_clk(0, pvid->pclk_name, pvid->sclk_div); */

	div64 = (u64)get_lcd_clk();

	/* get quotient and remainder. */
	remainder = do_div(div64, pixel_clock);
	div = (u32) div64;

	remainder *= 10;
	remainder_div = remainder / pixel_clock;

#ifdef CONFIG_S5PC110
	if (remainder)
#else
	/* round about one places of decimals. */
	if (remainder_div >= 5)
#endif
		div++;

	/* in case of dual lcd mode. */
	if (pvid->dual_lcd_enabled)
		div--;

	cfg |= S5P_VIDCON0_CLKVAL_F(div - 1);
	writel(cfg, ctrl_base + S5P_VIDCON0);

	udebug("src_clock = %d, pixel_clock = %d, div = %d\n",
		src_clock, pixel_clock, div);

	return;
}

void s3cfb_set_trigger(void)
{
	u32 reg = 0;
	reg = readl(ctrl_base + S5P_TRIGCON);

	reg |= 1 << 0 | 1 << 1;

	writel(reg, ctrl_base + S5P_TRIGCON);
}

int s3cfb_is_i80_frame_done(void)
{
	u32 reg = 0;
	int status;

	reg = readl(ctrl_base + S5P_TRIGCON);

	/* frame done func is valid only when TRIMODE[0] is set to 1. */
	status = (((reg & (0x1 << 2)) == (0x1 << 2)) ? 1 : 0);

	return status;
}

static void s5pc_fimd_lcd_on(unsigned int win_id)
{
	unsigned int cfg = 0;

	/* display on */
	cfg = readl(ctrl_base + S5P_VIDCON0);
	cfg |= (S5P_VIDCON0_ENVID_ENABLE | S5P_VIDCON0_ENVID_F_ENABLE);
	writel(cfg, ctrl_base + S5P_VIDCON0);
	udebug("vidcon0 = %x\n", cfg);
}

static void s5pc_fimd_window_on(unsigned int win_id)
{
	unsigned int cfg = 0;

	/* enable window */
	cfg = readl(ctrl_base + S5P_WINCON(win_id));
	cfg |= S5P_WINCON_ENWIN_ENABLE;
	writel(cfg, ctrl_base + S5P_WINCON(win_id));
	udebug("wincon%d=%x\n", win_id, cfg);

	/* evt1 */
	cfg = readl(ctrl_base + S5P_WINSHMAP);
	cfg |= S5P_WINSHMAP_CH_ENABLE(win_id);
	writel(cfg, ctrl_base + S5P_WINSHMAP);
}

void s5pc_fimd_lcd_off(unsigned int win_id)
{
	unsigned int cfg = 0;

	cfg = readl(ctrl_base + S5P_VIDCON0);
	cfg &= (S5P_VIDCON0_ENVID_DISABLE | S5P_VIDCON0_ENVID_F_DISABLE);
	writel(cfg, ctrl_base + S5P_VIDCON0);
}

void s5pc_fimd_window_off(unsigned int win_id)
{
	unsigned int cfg = 0;

	cfg = readl(ctrl_base + S5P_WINCON(win_id));
	cfg &= S5P_WINCON_ENWIN_DISABLE;
	writel(cfg, ctrl_base + S5P_WINCON(win_id));

	/* evt1 */
	cfg = readl(ctrl_base + S5P_WINSHMAP);
	cfg &= ~S5P_WINSHMAP_CH_DISABLE(win_id);
	writel(cfg, ctrl_base + S5P_WINSHMAP);
}

int s5pc_cpu_interface_timing(vidinfo_t *vid)
{
	unsigned int cpu_if_time_val;

	cpu_if_time_val = readl(ctrl_base + S5P_I80IFCONA0);
	cpu_if_time_val = S5C_LCD_CS_SETUP(vid->cs_setup) |
		S5C_LCD_WR_SETUP(vid->wr_setup) |
		S5C_LCD_WR_ACT(vid->wr_act) |
		S5C_LCD_WR_HOLD(vid->wr_hold) |
		S5C_RSPOL_LOW | /* in case of LCD MIPI module */
		/* S3C_RSPOL_HIGH | */
		S5C_I80IFEN_ENABLE;
	writel(cpu_if_time_val, ctrl_base + S5P_I80IFCONA0);

	return 0;
}

int s5pc_set_auto_cmd_rate(unsigned char cmd_rate, unsigned char ldi)
{
	unsigned int cmd_rate_val;
	unsigned int i80_if_con_reg_val;

	cmd_rate_val = (cmd_rate == DISABLE_AUTO_FRM) ? (0x0 << 0) :
		(cmd_rate == PER_TWO_FRM) ? (0x1 << 0) :
		(cmd_rate == PER_FOUR_FRM) ? (0x2 << 0) :
		(cmd_rate == PER_SIX_FRM) ? (0x3 << 0) :
		(cmd_rate == PER_EIGHT_FRM) ? (0x4 << 0) :
		(cmd_rate == PER_TEN_FRM) ? (0x5 << 0) :
		(cmd_rate == PER_TWELVE_FRM) ? (0x6 << 0) :
		(cmd_rate == PER_FOURTEEN_FRM) ? (0x7 << 0) :
		(cmd_rate == PER_SIXTEEN_FRM) ? (0x8 << 0) :
		(cmd_rate == PER_EIGHTEEN_FRM) ? (0x9 << 0) :
		(cmd_rate == PER_TWENTY_FRM) ? (0xa << 0) :
		(cmd_rate == PER_TWENTY_TWO_FRM) ? (0xb << 0) :
		(cmd_rate == PER_TWENTY_FOUR_FRM) ? (0xc << 0) :
		(cmd_rate == PER_TWENTY_SIX_FRM) ? (0xd << 0) :
		(cmd_rate == PER_TWENTY_EIGHT_FRM) ? (0xe << 0) : (0xf << 0);

	i80_if_con_reg_val = readl(ctrl_base + S5P_I80IFCONB0);
	i80_if_con_reg_val &= ~(0xf << 0);
	i80_if_con_reg_val |= cmd_rate_val;
	writel(i80_if_con_reg_val, ctrl_base + S5P_I80IFCONB0);

	return 0;
}

void s5pc_fimd_lcd_init(vidinfo_t *vid)
{
	unsigned int cfg = 0, rgb_mode, win_id = 3;

	/* store panel info to global variable */
	pvid = vid;

	/* select register base according to cpu type */
	ctrl_base = samsung_get_base_fimd();

	rgb_mode = MODE_RGB_P;

	if (vid->interface_mode == FIMD_RGB_INTERFACE) {
		cfg |= S5P_VIDCON0_VIDOUT_RGB;
		writel(cfg, ctrl_base + S5P_VIDCON0);

		cfg = readl(ctrl_base + S5P_VIDCON2);
		cfg &= ~(S5P_VIDCON2_WB_MASK | S5P_VIDCON2_TVFORMATSEL_MASK | \
						S5P_VIDCON2_TVFORMATSEL_YUV_MASK);
		cfg |= S5P_VIDCON2_WB_DISABLE;
		writel(cfg, ctrl_base + S5P_VIDCON2);

		/* set polarity */
		cfg = 0;
		if (!pvid->vl_clkp)
			cfg |= S5P_VIDCON1_IVCLK_RISING_EDGE;
		if (!pvid->vl_hsp)
			cfg |= S5P_VIDCON1_IHSYNC_INVERT;
		if (!pvid->vl_vsp)
			cfg |= S5P_VIDCON1_IVSYNC_INVERT;
		if (!pvid->vl_dp)
			cfg |= S5P_VIDCON1_IVDEN_INVERT;

		writel(cfg, ctrl_base + S5P_VIDCON1);

		/* set timing */
		cfg = S5P_VIDTCON0_VFPD(pvid->vl_vfpd - 1);
		cfg |= S5P_VIDTCON0_VBPD(pvid->vl_vbpd - 1);
		cfg |= S5P_VIDTCON0_VSPW(pvid->vl_vspw - 1);
		writel(cfg, ctrl_base + S5P_VIDTCON0);
		udebug("vidtcon0 = %x\n", cfg);

		cfg = S5P_VIDTCON1_HFPD(pvid->vl_hfpd - 1);
		cfg |= S5P_VIDTCON1_HBPD(pvid->vl_hbpd - 1);
		cfg |= S5P_VIDTCON1_HSPW(pvid->vl_hspw - 1);

		writel(cfg, ctrl_base + S5P_VIDTCON1);
		udebug("vidtcon1 = %x\n", cfg);

		/* set lcd size */
		cfg = S5P_VIDTCON2_HOZVAL(pvid->vl_col - 1);
		cfg |= S5P_VIDTCON2_LINEVAL(pvid->vl_row - 1);

		writel(cfg, ctrl_base + S5P_VIDTCON2);
		udebug("vidtcon2 = %x\n", cfg);

	} else {
		cfg |= S5P_VIDCON0_DSI_ENABLE | S5P_VIDCON0_VIDOUT_I80LDI0;
		writel(cfg, ctrl_base + S5P_VIDCON0);

		/* set lcd size */
		cfg = S5P_VIDTCON2_HOZVAL(pvid->vl_col - 1);
		cfg |= S5P_VIDTCON2_LINEVAL(pvid->vl_row - 1);
		writel(cfg, ctrl_base + S5P_VIDTCON2);
		udebug("vidtcon2 = %x\n", cfg);

		s5pc_cpu_interface_timing(vid);
		s5pc_set_auto_cmd_rate(DISABLE_AUTO_FRM, DDI_MAIN_LCD);

		udebug("MIPI Command mode.\n");
	}

	/* set display mode */
	cfg = readl(ctrl_base + S5P_VIDCON0);
	cfg &= ~S5P_VIDCON0_PNRMODE_MASK;
	cfg |= (rgb_mode << S5P_VIDCON0_PNRMODE_SHIFT);
	writel(cfg, ctrl_base + S5P_VIDCON0);

	/* set par */
	s5pc_fimd_set_par(win_id);

	/* set memory address */
	s5pc_fimd_set_buffer_address(win_id);

	/* set buffer size */
	cfg = S5P_VIDADDR_PAGEWIDTH(pvid->vl_col * pvid->vl_bpix / 8);
	writel(cfg, ctrl_base + S5P_VIDADDR_SIZE(win_id));
	udebug("vidaddr_pagewidth = %d\n", cfg);

	/* set clock */
	s5pc_fimd_set_clock(vid);

	/* set rgb mode to dual lcd. */
	if (pvid->dual_lcd_enabled)
		s5pc_fimd_set_dualrgb(1);
	else
		s5pc_fimd_set_dualrgb(0);

	/* display on */
	s5pc_fimd_lcd_on(win_id);

	/* window on */
	s5pc_fimd_window_on(win_id);

	udebug("lcd controller init completed.\n");

	return;
}

ulong s5pc_fimd_calc_fbsize(void)
{
	return (pvid->vl_col * pvid->vl_row * (pvid->vl_bpix / 8));
}
