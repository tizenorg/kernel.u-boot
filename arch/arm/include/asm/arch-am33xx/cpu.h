/*
 * cpu.h
 *
 * AM33xx specific header file
 *
 * Copyright (C) 2011, Texas Instruments, Incorporated - http://www.ti.com/
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _AM33XX_CPU_H
#define _AM33XX_CPU_H

#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))
#include <asm/types.h>
#endif /* !(__KERNEL_STRICT_NAMES || __ASSEMBLY__) */

#include <asm/arch/hardware.h>

#define BIT(x)				(1 << x)
#define CL_BIT(x)			(0 << x)

/* Timer register bits */
#define TCLR_ST				BIT(0)	/* Start=1 Stop=0 */
#define TCLR_AR				BIT(1)	/* Auto reload */
#define TCLR_PRE			BIT(5)	/* Pre-scaler enable */
#define TCLR_PTV_SHIFT			(2)	/* Pre-scaler shift value */
#define TCLR_PRE_DISABLE		CL_BIT(5) /* Pre-scalar disable */

/* device type */
#define DEVICE_MASK			(BIT(8) | BIT(9) | BIT(10))
#define TST_DEVICE			0x0
#define EMU_DEVICE			0x1
#define HS_DEVICE			0x2
#define GP_DEVICE			0x3

/* cpu-id for AM33XX and TI81XX family */
#define AM335X				0xB944
#define TI81XX				0xB81E
#define DEVICE_ID			(CTRL_BASE + 0x0600)
#define DEVICE_ID_MASK			0x1FFF

/* MPU max frequencies */
#define AM335X_ZCZ_300			0x1FEF
#define AM335X_ZCZ_600			0x1FAF
#define AM335X_ZCZ_720			0x1F2F
#define AM335X_ZCZ_800			0x1E2F
#define AM335X_ZCZ_1000			0x1C2F
#define AM335X_ZCE_300			0x1FDF
#define AM335X_ZCE_600			0x1F9F

/* This gives the status of the boot mode pins on the evm */
#define SYSBOOT_MASK			(BIT(0) | BIT(1) | BIT(2)\
					| BIT(3) | BIT(4))

#define PRM_RSTCTRL_RESET		0x01
#define PRM_RSTST_WARM_RESET_MASK	0x232

/*
 * Watchdog:
 * Using the prescaler, the OMAP watchdog could go for many
 * months before firing.  These limits work without scaling,
 * with the 60 second default assumed by most tools and docs.
 */
#define TIMER_MARGIN_MAX	(24 * 60 * 60)	/* 1 day */
#define TIMER_MARGIN_DEFAULT	60	/* 60 secs */
#define TIMER_MARGIN_MIN	1

#define PTV			0	/* prescale */
#define GET_WLDR_VAL(secs)	(0xffffffff - ((secs) * (32768/(1<<PTV))) + 1)
#define WDT_WWPS_PEND_WCLR	BIT(0)
#define WDT_WWPS_PEND_WLDR	BIT(2)
#define WDT_WWPS_PEND_WTGR	BIT(3)
#define WDT_WWPS_PEND_WSPR	BIT(4)

#define WDT_WCLR_PRE		BIT(5)
#define WDT_WCLR_PTV_OFF	2

#ifndef __KERNEL_STRICT_NAMES
#ifndef __ASSEMBLY__
struct gpmc_cs {
	u32 config1;		/* 0x00 */
	u32 config2;		/* 0x04 */
	u32 config3;		/* 0x08 */
	u32 config4;		/* 0x0C */
	u32 config5;		/* 0x10 */
	u32 config6;		/* 0x14 */
	u32 config7;		/* 0x18 */
	u32 nand_cmd;		/* 0x1C */
	u32 nand_adr;		/* 0x20 */
	u32 nand_dat;		/* 0x24 */
	u8 res[8];		/* blow up to 0x30 byte */
};

struct bch_res_0_3 {
	u32 bch_result_x[4];
};

struct gpmc {
	u8 res1[0x10];
	u32 sysconfig;		/* 0x10 */
	u8 res2[0x4];
	u32 irqstatus;		/* 0x18 */
	u32 irqenable;		/* 0x1C */
	u8 res3[0x20];
	u32 timeout_control;	/* 0x40 */
	u8 res4[0xC];
	u32 config;		/* 0x50 */
	u32 status;		/* 0x54 */
	u8 res5[0x8];		/* 0x58 */
	struct gpmc_cs cs[8];	/* 0x60, 0x90, .. */
	u8 res6[0x14];		/* 0x1E0 */
	u32 ecc_config;		/* 0x1F4 */
	u32 ecc_control;	/* 0x1F8 */
	u32 ecc_size_config;	/* 0x1FC */
	u32 ecc1_result;	/* 0x200 */
	u32 ecc2_result;	/* 0x204 */
	u32 ecc3_result;	/* 0x208 */
	u32 ecc4_result;	/* 0x20C */
	u32 ecc5_result;	/* 0x210 */
	u32 ecc6_result;	/* 0x214 */
	u32 ecc7_result;	/* 0x218 */
	u32 ecc8_result;	/* 0x21C */
	u32 ecc9_result;	/* 0x220 */
	u8 res7[12];		/* 0x224 */
	u32 testmomde_ctrl;	/* 0x230 */
	u8 res8[12];		/* 0x234 */
	struct bch_res_0_3 bch_result_0_3[2];	/* 0x240 */
};

/* Used for board specific gpmc initialization */
extern struct gpmc *gpmc_cfg;

#ifndef CONFIG_AM43XX
/* Encapsulating core pll registers */
struct cm_wkuppll {
	unsigned int wkclkstctrl;	/* offset 0x00 */
	unsigned int wkctrlclkctrl;	/* offset 0x04 */
	unsigned int wkgpio0clkctrl;	/* offset 0x08 */
	unsigned int wkl4wkclkctrl;	/* offset 0x0c */
	unsigned int resv2[4];
	unsigned int idlestdpllmpu;	/* offset 0x20 */
	unsigned int resv3[2];
	unsigned int clkseldpllmpu;	/* offset 0x2c */
	unsigned int resv4[1];
	unsigned int idlestdpllddr;	/* offset 0x34 */
	unsigned int resv5[2];
	unsigned int clkseldpllddr;	/* offset 0x40 */
	unsigned int resv6[4];
	unsigned int clkseldplldisp;	/* offset 0x54 */
	unsigned int resv7[1];
	unsigned int idlestdpllcore;	/* offset 0x5c */
	unsigned int resv8[2];
	unsigned int clkseldpllcore;	/* offset 0x68 */
	unsigned int resv9[1];
	unsigned int idlestdpllper;	/* offset 0x70 */
	unsigned int resv10[2];
	unsigned int clkdcoldodpllper;	/* offset 0x7c */
	unsigned int divm4dpllcore;	/* offset 0x80 */
	unsigned int divm5dpllcore;	/* offset 0x84 */
	unsigned int clkmoddpllmpu;	/* offset 0x88 */
	unsigned int clkmoddpllper;	/* offset 0x8c */
	unsigned int clkmoddpllcore;	/* offset 0x90 */
	unsigned int clkmoddpllddr;	/* offset 0x94 */
	unsigned int clkmoddplldisp;	/* offset 0x98 */
	unsigned int clkseldpllper;	/* offset 0x9c */
	unsigned int divm2dpllddr;	/* offset 0xA0 */
	unsigned int divm2dplldisp;	/* offset 0xA4 */
	unsigned int divm2dpllmpu;	/* offset 0xA8 */
	unsigned int divm2dpllper;	/* offset 0xAC */
	unsigned int resv11[1];
	unsigned int wkup_uart0ctrl;	/* offset 0xB4 */
	unsigned int wkup_i2c0ctrl;	/* offset 0xB8 */
	unsigned int wkup_adctscctrl;	/* offset 0xBC */
	unsigned int resv12[6];
	unsigned int divm6dpllcore;	/* offset 0xD8 */
};

/**
 * Encapsulating peripheral functional clocks
 * pll registers
 */
struct cm_perpll {
	unsigned int l4lsclkstctrl;	/* offset 0x00 */
	unsigned int l3sclkstctrl;	/* offset 0x04 */
	unsigned int l4fwclkstctrl;	/* offset 0x08 */
	unsigned int l3clkstctrl;	/* offset 0x0c */
	unsigned int resv1;
	unsigned int cpgmac0clkctrl;	/* offset 0x14 */
	unsigned int lcdclkctrl;	/* offset 0x18 */
	unsigned int usb0clkctrl;	/* offset 0x1C */
	unsigned int resv2;
	unsigned int tptc0clkctrl;	/* offset 0x24 */
	unsigned int emifclkctrl;	/* offset 0x28 */
	unsigned int ocmcramclkctrl;	/* offset 0x2c */
	unsigned int gpmcclkctrl;	/* offset 0x30 */
	unsigned int mcasp0clkctrl;	/* offset 0x34 */
	unsigned int uart5clkctrl;	/* offset 0x38 */
	unsigned int mmc0clkctrl;	/* offset 0x3C */
	unsigned int elmclkctrl;	/* offset 0x40 */
	unsigned int i2c2clkctrl;	/* offset 0x44 */
	unsigned int i2c1clkctrl;	/* offset 0x48 */
	unsigned int spi0clkctrl;	/* offset 0x4C */
	unsigned int spi1clkctrl;	/* offset 0x50 */
	unsigned int resv3[3];
	unsigned int l4lsclkctrl;	/* offset 0x60 */
	unsigned int l4fwclkctrl;	/* offset 0x64 */
	unsigned int mcasp1clkctrl;	/* offset 0x68 */
	unsigned int uart1clkctrl;	/* offset 0x6C */
	unsigned int uart2clkctrl;	/* offset 0x70 */
	unsigned int uart3clkctrl;	/* offset 0x74 */
	unsigned int uart4clkctrl;	/* offset 0x78 */
	unsigned int timer7clkctrl;	/* offset 0x7C */
	unsigned int timer2clkctrl;	/* offset 0x80 */
	unsigned int timer3clkctrl;	/* offset 0x84 */
	unsigned int timer4clkctrl;	/* offset 0x88 */
	unsigned int resv4[8];
	unsigned int gpio1clkctrl;	/* offset 0xAC */
	unsigned int gpio2clkctrl;	/* offset 0xB0 */
	unsigned int gpio3clkctrl;	/* offset 0xB4 */
	unsigned int resv5;
	unsigned int tpccclkctrl;	/* offset 0xBC */
	unsigned int dcan0clkctrl;	/* offset 0xC0 */
	unsigned int dcan1clkctrl;	/* offset 0xC4 */
	unsigned int resv6;
	unsigned int epwmss1clkctrl;	/* offset 0xCC */
	unsigned int emiffwclkctrl;	/* offset 0xD0 */
	unsigned int epwmss0clkctrl;	/* offset 0xD4 */
	unsigned int epwmss2clkctrl;	/* offset 0xD8 */
	unsigned int l3instrclkctrl;	/* offset 0xDC */
	unsigned int l3clkctrl;		/* Offset 0xE0 */
	unsigned int resv8[4];
	unsigned int mmc1clkctrl;	/* offset 0xF4 */
	unsigned int mmc2clkctrl;	/* offset 0xF8 */
	unsigned int resv9[8];
	unsigned int l4hsclkstctrl;	/* offset 0x11C */
	unsigned int l4hsclkctrl;	/* offset 0x120 */
	unsigned int resv10[8];
	unsigned int cpswclkstctrl;	/* offset 0x144 */
	unsigned int lcdcclkstctrl;	/* offset 0x148 */
};

/* Encapsulating Display pll registers */
struct cm_dpll {
	unsigned int resv1[2];
	unsigned int clktimer2clk;	/* offset 0x08 */
	unsigned int resv2[10];
	unsigned int clklcdcpixelclk;	/* offset 0x34 */
};
#else
/* Encapsulating core pll registers */
struct cm_wkuppll {
	unsigned int resv0[136];
	unsigned int wkl4wkclkctrl;	/* offset 0x220 */
	unsigned int resv1[55];
	unsigned int wkclkstctrl;	/* offset 0x300 */
	unsigned int resv2[15];
	unsigned int wkup_i2c0ctrl;	/* offset 0x340 */
	unsigned int resv3;
	unsigned int wkup_uart0ctrl;	/* offset 0x348 */
	unsigned int resv4[5];
	unsigned int wkctrlclkctrl;	/* offset 0x360 */
	unsigned int resv5;
	unsigned int wkgpio0clkctrl;	/* offset 0x368 */

	unsigned int resv6[109];
	unsigned int clkmoddpllcore;	/* offset 0x520 */
	unsigned int idlestdpllcore;	/* offset 0x524 */
	unsigned int resv61;
	unsigned int clkseldpllcore;	/* offset 0x52C */
	unsigned int resv7[2];
	unsigned int divm4dpllcore;	/* offset 0x538 */
	unsigned int divm5dpllcore;	/* offset 0x53C */
	unsigned int divm6dpllcore;	/* offset 0x540 */

	unsigned int resv8[7];
	unsigned int clkmoddpllmpu;	/* offset 0x560 */
	unsigned int idlestdpllmpu;	/* offset 0x564 */
	unsigned int resv9;
	unsigned int clkseldpllmpu;	/* offset 0x56c */
	unsigned int divm2dpllmpu;	/* offset 0x570 */

	unsigned int resv10[11];
	unsigned int clkmoddpllddr;	/* offset 0x5A0 */
	unsigned int idlestdpllddr;	/* offset 0x5A4 */
	unsigned int resv11;
	unsigned int clkseldpllddr;	/* offset 0x5AC */
	unsigned int divm2dpllddr;	/* offset 0x5B0 */

	unsigned int resv12[11];
	unsigned int clkmoddpllper;	/* offset 0x5E0 */
	unsigned int idlestdpllper;	/* offset 0x5E4 */
	unsigned int resv13;
	unsigned int clkseldpllper;	/* offset 0x5EC */
	unsigned int divm2dpllper;	/* offset 0x5F0 */
	unsigned int resv14[8];
	unsigned int clkdcoldodpllper;	/* offset 0x614 */

	unsigned int resv15[2];
	unsigned int clkmoddplldisp;	/* offset 0x620 */
	unsigned int resv16[2];
	unsigned int clkseldplldisp;	/* offset 0x62C */
	unsigned int divm2dplldisp;	/* offset 0x630 */
};

/*
 * Encapsulating peripheral functional clocks
 * pll registers
 */
struct cm_perpll {
	unsigned int l3clkstctrl;	/* offset 0x00 */
	unsigned int resv0[7];
	unsigned int l3clkctrl;		/* Offset 0x20 */
	unsigned int resv1[7];
	unsigned int l3instrclkctrl;	/* offset 0x40 */
	unsigned int resv2[3];
	unsigned int ocmcramclkctrl;	/* offset 0x50 */
	unsigned int resv3[9];
	unsigned int tpccclkctrl;	/* offset 0x78 */
	unsigned int resv4;
	unsigned int tptc0clkctrl;	/* offset 0x80 */

	unsigned int resv5[7];
	unsigned int l4hsclkctrl;	/* offset 0x0A0 */
	unsigned int resv6;
	unsigned int l4fwclkctrl;	/* offset 0x0A8 */
	unsigned int resv7[85];
	unsigned int l3sclkstctrl;	/* offset 0x200 */
	unsigned int resv8[7];
	unsigned int gpmcclkctrl;	/* offset 0x220 */
	unsigned int resv9[5];
	unsigned int mcasp0clkctrl;	/* offset 0x238 */
	unsigned int resv10;
	unsigned int mcasp1clkctrl;	/* offset 0x240 */
	unsigned int resv11;
	unsigned int mmc2clkctrl;	/* offset 0x248 */
	unsigned int resv12[3];
	unsigned int qspiclkctrl;       /* offset 0x258 */
	unsigned int resv121;
	unsigned int usb0clkctrl;	/* offset 0x260 */
	unsigned int resv13[103];
	unsigned int l4lsclkstctrl;	/* offset 0x400 */
	unsigned int resv14[7];
	unsigned int l4lsclkctrl;	/* offset 0x420 */
	unsigned int resv15;
	unsigned int dcan0clkctrl;	/* offset 0x428 */
	unsigned int resv16;
	unsigned int dcan1clkctrl;	/* offset 0x430 */
	unsigned int resv17[13];
	unsigned int elmclkctrl;	/* offset 0x468 */

	unsigned int resv18[3];
	unsigned int gpio1clkctrl;	/* offset 0x478 */
	unsigned int resv19;
	unsigned int gpio2clkctrl;	/* offset 0x480 */
	unsigned int resv20;
	unsigned int gpio3clkctrl;	/* offset 0x488 */
	unsigned int resv41;
	unsigned int gpio4clkctrl;	/* offset 0x490 */
	unsigned int resv42;
	unsigned int gpio5clkctrl;	/* offset 0x498 */
	unsigned int resv21[3];

	unsigned int i2c1clkctrl;	/* offset 0x4A8 */
	unsigned int resv22;
	unsigned int i2c2clkctrl;	/* offset 0x4B0 */
	unsigned int resv23[3];
	unsigned int mmc0clkctrl;	/* offset 0x4C0 */
	unsigned int resv24;
	unsigned int mmc1clkctrl;	/* offset 0x4C8 */

	unsigned int resv25[13];
	unsigned int spi0clkctrl;	/* offset 0x500 */
	unsigned int resv26;
	unsigned int spi1clkctrl;	/* offset 0x508 */
	unsigned int resv27[9];
	unsigned int timer2clkctrl;	/* offset 0x530 */
	unsigned int resv28;
	unsigned int timer3clkctrl;	/* offset 0x538 */
	unsigned int resv29;
	unsigned int timer4clkctrl;	/* offset 0x540 */
	unsigned int resv30[5];
	unsigned int timer7clkctrl;	/* offset 0x558 */

	unsigned int resv31[9];
	unsigned int uart1clkctrl;	/* offset 0x580 */
	unsigned int resv32;
	unsigned int uart2clkctrl;	/* offset 0x588 */
	unsigned int resv33;
	unsigned int uart3clkctrl;	/* offset 0x590 */
	unsigned int resv34;
	unsigned int uart4clkctrl;	/* offset 0x598 */
	unsigned int resv35;
	unsigned int uart5clkctrl;	/* offset 0x5A0 */
	unsigned int resv36[87];

	unsigned int emifclkstctrl;	/* offset 0x700 */
	unsigned int resv361[7];
	unsigned int emifclkctrl;	/* offset 0x720 */
	unsigned int resv37[3];
	unsigned int emiffwclkctrl;	/* offset 0x730 */
	unsigned int resv371;
	unsigned int otfaemifclkctrl;	/* offset 0x738 */
	unsigned int resv38[57];
	unsigned int lcdclkctrl;	/* offset 0x820 */
	unsigned int resv39[183];
	unsigned int cpswclkstctrl;	/* offset 0xB00 */
	unsigned int resv40[7];
	unsigned int cpgmac0clkctrl;	/* offset 0xB20 */
};

struct cm_device_inst {
	unsigned int cm_clkout1_ctrl;
	unsigned int cm_dll_ctrl;
};

struct cm_dpll {
	unsigned int resv1;
	unsigned int clktimer2clk;	/* offset 0x04 */
};
#endif /* CONFIG_AM43XX */

/* Control Module RTC registers */
struct cm_rtc {
	unsigned int rtcclkctrl;	/* offset 0x0 */
	unsigned int clkstctrl;		/* offset 0x4 */
};

/* Watchdog timer registers */
struct wd_timer {
	unsigned int resv1[4];
	unsigned int wdtwdsc;	/* offset 0x010 */
	unsigned int wdtwdst;	/* offset 0x014 */
	unsigned int wdtwisr;	/* offset 0x018 */
	unsigned int wdtwier;	/* offset 0x01C */
	unsigned int wdtwwer;	/* offset 0x020 */
	unsigned int wdtwclr;	/* offset 0x024 */
	unsigned int wdtwcrr;	/* offset 0x028 */
	unsigned int wdtwldr;	/* offset 0x02C */
	unsigned int wdtwtgr;	/* offset 0x030 */
	unsigned int wdtwwps;	/* offset 0x034 */
	unsigned int resv2[3];
	unsigned int wdtwdly;	/* offset 0x044 */
	unsigned int wdtwspr;	/* offset 0x048 */
	unsigned int resv3[1];
	unsigned int wdtwqeoi;	/* offset 0x050 */
	unsigned int wdtwqstar;	/* offset 0x054 */
	unsigned int wdtwqsta;	/* offset 0x058 */
	unsigned int wdtwqens;	/* offset 0x05C */
	unsigned int wdtwqenc;	/* offset 0x060 */
	unsigned int resv4[39];
	unsigned int wdt_unfr;	/* offset 0x100 */
};

/* Timer 32 bit registers */
struct gptimer {
	unsigned int tidr;		/* offset 0x00 */
	unsigned char res1[12];
	unsigned int tiocp_cfg;		/* offset 0x10 */
	unsigned char res2[12];
	unsigned int tier;		/* offset 0x20 */
	unsigned int tistatr;		/* offset 0x24 */
	unsigned int tistat;		/* offset 0x28 */
	unsigned int tisr;		/* offset 0x2c */
	unsigned int tcicr;		/* offset 0x30 */
	unsigned int twer;		/* offset 0x34 */
	unsigned int tclr;		/* offset 0x38 */
	unsigned int tcrr;		/* offset 0x3c */
	unsigned int tldr;		/* offset 0x40 */
	unsigned int ttgr;		/* offset 0x44 */
	unsigned int twpc;		/* offset 0x48 */
	unsigned int tmar;		/* offset 0x4c */
	unsigned int tcar1;		/* offset 0x50 */
	unsigned int tscir;		/* offset 0x54 */
	unsigned int tcar2;		/* offset 0x58 */
};

/* UART Registers */
struct uart_sys {
	unsigned int resv1[21];
	unsigned int uartsyscfg;	/* offset 0x54 */
	unsigned int uartsyssts;	/* offset 0x58 */
};

/* VTP Registers */
struct vtp_reg {
	unsigned int vtp0ctrlreg;
};

/* Control Status Register */
struct ctrl_stat {
	unsigned int resv1[16];
	unsigned int statusreg;		/* ofset 0x40 */
	unsigned int resv2[51];
	unsigned int secure_emif_sdram_config;	/* offset 0x0110 */
	unsigned int resv3[319];
	unsigned int dev_attr;
};

/* AM33XX GPIO registers */
#define OMAP_GPIO_REVISION		0x0000
#define OMAP_GPIO_SYSCONFIG		0x0010
#define OMAP_GPIO_SYSSTATUS		0x0114
#define OMAP_GPIO_IRQSTATUS1		0x002c
#define OMAP_GPIO_IRQSTATUS2		0x0030
#define OMAP_GPIO_CTRL			0x0130
#define OMAP_GPIO_OE			0x0134
#define OMAP_GPIO_DATAIN		0x0138
#define OMAP_GPIO_DATAOUT		0x013c
#define OMAP_GPIO_LEVELDETECT0		0x0140
#define OMAP_GPIO_LEVELDETECT1		0x0144
#define OMAP_GPIO_RISINGDETECT		0x0148
#define OMAP_GPIO_FALLINGDETECT		0x014c
#define OMAP_GPIO_DEBOUNCE_EN		0x0150
#define OMAP_GPIO_DEBOUNCE_VAL		0x0154
#define OMAP_GPIO_CLEARDATAOUT		0x0190
#define OMAP_GPIO_SETDATAOUT		0x0194

/* Control Device Register */
struct ctrl_dev {
	unsigned int deviceid;		/* offset 0x00 */
	unsigned int resv1[7];
	unsigned int usb_ctrl0;		/* offset 0x20 */
	unsigned int resv2;
	unsigned int usb_ctrl1;		/* offset 0x28 */
	unsigned int resv3;
	unsigned int macid0l;		/* offset 0x30 */
	unsigned int macid0h;		/* offset 0x34 */
	unsigned int macid1l;		/* offset 0x38 */
	unsigned int macid1h;		/* offset 0x3c */
	unsigned int resv4[4];
	unsigned int miisel;		/* offset 0x50 */
	unsigned int resv5[106];
	unsigned int efuse_sma;		/* offset 0x1FC */
};

/* gmii_sel register defines */
#define GMII1_SEL_MII		0x0
#define GMII1_SEL_RMII		0x1
#define GMII1_SEL_RGMII		0x2
#define GMII2_SEL_MII		0x0
#define GMII2_SEL_RMII		0x4
#define GMII2_SEL_RGMII		0x8
#define RGMII1_IDMODE		BIT(4)
#define RGMII2_IDMODE		BIT(5)
#define RMII1_IO_CLK_EN		BIT(6)
#define RMII2_IO_CLK_EN		BIT(7)

#define MII_MODE_ENABLE		(GMII1_SEL_MII | GMII2_SEL_MII)
#define RMII_MODE_ENABLE        (GMII1_SEL_RMII | GMII2_SEL_RMII)
#define RGMII_MODE_ENABLE	(GMII1_SEL_RGMII | GMII2_SEL_RGMII)
#define RGMII_INT_DELAY		(RGMII1_IDMODE | RGMII2_IDMODE)
#define RMII_CHIPCKL_ENABLE     (RMII1_IO_CLK_EN | RMII2_IO_CLK_EN)

/* PWMSS */
struct pwmss_regs {
	unsigned int idver;
	unsigned int sysconfig;
	unsigned int clkconfig;
	unsigned int clkstatus;
};
#define ECAP_CLK_EN		BIT(0)
#define ECAP_CLK_STOP_REQ	BIT(1)

struct pwmss_ecap_regs {
	unsigned int tsctr;
	unsigned int ctrphs;
	unsigned int cap1;
	unsigned int cap2;
	unsigned int cap3;
	unsigned int cap4;
	unsigned int resv1[4];
	unsigned short ecctl1;
	unsigned short ecctl2;
};

/* Capture Control register 2 */
#define ECTRL2_SYNCOSEL_MASK	(0x03 << 6)
#define ECTRL2_MDSL_ECAP	BIT(9)
#define ECTRL2_CTRSTP_FREERUN	BIT(4)
#define ECTRL2_PLSL_LOW		BIT(10)
#define ECTRL2_SYNC_EN		BIT(5)

#endif /* __ASSEMBLY__ */
#endif /* __KERNEL_STRICT_NAMES */

#endif /* _AM33XX_CPU_H */
