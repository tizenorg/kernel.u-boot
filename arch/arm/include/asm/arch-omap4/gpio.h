/*
 * Copyright (C) 2011 Samsung Electronics
 * Sanghee Kim <sh0130.kim@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __ASM_ARCH_GPIO_H_
#define __ASM_ARCH_GPIO_H_

#define OMAP44XX_GPIO_REVISION		0x0000
#define OMAP44XX_GPIO_EOI		0x0020
#define OMAP44XX_GPIO_IRQSTATUSRAW0	0x0024
#define OMAP44XX_GPIO_IRQSTATUSRAW1	0x0028
#define OMAP44XX_GPIO_IRQSTATUS0	0x002c
#define OMAP44XX_GPIO_IRQSTATUS1	0x0030
#define OMAP44XX_GPIO_IRQSTATUSSET0	0x0034
#define OMAP44XX_GPIO_IRQSTATUSSET1	0x0038
#define OMAP44XX_GPIO_IRQSTATUSCLR0	0x003c
#define OMAP44XX_GPIO_IRQSTATUSCLR1	0x0040
#define OMAP44XX_GPIO_IRQWAKEN0		0x0044
#define OMAP44XX_GPIO_IRQWAKEN1		0x0048
#define OMAP44XX_GPIO_IRQENABLE1	0x011c
#define OMAP44XX_GPIO_WAKE_EN		0x0120
#define OMAP44XX_GPIO_IRQSTATUS2	0x0128
#define OMAP44XX_GPIO_IRQENABLE2	0x012c
#define OMAP44XX_GPIO_CTRL		0x0130
#define OMAP44XX_GPIO_OE		0x0134
#define OMAP44XX_GPIO_DATAIN		0x0138
#define OMAP44XX_GPIO_DATAOUT		0x013c
#define OMAP44XX_GPIO_LEVELDETECT0	0x0140
#define OMAP44XX_GPIO_LEVELDETECT1	0x0144
#define OMAP44XX_GPIO_RISINGDETECT	0x0148
#define OMAP44XX_GPIO_FALLINGDETECT	0x014c
#define OMAP44XX_GPIO_DEBOUNCENABLE	0x0150
#define OMAP44XX_GPIO_DEBOUNCINGTIME	0x0154
#define OMAP44XX_GPIO_CLEARIRQENABLE1	0x0160
#define OMAP44XX_GPIO_SETIRQENABLE1	0x0164
#define OMAP44XX_GPIO_CLEARWKUENA	0x0180
#define OMAP44XX_GPIO_SETWKUENA		0x0184
#define OMAP44XX_GPIO_CLEARDATAOUT	0x0190
#define OMAP44XX_GPIO_SETDATAOUT	0x0194

struct gpio_bank {
	void *base;
	int method;
};

#define METHOD_GPIO_44XX		6

/* This is the interface */

/* Request a gpio before using it */
int omap_request_gpio(int gpio);
/* Reset and free a gpio after using it */
void omap_free_gpio(int gpio);
/* Sets the gpio as input or output */
void omap_set_gpio_direction(int gpio, int is_input);
/* Set or clear a gpio output */
void omap_set_gpio_dataout(int gpio, int enable);
/* Get the value of a gpio input */
int omap_get_gpio_datain(int gpio);

#endif /* __ASM_ARCH_GPIO_H__ */
