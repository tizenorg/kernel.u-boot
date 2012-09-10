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

#ifdef DEBUG_FG
#define DEBUG(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG(fmt,args...) do {} while(0)
#endif

/* Register address */
#define MAX77693_REG_STATUS		0x00
#define MAX77693_REG_CAPREP		0x05
#define MAX77693_REG_SOCREP		0x06
#define MAX77693_REG_VCELL		0x09
#define MAX77693_REG_CURRENT		0x0A
#define MAX77693_REG_AVG_CURRENT	0x0B
#define MAX77693_REG_CONFIG		0x1D
#define MAX77693_REG_VERSION		0x21
#define MAX77693_REG_VFOCV		0xFB
#define MAX77693_REG_VFSOC		0xFF

#define FIRMWARE_CONFIG_DEFAULT		0x2350
#define RCOMP0_DEFAULT			0x0060
#define TEMPCO_DEFAULT			0x1015

/* Parameter values from MAXIM 1650mA SDI 20110303 */
#define DSG_DV				65	/* mV */
#define CHG_DV				40	/* mV */
#define T_DV				100	/* mV, voltage tolerance */
#define T_SOC_DSG_UPPER			10	/* 10%, soc tolerance */
#define T_SOC_DSG_BELOW			20	/* 20%, soc tolerance */
#define T_SOC_CHG_UPPER			10	/* 10%, soc tolerance */
#define T_SOC_CHG_BELOW			10	/* 10%, soc tolerance */
#define T_SOC_W				27	/* 27%, soc tolerance(reset case) */
#define T_SOC_LOW			3	/* 3%, soc tolerance(low batt) */
#define T_SOC_HIGH			97	/* 97%, soc tolerance(high batt) */
#define PLAT_VOL_HIGH_DSG		380000	/* unit : 0.01mV */
#define PLAT_VOL_LOW_DSG		365000	/* unit : 0.01mV */
#define PLAT_VOL_HIGH_CHG		385000	/* unit : 0.01mV */
#define PLAT_VOL_LOW_CHG		370000	/* unit : 0.01mV */
#define PLAT_T_SOC			0	/* 0%, soc tolerance */
#define DSG_VCELL_COMP			-500	/* vcell compensation for table soc, unit : 0.01mV */
#define CHG_VCELL_COMP			8500	/* vcell compensation for table soc, unit : 0.01mV */
#define AVER_SAMPLE_CNT			6

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

u16 cell_character_80h[16] = {
	0xA2A0, 0xB6E0, 0xB850, 0xBAD0, 0xBB20, 0xBB70, 0xBBC0, 0xBC20,
	0xBC80, 0xBCE0, 0xBD80, 0xBE20, 0xC090, 0xC420, 0xC910, 0xD070
};

u16 cell_character_90h[16] = {
	0x0090, 0x1A50, 0x02F0, 0x2060, 0x2060, 0x2E60, 0x26A0, 0x2DB0,
	0x2DB0, 0x1870, 0x2A20, 0x16F0, 0x08F0, 0x0D40, 0x08C0, 0x08C0
};

u16 cell_character_a0h[16] = {
	0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100
};

static int fg_bus = 9;
static int rst_status;
static int charger_status;
static int por;
static u32 vfocv_table;
static const char pszfg[] = "max77693-fg:";

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

static u32 max77693_fg_get_vfocv(void)
{
	u16 value;
	u32 vfocv;

	value = fuelgauge_reg_read(MAX77693_REG_VFOCV);
	vfocv = (u32)(value >> 3);

	DEBUG("%s\tvfocv = 0x%x (0x%x)\n", pszfg, vcell, value);

	return vfocv * 625 / 1000;
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

u32 max77693_fg_get_soc(void)
{
	u16 value;
	u32 soc;

	value = fuelgauge_reg_read(MAX77693_REG_VFSOC);
	soc = (u32)(value >> 8);

	DEBUG("%s\tsoc = 0x%x (0x%x)\n", pszfg, soc, value);

	return soc;
}

static u16 max77693_fg_check_version(void)
{
	u16 version = fuelgauge_reg_read(MAX77693_REG_VERSION);

	DEBUG("%s\tversion: 0x%x\n", pszfg, version);

	return version;
}

static void max77693_fg_load_init_config(void)
{
	int i;
	u16 data0[16], data1[16], data2[16];
	u16 status;
	int rewrite_count = 5;

	/* 1. Delay 500mS */
	/* delay(500); */

	/* 2. Initilize Configuration */
	fuelgauge_reg_write(MAX77693_REG_CONFIG, 0x2210);

rewrite_model:
	/* 4. Unlock Model Access */
	fuelgauge_reg_write(0x62, 0x0059);
	fuelgauge_reg_write(0x63, 0x00C4);

	/* 5. Write/Read/Verify the Custom Model */
	fuelgauge_reg_write_16n(0x80, cell_character_80h);
	fuelgauge_reg_write_16n(0x90, cell_character_90h);
	fuelgauge_reg_write_16n(0xA0, cell_character_a0h);

	fuelgauge_reg_read_16n(0x80, data0);
	fuelgauge_reg_read_16n(0x90, data1);
	fuelgauge_reg_read_16n(0xA0, data2);

	for (i = 0; i < 16; i++) {
		if (cell_character_80h[i] != data0[i])
			goto rewrite_model;
		if (cell_character_90h[i] != data1[i])
			goto rewrite_model;
		if (cell_character_a0h[i] != data2[i])
			goto rewrite_model;
	}

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
	fuelgauge_reg_write_and_verify(0x38, RCOMP0_DEFAULT);
	fuelgauge_reg_write_and_verify(0x39, TEMPCO_DEFAULT);

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

	printf("%s\tinitialized\n", pszfg);
}

static int max77693_fg_check_validation(u32 vcell, u32 soc)
{
	s32 raw_table_soc = 0;
	u32 table_soc = 0;
	u32 i, idx = 0, idx_end, ret = 0;

	vfocv_table = 0;
	vcell *= 100;

	if (charger_status) {
		/* charging case */
		vcell += CHG_VCELL_COMP;
		idx_end = (int)ARRAY_SIZE(max77693_fg_chg_soc_table) - 1;
		if (vcell <= max77693_fg_chg_soc_table[0].table_v0) {
			if (soc > T_SOC_LOW)
				ret = 2;
			table_soc = 0;

			goto skip_table_soc;
		} else if (vcell > max77693_fg_chg_soc_table[idx_end].table_vcell) {
			if (soc < T_SOC_HIGH)
				ret = 2;
			table_soc = 100;

			goto skip_table_soc;
		}

		/* find range */
		for (i = 0; i < idx_end; i++) {
			if (vcell <= max77693_fg_chg_soc_table[0].table_vcell) {
				idx = 0;
				break;
			} else if (vcell > max77693_fg_chg_soc_table[i].table_vcell
				   && vcell <=
				   max77693_fg_chg_soc_table[i + 1].table_vcell) {
				idx = i + 1;
				break;
			}
		}

		/* calculate table soc */
		raw_table_soc = ((vcell -
			max77693_fg_chg_soc_table[idx].table_v0) *
			max77693_fg_chg_soc_table[idx].table_slope) / 100000;

		DEBUG("%d = (%d - %d)*%d/100000\n", raw_table_soc, vcell,
			max77693_fg_chg_soc_table[idx].table_v0,
			max77693_fg_chg_soc_table[idx].table_slope);

		if (raw_table_soc > 10000)
			raw_table_soc = 10000;
		else if (raw_table_soc < 0)
			raw_table_soc = 0;
		table_soc = raw_table_soc / 100;

		/* check validation */
		if (por == 1 &&
		    vcell <= PLAT_VOL_HIGH_CHG && vcell >= PLAT_VOL_LOW_CHG) {
			if (table_soc > (soc + PLAT_T_SOC) ||
			    soc > (table_soc + PLAT_T_SOC))
				ret = 2;
		} else if (rst_status == SWRESET) {
			if (table_soc > (soc + T_SOC_W) ||
			    soc > (table_soc + T_SOC_W))
				ret = 2;
		} else {
			if (table_soc > (soc + T_SOC_CHG_UPPER) ||
			    soc > (table_soc + T_SOC_CHG_BELOW))
				ret = 2;
		}

		vfocv_table = max77693_fg_get_vfocv_table(raw_table_soc);
	} else {
		/* discharging case */
		vcell += DSG_VCELL_COMP;
		idx_end = (int)ARRAY_SIZE(max77693_fg_dsg_soc_table) - 1;
		if (vcell <= max77693_fg_dsg_soc_table[idx_end].table_vcell ||
			vcell < 340000) {
			if (soc > T_SOC_LOW)
				ret = 1;
			table_soc = 0;

			goto skip_table_soc;
		} else if (vcell >= 417000) {
			if (soc < T_SOC_HIGH)
				ret = 1;
			table_soc = 100;

			goto skip_table_soc;
		}

		/* find range */
		for (i = 0; i < idx_end; i++) {
			if (vcell >= max77693_fg_dsg_soc_table[0].table_vcell) {
				idx = 0;
				break;
			} else if (vcell < max77693_fg_dsg_soc_table[i].table_vcell
				   && vcell >=
				   max77693_fg_dsg_soc_table[i + 1].table_vcell) {
				idx = i + 1;
				break;
			}
		}

		/* calculate table soc */
		raw_table_soc = ((vcell -
			max77693_fg_dsg_soc_table[idx].table_v0) *
			max77693_fg_dsg_soc_table[idx].table_slope) / 100000;

		DEBUG("%d = (%d - %d)*%d/100000\n", raw_table_soc, vcell,
			max77693_fg_dsg_soc_table[idx].table_v0,
			max77693_fg_dsg_soc_table[idx].table_slope);

		if (raw_table_soc > 10000)
			raw_table_soc = 10000;
		else if (raw_table_soc < 0)
			raw_table_soc = 0;
		table_soc = raw_table_soc / 100;

		/* check validation */
		if (por == 1 &&
		    vcell <= PLAT_VOL_HIGH_DSG && vcell >= PLAT_VOL_LOW_DSG) {
			if (table_soc > (soc + PLAT_T_SOC) ||
			    soc > (table_soc + PLAT_T_SOC))
				ret = 1;
		} else if (rst_status == SWRESET) {
			if (table_soc > (soc + T_SOC_W) ||
			    soc > (table_soc + T_SOC_W))
				ret = 1;
		} else {
			if (table_soc > (soc + T_SOC_DSG_UPPER) ||
			    soc > (table_soc + T_SOC_DSG_BELOW))
				ret = 1;
		}

		vfocv_table = max77693_fg_get_vfocv_table(raw_table_soc);
	}

skip_table_soc:
	return ret;
}

void max77693_fg_init(int charger_type)
{
	u32 vcell, soc, vfocv;
	u16 status;
	u8 recalculation_type = 0, soc_valid = 0;
	u8 raw_vcell[2], t_raw_vcell[2] = {0 , 0};
	u32 raw_data, t_raw_data;

	if (max77693_fg_probe()) {
		printf("%s\tinitialize failed\n", pszfg);
		return;
	}

	charger_status = charger_type;

	rst_status = get_reset_status();

	status = fuelgauge_reg_read(MAX77693_REG_STATUS);
	if (status & 0x02) {
		max77693_fg_load_init_config();
		por = 1;
	} else {
		por = 0;
	}

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

	soc_valid = max77693_fg_check_validation(vcell, soc);

	if (vfocv_table != 0) {
		t_raw_data = (vfocv_table * 1000) / 625;
		t_raw_data <<= 3;
		t_raw_vcell[1] = (t_raw_data & 0xff00) >> 8;
		t_raw_vcell[0] = t_raw_data & 0xff;
	}

	if (soc_valid == 1)	/* discharging */
		recalculation_type = 4;
	else if (soc_valid == 2)	/* charging */
		recalculation_type = 3;

	switch (recalculation_type) {
	case 1:
	case 3:
		/* 0x200 means 40mV */
		if (vfocv_table != 0)
			fuelgauge_reg_write(MAX77693_REG_VFOCV,
					(t_raw_vcell[1] << 8) | t_raw_vcell[0]);
		else
			fuelgauge_reg_write(MAX77693_REG_VFOCV,
					((raw_vcell[1] -
					  0x2) << 8) | (raw_vcell[0] - 0x00));
		mdelay(500);
		break;
	case 2:
	case 4:
		/* 0x2C0 means 55mV */
		/* 0x340 means 65mV */
		if (vfocv_table != 0)
			fuelgauge_reg_write(MAX77693_REG_VFOCV,
					(t_raw_vcell[1] << 8) | t_raw_vcell[0]);
		else
			fuelgauge_reg_write(MAX77693_REG_VFOCV,
					((raw_vcell[1] +
					  0x3) << 8) | (raw_vcell[0] + 0x40));
		mdelay(500);
		break;
	default:
		break;
	}
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
