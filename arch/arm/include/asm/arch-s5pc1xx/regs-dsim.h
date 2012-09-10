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

#define S5P_DSIMREG(x)		(x)

#define S5P_DSIM_STATUS		S5P_DSIMREG(0x0000)
#define S5P_DSIM_SWRST		S5P_DSIMREG(0x0004)
#define S5P_DSIM_CLKCTRL	S5P_DSIMREG(0x0008)
#define S5P_DSIM_TIMEOUT	S5P_DSIMREG(0x000C)
#define S5P_DSIM_CONFIG		S5P_DSIMREG(0x0010)
#define S5P_DSIM_ESCMODE	S5P_DSIMREG(0x0014)
#define S5P_DSIM_MDRESOL	S5P_DSIMREG(0x0018)
#define S5P_DSIM_MVPORCH	S5P_DSIMREG(0x001C)
#define S5P_DSIM_MHPORCH	S5P_DSIMREG(0x0020)
#define S5P_DSIM_MSYNC		S5P_DSIMREG(0x0024)
#define S5P_DSIM_SDRESOL	S5P_DSIMREG(0x0028)
#define S5P_DSIM_INTSRC		S5P_DSIMREG(0x002C)
#define S5P_DSIM_INTMSK		S5P_DSIMREG(0x0030)
#define S5P_DSIM_PKTHDR		S5P_DSIMREG(0x0034)
#define S5P_DSIM_PAYLOAD	S5P_DSIMREG(0x0038)
#define S5P_DSIM_RXFIFO		S5P_DSIMREG(0x003C)
#define S5P_DSIM_FIFOTHLD	S5P_DSIMREG(0x0040)
#define S5P_DSIM_FIFOCTRL	S5P_DSIMREG(0x0044)
#define S5P_DSIM_MEMACCHR	S5P_DSIMREG(0x0048)
#define S5P_DSIM_PLLCTRL	S5P_DSIMREG(0x004C)
#define S5P_DSIM_PLLTMR		S5P_DSIMREG(0x0050)
#define S5P_DSIM_PHYACCHR	S5P_DSIMREG(0x0054)
#define S5P_DSIM_PHYACCHR1	S5P_DSIMREG(0x0058)


/*
 * Bit Definitions
*/
#define DSIM_STATUS_PLLSTABLE		(0 << 31)
#define DSIM_STATUS_SWRSTRLS		(1 << 20)
#define DSIM_STATUS_SWRSTRST		(0 << 20)
#define DSIM_STATUS_DIRECTION_B		(1 << 16)
#define DSIM_STATUS_DIRECTION_F		(0 << 16)
#define DSIM_STATUS_TX_READY_HSCLK		(1 << 10)
#define DSIM_STATUS_TX_NOTREADY_HSCLK	(1 << 10)
#define DSIM_STATUS_ULPSCLK			(1 << 9)
#define DSIM_STATUS_NOULPSCLK		(0 << 9)
#define DSIM_STATUS_STOP_STATECLK		(0 << 8)
#define DSIM_STATUS_NOSTOP_STATECLK		(0 << 8)
#define DSIM_STATUS_ULPSDAT(x)		(((x) & 0xf) << 4)
#define DSIM_STATUS_STOP_STATEDAT(x)	((x) & 0xf)

#define DSIM_SWRST_FUNCRST_STANDBY		(0 << 16)
#define DSIM_SWRST_FUNCRST_RESET		(1 << 16)
#define DSIM_SWRST_SWRST_STANDBY		(0 << 0)
#define DSIM_SWRST_SWRST_RESET		(1 << 0)

/* S5P_DSIM_TIMEOUT */
#define DSIM_LPDR_TOUT_SHIFT	(0)
#define DSIM_BTA_TOUT_SHIFT	(16)
#define DSIM_LPDR_TOUT(x)	(((x) & 0xffff) << DSIM_LPDR_TOUT_SHIFT)
#define DSIM_BTA_TOUT(x)	(((x) & 0xff) << DSIM_BTA_TOUT_SHIFT)

/* S5P_DSIM_CLKCTRL */
#define DSIM_ESC_PRESCALER_SHIFT	(0)
#define DSIM_LANE_ESC_CLKEN_SHIFT	(19)
#define DSIM_BYTE_CLKEN_SHIFT		(24)
#define DSIM_BYTE_CLK_SRC_SHIFT		(25)
#define DSIM_PLL_BYPASS_SHIFT		(27)
#define DSIM_ESC_CLKEN_SHIFT		(28)
#define DSIM_TX_REQUEST_HSCLK_SHIFT	(31)
#define DSIM_ESC_PRESCALER(x)		(((x) & 0xffff) << \
						DSIM_ESC_PRESCALER_SHIFT)
#define DSIM_LANE_ESC_CLKEN(x)		(((x) & 0x1f) << \
						DSIM_LANE_ESC_CLKEN_SHIFT)
#define DSIM_BYTE_CLK_ENABLE		(1 << DSIM_BYTE_CLKEN_SHIFT)
#define DSIM_BYTE_CLK_DISABLE		(0 << DSIM_BYTE_CLKEN_SHIFT)
#define DSIM_BYTE_CLKSRC(x)		(((x) & 0x3) << DSIM_BYTE_CLK_SRC_SHIFT)
#define DSIM_PLL_BYPASS_PLL		(0 << DSIM_PLL_BYPASS_SHIFT)
#define DSIM_PLL_BYPASS_EXTERNAL	(1 << DSIM_PLL_BYPASS_SHIFT)
#define DSIM_ESC_CLKEN_ENABLE		(1 << DSIM_ESC_CLKEN_SHIFT)
#define DSIM_ESC_CLKEN_DISABLE		(0 << DSIM_ESC_CLKEN_SHIFT)

#define DSIM_TIMEOUT_BTATOUT(x)		(((x) & 0xff) << 16)
#define DSIM_TIMEOUT_LPDRTOUT(x)		(((x) & 0xffff) << 0)

/* S5P_DSIM_CONFIG */
#define DSIM_LANE_EN_SHIFT		(0)
#define DSIM_NUM_OF_DATALANE_SHIFT	(5)
#define DSIM_SUB_PIX_FORMAT_SHIFT	(8)
#define DSIM_MAIN_PIX_FORMAT_SHIFT	(12)
#define DSIM_SUB_VC_SHIFT		(16)
#define DSIM_MAIN_VC_SHIFT		(18)
#define DSIM_HSA_MODE_SHIFT		(20)
#define DSIM_HBP_MODE_SHIFT		(21)
#define DSIM_HFP_MODE_SHIFT		(22)
#define DSIM_HSE_MODE_SHIFT		(23)
#define DSIM_AUTO_MODE_SHIFT		(24)
#define DSIM_VIDEO_MODE_SHIFT		(25)
#define DSIM_BURST_MODE_SHIFT		(26)
#define DSIM_SYNC_INFORM_SHIFT		(27)
#define DSIM_EOT_R03_SHIFT		(28)
#define DSIM_LANE_ENx(x)		((1) << x)

/* in case of Gemunus, it should be 0x1. */
#define DSIM_NUM_OF_DATA_LANE(x)	((x) << 5)
#define DSIM_SUB_PIX_FORMAT_3BPP	(0 << 8)	/* command mode only */
#define DSIM_SUB_PIX_FORMAT_8BPP	(1 << 8)	/* command mode only */
#define DSIM_SUB_PIX_FORMAT_12BPP	(2 << 8)	/* command mode only */
#define DSIM_SUB_PIX_FORMAT_16BPP	(3 << 8)	/* command mode only */
#define DSIM_SUB_PIX_FORMAT_16BPP_RGB	(4 << 8)	/* video mode only */
#define DSIM_SUB_PIX_FORMAT_18BPP_PRGB	(5 << 8)	/* video mode only */
#define DSIM_SUB_PIX_FORMAT_18BPP_LRGB	(6 << 8)	/* common */
#define DSIM_SUB_PIX_FORMAT_24BPP_RGB	(7 << 8)	/* common */
#define DSIM_MAIN_PIX_FORMAT_3BPP	(0 << 12)	/* command mode only */
#define DSIM_MAIN_PIX_FORMAT_8BPP	(1 << 12)	/* command mode only */
#define DSIM_MAIN_PIX_FORMAT_12BPP	(2 << 12)	/* command mode only */
#define DSIM_MAIN_PIX_FORMAT_16BPP	(3 << 12)	/* command mode only */
#define DSIM_MAIN_PIX_FORMAT_16BPP_RGB	(4 << 12)	/* video mode only */
#define DSIM_MAIN_PIX_FORMAT_18BPP_PRGB	(5 << 12)	/* video mode only */
#define DSIM_MAIN_PIX_FORMAT_18BPP_LRGB	(6 << 12)	/* common */
#define DSIM_MAIN_PIX_FORMAT_24BPP_RGB	(7 << 12)	/* common */

/* Virtual channel number for sub display */
#define DSIM_SUB_VC(x)			(((x) & 0x3) << 16)
/* Virtual channel number for main display */
#define DSIM_MAIN_VC(x)			(((x) & 0x3) << 18)
#define DSIM_HSA_MODE_ENABLE		(1 << 20)
#define DSIM_HSA_MODE_DISABLE		(0 << 20)
#define DSIM_HBP_MODE_ENABLE		(1 << 21)
#define DSIM_HBP_MODE_DISABLE		(0 << 21)
#define DSIM_HFP_MODE_ENABLE		(1 << 22)
#define DSIM_HFP_MODE_DISABLE		(0 << 22)
#define DSIM_HSE_MODE_ENABLE		(1 << 23)
#define DSIM_HSE_MODE_DISABLE		(0 << 23)
#define DSIM_AUTO_MODE			(1 << 24)
#define DSIM_CONFIGURATION_MODE		(0 << 24)
#define DSIM_VIDEO_MODE			(1 << 25)
#define DSIM_COMMAND_MODE		(0 << 25)
#define DSIM_BURST_MODE			(1 << 26)
#define DSIM_NON_BURST_MODE		(0 << 26)
#define DSIM_SYNC_INFORM_PULSE		(1 << 27)
#define DSIM_SYNC_INFORM_EVENT		(0 << 27)
/* enable EoT packet generation for V1.01r11 */
#define DSIM_EOT_R03_ENABLE		(0 << 28)
/* disable EoT packet generation for V1.01r03 */
#define DSIM_EOT_R03_DISABLE		(1 << 28)

/* S5P_DSIM_MHPORCH */
#define DSIM_MAIN_HFP_SHIFT		(16)
#define DSIM_MAIN_HBP_SHIFT		(0)
#define DSIM_MAIN_HFP_MASK		((0xffff) << DSIM_MAIN_HFP_SHIFT)
#define DSIM_MAIN_HBP_MASK		((0xffff) << DSIM_MAIN_HBP_SHIFT)
#define DSIM_MAIN_HFP(x)		(((x) & 0xffff) << DSIM_MAIN_HFP_SHIFT)
#define DSIM_MAIN_HBP(x)		(((x) & 0xffff) << DSIM_MAIN_HBP_SHIFT)

/* S5P_DSIM_MSYNC */
#define DSIM_MAIN_VSA_SHIFT		(22)
#define DSIM_MAIN_HSA_SHIFT		(0)
#define DSIM_MAIN_VSA_MASK		((0x3ff) << DSIM_MAIN_VSA_SHIFT)
#define DSIM_MAIN_HSA_MASK		((0xffff) << DSIM_MAIN_HSA_SHIFT)
#define DSIM_MAIN_VSA(x)		(((x) & 0x3ff) << DSIM_MAIN_VSA_SHIFT)
#define DSIM_MAIN_HSA(x)		(((x) & 0xffff) << DSIM_MAIN_HSA_SHIFT)

/* S5P_DSIM_ESCMODE */
#define DSIM_STOP_STATE_CNT_SHIFT	(21)
#define DSIM_STOP_STATE_CNT(x)		(((x) & 0x3ff) << \
						DSIM_STOP_STATE_CNT_SHIFT)
#define DSIM_FORCE_STOP_STATE_SHIFT	(20)
#define DSIM_FORCE_BTA_SHIFT		(16)
#define DSIM_CMD_LPDT_HS_MODE		(0 << 7)
#define DSIM_CMD_LPDT_LP_MODE		(1 << 7)
#define DSIM_TX_LPDT_HS_MODE		(0 << 6)
#define DSIM_TX_LPDT_LP_MODE		(1 << 6)
#define DSIM_TX_TRIGGER_RST_SHIFT	(4)
#define DSIM_TX_UIPS_DAT_SHIFT		(3)
#define DSIM_TX_UIPS_EXIT_SHIFT		(2)
#define DSIM_TX_UIPS_CLK_SHIFT		(1)
#define DSIM_TX_UIPS_CLK_EXIT_SHIFT	(0)

/* S5P_DSIM_MDRESOL */
#define DSIM_MDRESOL_MAIN_NOT_STANDBY		(0 << 31)
#define DSIM_MDRESOL_MAIN_STAND_BY		(1 << 31)
#define DSIM_MDRESOL_MAIN_VRESOL(x)		(((x) & 0x7ff) << 16)
#define DSIM_MDRESOL_MAIN_HRESOL(x)		(((x) & 0x7ff) << 0)

/* S5P_DSIM_MVPORCH */
#define DSIM_CMD_ALLOW_SHIFT		(28)
#define DSIM_STABLE_VFP_SHIFT		(16)
#define DSIM_MAIN_VBP_SHIFT		(0)
#define DSIM_CMD_ALLOW_MASK		(0xf << DSIM_CMD_ALLOW_SHIFT)
#define DSIM_STABLE_VFP_MASK		(0x7ff << DSIM_STABLE_VFP_SHIFT)
#define DSIM_MAIN_VBP_MASK		(0x7ff << DSIM_MAIN_VBP_SHIFT)
#define DSIM_CMD_ALLOW(x)		(((x) & 0xf) << DSIM_CMD_ALLOW_SHIFT)
#define DSIM_STABLE_VFP(x)		(((x) & 0x7ff) << DSIM_STABLE_VFP_SHIFT)
#define DSIM_MAIN_VBP(x)		(((x) & 0x7ff) << DSIM_MAIN_VBP_SHIFT)

/* S5P_DSIM_MHPORCH */
#define DSIM_MHPORCH_MAIN_HFP(x)		(((x) & 0xffff) << 16)
#define DSIM_MHPORCH_MAIN_HBP(x)		(((x) & 0xffff) << 0)


#define DSIM_MSYNC_MAIN_VSA(x)		(((x) & 0x3ff) << 22)
#define DSIM_MSYNC_MAIN_HSA(x)		(((x) & 0xffff) << 0)

/* S5P_DSIM_SDRESOL */
#define DSIM_SUB_STANDY_SHIFT		(31)
#define DSIM_SUB_VRESOL_SHIFT		(16)
#define DSIM_SUB_HRESOL_SHIFT		(0)
#define DSIM_SUB_STANDY_MASK		((0x1) << DSIM_SUB_STANDY_SHIFT)
#define DSIM_SUB_VRESOL_MASK		((0x7ff) << DSIM_SUB_VRESOL_SHIFT)
#define DSIM_SUB_HRESOL_MASK		((0x7ff) << DSIM_SUB_HRESOL_SHIFT)
#define DSIM_SUB_STANDY			(1 << DSIM_SUB_STANDY_SHIFT)
#define DSIM_SUB_NOT_READY		(0 << DSIM_SUB_STANDY_SHIFT)
#define DSIM_SUB_VRESOL(x)		(((x) & 0x7ff) << DSIM_SUB_VRESOL_SHIFT)
#define DSIM_SUB_HRESOL(x)		(((x) & 0x7ff) << DSIM_SUB_HRESOL_SHIFT)


#define DSIM_INTSRC_PLL_STABLE			(1 << 31)
#define DSIM_INTSRC_SW_RST_RELEASE		(1 << 30)
#define DSIM_INTSRC_SFR_FIFO_EMPTY		(1 << 29)
#define DSIM_INTSRC_SYNC_OVERRIDE		(1 << 28)
#define DSIM_INTSRC_BUS_TURN_OVER		(1 << 25)
#define DSIM_INTSRC_FRAME_DONE			(1 << 24)
#define DSIM_INTSRC_LPDRTOUT			(1 << 21)
#define DSIM_INTSRC_TATOUT			(1 << 20)
#define DSIM_INTSRC_RX_DAT_DONE			(1 << 18)
#define DSIM_INTSRC_RX_TE			(1 << 17)
#define DSIM_INTSRC_RX_ACK			(1 << 16)
#define DSIM_INTSRC_ERR_RX_ECC			(1 << 15)
#define DSIM_INTSRC_ERR_RX_CRC			(1 << 14)
#define DSIM_INTSRC_ERR_ESC3			(1 << 13)
#define DSIM_INTSRC_ERR_ESC2			(1 << 12)
#define DSIM_INTSRC_ERR_ESC1			(1 << 11)
#define DSIM_INTSRC_ERR_ESC0			(1 << 10)
#define DSIM_INTSRC_ERR_SYNC3			(1 << 9)
#define DSIM_INTSRC_ERR_SYNC2			(1 << 8)
#define DSIM_INTSRC_ERR_SYNC1			(1 << 7)
#define DSIM_INTSRC_ERR_SYNC0			(1 << 6)
#define DSIM_INTSRC_ERR_CONTROL3		(1 << 5)
#define DSIM_INTSRC_ERR_CONTROL2		(1 << 4)
#define DSIM_INTSRC_ERR_CONTROL1		(1 << 3)
#define DSIM_INTSRC_ERR_CONTROL0		(1 << 2)
#define DSIM_INTSRC_ERR_CONTENT_LP0		(1 << 1)
#define DSIM_INTSRC_ERR_CONTENT_LP1		(1 << 0)

#define DSIM_INTMSK_ERR_CONTENT_LP1		(1 << 0)
#define DSIM_INTMSK_ERR_CONTENT_LP0		(1 << 1)
#define DSIM_INTMSK_ERR_CONTROL0		(1 << 2)
#define DSIM_INTMSK_ERR_CONTROL1		(1 << 3)
#define DSIM_INTMSK_ERR_CONTROL2		(1 << 4)
#define DSIM_INTMSK_ERR_CONTROL3		(1 << 5)
#define DSIM_INTMSK_ERR_SYNC0		(1 << 6)
#define DSIM_INTMSK_ERR_SYNC1		(1 << 7)
#define DSIM_INTMSK_ERR_SYNC2		(1 << 8)
#define DSIM_INTMSK_ERR_SYNC3		(1 << 9)
#define DSIM_INTMSK_ERR_ESC0		(1 << 10)
#define DSIM_INTMSK_ERR_ESC1		(1 << 11)
#define DSIM_INTMSK_ERR_ESC2		(1 << 12)
#define DSIM_INTMSK_ERR_ESC3		(1 << 13)
#define DSIM_INTMSK_ERR_RX_CRC		(1 << 14)
#define DSIM_INTMSK_ERR_RX_ECC		(1 << 15)
#define DSIM_INTMSK_RX_ACK			(1 << 16)
#define DSIM_INTMSK_RX_TE			(1 << 17)
#define DSIM_INTMSK_RX_DAT_DONE		(1 << 18)
#define DSIM_INTMSK_TA_TOUT			(1 << 20)
#define DSIM_INTMSK_LPDR_TOUT		(1 << 21)
#define DSIM_INTMSK_FRAME_DONE		(1 << 24)
#define DSIM_INTMSK_BUS_TURN_OVER		(1 << 25)
#define DSIM_INTMSK_SFR_FIFO_EMPTY		(1 << 29)
#define DSIM_INTMSK_SW_RST_RELEASE		(1 << 30)
#define DSIM_INTMSK_PLL_STABLE		(1 << 31)

#define DSIM_PKTHDR_HEADER_DI(x)		(((x) & 0xff) << 0)
#define DSIM_PKTHDR_HEADER_DAT0(x)		(((x) & 0xff) << 8)
#define DSIM_PKTHDR_HEADER_DAT1(x)		(((x) & 0xff) << 16)

/* S5P_DSIM_PHYACCHR */
#define DSIM_AFC_CTL(x)		(((x) & 0x7) << 5)
#define DSIM_AFC_ENABLE		(1 << 14)
#define DSIM_AFC_DISABLE		(0 << 14)

#define DSIM_RX_FIFO			(1 << 4)
#define DSIM_TX_SFR_FIFO			(1 << 3)
#define DSIM_I80_FIFO				(1 << 2)
#define DSIM_SUB_DISP_FIFO			(1 << 1)
#define DSIM_MAIN_DISP_FIFO			(1 << 0)

/* S5P_DSIM_PLLCTRL */
#define DSIM_PMS_SHIFT			(1)
#define DSIM_PLL_EN_SHIFT		(23)
#define DSIM_FREQ_BAND_SHIFT		(24)
#define DSIM_PMS(x)			(((x) & 0x7ffff) << DSIM_PMS_SHIFT)
#define DSIM_FREQ_BAND(x)		(((x) & 0xf) << DSIM_FREQ_BAND_SHIFT)

/* DSIM_SWRST */
#define DSIM_FUNCRST		(1 << 16)
#define DSIM_SWRST		(1 << 0)

