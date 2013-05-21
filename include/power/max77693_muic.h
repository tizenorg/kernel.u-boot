/*
 *  Copyright (C) 2013 Samsung Electronics
 *  Piotr Wilczek <p.wilczek@samsung.com>
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

#ifndef __MAX77693_MUIC_H_
#define __MAX77693_MUIC_H_

#include <power/power_chrg.h>

/*
 * MUIC REGISTER
 */

#define MAX77693_MUIC_PREFIX	"max77693-muic:"

/* MAX77693_MUIC_STATUS1 */
#define MAX77693_MUIC_ADC_MASK	0x1F

/* MAX77693_MUIC_STATUS2 */
#define MAX77693_MUIC_CHG_NO		0x00
#define MAX77693_MUIC_CHG_USB		0x01
#define MAX77693_MUIC_CHG_USB_D		0x02
#define MAX77693_MUIC_CHG_TA		0x03
#define MAX77693_MUIC_CHG_TA_500	0x04
#define MAX77693_MUIC_CHG_TA_1A		0x05
#define MAX77693_MUIC_CHG_MASK		0x07

/* MAX77693_MUIC_CONTROL1 */
#define MAX77693_MUIC_CTRL1_DN1DP2	((0x1 << 3) | 0x1)
#define MAX77693_MUIC_CTRL1_UT1UR2	((0x3 << 3) | 0x3)
#define MAX77693_MUIC_CTRL1_ADN1ADP2	((0x4 << 3) | 0x4)
#define MAX77693_MUIC_CTRL1_AUT1AUR2	((0x5 << 3) | 0x5)
#define MAX77693_MUIC_CTRL1_MASK	0xC0

#define MUIC_PATH_USB	0
#define MUIC_PATH_UART	1

#define MUIC_PATH_CP	0
#define MUIC_PATH_AP	1

enum muic_path {
	MUIC_PATH_USB_CP,
	MUIC_PATH_USB_AP,
	MUIC_PATH_UART_CP,
	MUIC_PATH_UART_AP,
};

/* MAX 777693 MUIC registers */
enum {
	MAX77693_MUIC_ID	= 0x00,
	MAX77693_MUIC_INT1	= 0x01,
	MAX77693_MUIC_INT2	= 0x02,
	MAX77693_MUIC_INT3	= 0x03,
	MAX77693_MUIC_STATUS1	= 0x04,
	MAX77693_MUIC_STATUS2	= 0x05,
	MAX77693_MUIC_STATUS3	= 0x06,
	MAX77693_MUIC_INTMASK1	= 0x07,
	MAX77693_MUIC_INTMASK2	= 0x08,
	MAX77693_MUIC_INTMASK3	= 0x09,
	MAX77693_MUIC_CDETCTRL	= 0x0A,
	MAX77693_MUIC_CONTROL1	= 0x0C,
	MAX77693_MUIC_CONTROL2	= 0x0D,
	MAX77693_MUIC_CONTROL3	= 0x0E,

	MUIC_NUM_OF_REGS = 0x0F,
};

#define MAX77693_MUIC_I2C_ADDR	(0x4A >> 1)

int power_muic_init(unsigned int bus);
#endif /* __MAX77693_MUIC_H_ */
