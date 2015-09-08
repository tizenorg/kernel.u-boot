/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _FUELGUAGE_BATTERY_DATA_H_
#define _FUELGUAGE_BATTERY_DATA_H_

/* common parameter */
#define DSG_DV			65	/* mV */
#define CHG_DV			40	/* mV */
#define T_DV			100	/* mV, voltage tolerance */
#define T_SOC_DSG_UPPER		10	/* 10%, soc tolerance */
#define T_SOC_DSG_BELOW		20	/* 20%, soc tolerance */
#define T_SOC_CHG_UPPER		10	/* 10%, soc tolerance */
#define T_SOC_CHG_BELOW		10	/* 10%, soc tolerance */
#define T_SOC_W			27	/* 27%, soc tolerance(reset case) */
#define T_SOC_LOW		3	/* 3%, soc tolerance(low batt) */
#define T_SOC_HIGH		97	/* 97%, soc tolerance(high batt) */

#define PLAT_VOL_HIGH_DSG	380000	/* unit : 0.01mV */
#define PLAT_VOL_LOW_DSG	365000	/* unit : 0.01mV */
#define PLAT_VOL_HIGH_CHG	385000	/* unit : 0.01mV */
#define PLAT_VOL_LOW_CHG	370000	/* unit : 0.01mV */
#define PLAT_T_SOC		0	/* 0%, soc tolerance */
#define DSG_VCELL_COMP		-2000	/* vcell compensation for table soc, unit : 0.01mV */
#define CHG_VCELL_COMP		-1500	/* vcell compensation for table soc, unit : 0.01mV */
#define AVER_SAMPLE_CNT		5

#define FUELGAUGE_POWER_CHECK_COUNT	20
#define SOC_DIFF_CHECK_COUNT	5
#define SOC_DIFF_TH	10

/* parameter for SDI 1750mA 2012.02.17 */
#define SDI_1750_RCOMP0			0x004F
#define SDI_1750_TEMPCO			0x102B

/* parameter for SDI 2100mA 2012.01.25 */
#define SDI_2100_RCOMP0			0x0065
#define SDI_2100_TEMPCO			0x0930

/* parameter for SDI 3030mA 2012.10.11 */
#define SDI_3000_RCOMP0			0x0059
#define SDI_3000_TEMPCO			0x0C1F

enum battery_type {
	BATTERY_UNKNOWN = 0,
	BATTERY_SDI_1750,
	BATTERY_SDI_2100,
	BATTERY_SDI_3000,
};

struct table_soc_data {
	u32 table_vcell;
	u32 table_soc;
	u32 table_slope;
	u32 table_yaxis;
};

#endif
