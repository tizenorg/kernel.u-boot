/*
 * (C) Copyright 2009 Samsung Electronics
 * Hakgoo Lee <goodguy.lee@samsung.com>
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

#ifndef __ASM_ARCH_INTR_H_
#define __ASM_ARCH_INTR_H_

#define S5P_VIC0_BASE 0xF2000000
#define S5P_VIC1_BASE 0xF2100000
#define S5P_VIC2_BASE 0xF2200000
#define S5P_VIC3_BASE 0xF2300000

#define S5P_GPIO_BASE 0xE0200000

/* VIC0 */
#define VIC0_IRQ_STATUS		 (S5P_VIC0_BASE + 0x00)
#define VIC0_INT_SELECT		 (S5P_VIC0_BASE + 0x0C)
#define VIC0_INT_ENABLE		 (S5P_VIC0_BASE + 0x10)
#define VIC0_INT_ENCLEAR	 (S5P_VIC0_BASE + 0x14)
#define VIC0_SOFT_INTCLEAR	 (S5P_VIC0_BASE + 0x1C)
#define VIC0_VECT_ADDR		 (S5P_VIC0_BASE + 0x100)
#define VIC0_ADDR			 (S5P_VIC0_BASE + 0xF00)
/* VIC1 */
#define VIC1_IRQ_STATUS		 (S5P_VIC1_BASE + 0x00)
#define VIC1_INT_SELECT		 (S5P_VIC1_BASE + 0x0C)
#define VIC1_INT_ENABLE		 (S5P_VIC1_BASE + 0x10)
#define VIC1_INT_ENCLEAR	 (S5P_VIC1_BASE + 0x14)
#define VIC1_SOFT_INTCLEAR	 (S5P_VIC1_BASE + 0x1C)
#define VIC1_VECT_ADDR		 (S5P_VIC1_BASE + 0x100)
#define VIC1_ADDR			 (S5P_VIC1_BASE + 0xF00)
/* VIC2 */
#define VIC2_IRQ_STATUS		 (S5P_VIC2_BASE + 0x00)
#define VIC2_INT_SELECT		 (S5P_VIC2_BASE + 0x0C)
#define VIC2_INT_ENABLE		 (S5P_VIC2_BASE + 0x10)
#define VIC2_INT_ENCLEAR	 (S5P_VIC2_BASE + 0x14)
#define VIC2_SOFT_INTCLEAR	 (S5P_VIC2_BASE + 0x1C)
#define VIC2_VECT_ADDR		 (S5P_VIC2_BASE + 0x100)
#define VIC2_ADDR			 (S5P_VIC2_BASE + 0xF00)
/* VIC3 */
#define VIC3_IRQ_STATUS		 (S5P_VIC3_BASE + 0x00)
#define VIC3_INT_SELECT		 (S5P_VIC3_BASE + 0x0C)
#define VIC3_INT_ENABLE		 (S5P_VIC3_BASE + 0x10)
#define VIC3_INT_ENCLEAR	 (S5P_VIC3_BASE + 0x14)
#define VIC3_SOFT_INTCLEAR	 (S5P_VIC3_BASE + 0x1C)
#define VIC3_VECT_ADDR		 (S5P_VIC3_BASE + 0x100)
#define VIC3_ADDR			 (S5P_VIC3_BASE + 0xF00)

/* GPHCON */
#define GPH0_CON			 (S5P_GPIO_BASE + 0xC00)
#define GPH1_CON			 (S5P_GPIO_BASE + 0xC20)
#define GPH2_CON			 (S5P_GPIO_BASE + 0xC40)
#define GPH3_CON			 (S5P_GPIO_BASE + 0xC60)
/* EXT_INT_CON */
#define EXT_INT0_CON		 (S5P_GPIO_BASE + 0xE00)
#define EXT_INT1_CON		 (S5P_GPIO_BASE + 0xE04)
#define EXT_INT2_CON		 (S5P_GPIO_BASE + 0xE08)
#define EXT_INT3_CON		 (S5P_GPIO_BASE + 0xE0C)
/* EXT_INT_MASK */
#define EXT_INT0_MASK		 (S5P_GPIO_BASE + 0xF00)
#define EXT_INT1_MASK		 (S5P_GPIO_BASE + 0xF04)
#define EXT_INT2_MASK		 (S5P_GPIO_BASE + 0xF08)
#define EXT_INT3_MASK		 (S5P_GPIO_BASE + 0xF0C)
/* EXT_INT_PEND */
#define EXT_INT0_PEND		 (S5P_GPIO_BASE + 0xF40)
#define EXT_INT1_PEND		 (S5P_GPIO_BASE + 0xF44)
#define EXT_INT2_PEND		 (S5P_GPIO_BASE + 0xF48)
#define EXT_INT3_PEND		 (S5P_GPIO_BASE + 0xF4C)

#define EXT_INT_CON_LOW			0x0
#define EXT_INT_CON_HIGH		0x1
#define EXT_INT_CON_FALL		0x2
#define EXT_INT_CON_RISE 		0x3
#define EXT_INT_CON_BOTH 		0x4

#define N_IRQS			128

void s5p_gpio_intr_config(u32 num);
void s5p_ext_intr_config(u32 num, u8 mode);
void s5p_ext_intr_enable(u32 num);
void s5p_ext_intr_disable(u32 num);
void s5p_ext_intr_clear(u32 num);
void s5p_vic_clear_vect_addr(void);
u32 s5p_vic_get_irq_num(void);
void s5p_vic_set_vect_addr(u32 num, void *handler);
void s5p_vic_intr_enable(u32 num);
void s5p_vic_intr_disable(u32 num);
void s5p_vic_soft_intr_clear(u32 num);
void s5p_intr_init(void);

#endif
