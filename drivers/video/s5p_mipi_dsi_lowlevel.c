/* linux/drivers/video/s5p_mipi_dsi_lowlevel.c
 *
 * Samsung SoC MIPI-DSI lowlevel driver.
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * InKi Dae, <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <ubi_uboot.h>
#include <common.h>
#include <lcd.h>
#include <mipi_ddi.h>
#include <asm/errno.h>
#include <asm/arch/regs-dsim.h>
#include <asm/arch/mipi_dsim.h>
#include <asm/arch/power.h>
#include <asm/arch/cpu.h>
#include <linux/types.h>
#include <linux/list.h>

#include <asm/arch/mipi_dsim.h>

#include "s5p_mipi_dsi_lowlevel.h"
#include "s5p_mipi_dsi_common.h"

void s5p_mipi_dsi_func_reset(struct mipi_dsim_device *dsim)
{
	unsigned int reg;

	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	reg = readl(&mipi_dsim->swrst);

	reg |= DSIM_FUNCRST;

	writel(reg, &mipi_dsim->swrst);
}

void s5p_mipi_dsi_sw_reset(struct mipi_dsim_device *dsim)
{
	unsigned int reg = 0;

	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	reg = readl(&mipi_dsim->swrst);

	reg |= DSIM_SWRST;
	reg |= DSIM_FUNCRST;

	writel(reg, &mipi_dsim->swrst);
}

void s5p_mipi_dsi_sw_release(struct mipi_dsim_device *dsim)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->intsrc);

	reg |= INTSRC_SWRST_RELEASE;

	writel(reg, &mipi_dsim->intsrc);

	reg = 0;
	reg = readl(&mipi_dsim->intsrc);
}

void s5p_mipi_dsi_set_interrupt_mask(struct mipi_dsim_device *dsim,
		unsigned int mode, unsigned int mask)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->intmsk);

	if (mask)
		reg |= mode;
	else
		reg &= ~mode;

	writel(reg, &mipi_dsim->intmsk);
}

void s5p_mipi_dsi_init_fifo_pointer(struct mipi_dsim_device *dsim,
		unsigned int cfg)
{
	unsigned int reg;

	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	reg = readl(&mipi_dsim->fifoctrl);

	writel(reg & ~(cfg), &mipi_dsim->fifoctrl);
	udelay(10000);
	reg |= cfg;

	writel(reg, &mipi_dsim->fifoctrl);
}

/*
 * this function set PLL P, M and S value in D-PHY
 */
void s5p_mipi_dsi_set_phy_tunning(struct mipi_dsim_device *dsim,
		unsigned int value)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	writel(DSIM_AFC_CTL(value), &mipi_dsim->phyacchr);
}

void s5p_mipi_dsi_set_main_disp_resol(struct mipi_dsim_device *dsim,
	unsigned int width_resol, unsigned int height_resol)
{
	unsigned int reg;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	/* standby should be set after configuration so set to not ready*/
	reg = (readl(&mipi_dsim->mdresol)) & ~(DSIM_MAIN_STAND_BY);
	writel(reg, &mipi_dsim->mdresol);

	reg &= ~((0x7ff << 16) | (0x7ff << 0));
	reg |= DSIM_MAIN_VRESOL(height_resol) | DSIM_MAIN_HRESOL(width_resol);

	reg |= DSIM_MAIN_STAND_BY;
	writel(reg, &mipi_dsim->mdresol);
}

void s5p_mipi_dsi_set_main_disp_vporch(struct mipi_dsim_device *dsim,
	unsigned int cmd_allow, unsigned int vfront, unsigned int vback)
{
	unsigned int reg;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	reg = (readl(&mipi_dsim->mvporch)) &
		~((DSIM_CMD_ALLOW_MASK) | (DSIM_STABLE_VFP_MASK) |
		(DSIM_MAIN_VBP_MASK));

	reg |= ((cmd_allow & 0xf) << DSIM_CMD_ALLOW_SHIFT) |
		((vfront & 0x7ff) << DSIM_STABLE_VFP_SHIFT) |
		((vback & 0x7ff) << DSIM_MAIN_VBP_SHIFT);

	writel(reg, &mipi_dsim->mvporch);
}

void s5p_mipi_dsi_set_main_disp_hporch(struct mipi_dsim_device *dsim,
	unsigned int front, unsigned int back)
{
	unsigned int reg;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	reg = (readl(&mipi_dsim->mhporch)) &
		~((DSIM_MAIN_HFP_MASK) | (DSIM_MAIN_HBP_MASK));

	reg |= (front << DSIM_MAIN_HFP_SHIFT) | (back << DSIM_MAIN_HBP_SHIFT);

	writel(reg, &mipi_dsim->mhporch);
}

void s5p_mipi_dsi_set_main_disp_sync_area(struct mipi_dsim_device *dsim,
	unsigned int vert, unsigned int hori)
{
	unsigned int reg;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	reg = (readl(&mipi_dsim->msync)) &
		~((DSIM_MAIN_VSA_MASK) | (DSIM_MAIN_HSA_MASK));

	reg |= ((vert & 0x3ff) << DSIM_MAIN_VSA_SHIFT) |
		(hori << DSIM_MAIN_HSA_SHIFT);

	writel(reg, &mipi_dsim->msync);
}

void s5p_mipi_dsi_set_sub_disp_resol(struct mipi_dsim_device *dsim,
	unsigned int vert, unsigned int hori)
{
	unsigned int reg;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	reg = (readl(&mipi_dsim->sdresol)) &
		~(DSIM_SUB_STANDY_MASK);

	writel(reg, &mipi_dsim->sdresol);

	reg &= ~(DSIM_SUB_VRESOL_MASK) | ~(DSIM_SUB_HRESOL_MASK);
	reg |= ((vert & 0x7ff) << DSIM_SUB_VRESOL_SHIFT) |
		((hori & 0x7ff) << DSIM_SUB_HRESOL_SHIFT);
	writel(reg, &mipi_dsim->sdresol);

	reg |= (1 << DSIM_SUB_STANDY_SHIFT);
	writel(reg, &mipi_dsim->sdresol);
}

void s5p_mipi_dsi_init_config(struct mipi_dsim_device *dsim)
{
	struct mipi_dsim_config *dsim_config = dsim->dsim_config;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	unsigned int cfg = (readl(&mipi_dsim->config)) &
		~((1 << 28) | (0x1f << 20) | (0x3 << 5));

	cfg =	(dsim_config->auto_flush << 29) |
		(dsim_config->eot_disable << 28) |
		(dsim_config->auto_vertical_cnt << DSIM_AUTO_MODE_SHIFT) |
		(dsim_config->hse << DSIM_HSE_MODE_SHIFT) |
		(dsim_config->hfp << DSIM_HFP_MODE_SHIFT) |
		(dsim_config->hbp << DSIM_HBP_MODE_SHIFT) |
		(dsim_config->hsa << DSIM_HSA_MODE_SHIFT) |
		(dsim_config->e_no_data_lane << DSIM_NUM_OF_DATALANE_SHIFT);

	writel(cfg, &mipi_dsim->config);
}

void s5p_mipi_dsi_display_config(struct mipi_dsim_device *dsim,
				struct mipi_dsim_config *dsim_config)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	u32 reg = (readl(&mipi_dsim->config)) &
		~((0x3 << 26) | (1 << 25) | (0x3 << 18) | (0x7 << 12) |
		(0x3 << 16) | (0x7 << 8));

	if (dsim_config->e_interface == DSIM_VIDEO)
		reg |= (1 << 25);
	else if (dsim_config->e_interface == DSIM_COMMAND)
		reg &= ~(1 << 25);
	else {
		printf("unknown lcd type.\n");
		return;
	}

	/* main lcd */
	reg |= ((u8) (dsim_config->e_burst_mode) & 0x3) << 26 |
		((u8) (dsim_config->e_virtual_ch) & 0x3) << 18 |
		((u8) (dsim_config->e_pixel_format) & 0x7) << 12;

	writel(reg, &mipi_dsim->config);
}

void s5p_mipi_dsi_enable_lane(struct mipi_dsim_device *dsim, unsigned int lane,
	unsigned int enable)
{
	unsigned int reg;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	reg = readl(&mipi_dsim->config);

	if (enable)
		reg |= DSIM_LANE_ENx(lane);
	else
		reg &= ~DSIM_LANE_ENx(lane);

	writel(reg, &mipi_dsim->config);
}


void s5p_mipi_dsi_set_data_lane_number(struct mipi_dsim_device *dsim,
	unsigned int count)
{
	unsigned int cfg;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	/* get the data lane number. */
	cfg = DSIM_NUM_OF_DATA_LANE(count);

	writel(cfg, &mipi_dsim->config);
}

void s5p_mipi_dsi_enable_afc(struct mipi_dsim_device *dsim, unsigned int enable,
	unsigned int afc_code)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->phyacchr);

	reg = 0;

	if (enable) {
		reg |= (1 << 14);
		reg &= ~(0x7 << 5);
		reg |= (afc_code & 0x7) << 5;
	} else
		reg &= ~(1 << 14);

	writel(reg, &mipi_dsim->phyacchr);
}

void s5p_mipi_dsi_enable_pll_bypass(struct mipi_dsim_device *dsim,
	unsigned int enable)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->clkctrl)) &
		~(DSIM_PLL_BYPASS_EXTERNAL);

	reg |= enable << DSIM_PLL_BYPASS_SHIFT;

	writel(reg, &mipi_dsim->clkctrl);
}

void s5p_mipi_dsi_set_pll_pms(struct mipi_dsim_device *dsim, unsigned int p,
	unsigned int m, unsigned int s)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->pllctrl);


	reg |= ((p & 0x3f) << 13) | ((m & 0x1ff) << 4) | ((s & 0x7) << 1);

	writel(reg, &mipi_dsim->pllctrl);
}

void s5p_mipi_dsi_pll_freq_band(struct mipi_dsim_device *dsim,
		unsigned int freq_band)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->pllctrl)) &
		~(0x1f << DSIM_FREQ_BAND_SHIFT);

	reg |= ((freq_band & 0x1f) << DSIM_FREQ_BAND_SHIFT);

	writel(reg, &mipi_dsim->pllctrl);
}

void s5p_mipi_dsi_pll_freq(struct mipi_dsim_device *dsim,
		unsigned int pre_divider, unsigned int main_divider,
		unsigned int scaler)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->pllctrl)) &
		~(0x7ffff << 1);

	reg |= (pre_divider & 0x3f) << 13 | (main_divider & 0x1ff) << 4 |
		(scaler & 0x7) << 1;

	writel(reg, &mipi_dsim->pllctrl);
}

void s5p_mipi_dsi_pll_stable_time(struct mipi_dsim_device *dsim,
	unsigned int lock_time)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	writel(lock_time, &mipi_dsim->plltmr);
}

void s5p_mipi_dsi_enable_pll(struct mipi_dsim_device *dsim, unsigned int enable)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->pllctrl)) &
		~(0x1 << DSIM_PLL_EN_SHIFT);

	reg |= ((enable & 0x1) << DSIM_PLL_EN_SHIFT);

	writel(reg, &mipi_dsim->pllctrl);
}

void s5p_mipi_dsi_set_byte_clock_src(struct mipi_dsim_device *dsim,
		unsigned int src)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->clkctrl)) &
		~(0x3 << DSIM_BYTE_CLK_SRC_SHIFT);

	reg |= ((unsigned int) src) << DSIM_BYTE_CLK_SRC_SHIFT;

	writel(reg, &mipi_dsim->clkctrl);
}

void s5p_mipi_dsi_enable_byte_clock(struct mipi_dsim_device *dsim,
		unsigned int enable)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->clkctrl)) &
		~(1 << DSIM_BYTE_CLKEN_SHIFT);

	reg |= enable << DSIM_BYTE_CLKEN_SHIFT;

	writel(reg, &mipi_dsim->clkctrl);
}

void s5p_mipi_dsi_set_esc_clk_prs(struct mipi_dsim_device *dsim,
		unsigned int enable, unsigned int prs_val)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->clkctrl)) &
		~((1 << DSIM_ESC_CLKEN_SHIFT) | (0xffff));

	reg |= enable << DSIM_ESC_CLKEN_SHIFT;
	if (enable)
		reg |= prs_val;

	writel(reg, &mipi_dsim->clkctrl);
}

void s5p_mipi_dsi_enable_esc_clk_on_lane(struct mipi_dsim_device *dsim,
		unsigned int lane_sel, unsigned int enable)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->clkctrl);

	if (enable)
		reg |= DSIM_LANE_ESC_CLKEN(lane_sel);
	else

		reg &= ~DSIM_LANE_ESC_CLKEN(lane_sel);

	writel(reg, &mipi_dsim->clkctrl);
}

void s5p_mipi_dsi_force_dphy_stop_state(struct mipi_dsim_device *dsim,
	unsigned int enable)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->escmode)) &
		~(0x1 << DSIM_FORCE_STOP_STATE_SHIFT);

	reg |= ((enable & 0x1) << DSIM_FORCE_STOP_STATE_SHIFT);

	writel(reg, &mipi_dsim->escmode);
}

unsigned int s5p_mipi_dsi_is_lane_state(struct mipi_dsim_device *dsim)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->status);

	/**
	 * check clock and data lane states.
	 * if MIPI-DSI controller was enabled at bootloader then
	 * TX_READY_HS_CLK is enabled otherwise STOP_STATE_CLK.
	 * so it should be checked for two case.
	 */
	if ((reg & DSIM_STOP_STATE_DAT(0xf)) &&
			((reg & DSIM_STOP_STATE_CLK) ||
			 (reg & DSIM_TX_READY_HS_CLK)))
		return 1;
	else
		return 0;

	return 0;
}

void s5p_mipi_dsi_set_stop_state_counter(struct mipi_dsim_device *dsim,
		unsigned int cnt_val)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->escmode)) &
		~(0x7ff << DSIM_STOP_STATE_CNT_SHIFT);

	reg |= ((cnt_val & 0x7ff) << DSIM_STOP_STATE_CNT_SHIFT);

	writel(reg, &mipi_dsim->escmode);
}

void s5p_mipi_dsi_set_bta_timeout(struct mipi_dsim_device *dsim,
		unsigned int timeout)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->timeout)) &
		~(0xff << DSIM_BTA_TOUT_SHIFT);

	reg |= (timeout << DSIM_BTA_TOUT_SHIFT);

	writel(reg, &mipi_dsim->timeout);
}

void s5p_mipi_dsi_set_lpdr_timeout(struct mipi_dsim_device *dsim,
		unsigned int timeout)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->timeout)) &
		~(0xffff << DSIM_LPDR_TOUT_SHIFT);

	reg |= (timeout << DSIM_LPDR_TOUT_SHIFT);

	writel(reg, &mipi_dsim->timeout);
}

void s5p_mipi_dsi_set_cpu_transfer_mode(struct mipi_dsim_device *dsim,
		unsigned int lp)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->escmode);

	reg &= ~DSIM_CMD_LPDT_LP;

	if (lp)
		reg |= DSIM_CMD_LPDT_LP;

	writel(reg, &mipi_dsim->escmode);
}

void s5p_mipi_dsi_set_lcdc_transfer_mode(struct mipi_dsim_device *dsim,
		unsigned int lp)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->escmode);

	reg &= ~DSIM_TX_LPDT_LP;

	if (lp)
		reg |= DSIM_TX_LPDT_LP;

	writel(reg, &mipi_dsim->escmode);
}

void s5p_mipi_dsi_enable_hs_clock(struct mipi_dsim_device *dsim,
		unsigned int enable)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->clkctrl)) &
		~(1 << DSIM_TX_REQUEST_HSCLK_SHIFT);

	reg |= enable << DSIM_TX_REQUEST_HSCLK_SHIFT;

	writel(reg, &mipi_dsim->clkctrl);
}

void s5p_mipi_dsi_dp_dn_swap(struct mipi_dsim_device *dsim,
		unsigned int swap_en)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->phyacchr1);

	reg &= ~(0x3 << 0);
	reg |= (swap_en & 0x3) << 0;

	writel(reg, &mipi_dsim->phyacchr1);
}

void s5p_mipi_dsi_hs_zero_ctrl(struct mipi_dsim_device *dsim,
		unsigned int hs_zero)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->pllctrl)) &
		~(0xf << 28);

	reg |= ((hs_zero & 0xf) << 28);

	writel(reg, &mipi_dsim->pllctrl);
}

void s5p_mipi_dsi_prep_ctrl(struct mipi_dsim_device *dsim, unsigned int prep)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (readl(&mipi_dsim->pllctrl)) &
		~(0x7 << 20);

	reg |= ((prep & 0x7) << 20);

	writel(reg, &mipi_dsim->pllctrl);
}

void s5p_mipi_dsi_clear_interrupt(struct mipi_dsim_device *dsim)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->intsrc);

	reg |= INTSRC_PLL_STABLE;

	writel(reg, &mipi_dsim->intsrc);
}

void s5p_mipi_dsi_clear_all_interrupt(struct mipi_dsim_device *dsim)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->intsrc);

	reg |= 0xffffffff;

	writel(reg, &mipi_dsim->intsrc);
}

unsigned int s5p_mipi_dsi_is_pll_stable(struct mipi_dsim_device *dsim)
{
	unsigned int reg;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	reg = readl(&mipi_dsim->status);

	return reg & (1 << 31) ? 1 : 0;
}

unsigned int s5p_mipi_dsi_get_fifo_state(struct mipi_dsim_device *dsim)
{
	unsigned int ret;
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();

	ret = readl(&mipi_dsim->fifoctrl) & ~(0x1f);

	return ret;
}

void s5p_mipi_dsi_wr_tx_header(struct mipi_dsim_device *dsim,
	unsigned int di, unsigned int data0, unsigned int data1)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = (data1 << 16) | (data0 << 8) | ((di & 0x3f) << 0);

	writel(reg, &mipi_dsim->pkthdr);
}

unsigned int _s5p_mipi_dsi_get_frame_done_status(struct mipi_dsim_device *dsim)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->intsrc);

	return (reg & INTSRC_FRAME_DONE) ? 1 : 0;
}

void _s5p_mipi_dsi_clear_frame_done(struct mipi_dsim_device *dsim)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	unsigned int reg = readl(&mipi_dsim->intsrc);

	writel(reg | INTSRC_FRAME_DONE, &mipi_dsim->intsrc);
}

void s5p_mipi_dsi_wr_tx_data(struct mipi_dsim_device *dsim,
		unsigned int tx_data)
{
	struct s5p_mipi_dsim *mipi_dsim =
		(struct s5p_mipi_dsim *)samsung_get_base_mipi_dsim();
	writel(tx_data, &mipi_dsim->payload);
}
