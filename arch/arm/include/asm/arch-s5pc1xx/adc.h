/*                                      
 * (C) Copyright 2009                   
 * Samsung Electronics, <www.samsung.com/sec>
 * MyungJoo Ham <myungjoo.ham@samsung.com>
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

#ifndef __ASM_ARM_ARCH_ADC_H_
#define __ASM_ARM_ARCH_ADC_H_

#ifndef __ASSEMBLY__
struct s5p_adc {
	unsigned int adccon; /* starts at 0xE1700000 */
	unsigned int adctsc;
	unsigned int adcdly;
	unsigned int adcdat0;
	unsigned int adcdat1; /* 0xE1700010 */
	unsigned int adcupdn;
	unsigned int adcclrint;
	unsigned int adcmux;
	unsigned int adcclrintpndnup;
};

unsigned short get_adc_value(int channel);
#endif

#endif /* __ASM_ARM_ARCH_ADC_H_ */
