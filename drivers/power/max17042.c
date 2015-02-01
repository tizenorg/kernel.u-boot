/*
 * Copyright (C) 2008, 2009,  Samsung Electronics Co. Ltd. All Rights Reserved.
 *       Written by Linux Lab, MITs Development Team, Mobile Communication Division.
 */

#include <common.h>
#include <asm/types.h>
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
#define CONFIG_REG		0x1D
#define LearnCFG_REG            0x28
#define FilterCFG_REG		0x29
#define RelaxCFG_REG		0x2A
#define MiscCFG_REG		0x2B
#define CGain_REG		0x2E
#define COff_REG		0x2F
#define RCOMP0_REG		0x38
#define VFOCV_REG 		0xFB
#define VFSOC_REG 		0xFF

static uchar chip_address = 0;
static u32 i2c_bus_num = 0;

typedef enum {
	POSITIVE = 0,
	NEGATIVE = 1
} sign_type;

u16 Kempty0;
u16 Vempty;
u16 SOCempty;
u16 ETC;
u16 RCOMP0;
u16 TempCo;
u16 Capacity;  // 4000mAh
u16 VFCapacity;  // 5333mAh
u16 ICHGTerm;
u16 TempNom;
u16 FilterCFG;

u16 cell_character0[16] = {0,};
u16 cell_character1[16] = {0,};
u16 cell_character2[16] = {0,};

u16 cell_character0_4k[16]
= { 0xB110,
	0xB610,
	0xB7A0,
	0xB980,
	0xBB50,
	0xBBB0,
	0xBCD0,
	0xBD20,
	0xBD70,
	0xBE10,
	0xBED0,
	0xC0E0,
	0xC320,
	0xC6D0,
	0xCAE0,
	0xCF30,
};

u16 cell_character1_4k[16] 	//Address 0x90h starts here
= { 0x02F0,
	0x0830,
	0x0CB0,
	0x0A70,
	0x1EF0,
	0x0900,
	0x33C0,
	0x4980,
	0x1A00,
	0x0E30,
	0x0A70,
	0x0900,
	0x0A70,
	0x0740,
	0x0770,
	0x0770

};

u16 cell_character2_4k[16]  //Address 0xA0h starts here
= { 0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100,
	0x0100
};


u16 cell_character0_7k[16]  //Address 0x80h starts here
= {0x7DF0,
0xB500,
0xB860,
0xB930,
0xB980,
0xBB70,
0xBC00,
0xBC70,
0xBD20,
0xBDB0,
0xBEF0,
0xC140,
0xC3C0,
0xC6E0,
0xCD20,
0xD100
};

u16 cell_character1_7k[16]  //Address 0x90h starts here
= {0x0020,
0x0340,
0x15D0,
0x2DD0,
0x0600,
0x18F0,
0x25A0,
0x1800,
0x22A0,
0x1670,
0x09A0,
0x08C0,
0x0B30,
0x04F0,
0x0870,
0x0870
};

u16 cell_character2_7k[16]  //Address 0xA0h starts here
= {0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200,
0x0200
};

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

u32 fuel_gauge_read_vcell(void)
{
	u16 data;
	u32 vcell = 0;
	u32 temp;
	u32 temp2;

	data = ReadFGRegister(VCELL_REG);

	temp = (data & 0xFFF) * 78125;
	vcell = temp / 1000000;

	temp = ((data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vcell += (temp2 << 4);

	printf("\tVCELL(%d), data(0x%04x)\n", vcell, data);

	return vcell;
}

u32 fuel_gauge_read_vfocv(void)
{
	u16 data;
	u32 vfocv = 0;
	u32 temp;
	u32 temp2;

	data = ReadFGRegister(VFOCV_REG);

	temp = (data & 0xFFF) * 78125;
	vfocv = temp / 1000000;

	temp = ((data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vfocv += (temp2 << 4);

	printf("\tVFOCV(%d), data(0x%04x)\n", vfocv, data);

	return vfocv;
}

u32 fuel_gauge_read_socvf(void)
{
    u16 data;
    u32 socvf = 0;
    u32 temp;

    data = ReadFGRegister(VFSOC_REG);

    temp = (data & 0xFF) * 39 / 1000;

    socvf = (data >> 8);

    printf("\tSOCVF(%d), data(0x%04x)\n", socvf, data);

    return socvf;
}

u32 fuel_gauge_read_soc(void)
{
	u16 data;
	u32 soc = 0;
	u32 temp;

	data = ReadFGRegister(SOCrep_REG);

	temp = (data & 0xFF) * 39 / 1000;

	soc = (data >> 8);
	if(soc == 0) {
		if(temp > 1)  // over 0.1 %
			soc = 1;
	}

	printf("\tSOC(%d), data(0x%04x)\n", soc, data);

	return soc;;
}

void fuel_gauge_read_current(void)
{
	u16 data1, data2;
	u32 temp, sign;
	s32 i_current = 0;
	u8 i_current_sign = 0;
	s32 avg_current = 0;
	u8 avg_current_sign = 0;

	data1 = ReadFGRegister(CURRENT_REG);
	data2 = ReadFGRegister(AVG_CURRENT_REG);

	temp = data1;
	if(temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~(temp) & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

//	printf("%s : temp(0x%08x), data1(0x%04x)\n", __func__, temp, data1);

	temp = temp * 15625;
	i_current = temp / 100000;

	if(sign)
		i_current_sign = '-';
	else
		i_current_sign = '+';

	temp = data2;
	if(temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~(temp) & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

//	printf("%s : temp(0x%08x), data2(0x%04x)\n", __func__, temp, data2);

	temp = temp * 15625;
	avg_current = temp / 100000;

	if(sign)
		avg_current_sign = '-';
	else
		avg_current_sign = '+';

	printf("\tCURRENT(%c%dmA), AVG_CURRENT(%c%dmA)\n", i_current_sign, i_current, avg_current_sign, avg_current);
}

static void mdelay(int msec)
{
	int i;
	for (i = 0; i < msec; i++)
		udelay(1000);
}

void fuel_gauge_reset_soc(void)
{
	u16 data;

	data = ReadFGRegister(MiscCFG_REG);

	data |= (0x1 << 10);  // Set bit10 makes quick start

	WriteFGRegister(MiscCFG_REG, data);
	mdelay(250);

	WriteFGRegister(0x10, Capacity);  // FullCAP
	mdelay(500);
}

//	(Voltage : Volt)		(SOC : %)		기울기			절편
//	~ 4.1252			~ 99.7		0.02449455		1.68235719
//	~ 4.0487			~ 91.8		0.009652643		3.162422924
//	~ 3.9306			~ 76.5		0.007691619		3.34245046
//	~ 3.7834			~ 52.8		0.006231218		3.454112251
//	~ 3.7310			~ 35.1		0.002956889		3.627138584
//	~ 3.6810			~ 18.6		0.00302521		3.624761671
//	~ 3.6368			~ 12.7		0.007498074		3.541660729
//	~ 3.6098			~ 5.0			0.0035141		3.592172753
//	~ 3.5484			~ 2.8			0.027434608		3.472565397
//	~ 3.4				~ 0			0.053905193		3.399566

#define VCELL_DELTA0		1682
#define VCELL_DELTA1		3162
#define VCELL_DELTA2		3342
#define VCELL_DELTA3		3454
#define VCELL_DELTA4		3627
#define VCELL_DELTA5		3625
#define VCELL_DELTA6		3542
#define VCELL_DELTA7		3592
#define VCELL_DELTA8		3473
#define VCELL_DELTA9		3399

#define TABLE_SOC_DIV0		24495
#define TABLE_SOC_DIV1		9653
#define TABLE_SOC_DIV2		7692
#define TABLE_SOC_DIV3		6231
#define TABLE_SOC_DIV4		2957
#define TABLE_SOC_DIV5		3025
#define TABLE_SOC_DIV6		7498
#define TABLE_SOC_DIV7		3514
#define TABLE_SOC_DIV8		27435
#define TABLE_SOC_DIV9		53905

u32 calculate_table_soc(u32 vcell)
{
	u32 table_soc;
	u32 temp1, temp2;

	if(vcell >= 4125)
	{
		temp1 = vcell - VCELL_DELTA0;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV0;
	}
	else if(vcell >= 4049 && vcell < 4125)
	{
		temp1 = vcell - VCELL_DELTA1;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV1;
	}
	else if(vcell >= 3931 && vcell < 4049)
	{
		temp1 = vcell - VCELL_DELTA2;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV2;
	}
	else if(vcell >= 3784 && vcell < 3931)
	{
		temp1 = vcell - VCELL_DELTA3;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV3;
	}
	else if(vcell >= 3731 && vcell < 3784)
	{
		temp1 = vcell - VCELL_DELTA4;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV4;
	}
	else if(vcell >= 3681 && vcell < 3731)
	{
		temp1 = vcell - VCELL_DELTA5;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV5;
	}
	else if(vcell >= 3637 && vcell < 3681)
	{
		temp1 = vcell - VCELL_DELTA6;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV6;
	}
	else if(vcell >= 3610 && vcell < 3637)
	{
		temp1 = vcell - VCELL_DELTA7;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV7;
	}
	else if(vcell >= 3549 && vcell < 3610)
	{
		temp1 = vcell - VCELL_DELTA8;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV8;
	}
	else if(vcell >= 3400 && vcell < 3549)
	{
		temp1 = vcell - VCELL_DELTA9;
		temp2 = temp1 * 1000000;
		table_soc = temp2 / TABLE_SOC_DIV9;
	}
	else if(vcell < 3400)
	{
		table_soc = 0;
	}

	if( (table_soc % 1000) >= 500 )
		table_soc += 1000;

	table_soc /= 1000;  // SOC is based on Voltage(V), not mV.

	printf("\ttable_soc(%d)\n", table_soc);

	return table_soc;
}

void fuel_gauge_compensate_soc(int dc_in)
{
	u32 vcell, rep_soc;
	u32 table_soc;
	u32 read_val, data;
	s32 differ;

	// Read Vcell(09h) & RepSOC(06h)
	vcell = fuel_gauge_read_vcell();
	rep_soc = fuel_gauge_read_soc();
	fuel_gauge_read_current();

	// Calculate Table_SOC by Vcell(09h)
	table_soc = calculate_table_soc(vcell);

	differ = table_soc - rep_soc;
	if(differ < 0)
		differ *= (-1);

	if(dc_in) {
		printf("\tReturn by DCIN input (TA or USB)\n");
		return ;
	}

	// Differ is greater than 10%, compensate it.
//	if(differ >= 10)
	{
		printf("\tStart SOC compensation (difference : %d)\n", differ);

		// Write RemCapREP(05h) = DesignCap(18h) * Table_SOC / 100.
//		read_val = ReadFGRegister(0x18);  // DesignCap
		data = Capacity * table_soc / 100;
		WriteFGRegister(0x05, (u16)data);

		// Write RepSOC(06h) = Table_SOC
		WriteFGRegister(0x06, (u16)(table_soc << 8));

		// Write MixSOC(0Dh) = Table_SOC
		WriteFGRegister(0x0D, (u16)(table_soc << 8));

		// Write RemCapMIX(0Fh) = RemCapREP(05h)
		read_val = ReadFGRegister(0x05);
		WriteFGRegister(0x0F, (u16)read_val);

		// Write FullCap(10h) = DesignCap(18h)
		read_val = ReadFGRegister(0x18);
		WriteFGRegister(0x10, (u16)Capacity);

		// Write FullCapNom(23h) = DesignCap(18h)
		WriteFGRegister(0x23, (u16)VFCapacity);

		// Write dQ_acc(45h) = DesignCap(18h)
		WriteFGRegister(0x45, (u16)(VFCapacity / 4));

		// Write dP_acc(46h) = 3200h
		WriteFGRegister(0x46, 0x3200);
	}
}

void fuel_gauge_operate_voltagemode()
{
    WriteFGRegister(CGain_REG, 0x0000);
    WriteFGRegister(MiscCFG_REG, 0x0003);
    WriteFGRegister(LearnCFG_REG, 0x0007);
}

int set_battery_capacity(int battery_capa)
{
	if (battery_capa == 4000) {
		Kempty0 = 0x057F;
		Vempty = 0x7D5A;
		SOCempty = 0x0000;
		ETC	= 0x1458;
		RCOMP0 = 0x0089;
		TempCo = 0x132A;
		Capacity = 0x1F40;  // 4000mAh
		VFCapacity = 0x29AA;  // 5333mAh
		ICHGTerm = 0x0520;
		TempNom = 0x1400;
		FilterCFG = 0x87A4;

		memcpy (cell_character0, cell_character0_4k, sizeof(u16) * 16);
		memcpy (cell_character1, cell_character1_4k, sizeof(u16) * 16);
		memcpy (cell_character2, cell_character2_4k, sizeof(u16) * 16);
	} else if (battery_capa == 7000) {
		Kempty0	= 0x0592;
		Vempty = 0x7d5A;
		SOCempty = 0x0000;
		ETC = 0x007D;
		RCOMP0 = 0x0076;
		TempCo = 0x1A1C;
		Capacity = 6661;
		VFCapacity = 9157;
		ICHGTerm = 0x0400;
		TempNom = 0x1400;
		FilterCFG = 0x87A4;
		memcpy (cell_character0, cell_character0_7k, sizeof(u16) * 16);
		memcpy (cell_character1, cell_character1_7k, sizeof(u16) * 16);
		memcpy (cell_character2, cell_character2_7k, sizeof(u16) * 16);
	} else {
		printf("Invalid battery_capa %d\n", battery_capa);
		return 1;
	}

	printf("Battery Capa:	%d\n", battery_capa);

	return 0;
}

void init_fuel_gauge(u32 i2c_bus, uchar chip, int dc_in, int jig_on, int sense)
{
	u16 SavedDesignCAP;
	u16 SavedMixedSOC;
	u16 Saved_dQ_acc;
	u16 Saved_dP_acc;
	u16 read_val, RemCap, VfSOC, FullCap0;
	u16 r_data0[16], r_data1[16], r_data2[16];
	u32 i = 0;

	i2c_bus_num = i2c_bus;
	i2c_set_bus_num(i2c_bus_num);

	printf("%s - START!!\n", __func__);

	if (i2c_probe(chip)) {
		printf("\tCan't find device 0x%x on the bus %d\n", chip, i2c_bus);
		return;
	}

	chip_address = chip;

	if (!sense) {
		/* without current sense setting */
		WriteFGRegister(CGain_REG, 0x0000);
		WriteFGRegister(MiscCFG_REG, 0x0003);
		WriteFGRegister(LearnCFG_REG, 0x0007);
		return;
	}

	read_val = ReadFGRegister(DESIGN_CAP_REG);  // DesignCAP

	// If DesignCap value is '0x2AB5', Init already finished!!
	if(read_val == VFCapacity) {
		printf("\tAlready initialized!!\n", __func__);
		goto init_end;
	}

	// 1. Delay 500ms

	// 2. Initialize configuration
	WriteFGRegister(0x2A, 0x083B);
	WriteFGRegister(0x1D, 0x2210);
	WriteFGRegister(0x29, 0x87A4);

	// 3. Save starting parameters
	SavedDesignCAP = ReadFGRegister(0x18);
	SavedMixedSOC = ReadFGRegister(0x0D);
	Saved_dQ_acc = ReadFGRegister(0x45);
	Saved_dP_acc = ReadFGRegister(0x46);

rewrite_model:
	// 4. Unlock model access
	WriteFGRegister(0x62, 0x0059);
	WriteFGRegister(0x63, 0x00C4);

	// 5. Write the custom model
	WriteFG16Register(0x80, cell_character0);
	WriteFG16Register(0x90, cell_character1);
	WriteFG16Register(0xA0, cell_character2);

	// 6. Read the model,  7. Verify the model
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
	// 8. Lock model access
	WriteFGRegister(0x62, 0x0000);
	WriteFGRegister(0x63, 0x0000);

	// 9. Verify the model access is locked
	for(i = 0; i < 16; i++) {
		read_val = ReadFGRegister( (0x80 + i));
		if(read_val)
			goto relock_model;
	}

	// 10. Write custom parameters (all paramaters from MAXIM)
	WriteAndVerifyFGRegister(0x38, RCOMP0);  // RCOMP0
	WriteAndVerifyFGRegister(0x39, TempCo);  // TempCo
	WriteFGRegister(0x3A, ETC);  // ETC
	WriteAndVerifyFGRegister(0x3B, Kempty0);  // Kempty0
	WriteFGRegister(0x33, SOCempty);  // SOCempty
	WriteFGRegister(0x1E, ICHGTerm);  // ICHGterm
	WriteAndVerifyFGRegister(0x28, 0x2676); // 2010.06.15 mihee
	WriteFGRegister(0x24, TempNom);  // TempNom
	WriteFGRegister(0x12, Vempty);  // Vempty
	WriteFGRegister(0x29, FilterCFG);  // FilterCFG

	// 11. Update full capacity parameters
	WriteAndVerifyFGRegister(0x10, Capacity);  // FullCAP
	WriteFGRegister(0x18, VFCapacity);  // DesignCAP
	WriteAndVerifyFGRegister(0x23, VFCapacity);  // FullCAPNom

	// 12. Soft POR
	read_val = ReadFGRegister(STATUS_REG);
	WriteAndVerifyFGRegister(STATUS_REG, (read_val & 0xFFFD));

	// 13. Delay at least 350ms
	mdelay(350);

	// 14. Write VFSOC value to VFSOC0
	VfSOC = ReadFGRegister(0xFF);
	WriteFGRegister(0x60, 0x0080);
	WriteAndVerifyFGRegister(0x48, VfSOC);
	WriteFGRegister(0x60, 0x0000);

	// 15. Restore capacity parameters
#if 0  // Not used
	read_val = ReadFGRegister(0x35);  // FullCAP0
	RemCap = ((SavedMixedSOC / 256) * read_val) / 100;
	WriteAndVerifyFGRegister(0x0F, RemCap);  // RemCAP
	WriteAndVerifyFGRegister(0x05, RemCap);  // RepCAP
	WriteAndVerifyFGRegister(0x45, Saved_dQ_acc);  // Saved_dQ_acc
	WriteAndVerifyFGRegister(0x46, Saved_dP_acc);  // Saved_dP_acc
#endif

	// 16. Load new capacity parameters
	FullCap0 = ReadFGRegister(0x35);
	RemCap = ((VfSOC /256) * VFCapacity) / 100;
//	printf("%s - RemCap (%d)\n", __func__, RemCap);
	WriteAndVerifyFGRegister(0x0F, RemCap);  // RemCAP
	WriteAndVerifyFGRegister(0x05, RemCap);  // RepCAP
	WriteAndVerifyFGRegister(0x45, (VFCapacity / 4));  // dQ_acc
	WriteAndVerifyFGRegister(0x46, 0x3200);  // dP_acc
	WriteAndVerifyFGRegister(0x10, Capacity);  // FullCAP
	WriteFGRegister(0x18, VFCapacity);  // DesignCAP
	WriteAndVerifyFGRegister(0x23, VFCapacity);  // FullCAPNom

	// 17. Addtional step : Write CGAIN, COFF register.
	// Note : This register values depend on H/W.
#ifdef CONFIG_MACH_P1_KOR
	WriteFGRegister(CGain_REG, 0x355f);  // CGAIN
	WriteFGRegister(COff_REG, 0x27);  // COFF
#else
	WriteFGRegister(CGain_REG, 0x303B);  // CGAIN
	WriteFGRegister(COff_REG, 0x4);  // COFF
#endif

	// Temp step : reset current soc.
	if(!dc_in)
		fuel_gauge_reset_soc();

	// Initialization Complete!!
	printf("\tInitialize END!!\n");

init_end:
	// SOC compensation from MAXIM.
	fuel_gauge_compensate_soc(dc_in);

	// Temp quick-start to re-calculate current SOC
	if(fuel_gauge_read_vcell() >= 3520 &&
			fuel_gauge_read_soc() < 2 &&
			!dc_in) { // No DCIN input
		fuel_gauge_reset_soc();
		printf("\tAssert SOC reset because level is too low!!\n");
	}

	// If JIG dev is attached, then reset DesignCAP.
	// Note : DesignCAP will be copied to FullCAPNom after Quick-start.
	if(jig_on)
		WriteFGRegister(0x18, VFCapacity-1);  // DesignCAP

	return ;
}

/* check quick start condition for max17042 fuel gauge */
int quick_start_fuel_gauge(unsigned int cell, unsigned int soc, fuel_gauge_linear_data max17042_quick_table[10])
{
    unsigned int vcell = cell * 100 * 1000;
    int run_quick_start = 1;
    int i, cur_idx, soc_cal;
    int limit_min, limit_max;

    for (i = 0; i < 9; i++) {
        if (vcell < max17042_quick_table[i].min_vcell && vcell >= max17042_quick_table[i+1].min_vcell) {
            cur_idx = i+1;
            break;
        }
    }

    if (cur_idx > 0 && cur_idx < 9) {
        printf("quick start index [%d] \n", cur_idx);
        soc_cal = (int) ((float)(vcell - max17042_quick_table[cur_idx].y_interception) / (float)max17042_quick_table[cur_idx].slope);

        limit_min = soc_cal - 20;
        limit_max = soc_cal + 20;
        if (limit_min < 0) {
            limit_min = 0;
        }

        printf("soc : %d, soc_cal=%d\r\n", soc, soc_cal);

        if (soc > limit_max || soc < limit_min) {
            return run_quick_start;
        }
    }

    return 0;
}

/* quick start solution for max17042 */
void quick_start_max17042(void)
{
    u16 data;
    int quick_count = 3;
    int i;

    //try 5 times to check quickstart done
    for (i = 0; i < quick_count; i++) {

        /* set the quickstart and verify bits */
        data = ReadFGRegister(MiscCFG_REG);
        data |= 0x1400;         //set 10 bit
        WriteFGRegister(MiscCFG_REG, data);

        /* verify no memory leaks during quickstart writing */
        data = ReadFGRegister(MiscCFG_REG);
        data &= 0x1000;

        /* check quick start success */
        if (data == 0x1000) {
            break;
        }
    }

    if (i == quick_count) {
        printf("<quick_start> failed!\n");
    } else {

        for (i = 0; i < quick_count; i++) {
            data = ReadFGRegister(MiscCFG_REG);
            data &= 0xefff;
            WriteFGRegister(MiscCFG_REG, data);

            data = ReadFGRegister(MiscCFG_REG);
            data &= 0x1000;
            if (data == 0x0000) {
                mdelay(500);
                printf("<quick start> success!\n");
                break;
            }
        }
    }
}

/* fuel gauge init with reset when large  gaps between voltage and soc */
void init_fuel_gauge_with_reset(u32 i2c_bus, uchar chip, int detbat, fuel_gauge_linear_data fuel_check_data[10])
{
    int run_quick_start = 1;
    int vcell;
    int soc;

	i2c_bus_num = i2c_bus;
	i2c_set_bus_num(i2c_bus_num);

	if (i2c_probe(chip)) {
		printf("\tCan't find device 0x%x on the bus %d\n", chip, i2c_bus);
		return;
	}

	chip_address = chip;

    /* set voltage mode (without current sense) */
	WriteFGRegister(CGain_REG, 0x0000);
	WriteFGRegister(MiscCFG_REG, 0x0003);
	WriteFGRegister(LearnCFG_REG, 0x0007);

    vcell = fuel_gauge_read_vcell();
    soc = fuel_gauge_read_socvf();

    /*
     * check quick start condition
     */

    printf("check fuel gauge vcell[%d], soc[%d], detbat[%d]\n", vcell, soc, detbat);
    run_quick_start = quick_start_fuel_gauge(vcell, soc, fuel_check_data);

    /*
     * run quick start operation for max17042
     */

    if (run_quick_start) {
        quick_start_max17042();
    }
}


/* command test. */
static s32 fuelgauge_cmd(s32 argc, s8 *argv[])
{
/*	u32 vcell, soc;

	vcell = fuel_gauge_read_vcell();
	soc = fuel_gauge_read_soc();

	printf("vcell = %dmV, soc = %d\n", vcell, soc);
*/
	fuel_gauge_test_read();

	return 0;
}

U_BOOT_CMD(fg, CONFIG_SYS_MAXARGS, 1, fuelgauge_cmd,
	"Utility to read/write fuelgauge(max17042) registers",
	"ex) fg\n"
	": Print vcell(mV), soc(%%)\n"
);

