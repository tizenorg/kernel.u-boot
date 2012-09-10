/*
 * Copyright (C) 2008, 2009,  Samsung Electronics Co. Ltd. All Rights Reserved.
 * Written by Linux Lab, MITs Development Team, Mobile Communication Division.
 *
 * MAX17042_8997.c is Max17042 driver code which integrated in MAX8997.
 * so It's another version of current max17042.c(for single type)
 */

#include <common.h>
#include <asm/types.h>
#include <asm/arch/power.h>
#include <command.h>
#include <i2c.h>
#include <max17042.h>

/* Register address */
#define STATUS_REG		0x00
#define SOCrep_REG		0x06
#define VCELL_REG		0x09
#define CURRENT_REG		0x0A
#define AVG_CURRENT_REG		0x0B
#define SOCmix_REG		0x0D
#define SOCav_REG		0x0E
#define DESIGN_CAP_REG		0x18
#define AVG_VCELL_REG		0x19
#define CONFIG_REG		0x1D
#define VERSION_REG		0x21
#define LearnCFG_REG            0x28
#define FilterCFG_REG		0x29
#define RelaxCFG_REG		0x2A
#define MiscCFG_REG		0x2B
#define CGAIN_REG		0x2E
#define COff_REG		0x2F
#define RCOMP0_REG		0x38
#define FSTAT_REG		0x3D
#define VFOCV_REG		0xFB
#define VFSOC_REG		0xFF

#define GPIO_FUEL_SDA18V
#define GPIO_FUEL_SCL18V

/* #define FG_DEBUG 1 */
static uchar chip_address = (0x6c >> 1);
static u32 i2c_bus_num = 0;

typedef enum {
	POSITIVE = 0,
	NEGATIVE = 1
} sign_type;

static u16 ReadFGRegister(u8 addr);
static void WriteFGRegister(u8 addr, u16 w_data);
static void ReadFG16Register(u8 addr, u16 *w_data);
static void WriteFG16Register(u8 addr, u16 *w_data);
static void WriteAndVerifyFGRegister(u8 addr, u16 w_data);

static void mdelay(int msec)
{
	int i;
	for (i = 0; i < msec; i++)
		udelay(1000);
}

#ifdef FG_DEBUG
#define POR_CONFIG_VALUE	0x2350 /* check it, frist */
u16 g_config_check, g_status_2;
u16 g_status, g_config, g_rcomp;
u16 g_vfsoc_2, g_vcell_2, g_vfocv_2;
u16 g_vfsoc_3, g_vcell_3, g_vfocv_3;
u16 g_recaculated, g_diff_vol, g_table_soc, g_diff_soc;
#endif
u16 g_por_check, g_table_vfocv, g_table_vfocv_soc;
u32 g_reset_state;
u32 g_charger_status;

/////////// Parameter values from MAXIM 1650mA SDI 2011.03.03 ///////////
#define DSG_DV				65 /* mV */
#define CHG_DV				40 /* mV */
#define T_DV				100 /* mV, voltage tolerance */
#define T_SOC_DSG_UPPER		10 /* 10%, soc tolerance */
#define T_SOC_DSG_BELOW		20 /* 20%, soc tolerance */
#define T_SOC_CHG_UPPER		10 /* 10%, soc tolerance */
#define T_SOC_CHG_BELOW		10 /* 10%, soc tolerance */
#define T_SOC_W				27 /* 27%, soc tolerance(reset case) */
#define T_SOC_LOW			3 /* 3%, soc tolerance(low batt) */
#define T_SOC_HIGH			97 /* 97%, soc tolerance(high batt) */
#define PLAT_VOL_HIGH_DSG	380000 /* unit : 0.01mV */
#define PLAT_VOL_LOW_DSG	365000 /* unit : 0.01mV */
#define PLAT_VOL_HIGH_CHG	385000 /* unit : 0.01mV */
#define PLAT_VOL_LOW_CHG	370000 /* unit : 0.01mV */
#define PLAT_T_SOC			0 /* 0%, soc tolerance */
#define DSG_VCELL_COMP		-2000 /* vcell compensation for table soc, unit : 0.01mV */
#define CHG_VCELL_COMP		-2000 /* vcell compensation for table soc, unit : 0.01mV */
#define AVER_SAMPLE_CNT		6

#define RCOMP0			0x004F
#define TempCo			0x102B

/* new model data 2011.03.18 */
u16 cell_character0[16] = {  // Address 0x80h starts here
	0xACB0,	0xB630,	0xB950,	0xBA20,	0xBBB0,	0xBBE0,	0xBC30,	0xBD00,
	0xBD60,	0xBDC0,	0xBF30,	0xC0A0,	0xC480,	0xC890,	0xCC40,	0xD010
};

u16 cell_character1[16] = {  //Address 0x90h starts here
	0x0180,	0x0F10,	0x0060,	0x0E40,	0x3DC0,	0x4E10,	0x2D50,	0x3680,
	0x3680,	0x0D50,	0x0D60,	0x0D80,	0x0C80,	0x0860,	0x0800,	0x0800
};

u16 cell_character2[16] = { //Address 0xA0h starts here
	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,
	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080,	0x0080
};

static struct fuelgauge_dsg_table_data {
	u32 table_vcell;
	u32 table_v0;
	u32 table_slope;
} max17042_dsg_soc_table[19] = {
{ 410100,	280167,	7638 },
{ 400000,	329011, 12264 },
{ 380000,	337808,	14022 },
{ 377600,	346818,	17883 },
{ 376100,	353171,	22312 },
{ 375600,	296446,	6367 },
{ 375000,	360396,	32929 },
{ 373000,	358802,	29874 },
{ 371900,	360851,	34015 },
{ 370900,	362898,	42288 },
{ 370000,	363902,	47744 },
{ 369000,	365484,	64460 },
{ 368300,	364487,	53280 },
{ 366800,	360937,	25881 },
{ 364500,	356525,	14484 },
{ 361000,	353762,	10324 },
{ 359400,	357230,	20593 },
{ 355700,	352425,	6500 },
{ 311600,	311349,	477 }
};

static struct fuelgauge_chg_table_data {
	u32 table_vcell;
	u32 table_v0;
	u32 table_slope;
} max17042_chg_soc_table[13] = {
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
/* vfocv = LSOC*G + V0 */
/* LSOC	V0	1/G */
static struct fuelgauge_ocv_table_data {
	u32 table_soc;
	u32 table_v0;
	u32 table_slope;
} max17042_ocv_table[17] = {
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

static int fuelgauge_read_i2c_word(u8 address, u8 *data)
{
	int ret = 0;
	ret = i2c_read(chip_address, address, 1, data, 2);
	if(ret == 1) {
		printf("read i2c result %d\n", ret);
		return -1;
	}
	return 0;
}

static void fuelgauge_write_i2c_word(u8 address, u16 *data)
{
	i2c_write(chip_address, address, 1, data, 2);
}

static u32 get_table_vfocv(u32 raw_soc)
{
	u32 i, idx = 0, idx_end, ret = 0;
	u32 table_vfocv = 0, temp_soc;

	/* find range */
	idx_end = (int)ARRAY_SIZE(max17042_ocv_table)-1;
	for (i = 0; i < idx_end; i++) {
		if (raw_soc == 0 || raw_soc >= 10000) {
			break;
		} else if (raw_soc <= max17042_ocv_table[i].table_soc &&
			raw_soc > max17042_ocv_table[i+1].table_soc) {
			idx = i;
			break;
		}
	}

	if (raw_soc == 0) {
		table_vfocv = 325000;
	} else if (raw_soc >= 10000) {
		table_vfocv = 417000;
	} else {
		table_vfocv = ((raw_soc * 100000) /
			max17042_ocv_table[idx].table_slope) +
			max17042_ocv_table[idx].table_v0;
#ifdef FG_DEBUG
		printf("[%d] %d = (%d * 100000) / %d + %d\n",
			    idx, table_vfocv, raw_soc,
				max17042_ocv_table[idx].table_slope,
				max17042_ocv_table[idx].table_v0);
#endif
	}
	table_vfocv /= 100;

	return table_vfocv;
}

static int check_validation_with_tablesoc(u32 vcell, u32 soc)
{
	s32 raw_table_soc = 0;
	u32 table_soc = 0;
	u32 i, idx = 0, idx_end, ret = 0;

	g_table_vfocv = 0;
	vcell *= 100;

	if (g_charger_status) {
		/* charging case */
		vcell += CHG_VCELL_COMP; /* voltage compensation */
		idx_end = (int)ARRAY_SIZE(max17042_chg_soc_table)-1;
		if (vcell <= max17042_chg_soc_table[0].table_v0) {
			if (soc > T_SOC_LOW)
				ret = 2;
			table_soc = 0;
#ifdef FG_DEBUG
			g_table_soc = 0;
#endif
			goto skip_table_soc;
		} else if (vcell > max17042_chg_soc_table[idx_end].table_vcell) {
			if (soc < T_SOC_HIGH)
				ret = 2;
			table_soc = 100;
#ifdef FG_DEBUG
			g_table_soc = 100;
#endif
			goto skip_table_soc;
		}

		/* find range */
		for (i = 0; i < idx_end; i++) {
			if (vcell <= max17042_chg_soc_table[0].table_vcell) {
				idx = 0;
				break;
			} else if (vcell > max17042_chg_soc_table[i].table_vcell &&
					vcell <= max17042_chg_soc_table[i+1].table_vcell) {
				idx = i+1;
				break;
			}
		}

		/* calculate table soc */
		raw_table_soc = ((vcell - max17042_chg_soc_table[idx].table_v0) *
					max17042_chg_soc_table[idx].table_slope) / 100000;
#ifdef FG_DEBUG
		printf("%d = (%d - %d)*%d/100000\n", raw_table_soc, vcell,
			max17042_chg_soc_table[idx].table_v0,
			max17042_chg_soc_table[idx].table_slope);
#endif
		if (raw_table_soc > 10000)
			raw_table_soc = 10000;
		else if (raw_table_soc < 0)
			raw_table_soc = 0;
		table_soc = raw_table_soc / 100;
#ifdef FG_DEBUG
		g_table_soc = table_soc; /* % */
#endif
		/* check validation */
		if (g_por_check == 1 &&
			vcell <= PLAT_VOL_HIGH_CHG && vcell >= PLAT_VOL_LOW_CHG) {
			if (table_soc > (soc + PLAT_T_SOC) ||
				soc > (table_soc + PLAT_T_SOC))
				ret = 2;
		} else if (g_reset_state == SWRESET) { /* reset case */
			if (table_soc > (soc + T_SOC_W) ||
				soc > (table_soc + T_SOC_W))
				ret = 2;
		} else {
			if (table_soc > (soc + T_SOC_CHG_UPPER) ||
				soc > (table_soc + T_SOC_CHG_BELOW))
				ret = 2;
		}

		g_table_vfocv = get_table_vfocv(raw_table_soc);
	} else {
		/* discharging case */
		vcell += DSG_VCELL_COMP; /* voltage compensation */
		idx_end = (int)ARRAY_SIZE(max17042_dsg_soc_table)-1;
		if (vcell <= max17042_dsg_soc_table[idx_end].table_vcell ||
			vcell < 340000) {
			if (soc > T_SOC_LOW)
				ret = 1;
			table_soc = 0;
#ifdef FG_DEBUG
			g_table_soc = 0;
#endif
			goto skip_table_soc;
		} else if (vcell >= 417000) {
			if (soc < T_SOC_HIGH)
				ret = 1;
			table_soc = 100;
#ifdef FG_DEBUG
			g_table_soc = 100;
#endif
			goto skip_table_soc;
		}

		/* find range */
		for (i = 0; i < idx_end; i++) {
			if (vcell >= max17042_dsg_soc_table[0].table_vcell) {
				idx = 0;
				break;
			} else if (vcell < max17042_dsg_soc_table[i].table_vcell &&
					vcell >= max17042_dsg_soc_table[i+1].table_vcell) {
				idx = i+1;
				break;
			}
		}

		/* calculate table soc */
		raw_table_soc = ((vcell - max17042_dsg_soc_table[idx].table_v0) *
					max17042_dsg_soc_table[idx].table_slope) / 100000;
#ifdef FG_DEBUG
		printf("%d = (%d - %d)*%d/100000\n", raw_table_soc, vcell,
			max17042_dsg_soc_table[idx].table_v0,
			max17042_dsg_soc_table[idx].table_slope);
#endif
		if (raw_table_soc > 10000)
			raw_table_soc = 10000;
		else if (raw_table_soc < 0)
			raw_table_soc = 0;
		table_soc = raw_table_soc / 100;
#ifdef FG_DEBUG
		g_table_soc = table_soc; /* % */
#endif
		/* check validation */
		if (g_por_check == 1 &&
			vcell <= PLAT_VOL_HIGH_DSG && vcell >= PLAT_VOL_LOW_DSG) {
			if (table_soc > (soc + PLAT_T_SOC) ||
				soc > (table_soc + PLAT_T_SOC))
				ret = 1;
		} else if (g_reset_state == SWRESET) { /* reset case */
			if (table_soc > (soc + T_SOC_W) ||
				soc > (table_soc + T_SOC_W))
				ret = 1;
		} else {
			if (table_soc > (soc + T_SOC_DSG_UPPER) ||
				soc > (table_soc + T_SOC_DSG_BELOW))
				ret = 1;
		}

		g_table_vfocv = get_table_vfocv(raw_table_soc);
	}

skip_table_soc:
	return ret;
}

u32 fuel_gauge_get_vcell(void)
{
	u8 tmpADC[2] = {0,};
	u32 vcell;
	int ret = 0;

	ret = fuelgauge_read_i2c_word(VCELL_REG, tmpADC);

	if(ret == -1)
		return 0;

	vcell = (tmpADC[0] >> 3) + (tmpADC[1] << 5);
#ifdef FG_DEBUG
	printf("%s: [1]=%x, [0]=%x\n", __func__, tmpADC[1], tmpADC[0]);
#endif
	return vcell * 625/1000;
}

u32 get_average_fg_vcell(void)
{
 	u32 vcell_data;
	u32 vcell_max = 0;
	u32 vcell_min = 0;
	u32 vcell_total = 0;
	u32 i;

	for (i = 0; i < AVER_SAMPLE_CNT; i++) {
		vcell_data = fuel_gauge_get_vcell();

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

	return (vcell_total - vcell_max - vcell_min) / (AVER_SAMPLE_CNT-2);
}

u32 fuel_gauge_get_vfocv(void)
{
	u8 tmpADC[2];
	u32 vfocv;

	fuelgauge_read_i2c_word(VFOCV_REG, tmpADC);
	vfocv = (tmpADC[0] >> 3) + (tmpADC[1] << 5);
#if FG_DEBUG
	printf("%s: [1]=%x, [0]=%x\n", __func__, tmpADC[1], tmpADC[0]);
#endif
	return vfocv * 625/1000;
}

u32 fuel_gauge_get_soc(void)
{
	u8 tmpSOC[2] = {0,};
	u32 soc;

	fuelgauge_read_i2c_word(VFSOC_REG, tmpSOC);
	soc = tmpSOC[1];
#ifdef FG_DEBUG
	printf("%s: [1]=%x, [0]=%x\n", __func__, tmpSOC[1], tmpSOC[0]);
#endif
	return soc;
}

void fuel_gauge_get_version(void)
{
	u8 version[2];

	fuelgauge_read_i2c_word(VERSION_REG, version);

	printf("%s: [1]=%x, [0]=%x\n", __func__, version[1], version[0]);

	return;
}

void fuelgauge_implementaion(void)
{
	u16 r_data0[16], r_data1[16], r_data2[16];
	u16 Saved_RCOMP0;
	u16 Saved_TempCo;
	u32 i = 0;
	u16 read_val;
	u32 rewrite_count = 5;

	printf("%s - start\n",__func__);

initialize_fuelgauge:
	/* 1. Delay 500mS */
	/*mdelay(500); */

	/* 2. Initilize Configuration */
	WriteFGRegister(0x1D, 0x2310);			//Write CONFIG

rewrite_model:
	/* 4. Unlock Model Access */
	WriteFGRegister(0x62, 0x0059);			// Unlock Model Access
	WriteFGRegister(0x63, 0x00C4);

	/* 5. Write/Read/Verify the Custom Model */
	WriteFG16Register(0x80, cell_character0);
	WriteFG16Register(0x90, cell_character1);
	WriteFG16Register(0xA0, cell_character2);

	ReadFG16Register(0x80, r_data0);
	ReadFG16Register(0x90, r_data1);
	ReadFG16Register(0xA0, r_data2);

	for(i = 0; i < 16; i++) {
		if( (cell_character0[i] != r_data0[i])
			|| (cell_character1[i] != r_data1[i])
			|| (cell_character2[i] != r_data2[i]) )
			goto rewrite_model;
		}

relock_model:
	/* 8. Lock model access */
	WriteFGRegister(0x62, 0x0000);  			// Lock Model Access
	WriteFGRegister(0x63, 0x0000);

	/* 9. Verify the model access is locked */
	ReadFG16Register(0x80, r_data0);
	ReadFG16Register(0x90, r_data1);
	ReadFG16Register(0xA0, r_data2);

	for(i = 0; i < 16; i++) {
		if(r_data0[i]  || r_data1[i] || r_data2[i]) {  // if any model data is non-zero, it's not locked.
			if(rewrite_count--) {  // Repeat writing model data.
				printf("%s - Lock model access failed, Rewrite it now!!\n", __func__);
				goto rewrite_model;
			}
			else {  // Ignore and continue boot up.
				printf("%s - Lock model access failed, but Ignore it!!\n", __func__);
			}
		}
	;}

	/* 10. Write Custom Parameters */
	WriteAndVerifyFGRegister(0x38, RCOMP0);  // Write and Verify RCOMP0
	WriteAndVerifyFGRegister(0x39, TempCo);  // Write and Verify TempCo

	/* 11. Delay at least 350mS */
	mdelay(350);

	/* 12. Initialization Complete */
	read_val = ReadFGRegister(STATUS_REG);  // Read Status
	WriteAndVerifyFGRegister(STATUS_REG, (read_val & 0xFFFD));  // Write and Verify Status with POR bit Cleared

#if 0 /* TODO: Need to check
	/* 13. Idendify Battery */
	/* 14. Check for Fuelgauge Reset */
	read_val = ReadFGRegister(STATUS_REG);

	if(read_val & 0x2)
		goto initialize_fuelgauge;

	/* 16. Save Learned Parameters */
	Saved_RCOMP0 = ReadFGRegister(0x38);
	Saved_TempCo = ReadFGRegister(0x39);

	/* 17. Restore Learned Parameters */
	WriteAndVerifyFGRegister(0x38, Saved_RCOMP0);
	WriteAndVerifyFGRegister(0x39, Saved_TempCo);
#endif
	/* 18. Delay at least 350mS */
	mdelay(350);

	printf("%s - end\n",__func__);

}

void max17042_vfocv_test(void)
{
	int i, i_end;
	u32 vcell, r_vcell, r_vfocv, r_soc;
	u32 raw_data;
	u8 raw_vcell[2];
	int table_data[] = {
		4167, 4021, 3922, 3851, 3802,
		3790, 3771, 3766, 3762, 3758,
		3741, 3739, 3736, 3686, 3657,
		3252 };

	i_end = (int)ARRAY_SIZE(table_data)-1;
	for (i = 0; i < i_end; i++) {
		vcell = table_data[i];
		raw_data = (vcell*1000)/625;
		raw_data <<= 3;
		raw_vcell[1] = (raw_data&0xff00) >> 8;
		raw_vcell[0] = raw_data&0xff;
		WriteFGRegister(VFOCV_REG, (raw_vcell[1]<<8) | raw_vcell[0]);
		mdelay(500);
		r_vcell = fuel_gauge_get_vcell();
		r_soc = fuel_gauge_get_soc();
		r_vfocv = fuel_gauge_get_vfocv();
		printf("[%dmV]: V = %d mV, VF = %d mV, S = %d \n", vcell,
			r_vcell, r_vfocv, r_soc);
	}
}

void init_fuel_gauge(int charger_status)
{
	u32 vcell, soc, vfocv;
	u16 val;
	u8 status[2], fstat[2];
	u8 raw_vcell[2], t_raw_vcell[2];
	u8 recalculation_type = 0, soc_validation = 0;
	u32 raw_data, t_raw_data;
	u32 i;

	g_charger_status = charger_status;

	g_reset_state = get_reset_status();

	fuelgauge_read_i2c_word(STATUS_REG, status);

#ifdef FG_DEBUG
	g_config_check = ReadFGRegister(0x1D);
#endif

	if ((status[0] & 0x02) == 0x02) {
		fuelgauge_implementaion();
		g_por_check = 1;
	} else {
		printf("%s : not por status %x:%x\n", __func__, status[1],status[0]);
		g_por_check = 0;
	}

	/* need some delay for stable vcell */
	mdelay(100);
	vcell = get_average_fg_vcell();
	raw_data = (vcell*1000)/625;
	raw_data <<= 3;
	raw_vcell[1] = (raw_data&0xff00) >> 8;
	raw_vcell[0] = raw_data&0xff;

	/* NOTICE : use soc after initializing, soc can be changed */
	soc = fuel_gauge_get_soc();
	vfocv = fuel_gauge_get_vfocv();
	fuel_gauge_get_version();

	printf("%s: vcell = %d mV, vfocv = %d mV, soc = %d \n", __func__,
			vcell, vfocv, soc);

#ifdef FG_DEBUG
	g_status = status;
	g_config = ReadFGRegister(0x1D);
	g_rcomp = ReadFGRegister(0x38);
	g_status_2 = ReadFGRegister(STATUS_REG);

	g_vcell_2 = fuel_gauge_get_vcell();
	g_vfsoc_2 = soc;
	g_vfocv_2 = vfocv;

	printf("%s: g_status = 0x%x, g_status_2 = 0x%x, g_config = 0x%x\n",
			__func__, g_status, g_status_2, g_config);
#endif

	g_table_vfocv = 0;
	g_table_vfocv_soc = 0;
	recalculation_type = 0;
	if (g_reset_state == SWRESET) { /* reset case : 0x20000000 */
		printf("%s : check s/w reset (%x) : use wide tolerance\n",
			__func__, g_reset_state);
	} else {
		if (g_charger_status) {
			if (((vcell-CHG_DV) > (vfocv + T_DV)) || /* h/w POR and cable booting case */
				(vfocv > vcell)) /* h/w POR with cable present case */
				recalculation_type = 1;
		} else {
			if (((vcell+DSG_DV) > (vfocv + T_DV)) || /* fast batt exchange (low -> high) */
				(vfocv > ((vcell+DSG_DV) + T_DV))) /* fast batt exchange (high -> low) */
				recalculation_type = 2;
		}
	}

	soc_validation = check_validation_with_tablesoc(vcell, soc);

	if (g_table_vfocv != 0) {
		t_raw_data = (g_table_vfocv*1000)/625;
		t_raw_data <<= 3;
		t_raw_vcell[1] = (t_raw_data&0xff00) >> 8;
		t_raw_vcell[0] = t_raw_data&0xff;
	}

	if (soc_validation==1) /* discharging */
		recalculation_type = 4;
	else if (soc_validation==2) /* charging */
		recalculation_type = 3;

	if (recalculation_type == 1 || recalculation_type == 3) {
		/* 0x200 means 40mV */
		if (g_table_vfocv != 0)
			WriteFGRegister(VFOCV_REG, (t_raw_vcell[1]<<8) | t_raw_vcell[0]);
		else
			WriteFGRegister(VFOCV_REG, ((raw_vcell[1]-0x2)<<8) | (raw_vcell[0]-0x00));
		mdelay(500);
	} else if (recalculation_type == 2 || recalculation_type == 4) {
		/* 0x2C0 means 55mV */
		/* 0x340 means 65mV */
		if (g_table_vfocv != 0)
			WriteFGRegister(VFOCV_REG, (t_raw_vcell[1]<<8) | t_raw_vcell[0]);
		else
			WriteFGRegister(VFOCV_REG, ((raw_vcell[1]+0x3)<<8) | (raw_vcell[0]+0x40));
		mdelay(500);
	}

#ifdef FG_DEBUG
	g_recaculated = recalculation_type;

	if (g_recaculated != 0 && g_table_vfocv != 0) {
		g_table_vfocv_soc = fuel_gauge_get_soc();
	} else {
		g_table_vfocv_soc = 9999; /* no re-cal */
	}

	if (g_charger_status)
		if ((vcell-CHG_DV) > vfocv)
			g_diff_vol = (vcell-CHG_DV) - vfocv;
		else
			g_diff_vol = vfocv - (vcell-CHG_DV);
	else
		if ((vcell+DSG_DV) > vfocv)
			g_diff_vol = (vcell+DSG_DV) - vfocv;
		else
			g_diff_vol = vfocv - (vcell+DSG_DV);

	if (g_table_soc > soc)
		g_diff_soc = g_table_soc - soc;
	else
		g_diff_soc = soc - g_table_soc;

	g_vcell_3 = fuel_gauge_get_vcell();
	g_vfsoc_3 = fuel_gauge_get_soc();
	g_vfocv_3 = fuel_gauge_get_vfocv();
#endif
}

#ifdef FG_DEBUG
void fuel_gauge_debug_print(void)
{
	u8 status_string[50];

	memset(status_string, 0x0, 50);
	sprintf(status_string, "status (after) = 0x%x",
		g_status_2);
	lcd_draw_font(10, 780, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	sprintf(status_string, "status = 0x%x, config = 0x%x, "
		"rcomp = 0x%x", g_status, g_config, g_rcomp);
	lcd_draw_font(10, 760, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	sprintf(status_string, "soc2 = %d%%, vcell2 = %dmV, "
		"vfocv2 = %dmV", g_vfsoc_2, g_vcell_2, g_vfocv_2);
	lcd_draw_font(10, 720, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	if (g_por_check)
		sprintf(status_string, "POR OK");
	else
		sprintf(status_string, "POR NOT OK");
	lcd_draw_font(10, 700, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	if (g_config_check == POR_CONFIG_VALUE)
		sprintf(status_string, "POR config (0x%x)",
			g_config_check);
	else
		sprintf(status_string, "NOT POR config (0x%x)",
			g_config_check);
	lcd_draw_font(10, 680, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	if (g_rcomp == RCOMP0)
		sprintf(status_string, "Valid rcomp");
	else
		sprintf(status_string, "Invalid rcomp (0x%x)",
			g_rcomp);
	lcd_draw_font(10, 660, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	if (g_recaculated > 0)
		sprintf(status_string, "recal. soc, d_v = %dmV, d_s = %d%%, recal = %d",
								g_diff_vol, g_diff_soc, g_recaculated);
	else
		sprintf(status_string, "normal soc, d_v = %dmV, d_s = %d%%, recal = %d",
								g_diff_vol, g_diff_soc, g_recaculated);
	lcd_draw_font(10, 640, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	sprintf(status_string, "t_soc = %d%%->t_vfocv = %dmV->t_vfsoc = %d%%",
			g_table_soc, g_table_vfocv, g_table_vfocv_soc);
	lcd_draw_font(10, 620, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	sprintf(status_string, "soc3 = %d%%, vcell3 = %dmV, "
		"vfocv3 = %dmV", g_vfsoc_3, g_vcell_3, g_vfocv_3);
	lcd_draw_font(10, 600, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);

	memset(status_string, 0x0, 50);
	sprintf(status_string, "reset state = 0x%x", g_reset_state);
	lcd_draw_font(10, 580, COLOR_WHITE, COLOR_BLACK,
				strlen(status_string), status_string);
}
#endif

static u16 ReadFGRegister(u8 addr)
{
	u8 data[2];

	i2c_read(chip_address, addr, 1, data, 2);

	return ( (data[1] << 8) | data[0] );
}

static void WriteFGRegister(u8 addr, u16 w_data)
{
	u8 data[2];
	data[0] = w_data & 0xFF;
	data[1] = (w_data >> 8);

	i2c_write(chip_address, addr, 1, data, 2);
}

static void ReadFG16Register(u8 addr, u16 *w_data)
{
	u8 *data = (u8*)w_data;
	u32 i;

	for(i=0; i<16; i++)
		i2c_read(chip_address, addr+i, 1, &data[2*i], 2);
}

static void WriteFG16Register(u8 addr, u16 *w_data)
{
	u8 *data = (u8*)w_data;
	u32 i;

	for(i=0; i<16; i++)
		i2c_write(chip_address, addr+i, 1, &data[2*i], 2);
}

static void WriteAndVerifyFGRegister(u8 addr, u16 w_data)
{
	WriteFGRegister(addr, w_data);
	ReadFGRegister(addr);
}

static void fuel_gauge_test_read(void)
{
	u8 data[2], reg;

	for(reg=0; reg<0x50; reg++)
	{
		if(reg == 0)
			printf("0    1    2    3    4    5    6    7    : 8    9    A    B    C    D    E    F   \n");
		else if((reg & 0xf) == 0)
			printf("\n");
		else if((reg & 0xf) == 0x8)
			printf(": ");

		i2c_read(chip_address, reg, 1, data, (u8)2);
		printf("%04x ", *(u16*)data);
	}
	printf("\n");
}

/* command test. */
static s32 fuelgauge_cmd(s32 argc, s8 *argv[])
{
	fuel_gauge_test_read();
	return 0;
}

U_BOOT_CMD(fg, CONFIG_SYS_MAXARGS, 1, fuelgauge_cmd,
	"Utility to read/write fuelgauge(max17042) registers",
	"ex) fg\n"
	": Print vcell(mV), soc(%%)\n"
);
