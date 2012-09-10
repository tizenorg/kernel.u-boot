/*
 * Copyright (c) 2009 Samsung Electronics
 * Kyungmin Park <kyungmin.park@samsung.com>
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

#ifndef __ASM_ARM_ARCH_POWER_H_
#define __ASM_ARM_ARCH_POWER_H_

/*
 * Power control
 */
#define S5P6442_RST_STAT		0xE010A000
#define S5P6442_SLEEP_WAKEUP		(1 << 16)
#define S5P6442_OSC_CON			0xE0108000
#define S5P6442_PWR_CFG			0xE010C000
#define S5P6442_CFG_STANDBYWFI_MASK	(0x3 << 8)
#define S5P6442_CFG_STANDBYWFI_IGNORE	(0x0 << 8)
#define S5P6442_CFG_STANDBYWFI_IDLE	(0x1 << 8)
#define S5P6442_CFG_STANDBYWFI_STOP	(0x2 << 8)
#define S5P6442_CFG_STANDBYWFI_SLEEP	(0x3 << 8)
#define S5P6442_EINT_WAKEUP_MASK	0xE010C004
#define S5P6442_WAKEUP_MASK		0xE010C008
#define S5P6442_PWR_MODE		0xE010C00C
#define S5P6442_PWR_MODE_SLEEP		(1 << 2)
#define S5P6442_NORMAL_CFG		0xE010C010
#define S5P6442_IDLE_CFG		0xE010C020
#define S5P6442_STOP_CFG		0xE010C030
#define S5P6442_STOP_MEM_CFG		0xE010C034
#define S5P6442_SLEEP_CFG		0xE010C040
#define S5P6442_OSC_FREQ		0xE010C100
#define S5P6442_OSC_STABLE		0xE010C104
#define S5P6442_PWR_STABLE		0xE010C108
#define S5P6442_MTC_STABLE		0xE010C110
#define S5P6442_CLAMP_STABLE		0xE010C114
#define S5P6442_WAKEUP_STAT		0xE010C200
#define S5P6442_OTHERS			0xE010E000
#define S5P6442_OTHERS_SYSCON_INT_DISABLE	(1 << 0)
#define S5P6442_MIE_CONTROL		0xE010E800
#define S5P6442_HDMI_CONTROL		0xE010E804
#define S5P6442_USB_PHY_CON		0xE010E80C
#define S5P6442_DAC_CONTROL		0xE010E810
#define S5P6442_MIPI_DPHY_CONTROL	0xE010E814
#define S5P6442_ADC_CONTROL		0xE010E818
#define S5P6442_PS_HOLD_CONTROL		0xE010E81C
#define S5P6442_PS_HOLD_DIR_OUTPUT	(1 << 9)
#define S5P6442_PS_HOLD_DIR_INPUT	(0 << 9)
#define S5P6442_PS_HOLD_DATA_HIGH	(1 << 8)
#define S5P6442_PS_HOLD_DATA_LOW	(0 << 8)
#define S5P6442_PS_HOLD_OUT_EN		(1 << 0)
#define S5P6442_INFORM0			0xE010F000
#define S5P6442_INFORM1			0xE010F004
#define S5P6442_INFORM2			0xE010F008
#define S5P6442_INFORM3			0xE010F00C
#define S5P6442_INFORM4			0xE010F010
#define S5P6442_INFORM5			0xE010F014
#define S5P6442_INFORM6			0xE010F018
#define S5P6442_INFORM7			0xE010F01C

#endif
