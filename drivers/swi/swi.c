/*
 * linux/driver/swi/swi.c 
 *
 * Single-Wired Interface Driver.
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 *	Donghwa Lee <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTAswiLITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <asm/arch/gpio.h>
#include <swi.h>

struct swi_platform_data *swi;

int swi_transfer_command(struct swi_platform_data *swi, unsigned int command)
{
	switch (command) {
	case SWI_LOW:
		gpio_set_value(swi->swi_bank, swi->controller_data, 0);
		break;
	case SWI_HIGH:
		gpio_set_value(swi->swi_bank, swi->controller_data, 1);
		break;
	case SWI_CHANGE:
		gpio_direction_output(swi->swi_bank, swi->controller_data, 0);
		udelay(swi->low_period);

		gpio_set_value(swi->swi_bank, swi->controller_data, 1);
		udelay(swi->high_period);
		break;
	default:
		printf("invalid command.\n");
		return 0;
	}

	return 1;
}
