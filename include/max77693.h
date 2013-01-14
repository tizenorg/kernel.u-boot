/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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

#ifndef _MAX77693_H_
#define _MAX77693_H_
#include "fuelgauge_battery_data.h"

#define MAX77693_PMIC_ADDR	(0xCC >> 1)
#define MAX77693_MUIC_ADDR	(0x4A >> 1)
#define MAX77693_FG_ADDR	(0x6C >> 1)

enum {
	CHARGER_NO = 0,
	CHARGER_TA,
	CHARGER_USB,
	CHARGER_TA_500,
	CHARGER_UNKNOWN
};


enum {
	MAX77693_ADC_GND		= 0x00,
	MAX77693_ADC_MHL		= 0x01,
	MAX77693_ADC_DOCK_VOL_DN	= 0x0a,
	MAX77693_ADC_DOCK_VOL_UP	= 0x0b,
	MAX77693_ADC_CEA936ATYPE1_CHG	= 0x17,
	MAX77693_ADC_JIG_USB_OFF	= 0x18,
	MAX77693_ADC_JIG_USB_ON		= 0x19,
	MAX77693_ADC_DESKDOCK		= 0x1a,
	MAX77693_ADC_CEA936ATYPE2_CHG	= 0x1b,
	MAX77693_ADC_JIG_UART_OFF	= 0x1c,
	MAX77693_ADC_JIG_UART_ON	= 0x1d,
	MAX77693_ADC_CARDOCK		= 0x1e,
	MAX77693_ADC_OPEN		= 0x1f
};

#ifdef CONFIG_PMIC_MAX77693
void max77693_pmic_bus_init(int bus_num);
void max77693_muic_bus_init(int bus_num);
void max77693_fg_bus_init(int bus_num);
#else
#define max77693_pmic_bus_init(x) do {} while(0)
#define max77693_muic_bus_init(x) do {} while(0)
#define max77693_fg_bus_init(x) do {} while(0)
#endif
int max77693_init(void);
int max77693_pmic_probe(void);
int max77693_charger_detbat(void);
int max77693_muic_probe(void);
int max77693_muic_check(void);
int max77693_muic_get_adc(void);
int max77693_fg_probe(void);
void max77693_fg_init(enum battery_type batt_type, int charger_type);
u32 max77693_fg_get_soc(void);
u32 max77693_fg_get_vcell(void);
void max77693_fg_show_battery(void);

#endif
