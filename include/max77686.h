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

#ifndef _MAX77686_H_
#define _MAX77686_H_

#define MAX77686_PMIC_ADDR	(0x12 >> 1)
#define MAX77686_RTC_ADDR	(0x0C >> 1)

#define MAX77686_EN32KHZ_AP	(1 << 0)
#define MAX77686_EN32KHZ_CP	(1 << 1)
#define MAX77686_EN32KHZ	(1 << 2)
#define MAX77686_EN32KHZ_LJM	(1 << 3)
#define MAX77686_EN32KHZ_DFLT	(MAX77686_EN32KHZ_AP | \
				 MAX77686_EN32KHZ_CP | \
				 MAX77686_EN32KHZ | \
				 MAX77686_EN32KHZ_LJM)

typedef enum {
	OPMODE_OFF,
	OPMODE_STANDBY,
	OPMODE_LPM,
	OPMODE_ON,
} opmode_type;

#ifdef CONFIG_PMIC_MAX77686
void max77686_bus_init(int bus_num);
#else
#define max77686_bus_init(x) do {} while(0)
#endif
int max77686_rtc_init(void);
int max77686_init(void);
int max77686_check_pwron_pwrkey(void);
int max77686_check_pwron_wtsr(void);
int max77686_check_pwron_smpl(void);
int max77686_check_pwrkey(void);
int max77686_clear_irq(void);
int max77686_set_ldo_voltage(int ldo, ulong uV);
int max77686_set_buck_voltage(int buck, ulong uV);
int max77686_set_ldo_mode(int ldo, opmode_type mode);
int max77686_set_buck_mode(int buck, opmode_type mode);
int max77686_set_32khz(unsigned char mode);
void show_pwron_source(char *buf);
int max77686_check_acokb_pwron(void);

#endif
