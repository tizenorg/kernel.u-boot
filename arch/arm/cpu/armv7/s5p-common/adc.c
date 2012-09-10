/*
 *  Copyright (C) 2011 Samsung Electronics
 *  MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/adc.h>

unsigned short get_adc_value(int channel)
{
	struct s5p_adc *adc = (struct s5p_adc *)samsung_get_base_adc();
	unsigned short ret = 0;
	unsigned int reg;
	unsigned int loop = 0;

	writel(channel & 0xF, &adc->adcmux);
	writel((1 << 14) | (49 << 6), &adc->adccon);
	writel(1000 & 0xffff, &adc->adcdly);
	writel(readl(&adc->adccon) | (1 << 16), &adc->adccon); /* 12 bit */
	udelay(10);
	writel(readl(&adc->adccon) | (1 << 0), &adc->adccon); /* Enable */
	udelay(10);

	do {
		udelay(1);
		reg = readl(&adc->adccon);
	} while (!(reg & (1 << 15)) && (loop++ < 1000));

	ret = readl(&adc->adcdat0) & 0xFFF;

	return ret;
}
