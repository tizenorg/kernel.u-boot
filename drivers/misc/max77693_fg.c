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

#include <common.h>
#include <command.h>
#include <asm/arch/power.h>
#include <i2c.h>
#include <max77693.h>
#include "fuelgauge_battery_data.h"

#ifdef DEBUG_FG
#define DEBUG(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG(fmt,args...) do {} while(0)
#endif

#define msleep(a)	udelay(a * 1000)

/* Register address */
#define MAX77693_REG_STATUS		0x00
#define MAX77693_REG_CAPREP		0x05
#define MAX77693_REG_SOCREP		0x06
#define MAX77693_REG_VCELL		0x09
#define MAX77693_REG_CURRENT		0x0A
#define MAX77693_REG_AVG_CURRENT	0x0B
#define MAX77693_REG_CONFIG		0x1D
#define MAX77693_REG_VERSION		0x21
#define MAX77693_REG_OCV_RO		0xEE
#define MAX77693_REG_OCV_WR		0xFB
#define MAX77693_REG_VFSOC		0xFF

static int fg_bus = -1;
static int rst_status;
static int charger_status;
static int por;
static u32 vfocv_table;
static const char pszfg[] = "max77693-fg:";

/* parameter for SDI 1750mA 2012.02.17 */
/* Address 0x80h */
static u16 sdi_1750_cell_character0[16] = {
	0xACB0,	0xB630,	0xB950,	0xBA20,	0xBBB0,	0xBBE0,	0xBC30,	0xBD00,
	0xBD60,	0xBDC0,	0xBF30,	0xC0A0,	0xC480,	0xC890,	0xCC40,	0xD010
};

/* Address 0x90h */
static u16 sdi_1750_cell_character1[16] = {
	0x0180,	0x0F10,	0x0060,	0x0E40,	0x3DC0,	0x4E10,	0x2D50,	0x3680,
	0x3680,	0x0D50,	0x0D60,	0x0D80,	0x0C80,	0x0860,	0x0800,	0x0800
};

/* Address 0xA0h */
static u16 sdi_1750_cell_character2[16] = {
	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,
	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080
};

/* parameter for SDI 2100mA 2012.01.25 */
/* Address 0x80h */
static u16 sdi_2100_cell_character0[16] = {
	0xA890,	0xB780,	0xB9A0,	0xBBF0,	0xBC30,	0xBC80,	0xBCF0,	0xBD50,
	0xBE60,	0xBFB0,	0xC1B0,	0xC4B0,	0xC560,	0xCCE0,	0xD170,	0xD7A0
};

/* Address 0x90h */
static u16 sdi_2100_cell_character1[16] = {
	0x0150,	0x1000,	0x0C10,	0x3850,	0x2E50,	0x32F0,	0x3040,	0x12F0,
	0x0FE0,	0x1090,	0x09E0,	0x0BD0,	0x0820,	0x0720,	0x0700,	0x0700
};

/* Address 0xA0h */
static u16 sdi_2100_cell_character2[16] = {
	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,
	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100
};

/* parameter for SDI 2100mA 2012.10.11 */
/* Address 0x80h */
static u16 sdi_3000_cell_character0[16] = {
	0xAF70, 0xB570, 0xB7A0, 0xB900, 0xBA70, 0xBBF0, 0xBC40, 0xBC90,
	0xBE20, 0xBF80, 0xC340, 0xC600, 0xCA90, 0xCD90, 0xD3B0, 0xD7C0
};

/* Address 0x90h */
static u16 sdi_3000_cell_character1[16] = {
	0x0170, 0x0D20, 0x0BA0, 0x0BF0, 0x0DF0, 0x3F40, 0x3F00, 0x1A00,
	0x18E0, 0x09F0, 0x0970, 0x0920, 0x0860, 0x0680, 0x0600, 0x0600
};

/* Address 0xA0h */
static u16 sdi_3000_cell_character2[16] = {
	0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100
};

/* table_soc = (new_vcell - new_v0)*new_slope/100000 (unit : 0.01% ) */
static struct fuelgauge_dsg_data_table {
	u32 table_vcell;
	u32 table_v0;
	u32 table_slope;
} max77693_fg_dsg_soc_table[19] = {
	{ 410100, 280167, 7638 },
	{ 400000, 329011, 12264 },
	{ 380000, 337808, 14022 },
	{ 377600, 346818, 17883 },
	{ 376100, 353171, 22312 },
	{ 375600, 296446, 6367 },
	{ 375000, 360396, 32929 },
	{ 373000, 358802, 29874 },
	{ 371900, 360851, 34015 },
	{ 370900, 362898, 42288 },
	{ 370000, 363902, 47744 },
	{ 369000, 365484, 64460 },
	{ 368300, 364487, 53280 },
	{ 366800, 360937, 25881 },
	{ 364500, 356525, 14484 },
	{ 361000, 353762, 10324 },
	{ 359400, 357230, 20593 },
	{ 355700, 352425, 6500 },
	{ 311600, 311349, 477 }
};

static struct fuelgauge_chg_data_table {
	u32 table_vcell;
	u32 table_v0;
	u32 table_slope;
} max77693_fg_chg_soc_table[13] = {
	{ 348100, 326900, 634 },
	{ 365300, 331362, 798 },
	{ 368200, 351400, 1939 },
	{ 372200, 366842, 23987 },
	{ 378900, 361312, 11738 },
	{ 381900, 375992, 73529 },
	{ 382400, 374780, 61013 },
	{ 386300, 361965, 22635 },
	{ 388500, 339987, 11946 },
	{ 391200, 361291, 21538 },
	{ 399200, 350354, 15312 },
	{ 418700, 338278, 12277 },
	{ 419700, 365571, 18646 }
};

/* ocv table for finding vfocv */
static struct fuelgauge_ocv_data_table {
	u32 table_soc;
	u32 table_v0;
	u32 table_slope;
} max77693_fg_ocv_table[17] = {
	{ 10000, -1421176, 543 },
	{ 9983, 325581, 10936 },
	{ 8370, 351593, 16565 },
	{ 6735, 331923, 11164 },
	{ 5939, 364431, 28706 },
	{ 4541, 371623, 52640 },
	{ 3883, 366294, 30560 },
	{ 3501, 371615, 57067 },
	{ 3073, 371628, 57200 },
	{ 2644, 370772, 48267 },
	{ 2282, 371529, 57460 },
	{ 1920, 370165, 40806 },
	{ 1667, 370099, 40159 },
	{ 1414, 335196, 3680 },
	{ 1230, 364884, 32927 },
	{ 285, 325250, 704 },
	{ 0, 0, 0 }
};

static struct table_soc_data sdi_2100_table_soc_data[] = {
	{	3000, 0, 0, 0			},
	{	3306, 1289, 105100, 3170600	},
	{	3597, 4059, 4100, 3580300	},
	{	3622, 10148, 8500, 3535900	},
	{	3672, 16039, 2200, 3637100	},
	{	3714, 35344, 4100, 3569600	},
	{	3782, 51984, 7000, 3420600	},
	{	3902, 69246, 11400, 3112200	},
	{	4225, 97566, 0, 100000000	},
};

static struct table_soc_data sdi_3000_table_soc_data[] = {
	{ 3301, 1148, 0, 0 },
	{ 3609, 4230, 0, 0 },
	{ 3640, 11828, 0, 0 },
	{ 3657, 13469, 0, 0 },
	{ 3707, 21840, 0, 0 },
	{ 3733, 34992, 0, 0 },
	{ 3758, 43629, 0, 0 },
	{ 3806, 53027, 0, 0 },
	{ 3900, 65387, 0, 0 },
	{ 4265, 100492, 0, 0 },
};

static inline void mdelay(int msec)
{
	udelay(msec * 1000);
}

static inline u16 fuelgauge_reg_read(int reg)
{
	u16 data;

	i2c_read(MAX77693_FG_ADDR, reg, 1, (uchar *)&data, 2);

	return data;
}

static inline void fuelgauge_reg_write(int reg, u16 data)
{
	i2c_write(MAX77693_FG_ADDR, reg, 1, (uchar *)&data, 2);
}

static inline void fuelgauge_reg_read_16n(int reg, u16 *data)
{
	int i;

	for (i = 0; i < 16; i++)
		i2c_read(MAX77693_FG_ADDR, reg + i, 1, (uchar *)(data + i), 2);
}

static inline void fuelgauge_reg_write_16n(int reg, u16 *data)
{
	int i;

	for (i = 0; i < 16; i++)
		i2c_write(MAX77693_FG_ADDR, reg + i, 1, (uchar *)(data + i), 2);
}

static void fuelgauge_reg_write_and_verify(int reg, u16 data)
{
	fuelgauge_reg_write(reg, data);
	fuelgauge_reg_read(reg);
}

void max77693_fg_bus_init(int bus_num)
{
	fg_bus = bus_num;
}

int max77693_fg_probe(void)
{
	unsigned char addr = MAX77693_FG_ADDR;

	i2c_set_bus_num(fg_bus);

	if (i2c_probe(addr)) {
		puts("Can't found max77693 fuel gauge\n");
		return 1;
	}

	return 0;
}

u32 max77693_fg_get_cap(void)
{
	u16 value;

	value = fuelgauge_reg_read(MAX77693_REG_CAPREP);

	DEBUG("%s\tcaprep = 0x%x\n", pszfg, value);

	return (u32)value;
}

u32 max77693_fg_get_vcell(void)
{
	u16 value;
	u32 vcell;

	value = fuelgauge_reg_read(MAX77693_REG_VCELL);

	vcell = (u32)(value >> 3);

	DEBUG("%s\tvcell = 0x%x (0x%x)\n", pszfg, vcell, value);

	/* uV */
	return vcell * 625;
}

static u32 max77693_fg_get_ocv(void)
{
	u16 value;
	u32 ocv;

	value = fuelgauge_reg_read(MAX77693_REG_OCV_RO);
	ocv = (u32)(value >> 3);

	DEBUG("%s\tocv = 0x%x (0x%x)\n", pszfg, vcell, value);

	return ocv * 625 / 1000;
}

u32 max77693_fg_get_soc(void)
{
	u16 value;
	u32 soc;

	value = fuelgauge_reg_read(MAX77693_REG_VFSOC);
	soc = (u32)(value >> 8);

	DEBUG("%s\tsoc = 0x%x (0x%x)\n", pszfg, soc, value);

	return soc;
}

static u32 max77693_fg_get_vfocv(void)
{
	u16 value;
	u32 vfocv;

	value = fuelgauge_reg_read(MAX77693_REG_OCV_WR);
	vfocv = (u32)(value >> 3);

	DEBUG("%s\tvfocv = 0x%x (0x%x)\n", pszfg, vcell, value);

	return vfocv * 625 / 1000;
}

static u32 max77693_fg_get_average_vcell(void)
{
	int i;
	u32 vcell_data;
	u32 vcell_max = 0;
	u32 vcell_min = 0;
	u32 vcell_total = 0;

	for (i = 0; i < AVER_SAMPLE_CNT; i++) {
		vcell_data = max77693_fg_get_vcell() / 1000;

		if (i != 0) {
			if (vcell_data > vcell_max)
				vcell_max = vcell_data;
			else if (vcell_data < vcell_min)
				vcell_min = vcell_data;
		} else {
			vcell_max = vcell_data;
			vcell_min = vcell_data;
		}
		vcell_total += vcell_data;
	}

	return (vcell_total - vcell_max - vcell_min) / (AVER_SAMPLE_CNT - 2);
}

static u32 max77693_fg_get_vfocv_table(u32 raw_soc)
{
	int i, idx, idx_end;
	u32 vfocv;

	/* find range */
	idx_end = (int)ARRAY_SIZE(max77693_fg_ocv_table) - 1;
	for (i = 0, idx = 0; i < idx_end; i++) {
		if (raw_soc == 0 || raw_soc >= 10000) {
			break;
		} else if (raw_soc <= max77693_fg_ocv_table[i].table_soc &&
			   raw_soc > max77693_fg_ocv_table[i + 1].table_soc) {
			idx = i;
			break;
		}
	}

	if (raw_soc == 0) {
		vfocv = 325000;
	} else if (raw_soc >= 10000) {
		vfocv = 417000;
	} else {
		vfocv = ((raw_soc * 100000) /
				max77693_fg_ocv_table[idx].table_slope) +
				max77693_fg_ocv_table[idx].table_v0;

		DEBUG("[%d] %d = (%d * 100000) / %d + %d\n",
		       idx, vfocv, raw_soc,
		       max77693_fg_ocv_table[idx].table_slope,
		       max77693_fg_ocv_table[idx].table_v0);
	}

	vfocv /= 100;

	return vfocv;
}

static u16 max77693_fg_check_version(void)
{
	u16 version = fuelgauge_reg_read(MAX77693_REG_VERSION);

	DEBUG("%s\tversion: 0x%x\n", pszfg, version);

	return version;
}

static void max77693_fg_load_init_config(enum battery_type batt_type)
{
	u16 data0[16], data1[16], data2[16];
	u16 status;
	u16 rcomp0, tempco;
	u16 *cell_character0, *cell_character1, *cell_character2;
	u32 vcell, soc, vfocv;
	u32 i;
	u32 rewrite_count = 5;

	vcell = max77693_fg_get_average_vcell();
	soc = max77693_fg_get_soc();
	vfocv = max77693_fg_get_ocv();

	printf("%s\tPOR start: vcell(%d), vfocv(%d), soc(%d)\n", pszfg, vcell, vfocv, soc);

	/* update battery parameter */
	switch (batt_type) {
	case BATTERY_SDI_1750:
		printf("%s\tupdate SDI 1750 parameter\n", pszfg);
		rcomp0 = SDI_1750_RCOMP0;
		tempco = SDI_1750_TEMPCO;
		cell_character0 = sdi_1750_cell_character0;
		cell_character1 = sdi_1750_cell_character1;
		cell_character2 = sdi_1750_cell_character2;
		break;
	case BATTERY_SDI_2100:
		printf("%s\tupdate SDI 2100 parameter\n", pszfg);
		rcomp0 = SDI_2100_RCOMP0;
		tempco = SDI_2100_TEMPCO;
		cell_character0 = sdi_2100_cell_character0;
		cell_character1 = sdi_2100_cell_character1;
		cell_character2 = sdi_2100_cell_character2;
		break;
	case BATTERY_SDI_3000:
		printf("%s\tupdate SDI 3000 parameter\n", pszfg);
		rcomp0 = SDI_3000_RCOMP0;
		tempco = SDI_3000_TEMPCO;
		cell_character0 = sdi_3000_cell_character0;
		cell_character1 = sdi_3000_cell_character1;
		cell_character2 = sdi_3000_cell_character2;
		break;

	default:
		printf("%s\tunknown battery type, keep parameter\n", pszfg);
		break;
	}

	/* 1. Delay 500mS */
	/* delay(500); */

	/* 2. Initilize Configuration */
	fuelgauge_reg_write(MAX77693_REG_CONFIG, 0x2210);

rewrite_model:
	/* 4. Unlock Model Access */
	fuelgauge_reg_write(0x62, 0x0059);
	fuelgauge_reg_write(0x63, 0x00C4);

	/* 5. Write/Read/Verify the Custom Model */
	fuelgauge_reg_write_16n(0x80, cell_character0);
	fuelgauge_reg_write_16n(0x90, cell_character1);
	fuelgauge_reg_write_16n(0xA0, cell_character2);

	fuelgauge_reg_read_16n(0x80, data0);
	fuelgauge_reg_read_16n(0x90, data1);
	fuelgauge_reg_read_16n(0xA0, data2);

	for (i = 0; i < 16; i++) {
		if (cell_character0[i] != data0[i])
			goto rewrite_model;
		if (cell_character1[i] != data1[i])
			goto rewrite_model;
		if (cell_character2[i] != data2[i])
			goto rewrite_model;
	}
relock_model:
	/* 8. Lock model access */
	fuelgauge_reg_write(0x62, 0x0000);
	fuelgauge_reg_write(0x63, 0x0000);

	/* 9. Verify the model access is locked */
	fuelgauge_reg_read_16n(0x80, data0);
	fuelgauge_reg_read_16n(0x90, data1);
	fuelgauge_reg_read_16n(0xA0, data2);

	for (i = 0; i < 16; i++) {
		/* if any model data is non-zero, it's not locked. */
		if (data0[i] || data1[i] || data2[i]) {
			if (rewrite_count--) {
				printf("%s\tLock model access failed, rewrite it\n", pszfg);
				goto rewrite_model;
			} else {
				printf("%s\tLock model access failed, but ignore it\n", pszfg);
			}
		}
	}

	/* 10. Write Custom Parameters */
	fuelgauge_reg_write_and_verify(0x38, rcomp0);
	fuelgauge_reg_write_and_verify(0x39, tempco);

	/* 11. Delay at least 350mS */
	mdelay(350);

	/* 12. Initialization Complete */
	status = fuelgauge_reg_read(MAX77693_REG_STATUS);
	/* Write and Verify Status with POR bit Cleared */
	fuelgauge_reg_write_and_verify(MAX77693_REG_STATUS, (status & 0xFFFD));

	/* 13. Idendify Battery */

	/* 14. Check for Fuelgauge Reset */

	/* 16. Save Learned Parameters */

	/* 17. Restore Learned Parameters */

	/* 18. Delay at least 350mS */

	vcell = max77693_fg_get_average_vcell();
	soc = max77693_fg_get_soc();
	vfocv = max77693_fg_get_ocv();

	printf("%s\tPOR finish: vcell(%d), vfocv(%d), soc(%d)\n", pszfg, vcell, vfocv, soc);
}

static int get_table_soc(u32 vcell, enum battery_type batt_type)
{
	s32 raw_table_soc = 0;
	u32 table_soc;
	u32 idx_start, idx_end, idx;
	struct table_soc_data *table_soc_data;

	idx_start = 0;
	idx_end = 0;

	switch (batt_type) {
	case BATTERY_SDI_2100:
		table_soc_data = sdi_2100_table_soc_data;
		idx_end = (int)ARRAY_SIZE(sdi_2100_table_soc_data) - 1;
		break;
	case BATTERY_SDI_3000:
		table_soc_data = sdi_3000_table_soc_data;
		idx_end = (int)ARRAY_SIZE(sdi_3000_table_soc_data) - 1;
		break;
	default:
		printf("%s\tunknown battery type. uses 2100mAh data default.\n", pszfg);
		table_soc_data = sdi_2100_table_soc_data;
		idx_end = (int)ARRAY_SIZE(sdi_2100_table_soc_data) - 1;
		break;
	}

	if (vcell < table_soc_data[idx_start].table_vcell) {
		printf("%s: vcell(%d) out of range, set table soc as 0\n", __func__, vcell);
		table_soc = 0;
		goto calculate_finish;
	} else if (vcell > table_soc_data[idx_end].table_vcell) {
		printf("%s: vcell(%d) out of range, set table soc as 100\n", __func__, vcell);
		table_soc = 100;
		goto calculate_finish;
	}

	while (idx_start <= idx_end) {
		idx = (idx_start + idx_end) / 2;
		if (table_soc_data[idx].table_vcell > vcell) {
			idx_end = idx - 1;
		} else if (table_soc_data[idx].table_vcell < vcell) {
			idx_start = idx + 1;
		} else
			break;
	}
	table_soc = table_soc_data[idx].table_soc;

	/* high resolution */
	if (vcell < table_soc_data[idx].table_vcell)
		table_soc = table_soc_data[idx].table_soc -
			((table_soc_data[idx].table_soc - table_soc_data[idx-1].table_soc) *
			(table_soc_data[idx].table_vcell - vcell) /
			(table_soc_data[idx].table_vcell - table_soc_data[idx-1].table_vcell));
	else
		table_soc = table_soc_data[idx].table_soc +
			((table_soc_data[idx+1].table_soc - table_soc_data[idx].table_soc) *
			(vcell - table_soc_data[idx].table_vcell) /
			(table_soc_data[idx+1].table_vcell - table_soc_data[idx].table_vcell));

	printf("%s: vcell(%d) is caculated to t-soc(%d.%d)\n", __func__, vcell, table_soc / 1000, table_soc % 1000);
	table_soc /= 1000;

calculate_finish:
	return table_soc;
}

static int check_fuelgauge_powered(void)
{
	int pwr_chk = 0;

	pwr_chk = fuelgauge_reg_read(MAX77693_REG_OCV_RO);

	if (pwr_chk < 0)
		return 0;
	else
		return 1;
}

void max77693_fg_init(enum battery_type batt_type, int charger_type)
{
	u32 soc, vcell, vfocv, power_check, table_soc, soc_diff, soc_chk_cnt, check_cnt;
	u32 raw_data, t_raw_data;
	u16 status, reg_data;
	u8 recalculation_type = 0, soc_valid = 0;
	u8 raw_vcell[2], t_raw_vcell[2] = {0 , 0};

	if (max77693_fg_probe()) {
		printf("%s\tinitialize failed\n", pszfg);
		return;
	}

	power_check = check_fuelgauge_powered();
	while (power_check == 0) {
		msleep(500);
		power_check = check_fuelgauge_powered();

		check_cnt++;
		printf("%s\t:fuelgauge is not powered(%d).\n", pszfg, check_cnt);

		if (check_cnt >= FUELGAUGE_POWER_CHECK_COUNT) {
			printf("%s\t:fuelgauge power failed.\n", pszfg);
			return;
		}
	}

	charger_status = charger_type;

	rst_status = get_reset_status();

	status = fuelgauge_reg_read(MAX77693_REG_STATUS);
	if (status & 0x02) {
		max77693_fg_load_init_config(batt_type);
		por = 1;
	} else {
		por = 0;
	}

	printf("%s\tpor = 0x%04x\n", pszfg, status);

	vcell = max77693_fg_get_average_vcell();
	raw_data = (vcell * 1000) / 625;
	raw_data <<= 3;
	raw_vcell[1] = (raw_data & 0xff00) >> 8;
	raw_vcell[0] = raw_data & 0xff;

	/* NOTICE : use soc after initializing, soc can be changed */
	soc = max77693_fg_get_soc();
	vfocv = max77693_fg_get_vfocv();
	max77693_fg_check_version();

	printf("%s\tvcell = %d mV, soc = %d, vfocv = %d mV\n",
		pszfg, vcell, soc, vfocv);

	if ((batt_type == BATTERY_SDI_2100) || (batt_type == BATTERY_SDI_3000)) {
		table_soc = get_table_soc(vcell, batt_type);

		if (por == 0)
			goto init_finish;

check_soc_validation:
		/* check validation */
		if (soc > table_soc)
			soc_diff = soc - table_soc;
		else
			soc_diff = table_soc - soc;

		if (soc_diff > SOC_DIFF_TH) {
			printf("%s: [#%d] diff(%d), soc(%d) and table soc(%d)\n", __func__,
				++soc_chk_cnt, soc_diff, soc, table_soc);

			raw_data = (vcell * 1000) / 625;
			raw_data <<= 3;
			raw_vcell[1] = (raw_data & 0xff00) >> 8;
			raw_vcell[0] = raw_data & 0xff;
			fuelgauge_reg_write(MAX77693_REG_OCV_WR, (raw_vcell[1] << 8) | raw_vcell[0]);

			mdelay(300);

			soc = max77693_fg_get_soc();
			vfocv = max77693_fg_get_vfocv();
			vcell = max77693_fg_get_average_vcell();
			table_soc = get_table_soc(vcell, batt_type);

			if (soc_chk_cnt < SOC_DIFF_CHECK_COUNT)
				goto check_soc_validation;
		}
		goto init_finish;
	}

	vfocv_table = 0;
	recalculation_type = 0;

	if (rst_status != SWRESET) {
		if (charger_status) {
			if (((vcell - CHG_DV) > (vfocv + T_DV)) ||	/* h/w POR and cable booting case */
			    (vfocv > vcell))				/* h/w POR with cable present case */
				recalculation_type = 1;
		} else {
			if (((vcell + DSG_DV) > (vfocv + T_DV)) ||	/* fast batt exchange (low -> high) */
			    (vfocv > ((vcell + DSG_DV) + T_DV)))	/* fast batt exchange (high -> low) */
				recalculation_type = 2;
		}
	}

	if (vfocv_table != 0) {
		t_raw_data = (vfocv_table * 1000) / 625;
		t_raw_data <<= 3;
		t_raw_vcell[1] = (t_raw_data & 0xff00) >> 8;
		t_raw_vcell[0] = t_raw_data & 0xff;
	}

	switch (recalculation_type) {
	case 1:
	case 3:
		/* 0x200 means 40mV */
		if (vfocv_table != 0)
			fuelgauge_reg_write(MAX77693_REG_OCV_WR,
					(t_raw_vcell[1] << 8) | t_raw_vcell[0]);
		else
			fuelgauge_reg_write(MAX77693_REG_OCV_WR,
					((raw_vcell[1] -
					  0x2) << 8) | (raw_vcell[0] - 0x00));
		mdelay(500);
		break;
	case 2:
	case 4:
		/* 0x2C0 means 55mV */
		/* 0x340 means 65mV */
		if (vfocv_table != 0)
			fuelgauge_reg_write(MAX77693_REG_OCV_WR,
					(t_raw_vcell[1] << 8) | t_raw_vcell[0]);
		else
			fuelgauge_reg_write(MAX77693_REG_OCV_WR,
					((raw_vcell[1] +
					  0x3) << 8) | (raw_vcell[0] + 0x40));
		mdelay(500);
		break;
	default:
		break;
	}

init_finish:
	/* NOTICE : use soc after initializing, soc can be changed */
	soc = max77693_fg_get_soc();
	vfocv = max77693_fg_get_vfocv();
	max77693_fg_check_version();

	printf("%s\tvcell = %d mV, soc = %d, vfocv = %d mV\n",
		pszfg, vcell, soc, vfocv);

}

static int max77693_fg_reg_dump(void)
{
	int i;

	printf("   0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F");
	for (i = 0; i < 0x100; i++) {
		if ((i % 0x10) == 0)
			printf("\n%02X ", i);

		printf("%04x ", fuelgauge_reg_read(i));
	}
	printf("\n");

	return 0;
}

void max77693_fg_show_battery(void)
{
	unsigned int soc = max77693_fg_get_soc();
	unsigned int uV = max77693_fg_get_vcell();

	if (max77693_fg_probe())
		return;

	printf("battery:\t%d%%\n", soc);
	printf("voltage:\t%d.%6.6d V\n", uV / 1000000, uV % 1000000);
}

static int do_max77693_fg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	switch (argc) {
	case 2:
		if (!strncmp(argv[1], "dump", 4)) {
			max77693_fg_probe();
			max77693_fg_reg_dump();
			return 0;
		} else {
			cmd_usage(cmdtp);
			return 1;
		}
		break;
	default:
		cmd_usage(cmdtp);
		return 1;
	}
}

U_BOOT_CMD(
	fg, 4, 1, do_max77693_fg,
	"Fuel gauge utility for MAX77693-FG",
	"dump - dump the register\n"
);
