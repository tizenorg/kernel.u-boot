/* linux/asm/arch/regs-fb.h
 *
 * Register definition file for Samsung Display Controller (FIMD) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _REGS_FB_H
#define _REGS_FB_H

#define S5P_LCDREG(x)		(x)
#define S5P_WINCON(x)		S5P_LCDREG(0x0020 + (x * 0x04))
#define S5P_VIDOSD_A(x)		S5P_LCDREG(0x0040 + (x * 0x10))
#define S5P_VIDOSD_B(x)		S5P_LCDREG(0x0044 + (x * 0x10))
#define S5P_VIDOSD_C(x)		S5P_LCDREG(0x0048 + (x * 0x10))
#define S5P_VIDOSD_D(x)		S5P_LCDREG(0x004C + (x * 0x10))
#define S5P_VIDADDR_START0(x)	S5P_LCDREG(0x00A0 + (x * 0x08))
#define S5P_VIDADDR_START1(x)	S5P_LCDREG(0x00A4 + (x * 0x08))
#define S5P_VIDADDR_END0(x)	S5P_LCDREG(0x00D0 + (x * 0x08))
#define S5P_VIDADDR_END1(x)	S5P_LCDREG(0x00D4 + (x * 0x08))
#define S5P_VIDADDR_SIZE(x)	S5P_LCDREG(0x0100 + (x * 0x04))
#define S5P_KEYCON(x)		S5P_LCDREG(0x0140 + ((x - 1) * 0x08))
#define S5P_KEYVAL(x)		S5P_LCDREG(0x0144 + ((x - 1) * 0x08))

/*
 * Register Map
*/
#define S5P_VIDCON0		S5P_LCDREG(0x0000)	/* Video control 0 */
#define S5P_VIDCON1		S5P_LCDREG(0x0004)	/* Video control 1 */
#define S5P_VIDCON2		S5P_LCDREG(0x0008)	/* Video control 2 */
#define S5P_PRTCON		S5P_LCDREG(0x000C)	/* Protect control */

#define S5P_VIDTCON0		S5P_LCDREG(0x0010)	/* Video time control 0 */
#define S5P_VIDTCON1		S5P_LCDREG(0x0014)	/* Video time control 1 */
#define S5P_VIDTCON2		S5P_LCDREG(0x0018)	/* Video time control 2 */

#define S5P_WINCON0		S5P_LCDREG(0x0020)	/* Window control 0 */
#define S5P_WINCON1		S5P_LCDREG(0x0024)	/* Window control 1 */
#define S5P_WINCON2		S5P_LCDREG(0x0028)	/* Window control 2 */
#define S5P_WINCON3		S5P_LCDREG(0x002C)	/* Window control 3 */
#define S5P_WINCON4		S5P_LCDREG(0x0030)	/* Window control 4 */
#define S5P_WINSHMAP		S5P_LCDREG(0x0034)	/* Window Shodow control */
#define S5P_WINCHMAP2		S5P_LCDREG(0x003C)	/* Channel mapping control register */

#define S5P_VIDOSD0A		S5P_LCDREG(0x0040)	/* Video Window 0 position control */
#define S5P_VIDOSD0B		S5P_LCDREG(0x0044)	/* Video Window 0 position control1 */
#define S5P_VIDOSD0C		S5P_LCDREG(0x0048)	/* Video Window 0 position control */

#define S5P_VIDOSD1A		S5P_LCDREG(0x0050)	/* Video Window 1 position control */
#define S5P_VIDOSD1B		S5P_LCDREG(0x0054)	/* Video Window 1 position control */
#define S5P_VIDOSD1C		S5P_LCDREG(0x0058)	/* Video Window 1 position control */
#define S5P_VIDOSD1D		S5P_LCDREG(0x005C)	/* Video Window 1 position control */

#define S5P_VIDOSD2A		S5P_LCDREG(0x0060)	/* Video Window 2 position control */
#define S5P_VIDOSD2B		S5P_LCDREG(0x0064)	/* Video Window 2 position control */
#define S5P_VIDOSD2C		S5P_LCDREG(0x0068)	/* Video Window 2 position control */
#define S5P_VIDOSD2D		S5P_LCDREG(0x006C)	/* Video Window 2 position control */

#define S5P_VIDOSD3A		S5P_LCDREG(0x0070)	/* Video Window 3 position control */
#define S5P_VIDOSD3B		S5P_LCDREG(0x0074)	/* Video Window 3 position control */
#define S5P_VIDOSD3C		S5P_LCDREG(0x0078)	/* Video Window 3 position control */

#define S5P_VIDOSD4A		S5P_LCDREG(0x0080)	/* Video Window 4 position control */
#define S5P_VIDOSD4B		S5P_LCDREG(0x0084)	/* Video Window 4 position control */
#define S5P_VIDOSD4C		S5P_LCDREG(0x0088)	/* Video Window 4 position control */

#define S5P_VIDW00ADD0B0	S5P_LCDREG(0x00A0)	/* Window 0 buffer start address, buffer 0 */
#define S5P_VIDW00ADD0B1	S5P_LCDREG(0x00A4)	/* Window 0 buffer start address, buffer 1 */
#define S5P_VIDW01ADD0B0	S5P_LCDREG(0x00A8)	/* Window 1 buffer start address, buffer 0 */
#define S5P_VIDW01ADD0B1	S5P_LCDREG(0x00AC)	/* Window 1 buffer start address, buffer 1 */
#define S5P_VIDW02ADD0		S5P_LCDREG(0x00B0)	/* Window 2 buffer start address, buffer 0 */
#define S5P_VIDW03ADD0		S5P_LCDREG(0x00B8)	/* Window 3 buffer start address, buffer 0 */
#define S5P_VIDW04ADD0		S5P_LCDREG(0x00C0)	/* Window 4 buffer start address, buffer 0 */
#define S5P_VIDW00ADD1B0	S5P_LCDREG(0x00D0)	/* Window 0 buffer end address, buffer 0 */
#define S5P_VIDW00ADD1B1	S5P_LCDREG(0x00D4)	/* Window 0 buffer end address, buffer 1 */
#define S5P_VIDW01ADD1B0	S5P_LCDREG(0x00D8)	/* Window 1 buffer end address, buffer 0 */
#define S5P_VIDW01ADD1B1	S5P_LCDREG(0x00DC)	/* Window 1 buffer end address, buffer 1 */
#define S5P_VIDW02ADD1		S5P_LCDREG(0x00E0)	/* Window 2 buffer end address */
#define S5P_VIDW03ADD1		S5P_LCDREG(0x00E8)	/* Window 3 buffer end address */
#define S5P_VIDW04ADD1		S5P_LCDREG(0x00F0)	/* Window 4 buffer end address */
#define S5P_VIDW00ADD2		S5P_LCDREG(0x0100)	/* Window 0 buffer size */
#define S5P_VIDW01ADD2		S5P_LCDREG(0x0104)	/* Window 1 buffer size */
#define S5P_VIDW02ADD2		S5P_LCDREG(0x0108)	/* Window 2 buffer size */
#define S5P_VIDW03ADD2		S5P_LCDREG(0x010C)	/* Window 3 buffer size */
#define S5P_VIDW04ADD2		S5P_LCDREG(0x0110)	/* Window 4 buffer size */

#define S5P_VP1TCON0		S5P_LCDREG(0x0118)	/* VP1 interface timing control 0 */
#define S5P_VP1TCON1		S5P_LCDREG(0x011C)	/* VP1 interface timing control 1 */

#define S5P_VIDINTCON0		S5P_LCDREG(0x0130)	/* Indicate the Video interrupt control */
#define S5P_VIDINTCON1		S5P_LCDREG(0x0134)	/* Video Interrupt Pending */

#define S5P_W1KEYCON0		S5P_LCDREG(0x0140)	/* Color key control */
#define S5P_W1KEYCON1		S5P_LCDREG(0x0144)	/* Color key value (transparent value) */
#define S5P_W2KEYCON0		S5P_LCDREG(0x0148)	/* Color key control */
#define S5P_W2KEYCON1		S5P_LCDREG(0x014C)	/* Color key value (transparent value) */
#define S5P_W3KEYCON0		S5P_LCDREG(0x0150)	/* Color key control */
#define S5P_W3KEYCON1		S5P_LCDREG(0x0154)	/* Color key value (transparent value) */
#define S5P_W4KEYCON0		S5P_LCDREG(0x0158)	/* Color key control */
#define S5P_W4KEYCON1		S5P_LCDREG(0x015C)	/* Color key value (transparent value) */

#define S5P_W1KEYALPHA		S5P_LCDREG(0x0160)	/* Color key alpha value */
#define S5P_W2KEYALPHA		S5P_LCDREG(0x0164)	/* Color key alpha value */
#define S5P_W3KEYALPHA		S5P_LCDREG(0x0168)	/* Color key alpha value */
#define S5P_W4KEYALPHA		S5P_LCDREG(0x016C)	/* Color key alpha value */

#define S5P_DITHMODE		S5P_LCDREG(0x0170)	/* Dithering mode */

#define S5P_WIN0MAP		S5P_LCDREG(0x0180)	/* Window color control	*/
#define S5P_WIN1MAP		S5P_LCDREG(0x0184)	/* Window color control	*/
#define S5P_WIN2MAP		S5P_LCDREG(0x0188)	/* Window color control	*/
#define S5P_WIN3MAP		S5P_LCDREG(0x018C)	/* Window color control	*/
#define S5P_WIN4MAP		S5P_LCDREG(0x0190)	/* Window color control	*/

#define S5P_WPALCON_H		S5P_LCDREG(0x019C)	/* Window Palette control */
#define S5P_WPALCON_L		S5P_LCDREG(0x01A0)	/* Window Palette control */

#define S5P_VIDW0ALPHA0		S5P_LCDREG(0x0200)	/* Window 0 alpha value 0 */
#define S5P_VIDW0ALPHA1		S5P_LCDREG(0x0204)	/* Window 0 alpha value 1 */
#define S5P_VIDW1ALPHA0		S5P_LCDREG(0x0208)	/* Window 1 alpha value 0 */
#define S5P_VIDW1ALPHA1		S5P_LCDREG(0x020C)	/* Window 1 alpha value 1 */
#define S5P_VIDW2ALPHA0		S5P_LCDREG(0x0210)	/* Window 2 alpha value 0 */
#define S5P_VIDW2ALPHA1		S5P_LCDREG(0x0214)	/* Window 2 alpha value 1 */
#define S5P_VIDW3ALPHA0		S5P_LCDREG(0x0218)	/* Window 3 alpha value 0 */
#define S5P_VIDW3ALPHA1		S5P_LCDREG(0x021C)	/* Window 3 alpha value 1 */
#define S5P_VIDW4ALPHA0		S5P_LCDREG(0x0220)	/* Window 4 alpha value 0 */
#define S5P_VIDW4ALPHA1		S5P_LCDREG(0x0224)	/* Window 4 alpha value 1 */

#define S5P_BLENDEQ1		S5P_LCDREG(0x0244)	/* Window 1 blending equation control */
#define S5P_BLENDEQ2		S5P_LCDREG(0x0248)	/* Window 2 blending equation control */
#define S5P_BLENDEQ3		S5P_LCDREG(0x024C)	/* Window 3 blending equation control */
#define S5P_BLENDEQ4		S5P_LCDREG(0x0250)	/* Window 4 blending equation control */
#define S5P_BLENDCON		S5P_LCDREG(0x0260)	/* Blending control */
#define S5P_DUALRGB			S5P_LCDREG(0x027C)	/* DUALRGB Interface Setting Register */

/*
 * Bit Definitions
*/

/* VIDCON0 */
#define S5P_VIDCON0_DSI_DISABLE			(0 << 30)
#define S5P_VIDCON0_DSI_ENABLE			(1 << 30)
#define S5P_VIDCON0_SCAN_PROGRESSIVE		(0 << 29)
#define S5P_VIDCON0_SCAN_INTERLACE		(1 << 29)
#define S5P_VIDCON0_SCAN_MASK			(1 << 29)
#define S5P_VIDCON0_VIDOUT_RGB			(0 << 26)
#define S5P_VIDCON0_VIDOUT_ITU			(1 << 26)
#define S5P_VIDCON0_VIDOUT_I80LDI0		(2 << 26)
#define S5P_VIDCON0_VIDOUT_I80LDI1		(3 << 26)
#define S5P_VIDCON0_VIDOUT_MASK			(3 << 26)
#define S5P_VIDCON0_PNRMODE_RGB_P		(0 << 17)
#define S5P_VIDCON0_PNRMODE_BGR_P		(1 << 17)
#define S5P_VIDCON0_PNRMODE_RGB_S		(2 << 17)
#define S5P_VIDCON0_PNRMODE_BGR_S		(3 << 17)
#define S5P_VIDCON0_PNRMODE_MASK		(3 << 17)
#define S5P_VIDCON0_PNRMODE_SHIFT		(17)
#define S5P_VIDCON0_CLKVALUP_ALWAYS		(0 << 16)
#define S5P_VIDCON0_CLKVALUP_START_FRAME	(1 << 16)
#define S5P_VIDCON0_CLKVALUP_MASK		(1 << 16)
#define S5P_VIDCON0_CLKVAL_F(x)			(((x) & 0xff) << 6)
#define S5P_VIDCON0_VCLKEN_NORMAL		(0 << 5)
#define S5P_VIDCON0_VCLKEN_FREERUN		(1 << 5)
#define S5P_VIDCON0_VCLKEN_MASK			(1 << 5)
#define S5P_VIDCON0_CLKDIR_DIRECTED		(0 << 4)
#define S5P_VIDCON0_CLKDIR_DIVIDED		(1 << 4)
#define S5P_VIDCON0_CLKDIR_MASK			(1 << 4)
#define S5P_VIDCON0_CLKSEL_HCLK			(0 << 2)
#define S5P_VIDCON0_CLKSEL_SCLK			(1 << 2)
#define S5P_VIDCON0_CLKSEL_MASK			(1 << 2)
#define S5P_VIDCON0_ENVID_ENABLE		(1 << 1)
#define S5P_VIDCON0_ENVID_DISABLE		(0 << 1)
#define S5P_VIDCON0_ENVID_F_ENABLE		(1 << 0)
#define S5P_VIDCON0_ENVID_F_DISABLE		(0 << 0)

/* VIDCON1 */
#define S5P_VIDCON1_IVCLK_FALLING_EDGE		(0 << 7)
#define S5P_VIDCON1_IVCLK_RISING_EDGE		(1 << 7)
#define S5P_VIDCON1_IHSYNC_NORMAL		(0 << 6)
#define S5P_VIDCON1_IHSYNC_INVERT		(1 << 6)
#define S5P_VIDCON1_IVSYNC_NORMAL		(0 << 5)
#define S5P_VIDCON1_IVSYNC_INVERT		(1 << 5)
#define S5P_VIDCON1_IVDEN_NORMAL		(0 << 4)
#define S5P_VIDCON1_IVDEN_INVERT		(1 << 4)

/* VIDCON2 */
#define S5P_VIDCON2_EN601_DISABLE		(0 << 23)
#define S5P_VIDCON2_EN601_ENABLE		(1 << 23)
#define S5P_VIDCON2_EN601_MASK			(1 << 23)
#define S5P_VIDCON2_ORGYUV_YCBCR		(0 << 8)
#define S5P_VIDCON2_ORGYUV_CBCRY		(1 << 8)
#define S5P_VIDCON2_ORGYUV_MASK			(1 << 8)
#define S5P_VIDCON2_YUVORD_CBCR			(0 << 7)
#define S5P_VIDCON2_YUVORD_CRCB			(1 << 7)
#define S5P_VIDCON2_YUVORD_MASK			(1 << 7)

/* PRTCON */
#define S5P_PRTCON_UPDATABLE			(0 << 11)
#define S5P_PRTCON_PROTECT			(1 << 11)

/* VIDTCON0 */
#define S5P_VIDTCON0_VBPDE(x)			(((x) & 0xff) << 24)
#define S5P_VIDTCON0_VBPD(x)			(((x) & 0xff) << 16)
#define S5P_VIDTCON0_VFPD(x)			(((x) & 0xff) << 8)
#define S5P_VIDTCON0_VSPW(x)			(((x) & 0xff) << 0)

/* VIDTCON1 */
#define S5P_VIDTCON1_VFPDE(x)			(((x) & 0xff) << 24)
#define S5P_VIDTCON1_HBPD(x)			(((x) & 0xff) << 16)
#define S5P_VIDTCON1_HFPD(x)			(((x) & 0xff) << 8)
#define S5P_VIDTCON1_HSPW(x)			(((x) & 0xff) << 0)

/* VIDTCON2 */
#define S5P_VIDTCON2_LINEVAL(x)			(((x) & 0x7ff) << 11)
#define S5P_VIDTCON2_HOZVAL(x)			(((x) & 0x7ff) << 0)

/* Window 0~4 Control - WINCONx */
#define S5P_WINCON_DATAPATH_DMA			(0 << 22)
#define S5P_WINCON_DATAPATH_LOCAL		(1 << 22)
#define S5P_WINCON_DATAPATH_MASK		(1 << 22)
#define S5P_WINCON_BUFSEL_0			(0 << 20)
#define S5P_WINCON_BUFSEL_1			(1 << 20)
#define S5P_WINCON_BUFSEL_MASK			(1 << 20)
#define S5P_WINCON_BUFSEL_SHIFT			(20)
#define S5P_WINCON_BUFAUTO_DISABLE		(0 << 19)
#define S5P_WINCON_BUFAUTO_ENABLE		(1 << 19)
#define S5P_WINCON_BUFAUTO_MASK			(1 << 19)
#define S5P_WINCON_BITSWP_DISABLE		(0 << 18)
#define S5P_WINCON_BITSWP_ENABLE		(1 << 18)
#define S5P_WINCON_BITSWP_SHIFT			(18)
#define S5P_WINCON_BYTESWP_DISABLE		(0 << 17)
#define S5P_WINCON_BYTESWP_ENABLE		(1 << 17)
#define S5P_WINCON_BYTESWP_SHIFT		(17)
#define S5P_WINCON_HAWSWP_DISABLE		(0 << 16)
#define S5P_WINCON_HAWSWP_ENABLE		(1 << 16)
#define S5P_WINCON_HAWSWP_SHIFT			(16)
#define S5P_WINCON_WSWP_DISABLE			(0 << 15)
#define S5P_WINCON_WSWP_ENABLE			(1 << 15)
#define S5P_WINCON_WSWP_SHIFT			(15)
#define S5P_WINCON_INRGB_RGB			(0 << 13)
#define S5P_WINCON_INRGB_YUV			(1 << 13)
#define S5P_WINCON_INRGB_MASK			(1 << 13)
#define S5P_WINCON_BURSTLEN_16WORD		(0 << 9)
#define S5P_WINCON_BURSTLEN_8WORD		(1 << 9)
#define S5P_WINCON_BURSTLEN_4WORD		(2 << 9)
#define S5P_WINCON_BURSTLEN_MASK		(3 << 9)
#define S5P_WINCON_ALPHA_MULTI_DISABLE		(0 << 7)
#define S5P_WINCON_ALPHA_MULTI_ENABLE		(1 << 7)
#define S5P_WINCON_BLD_PLANE			(0 << 6)
#define S5P_WINCON_BLD_PIXEL			(1 << 6)
#define S5P_WINCON_BLD_MASK			(1 << 6)
#define S5P_WINCON_BPPMODE_1BPP			(0 << 2)
#define S5P_WINCON_BPPMODE_2BPP			(1 << 2)
#define S5P_WINCON_BPPMODE_4BPP			(2 << 2)
#define S5P_WINCON_BPPMODE_8BPP_PAL		(3 << 2)
#define S5P_WINCON_BPPMODE_8BPP			(4 << 2)
#define S5P_WINCON_BPPMODE_16BPP_565		(5 << 2)
#define S5P_WINCON_BPPMODE_16BPP_A555		(6 << 2)
#define S5P_WINCON_BPPMODE_18BPP_666		(8 << 2)
#define S5P_WINCON_BPPMODE_18BPP_A665		(9 << 2)
#define S5P_WINCON_BPPMODE_24BPP_888		(0xb << 2)
#define S5P_WINCON_BPPMODE_24BPP_A887		(0xc << 2)
#define S5P_WINCON_BPPMODE_32BPP		(0xd << 2)
#define S5P_WINCON_BPPMODE_16BPP_A444		(0xe << 2)
#define S5P_WINCON_BPPMODE_15BPP_555		(0xf << 2)
#define S5P_WINCON_BPPMODE_MASK			(0xf << 2)
#define S5P_WINCON_BPPMODE_SHIFT		(2)
#define S5P_WINCON_ALPHA0_SEL			(0 << 1)
#define S5P_WINCON_ALPHA1_SEL			(1 << 1)
#define S5P_WINCON_ALPHA_SEL_MASK		(1 << 1)
#define S5P_WINCON_ENWIN_DISABLE		(0 << 0)
#define S5P_WINCON_ENWIN_ENABLE			(1 << 0)

/* WINSHMAP */
#define S5P_WINSHMAP_CH_ENABLE(x)		(1 << x)
#define S5P_WINSHMAP_CH_DISABLE(x)		(0 << x)
#define S5P_WINSHMAP_CH_LOCAL_ENABLE(x)		(1 << (x + 5))
#define S5P_WINSHMAP_CH_LOCAL_DISABLE(x)	(0 << (x + 5))

/* WINCON1 special */
#define S5P_WINCON1_VP_DISABLE			(0 << 24)
#define S5P_WINCON1_VP_ENABLE			(1 << 24)
#define S5P_WINCON1_LOCALSEL_FIMC1		(0 << 23)
#define S5P_WINCON1_LOCALSEL_VP			(1 << 23)
#define S5P_WINCON1_LOCALSEL_MASK		(1 << 23)

/* VIDOSDxA, VIDOSDxB */
#define S5P_VIDOSD_LEFT_X(x)			(((x) & 0x7ff) << 11)
#define S5P_VIDOSD_TOP_Y(x)			(((x) & 0x7ff) << 0)
#define S5P_VIDOSD_RIGHT_X(x)			(((x) & 0x7ff) << 11)
#define S5P_VIDOSD_BOTTOM_Y(x)			(((x) & 0x7ff) << 0)

/* VIDOSD0C, VIDOSDxD */
#define S5P_VIDOSD_SIZE(x)			(((x) & 0xffffff) << 0)

/* VIDOSDxC (1~4) */
#define S5P_VIDOSD_ALPHA0_R(x)			(((x) & 0xf) << 20)
#define S5P_VIDOSD_ALPHA0_G(x)			(((x) & 0xf) << 16)
#define S5P_VIDOSD_ALPHA0_B(x)			(((x) & 0xf) << 12)
#define S5P_VIDOSD_ALPHA1_R(x)			(((x) & 0xf) << 8)
#define S5P_VIDOSD_ALPHA1_G(x)			(((x) & 0xf) << 4)
#define S5P_VIDOSD_ALPHA1_B(x)			(((x) & 0xf) << 0)
#define S5P_VIDOSD_ALPHA0_SHIFT			(12)
#define S5P_VIDOSD_ALPHA1_SHIFT			(0)

/* Start Address */
#define S5P_VIDADDR_START_VBANK(x)		(((x) & 0xff) << 24)
#define S5P_VIDADDR_START_VBASEU(x)		(((x) & 0xffffff) << 0)

/* End Address */
#define S5P_VIDADDR_END_VBASEL(x)		(((x) & 0xffffff) << 0)

/* Buffer Size */
#define S5P_VIDADDR_OFFSIZE(x)			(((x) & 0x1fff) << 13)
#define S5P_VIDADDR_PAGEWIDTH(x)		(((x) & 0x1fff) << 0)

/* VIDINTCON0 */
#define S5P_VIDINTCON0_SYSMAINCON_DISABLE	(0 << 19)
#define S5P_VIDINTCON0_SYSMAINCON_ENABLE	(1 << 19)
#define S5P_VIDINTCON0_SYSSUBCON_DISABLE	(0 << 18)
#define S5P_VIDINTCON0_SYSSUBCON_ENABLE		(1 << 18)
#define S5P_VIDINTCON0_SYSIFDONE_DISABLE	(0 << 17)
#define S5P_VIDINTCON0_SYSIFDONE_ENABLE		(1 << 17)
#define S5P_VIDINTCON0_FRAMESEL0_BACK		(0 << 15)
#define S5P_VIDINTCON0_FRAMESEL0_VSYNC		(1 << 15)
#define S5P_VIDINTCON0_FRAMESEL0_ACTIVE		(2 << 15)
#define S5P_VIDINTCON0_FRAMESEL0_FRONT		(3 << 15)
#define S5P_VIDINTCON0_FRAMESEL0_MASK		(3 << 15)
#define S5P_VIDINTCON0_FRAMESEL1_NONE		(0 << 13)
#define S5P_VIDINTCON0_FRAMESEL1_BACK		(1 << 13)
#define S5P_VIDINTCON0_FRAMESEL1_VSYNC		(2 << 13)
#define S5P_VIDINTCON0_FRAMESEL1_FRONT		(3 << 13)
#define S5P_VIDINTCON0_INTFRMEN_DISABLE		(0 << 12)
#define S5P_VIDINTCON0_INTFRMEN_ENABLE		(1 << 12)
#define S5P_VIDINTCON0_FIFOSEL_WIN4		(1 << 11)
#define S5P_VIDINTCON0_FIFOSEL_WIN3		(1 << 10)
#define S5P_VIDINTCON0_FIFOSEL_WIN2		(1 << 9)
#define S5P_VIDINTCON0_FIFOSEL_WIN1		(1 << 6)
#define S5P_VIDINTCON0_FIFOSEL_WIN0		(1 << 5)
#define S5P_VIDINTCON0_FIFOSEL_ALL		(0x73 << 5)
#define S5P_VIDINTCON0_FIFOSEL_MASK		(0x73 << 5)
#define S5P_VIDINTCON0_FIFOLEVEL_25		(0 << 2)
#define S5P_VIDINTCON0_FIFOLEVEL_50		(1 << 2)
#define S5P_VIDINTCON0_FIFOLEVEL_75		(2 << 2)
#define S5P_VIDINTCON0_FIFOLEVEL_EMPTY		(3 << 2)
#define S5P_VIDINTCON0_FIFOLEVEL_FULL		(4 << 2)
#define S5P_VIDINTCON0_FIFOLEVEL_MASK		(7 << 2)
#define S5P_VIDINTCON0_INTFIFO_DISABLE		(0 << 1)
#define S5P_VIDINTCON0_INTFIFO_ENABLE		(1 << 1)
#define S5P_VIDINTCON0_INT_DISABLE		(0 << 0)
#define S5P_VIDINTCON0_INT_ENABLE		(1 << 0)
#define S5P_VIDINTCON0_INT_MASK			(1 << 0)

/* VIDINTCON1 */
#define S5P_VIDINTCON1_INTVPPEND		(1 << 5)
#define S5P_VIDINTCON1_INTI80PEND		(1 << 2)
#define S5P_VIDINTCON1_INTFRMPEND		(1 << 1)
#define S5P_VIDINTCON1_INTFIFOPEND		(1 << 0)

/* WxKEYCON0 (1~4) */
#define S5P_KEYCON0_KEYBLEN_DISABLE		(0 << 26)
#define S5P_KEYCON0_KEYBLEN_ENABLE		(1 << 26)
#define S5P_KEYCON0_KEY_DISABLE		(0 << 25)
#define S5P_KEYCON0_KEY_ENABLE			(1 << 25)
#define S5P_KEYCON0_DIRCON_MATCH_FG		(0 << 24)
#define S5P_KEYCON0_DIRCON_MATCH_BG		(1 << 24)
#define S5P_KEYCON0_COMPKEY(x)			(((x) & 0xffffff) << 0)

/* WxKEYCON1 (1~4) */
#define S5P_KEYCON1_COLVAL(x)			(((x) & 0xffffff) << 0)

/* DUALRGB */
#define S5P_DUALRGB_BYPASS_SINGLE		(0x00 << 0)
#define S5P_DUALRGB_BYPASS_DUAL			(0x01 << 0)
#define S5P_DUALRGB_MIE_DUAL			(0x10 << 0)
#define S5P_DUALRGB_MIE_SINGLE			(0x11 << 0)
#define S5P_DUALRGB_LINESPLIT			(0x0 << 2)
#define S5P_DUALRGB_FRAMESPLIT			(0x1 << 2)
#define S5P_DUALRGB_SUB_CNT(x)			((x & 0xfff) << 4)
#define S5P_DUALRGB_VDEN_EN_DISABLE		(0x0 << 16)
#define S5P_DUALRGB_VDEN_EN_ENABLE		(0x1 << 16)
#define S5P_DUALRGB_MAIN_CNT(x)			((x & 0xfff) << 18)

#endif /* _REGS_FB_H */
