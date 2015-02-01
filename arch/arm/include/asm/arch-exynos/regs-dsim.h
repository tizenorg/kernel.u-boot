/* linux/asm/arch/regs-dsim.h
 *
 * Register definition file for MIPI-DSI driver
 *
 * Donghwa Lee, Copyright (c) 2010 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_DSIM_H_
#define __ASM_ARM_ARCH_DSIM_H_

/*
 * Bit Definitions
 */
/* DSIM_STATUS */
#define DSIM_STOP_STATE_DAT(x)	(((x) & 0xf) << 0)
#define DSIM_STOP_STATE_CLK	(1 << 8)
#define DSIM_TX_READY_HS_CLK	(1 << 10)

/* DSIM_SWRST */
#define DSIM_FUNCRST		(1 << 16)
#define DSIM_SWRST		(1 << 0)

/* S5P_DSIM_TIMEOUT */
#define DSIM_LPDR_TOUT_SHIFT	(0)
#define DSIM_BTA_TOUT_SHIFT	(16)

/* S5P_DSIM_CLKCTRL */
#define DSIM_LANE_ESC_CLKEN_SHIFT	(19)
#define DSIM_BYTE_CLKEN_SHIFT		(24)
#define DSIM_BYTE_CLK_SRC_SHIFT		(25)
#define DSIM_PLL_BYPASS_SHIFT		(27)
#define DSIM_ESC_CLKEN_SHIFT		(28)
#define DSIM_TX_REQUEST_HSCLK_SHIFT	(31)
#define DSIM_LANE_ESC_CLKEN(x)		(((x) & 0x1f) << \
						DSIM_LANE_ESC_CLKEN_SHIFT)
#define DSIM_BYTE_CLK_ENABLE		(1 << DSIM_BYTE_CLKEN_SHIFT)
#define DSIM_BYTE_CLK_DISABLE		(0 << DSIM_BYTE_CLKEN_SHIFT)
#define DSIM_PLL_BYPASS_EXTERNAL	(1 << DSIM_PLL_BYPASS_SHIFT)
#define DSIM_ESC_CLKEN_ENABLE		(1 << DSIM_ESC_CLKEN_SHIFT)
#define DSIM_ESC_CLKEN_DISABLE		(0 << DSIM_ESC_CLKEN_SHIFT)

/* S5P_DSIM_CONFIG */
#define DSIM_NUM_OF_DATALANE_SHIFT	(5)
#define DSIM_HSA_MODE_SHIFT		(20)
#define DSIM_HBP_MODE_SHIFT		(21)
#define DSIM_HFP_MODE_SHIFT		(22)
#define DSIM_HSE_MODE_SHIFT		(23)
#define DSIM_AUTO_MODE_SHIFT		(24)
#define DSIM_LANE_ENx(x)		(((x) & 0x1f) << 0)

#define DSIM_NUM_OF_DATA_LANE(x)	((x) << DSIM_NUM_OF_DATALANE_SHIFT)

/* S5P_DSIM_ESCMODE */
#define DSIM_TX_LPDT_SHIFT		(6)
#define DSIM_CMD_LPDT_SHIFT		(7)
#define DSIM_TX_LPDT_LP			(1 << DSIM_TX_LPDT_SHIFT)
#define DSIM_CMD_LPDT_LP		(1 << DSIM_CMD_LPDT_SHIFT)
#define DSIM_STOP_STATE_CNT_SHIFT	(21)
#define DSIM_FORCE_STOP_STATE_SHIFT	(20)

/* S5P_DSIM_MDRESOL */
#define DSIM_MAIN_STAND_BY		(1 << 31)
#define DSIM_MAIN_VRESOL(x)		(((x) & 0x7ff) << 16)
#define DSIM_MAIN_HRESOL(x)		(((x) & 0X7ff) << 0)

/* S5P_DSIM_MVPORCH */
#define DSIM_CMD_ALLOW_SHIFT		(28)
#define DSIM_STABLE_VFP_SHIFT		(16)
#define DSIM_MAIN_VBP_SHIFT		(0)
#define DSIM_CMD_ALLOW_MASK		(0xf << DSIM_CMD_ALLOW_SHIFT)
#define DSIM_STABLE_VFP_MASK		(0x7ff << DSIM_STABLE_VFP_SHIFT)
#define DSIM_MAIN_VBP_MASK		(0x7ff << DSIM_MAIN_VBP_SHIFT)

/* S5P_DSIM_MHPORCH */
#define DSIM_MAIN_HFP_SHIFT		(16)
#define DSIM_MAIN_HBP_SHIFT		(0)
#define DSIM_MAIN_HFP_MASK		((0xffff) << DSIM_MAIN_HFP_SHIFT)
#define DSIM_MAIN_HBP_MASK		((0xffff) << DSIM_MAIN_HBP_SHIFT)

/* S5P_DSIM_MSYNC */
#define DSIM_MAIN_VSA_SHIFT		(22)
#define DSIM_MAIN_HSA_SHIFT		(0)
#define DSIM_MAIN_VSA_MASK		((0x3ff) << DSIM_MAIN_VSA_SHIFT)
#define DSIM_MAIN_HSA_MASK		((0xffff) << DSIM_MAIN_HSA_SHIFT)

/* S5P_DSIM_SDRESOL */
#define DSIM_SUB_STANDY_SHIFT		(31)
#define DSIM_SUB_VRESOL_SHIFT		(16)
#define DSIM_SUB_HRESOL_SHIFT		(0)
#define DSIM_SUB_STANDY_MASK		((0x1) << DSIM_SUB_STANDY_SHIFT)
#define DSIM_SUB_VRESOL_MASK		((0x7ff) << DSIM_SUB_VRESOL_SHIFT)
#define DSIM_SUB_HRESOL_MASK		((0x7ff) << DSIM_SUB_HRESOL_SHIFT)

/* S5P_DSIM_INTSRC */
#define INTSRC_FRAME_DONE		(1 << 24)
#define INTSRC_PLL_STABLE		(1 << 31)
#define INTSRC_SWRST_RELEASE		(1 << 30)

/* S5P_DSIM_INTMSK */
#define INTMSK_FRAME_DONE		(1 << 24)

/* S5P_DSIM_FIFOCTRL */
#define SFR_HEADER_EMPTY		(1 << 22)

/* S5P_DSIM_PHYACCHR */
#define DSIM_AFC_CTL(x)			(((x) & 0x7) << 5)

/* S5P_DSIM_PLLCTRL */
#define DSIM_PLL_EN_SHIFT		(23)
#define DSIM_FREQ_BAND_SHIFT		(24)

#ifndef __ASSEMBLY__

struct s5p_mipi_dsim {
	unsigned int	status;
	unsigned int	swrst;
	unsigned int	clkctrl;
	unsigned int	timeout;
	unsigned int	config;
	unsigned int	escmode;
	unsigned int	mdresol;
	unsigned int	mvporch;
	unsigned int	mhporch;
	unsigned int	msync;
	unsigned int	sdresol;
	unsigned int	intsrc;
	unsigned int	intmsk;
	unsigned int	pkthdr;
	unsigned int	payload;
	unsigned int	rxfifo;
	unsigned int	fifothld;
	unsigned int	fifoctrl;
	unsigned int	memacchr;
	unsigned int	pllctrl;
	unsigned int	plltmr;
	unsigned int	phyacchr;
	unsigned int	phyacchr1;
};

#endif	/* __ASSEMBLY__ */

#endif
