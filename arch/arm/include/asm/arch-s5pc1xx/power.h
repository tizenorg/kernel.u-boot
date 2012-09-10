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
#define S5PC100_OTHERS			0xE0108200
#define S5PC100_RST_STAT		0xE0108300
#define S5PC100_SLEEP_WAKEUP		(1 << 3)
#define S5PC100_WAKEUP_STAT		0xE0108304
#define S5PC100_INFORM0			0xE0108400

#define S5PC110_RST_STAT		0xE010A000
#define S5PC110_SLEEP_WAKEUP		(1 << 16)
#define S5PC110_DEEPSTOP		(1 << 18)
#define S5PC110_DEEPIDLE_WAKEUP		(1 << 19)
#define S5PC110_RST_STAT_WAKEUP_MODE_MASK	0x000D0000
#define S5PC110_OSC_CON			0xE0108000
#define S5PC110_PWR_CFG			0xE010C000
#define S5PC110_CFG_STANDBYWFI_MASK	(0x3 << 8)
#define S5PC110_CFG_STANDBYWFI_IGNORE	(0x0 << 8)
#define S5PC110_CFG_STANDBYWFI_IDLE	(0x1 << 8)
#define S5PC110_CFG_STANDBYWFI_STOP	(0x2 << 8)
#define S5PC110_CFG_STANDBYWFI_SLEEP	(0x3 << 8)
#define S5PC110_EINT_WAKEUP_MASK	0xE010C004
#define S5PC110_WAKEUP_MASK		0xE010C008
#define S5PC110_PWR_MODE		0xE010C00C
#define S5PC110_PWR_MODE_SLEEP		(1 << 2)
#define S5PC110_NORMAL_CFG		0xE010C010
#define S5PC110_IDLE_CFG		0xE010C020
#define S5PC110_STOP_CFG		0xE010C030
#define S5PC110_STOP_MEM_CFG		0xE010C034
#define S5PC110_SLEEP_CFG		0xE010C040
#define S5PC110_OSC_FREQ		0xE010C100
#define S5PC110_OSC_STABLE		0xE010C104
#define S5PC110_PWR_STABLE		0xE010C108
#define S5PC110_MTC_STABLE		0xE010C110
#define S5PC110_CLAMP_STABLE		0xE010C114
#define S5PC110_WAKEUP_STAT		0xE010C200
#define S5PC110_OTHERS			0xE010E000
#define S5PC110_OTHERS_SYSCON_INT_DISABLE	(1 << 0)
#define S5PC110_MIE_CONTROL		0xE010E800
#define S5PC110_HDMI_CONTROL		0xE010E804
#define S5PC110_USB_PHY_CON		0xE010E80C
#define S5PC110_DAC_CONTROL		0xE010E810
#define S5PC110_MIPI_DPHY_CONTROL	0xE010E814
#define S5PC110_ADC_CONTROL		0xE010E818
#define S5PC110_PS_HOLD_CONTROL		0xE010E81C
#define S5PC110_PS_HOLD_DIR_OUTPUT	(1 << 9)
#define S5PC110_PS_HOLD_DIR_INPUT	(0 << 9)
#define S5PC110_PS_HOLD_DATA_HIGH	(1 << 8)
#define S5PC110_PS_HOLD_DATA_LOW	(0 << 8)
#define S5PC110_PS_HOLD_OUT_EN		(1 << 0)
#define S5PC110_INFORM0			0xE010F000
#define S5PC110_INFORM1			0xE010F004
#define S5PC110_INFORM2			0xE010F008
#define S5PC110_INFORM3			0xE010F00C
#define S5PC110_INFORM4			0xE010F010
#define S5PC110_INFORM5			0xE010F014
#define S5PC110_INFORM6			0xE010F018
#define S5PC110_INFORM7			0xE010F01C

#define S5P_RST_STAT			S5PC110_RST_STAT

#ifndef __ASSEMBLY__

/* RESET */

enum reset_status {
	EXTRESET,
	WARMRESET,
	WDTRESET,
	SWRESET,
};

#define RESET_SW		(0x1 << 3)
#define RESET_WDT		(0x1 << 2)
#define RESET_WARM		(0x1 << 1)
#define RESET_PIN		(0x1 << 0)

static inline unsigned int get_reset_status(void)
{
	unsigned int val;

	val = readl(S5PC110_RST_STAT);

	if (val & RESET_SW)
		return SWRESET;
	else if (val & RESET_WARM)
		return WARMRESET;
	else if (val & RESET_WDT)
		return WDTRESET;
	else if (val & RESET_PIN)
		return EXTRESET;

	return val;
}
#endif

#endif
