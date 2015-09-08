/*
 * (C) Copyright 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#ifndef __ASM_ARCH_USB_HS_OTG_H_
#define __ASM_ARCH_USB_HS_OTG_H_

/* Core Global Registers */
#define OTG_GOTGCTL		0x000
#define OTG_GOTGINT		0x004
#define OTG_GAHBCFG		0x008
#define OTG_GUSBCFG		0x00C
#define OTG_GRSTCTL		0x010
#define OTG_GINTSTS		0x014
#define OTG_GINTMSK		0x018
#define OTG_GRXSTSR		0x01C
#define OTG_GRXSTSP		0x020
#define OTG_GRXFSIZ		0x024
#define OTG_GNPTXFSIZ		0x028
#define OTG_GNPTXSTS		0x02C

#define OTG_HPTXFSIZ		0x100
#define OTG_DPTXFSIZ1		0x104
#define OTG_DPTXFSIZ2		0x108
#define OTG_DPTXFSIZ3		0x10C
#define OTG_DPTXFSIZ4		0x110
#define OTG_DPTXFSIZ5		0x114
#define OTG_DPTXFSIZ6		0x118
#define OTG_DPTXFSIZ7		0x11C
#define OTG_DPTXFSIZ8		0x120
#define OTG_DPTXFSIZ9		0x124
#define OTG_DPTXFSIZ10		0x128
#define OTG_DPTXFSIZ11		0x12C
#define OTG_DPTXFSIZ12		0x130
#define OTG_DPTXFSIZ13		0x134
#define OTG_DPTXFSIZ14		0x138
#define OTG_DPTXFSIZ15		0x13C

/* Host Global Registers */
#define OTG_HCFG		0x400
#define OTG_HFIR		0x404
#define OTG_HFNUM		0x408
#define OTG_HPTXSTS		0x410
#define OTG_HAINT		0x414
#define OTG_HAINTMSK		0x418

/* Host Port Control & Status Registers */
#define OTG_HPRT		0x440

/* Host Channel-Specific Registers */
#define OTG_HCCHAR0		0x500
#define OTG_HCSPLT0		0x504
#define OTG_HCINT0		0x508
#define OTG_HCINTMSK0		0x50C
#define OTG_HCTSIZ0		0x510
#define OTG_HCDMA0		0x514

/* Device Global Registers */
#define OTG_DCFG		0x800
#define OTG_DCTL		0x804
#define OTG_DSTS		0x808
#define OTG_DIEPMSK		0x810
#define OTG_DOEPMSK		0x814
#define OTG_DAINT		0x818
#define OTG_DAINTMSK		0x81C
#define OTG_DTKNQR1		0x820
#define OTG_DTKNQR2		0x824
#define OTG_DVBUSDIS		0x828
#define OTG_DVBUSPULSE		0x82C
#define OTG_DTKNQR3		0x830
#define OTG_DTKNQR4		0x834

/* Device Logical IN Endpoint-Specific Registers */
#define OTG_DIEPCTL0		0x900
#define OTG_DIEPINT0		0x908
#define OTG_DIEPTSIZ0		0x910
#define OTG_DIEPDMA0		0x914

/* Device Logical OUT Endpoint-Specific Registers */
#define OTG_DOEPCTL0		0xB00
#define OTG_DOEPINT0		0xB08
#define OTG_DOEPTSIZ0		0xB10
#define OTG_DOEPDMA0		0xB14

/* Power & clock gating registers */
#define OTG_PCGCCTRL		0xE00

/* Endpoint FIFO address */
#define OTG_EP0_FIFO		0x1000

/* OTG PHY CORE REGISTERS */
#define OTG_PHYPWR		0x0
#define OTG_PHYCTRL		0x4
#define OTG_RSTCON		0x8
#define OTG_PHYTUN0		0x20
#define OTG_PHYTUN1		0x24

/* register for power control on PMU */
static inline unsigned int s5p_get_usb_power_reg(void)
{
	return 0x10020704;
}

#endif
