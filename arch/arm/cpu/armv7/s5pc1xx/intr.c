/*
 * Copyright (C) 2009 Samsung Electronics
 * Hakgoo Lee <goodguy.lee@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/intr.h>
#include <asm/io.h>

void s5p_gpio_intr_config(u32 num)
{
	volatile u32 val;
	
	if (num < 8) {
		val = readl(GPH0_CON);
		val |= (0xF<<(num<<2));
		writel(val, GPH0_CON);
	} else if (num < 16) {
		val = readl(GPH1_CON);
		val |= (0xF<<((num-8)<<2));
		writel(val, GPH1_CON);
	} else if (num < 24) {
		val = readl(GPH2_CON);
		val |= (0xF<<((num-16)<<2));
		writel(val, GPH2_CON);
	} else if (num <32) {
		val = readl(GPH3_CON);
		val |= (0xF<<((num-24)<<2));
		writel(val, GPH3_CON);
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_ext_intr_config(u32 num, u8 mode)
{
	volatile u32 val;
	
	if (num < 8) {
		val = readl(EXT_INT0_CON);
		val |= (mode<<(num<<2));
		writel(val, EXT_INT0_CON);
	} else if (num < 16) {
		val = readl(EXT_INT1_CON);
		val |= (mode<<((num-8)<<2));
		writel(val, EXT_INT1_CON);
	} else if (num < 24) {
		val = readl(EXT_INT2_CON);
		val |= (mode<<((num-16)<<2));
		writel(val, EXT_INT2_CON);
	} else if (num <32) {
		val = readl(EXT_INT3_CON);
		val |= (mode<<((num-24)<<2));
		writel(val, EXT_INT3_CON);
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_ext_intr_enable(u32 num)
{
	volatile u32 val;
	
	if (num < 8) {
		val = readl(EXT_INT0_MASK);
		val &= ~(1<<(num));
		writel(val, EXT_INT0_MASK);
	} else if (num < 16) {
		val = readl(EXT_INT1_MASK);
		val &= ~(1<<(num-8));
		writel(val, EXT_INT1_MASK);
	} else if (num < 24) {
		val = readl(EXT_INT2_MASK);
		val &= ~(1<<(num-16));
		writel(val, EXT_INT2_MASK);
	} else if (num < 32) {
		val = readl(EXT_INT3_MASK);
		val &= ~(1<<(num-24));
		writel(val, EXT_INT3_MASK);
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_ext_intr_disable(u32 num)
{
	volatile u32 val;
	
	if (num < 8) {
		val = readl(EXT_INT0_MASK);
		val |= (1<<(num));
		writel(val, EXT_INT0_MASK);
	} else if (num < 16) {
		val = readl(EXT_INT1_MASK);
		val |= (1<<(num-8));
		writel(val, EXT_INT1_MASK);
	} else if (num < 24) {
		val = readl(EXT_INT2_MASK);
		val |= (1<<(num-16));
		writel(val, EXT_INT2_MASK);
	} else if (num < 32) {
		val = readl(EXT_INT3_MASK);
		val |= (1<<(num-24));
		writel(val, EXT_INT3_MASK);
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_ext_intr_clear(u32 num)
{
	volatile u32 val;
	if (num < 8) {
		val = (1<<(num));
		writel(val, EXT_INT0_PEND);
	} else if (num < 16) {
		val = (1<<(num-8));
		writel(val, EXT_INT1_PEND);
	} else if (num < 24) {
		val = (1<<(num-16));
		writel(val, EXT_INT2_PEND);
	} else if (num < 32) {
		val = (1<<(num-24));
		writel(val, EXT_INT3_PEND);
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_vic_clear_vect_addr(void)
{
	/* VECTORADDR */
	writel(0x0, VIC0_ADDR);
	writel(0x0, VIC1_ADDR);
	writel(0x0, VIC2_ADDR);
	writel(0x0, VIC3_ADDR);
}

static u32 s5p_vic_get_irq_sta(u32 val)
{
	u32 shift = 0;
	u32 comp;

	do {
		comp = 0x1 << shift;
		if (val==comp) {
			break;
		}
		shift++;
	} while (shift < 32);
	return  shift;
}


u32 s5p_vic_get_irq_num(void)
{
	volatile u32 val;

	val = readl(VIC0_IRQ_STATUS);
	if (val) {
		return s5p_vic_get_irq_sta(val);
	}

	val = readl(VIC1_IRQ_STATUS);
	if (val) {
		return (s5p_vic_get_irq_sta(val) + 32) ;
	}

	val = readl(VIC2_IRQ_STATUS);
	if (val) {
		return (s5p_vic_get_irq_sta(val) + 64);
	}

	val = readl(VIC3_IRQ_STATUS);
	if (val) {
		return (s5p_vic_get_irq_sta(val) + 96);
	}
}

void s5p_vic_set_vect_addr(u32 num, void *handler)
{
	if (num < 32) {
		writel(handler, (VIC0_VECT_ADDR + 4 * num));
	} else if (num < 64) {
		writel(handler, (VIC1_VECT_ADDR + 4 * (num - 32)));
	} else if (num < 96) {
		writel(handler, (VIC2_VECT_ADDR + 4 * (num - 64)));
	} else if (num <128) {
		writel(handler, (VIC3_VECT_ADDR + 4 * (num - 96)));
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_vic_intr_enable(u32 num)
{
	volatile u32 val;
	
	if (num < 32) {
		val = readl(VIC0_INT_ENABLE);
		val |= (0x1<<num);
		writel(val, VIC0_INT_ENABLE);
	} else if (num < 64) {
		val = readl(VIC1_INT_ENABLE);
		val |= (0x1<<(num - 32));
		writel(val, VIC1_INT_ENABLE);
	} else if (num < 96) {
		val = readl(VIC2_INT_ENABLE);
		val |= (0x1<<(num - 64));
		writel(val, VIC2_INT_ENABLE);
	} else if (num <128) {
		val = readl(VIC3_INT_ENABLE);
		val |= (0x1<<(num - 96));
		writel(val, VIC3_INT_ENABLE);
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_vic_intr_disable(u32 num)
{
	volatile u32 val;
	
	if (num < 32) {
		val = readl(VIC0_INT_ENCLEAR);
		val |= (0x1<<num);
		writel(val, VIC0_INT_ENCLEAR);
	} else if (num < 64) {
		val = readl(VIC1_INT_ENCLEAR);
		val |= (0x1<<(num - 32));
		writel(val, VIC1_INT_ENCLEAR);
	} else if (num < 96) {
		val = readl(VIC2_INT_ENCLEAR);
		val |= (0x1<<(num - 64));
		writel(val, VIC2_INT_ENCLEAR);
	} else if (num <128) {
		val = readl(VIC3_INT_ENCLEAR);
		val |= (0x1<<(num - 96));
		writel(val, VIC3_INT_ENCLEAR);
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_vic_soft_intr_clear(u32 num)
{
	volatile u32 val;
	
	if (num < 32) {
		val |= (0x1<<num);
		writel(val, VIC0_SOFT_INTCLEAR);
	} else if (num < 64) {
		val |= (0x1<<(num - 32));
		writel(val, VIC1_SOFT_INTCLEAR);
	} else if (num < 96) {
		val |= (0x1<<(num - 64));
		writel(val, VIC2_SOFT_INTCLEAR);
	} else if (num <128) {
		val |= (0x1<<(num - 96));
		writel(val, VIC3_SOFT_INTCLEAR);
	} else {
		printf("s5p_intr: exceed intr num (%d)\n", num);
	}
}

void s5p_intr_init(void)
{
	/* INTENCLEAR */
	writel(0xFFFFFFFF, VIC0_INT_ENCLEAR);
	writel(0xFFFFFFFF, VIC1_INT_ENCLEAR);
	writel(0xFFFFFFFF, VIC2_INT_ENCLEAR);
	writel(0xFFFFFFFF, VIC3_INT_ENCLEAR);

	/* INTSELECT */
	writel(0x0, VIC0_INT_SELECT);
	writel(0x0, VIC1_INT_SELECT);
	writel(0x0, VIC2_INT_SELECT);
	writel(0x0, VIC3_INT_SELECT);

	s5p_vic_clear_vect_addr();
}
