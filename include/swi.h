/*
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 *	Donghwa Lee <dh09.lee@samsung.com>
 *
 * Single-Wired Interface header definitions
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

#ifndef _SWI_H_
#define _SWI_H_


/*
 * It controls the status of a device using single-wired interface.
 *
 * SWI_LOW - signal low.
 * SWI_HIGH - signal high.
 * SWI_CHANGE - toggle signal low to high.
 */
enum {
	SWI_LOW = 0,
	SWI_HIGH,
	SWI_CHANGE
};

/*
 * Single-Wired Interface board information definitions.
 *
 * @name : device name.
 * @controller_data : control pin for seding signal. (gpio pin)
 * @low_period : low period of signal. (usecond)
 * @high_period : high period of signal. (usecond)
 * @init : function pointer for initializing backlight driver.
 */
struct swi_platform_data {
	struct s5p_gpio_bank *swi_bank;
	unsigned int controller_data;
	unsigned int low_period;
	unsigned int high_period;
};


int swi_transfer_command(struct swi_platform_data *swi, unsigned int command);

#endif /*_SWI_H_*/
