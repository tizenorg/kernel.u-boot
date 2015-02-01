/*
 * Copyright (C) 2011 System S/W Group, Samsung Electronics.
 * Chiwoong Byun <woong.byun@samsung.com>
 *
 * Modify for GS2+ H/W specific base codes.
 * jaecheol kim <jc22.kim@samsung.com>
 *
 * Pegasus h/w specific base codes.
 */

#include <config.h>
#include <common.h>
#include <pmic.h>
#include <i2c.h>
#include <pmic_s5m8767.h>

#define S5M8767_I2C_CH 5

/* S5M8767_INT1 register bit */

/* S5M8767_INT2 register bit */
#define S5M8767_INT2_CHGINS	(1<<0)

/* S5M8767_INT3 register bit */
#define S5M8767_INT3_SMPL_INT	(1<<3)
#define S5M8767_INT3_WTSR		(1<<5)

/* S5M8767_BUCKx_CTRL register bit */
#define S5M8767_BUCK_EN_OFF		(0<<6)
#define S5M8767_BUCK_EN_ON		(1<<6)
#define S5M8767_BUCK_EN_ALWAYSON	(3<<6)
#define S5M8767_BUCK_REMOTE_OFF	(0<<5)
#define S5M8767_BUCK_NACTIVE_DISC	(0<<4)
#define S5M8767_BUCK_ACTIVE_DISC	(1<<4)
#define S5M8767_BUCK_AUTOMODE		(2<<2)
#define S5M8767_BUCK_PWMMODE		(3<<2)
#define S5M8767_BUCK_DVSOFF		(0<<1)
#define S5M8767_BUCK_DVSON		(1<<1)

/* S5M8767_STATUS1 register bit (0x07) */
#define S5M8767_STATUS1_WTSR		(1<<7)
#define S5M8767_STATUS1_SMPL		(1<<6)
#define S5M8767_STATUS1_RTC_A2S	(1<<5)
#define S5M8767_STATUS1_RTC_A1S	(1<<4)
#define S5M8767_STATUS1_LOWBAT2S	(1<<3)
#define S5M8767_STATUS1_LOWBAT1S	(1<<2)
#define S5M8767_STATUS1_JIGON		(1<<1)
#define S5M8767_STATUS1_PWRON		(1<<0)

/* S5M8767_STATUS2 register bit (0x08) */
#define S5M8767_STATUS2_DVSSET3S	(1<<7)
#define S5M8767_STATUS2_DVSSET2S	(1<<6)
#define S5M8767_STATUS2_DVSSET1S	(1<<5)
#define S5M8767_STATUS2_PWREN		(1<<4)
#define S5M8767_STATUS2_MR2BS		(1<<2)
#define S5M8767_STATUS2_MR1BS		(1<<1)
#define S5M8767_STATUS2_ACINOK	(1<<0)

/* S5M8767_STATUS3 register bit (0x09) */
#define S5M8767_STATUS3_DVS4S		(1<<2)
#define S5M8767_STATUS3_DVS3S		(1<<1)
#define S5M8767_STATUS3_DVS2S		(1<<0)

/* S5M8767 PWRONSRC register bit (0xE0) */
#define S5M8767_PWRONSRC_SMPL		(1<<6)
#define S5M8767_PWRONSRC_ALARM2	(1<<5)
#define S5M8767_PWRONSRC_ALARM1	(1<<4)
#define S5M8767_PWRONSRC_MRST		(1<<3)
#define S5M8767_PWRONSRC_ACOKB		(1<<2)
#define S5M8767_PWRONSRC_JIGON		(1<<1)
#define S5M8767_PWRONSRC_PWRON		(1<<0)

/* S5M8767 BUCK9_CTRL1 register bit (0x18) */
#define S5M8767_BUCK9CTRL1_AOFF	(0<<6)
#define S5M8767_BUCK9CTRL1_EN		(1<<6)
#define S5M8767_BUCK9CTRL1_AON		(2<<6)
#define S5M8767_BUCK9CTRL1_DSCH	(1<<4)
#define S5M8767_BUCK9CTRL1_MODE_AUTO	(2<<2)
#define S5M8767_BUCK9CTRL1_MODE_PWM	(3<<2)

#define PMIC_REG_WRITE			0
#define PMIC_REG_DELAY			0xFE
#define PMIC_REG_END			0xFF

#define CALC_S5M8767_VOLT1(x)  ((x < 650) ? 0 : ((x-650)/6.25))
#define CALC_S5M8767_VOLT2(x)  ((x < 600) ? 0 : ((x-600)/6.25))

#define CONFIG_PM_VDD_ARM		(1.1)
#define CONFIG_PM_VDD_INT		(1.0)
#define CONFIG_PM_VDD_G3D		(1.1)
#define CONFIG_PM_VDD_MIF		(1.1)

/* default s5m8767 pmic ldo status */
enum PMIC_DEFAULTONOFF {
	DEFAULT_OFF = 0,
	DEFAULT_ON = 3,
};

/* model specific pmic ldo status */
enum PMIC_ONOFF {
	PMIC_OFF = 0,
	PMIC_ON,
	PMIC_LPON,
	PMIC_ALWAYSON,
};

/* PMIC LDO TYPE */
#define PMIC_LDO_NTYPE		(1)
#define PMIC_LDO_PTYPE		(2)

#define PMIC_VOL_NOCHANGE	(0)
#define PMIC_VOL_MASK		(0x3F)
#define PMIC_ONOFF_MASK	(0xC0)
#define PMIC_MASK_ALL		(PMIC_VOL_MASK|PMIC_ONOFF_MASK)

#define PMIC_LDO_REG_BASE	(0x5C)

#define S5M8767_LDO_ONOFF(x)		((x) << 6)

/* base voltage x sould be mV */
#define S5M8767_NTYPE_VOLTAGE(x)	((u8)((x-800)/25))
#define S5M8767_PTYPE_VOLTAGE(x)	((u8)((x-800)/50))

struct s5m9767_ldo_control {
	u8 def_state;
	u8 mdef_state;
	u32 voltage_update;
};

/* default on /off control for LDOs based on S2PLUS rev0.1*/
/*
	======================================
	ldoX  : name					: src	: default
	======================================
	LDO1 : VALIVE_1.0V_AP		: buck8	: on : 1.0v
	LDO2 : VMIM2_1.2V_AP		: buck8	: on : 1.2v
	LDO3 : VCC_1.8V_AP			: buck7	: on : 1.8v
	LDO4 : VDDQ_PRE_1.8V		: buck7	: on : 1.8v
	LDO5 : VCC_PLS_2.8V		: v_bat	: on : 1.8v
	LDO6 : VMPLL_1.0V_AP		: buck8	: on : 1.8v
	LDO7 : VPLL_1.0V_AP		: buck8	: on : 1.0v
	LDO8 : VMIPI_1.0V_AP		: buck8	: on : 1.0v
	LDO9 : CAM_ISP_1.8V		: buck7	: off : 3.0v
	LDO10 : VT_CAM_DVDD_1.8V	: buck7	: on : 1.8v
	LDO11 : VABB1_1.8V_AP		: buck7	: on : 1.8v
	LDO12 : VUOTG_3.0V_AP		: v_bat	: on : 3.0v
	LDO13 : VMIPI_1.8V_AP		: buck7	: on : 1.8v
	LDO14 : VABB2_1.8V_AP		: buck7	: on : 1.8v
	LDO15 : VHSIC_1.0V_AP		: buck8	: on : 1.0v
	LDO16 : VHSIC_1.8V_AP		: buck7	: on : 1.8v
	LDO17 : VCC_2.8V_AP		: v_bat	: on : 2.8v
	LDO18 : VMEM_VDD_2.8V		: v_bat	: off : 2.8v
	LDO19 : CAM_AF_2.8V		: v_bat	: off : 3.0v
	LDO20 : VCC_3.0V_LCD		: v_bat	: off : 3.0v
	LDO21 : VCC_3.0V_MOTOR	: v_bat	: off : 3.0v
	LDO22 : CAM_SENSOR_A2.8V	: v_bat	: off : 3.3v
	LDO23 : VTF_2.8V			: v_bat	: off : 2.8v
	LDO24 : TSP_AVDD_3.3V		: v_bat	: off : 3.0v
	LDO25 : CAM_SENSOR_CORE_1.2V	: buck7	: off : 1.2v
	LDO26 : CAM_ISP_SEN_IO_1.8V	: buck7	: off : 1.8v
	LDO27 : VT_CAM_1.8V		: buck7	: off : 1.8v
	LDO28 : 3_TOUCH_1.8V		: buck7	: off : 1.8v
	======================================
*/

static struct s5m9767_ldo_control s5m8767_default_ldo_control[] = {
	{ DEFAULT_ON,	PMIC_ALWAYSON,	PMIC_VOL_NOCHANGE }, /*LDO1*/
	{ DEFAULT_ON,	PMIC_ON,		PMIC_VOL_NOCHANGE }, /*LDO2*/
	{ DEFAULT_ON,	PMIC_ALWAYSON,	PMIC_VOL_NOCHANGE }, /*LDO3*/
	{ DEFAULT_ON,	PMIC_ON,	PMIC_VOL_NOCHANGE }, /*LDO4*/
	{ DEFAULT_ON,	PMIC_ALWAYSON,		2800 }, /*LDO5*/
	{ DEFAULT_ON,	PMIC_ON,		1000 }, /*LDO6*/
	{ DEFAULT_ON,	PMIC_ON,		PMIC_VOL_NOCHANGE }, /*LDO7*/
	{ DEFAULT_ON,	PMIC_ON,	PMIC_VOL_NOCHANGE }, /*LDO8*/
	{ DEFAULT_ON,	PMIC_OFF,		1800 }, /*LDO9*/
	{ DEFAULT_ON,	PMIC_OFF,		1500 }, /*LDO10*/
	{ DEFAULT_ON,	PMIC_ON,		PMIC_VOL_NOCHANGE }, /*LDO11*/
	{ DEFAULT_ON,	PMIC_ON,		PMIC_VOL_NOCHANGE }, /*LDO12*/
	{ DEFAULT_ON,	PMIC_ON,		PMIC_VOL_NOCHANGE }, /*LDO13*/
	{ DEFAULT_ON,	PMIC_ON,		PMIC_VOL_NOCHANGE }, /*LDO14*/
	{ DEFAULT_ON,	PMIC_ON,		PMIC_VOL_NOCHANGE }, /*LDO15*/
	{ DEFAULT_ON,	PMIC_ON,		PMIC_VOL_NOCHANGE }, /*LDO16*/
	{ DEFAULT_ON,	PMIC_ALWAYSON,	PMIC_VOL_NOCHANGE }, /*LDO17*/
	{ DEFAULT_OFF,	PMIC_OFF,		PMIC_VOL_NOCHANGE }, /*LDO18*/
	{ DEFAULT_OFF,	PMIC_OFF,		2800 }, /*LDO19*/
	{ DEFAULT_OFF,	PMIC_ALWAYSON,	PMIC_VOL_NOCHANGE }, /*LDO20*/
	{ DEFAULT_OFF,	PMIC_OFF,		PMIC_VOL_NOCHANGE }, /*LDO21*/
	{ DEFAULT_OFF,	PMIC_OFF,		2800 }, /*LDO22*/
	{ DEFAULT_OFF,	PMIC_OFF,		PMIC_VOL_NOCHANGE }, /*LDO23*/
	{ DEFAULT_OFF,	PMIC_OFF,		2800 }, /*LDO24*/
	{ DEFAULT_OFF,	PMIC_OFF,		PMIC_VOL_NOCHANGE }, /*LDO25*/
	{ DEFAULT_OFF,	PMIC_OFF,		PMIC_VOL_NOCHANGE }, /*LDO26*/
	{ DEFAULT_OFF,	PMIC_OFF,		PMIC_VOL_NOCHANGE }, /*LDO27*/
	{ DEFAULT_OFF,	PMIC_OFF,		PMIC_VOL_NOCHANGE }, /*LDO28*/

	/* END OF TABLE */
	{PMIC_REG_END, PMIC_REG_END, PMIC_REG_WRITE}
};

struct pmic_reg {
	u8 addr;
	u8 data;
	u8 mask;
};

#define PMIC_READ(A, B)	i2c_read(S5M8767_I2C_ADDRESS>>1, A, 1, B, 1)

static void PMIC_WRITE(char address, char data)
{
	i2c_write(S5M8767_I2C_ADDRESS>>1, address, 1, &data, 1);
}

static struct pmic_reg s5m8767_init[] = {
	/* BUCK9 : 2.85V (VMEM_VDDF_2.85V) */
	{S5M8767_REG_BUCK9CTRL2, 0xA8, PMIC_REG_WRITE},

	/* BUCK3 remote3 sense off */
	{ S5M8767_REG_BUCK3CTRL,
		S5M8767_BUCK_EN_ON |
		S5M8767_BUCK_REMOTE_OFF |
		S5M8767_BUCK_ACTIVE_DISC |
		S5M8767_BUCK_AUTOMODE |
		S5M8767_BUCK_DVSOFF,
		PMIC_REG_WRITE },

	/* BUCK1 CTRL */
	{S5M8767_REG_BUCK1CTRL1, 0x40, PMIC_ONOFF_MASK},
	/* VMIF_1.1V_AP */
	/* 32KHz Enable */
	{S5M8767_32KHZ, 0x6, 0x6}, /* ENP32KH/EN32KHCP */

	/* END OF TABLE */
	{PMIC_REG_END, PMIC_REG_END, PMIC_REG_WRITE}
};

static u8 S5M8767_regs[5];
static u8 S5M8767_irq[4];

static void pmic_reg_update(u8 addr, u8 data, u8 mask)
{
	u8 tmp;

	if (S5M8767_probe())
		return;
	PMIC_READ(addr, &tmp);
	tmp &= ~(mask);
	tmp |= data;
	PMIC_WRITE(addr, tmp);
}

static void pmic_write_table(struct pmic_reg *pmic_reg)
{
	u8 i;
	if (S5M8767_probe())
		return;

	for (i = 0; ; i++) {
		if (pmic_reg[i].addr == PMIC_REG_END &&
			pmic_reg[i].data == PMIC_REG_END)
			break;

		if (pmic_reg[i].addr == PMIC_REG_DELAY) {
			udelay(pmic_reg[i].data);
			continue;
		}

		if (pmic_reg[i].mask == PMIC_REG_WRITE)
			PMIC_WRITE(pmic_reg[i].addr, pmic_reg[i].data);
		else
			pmic_reg_update(pmic_reg[i].addr, pmic_reg[i].data,
							pmic_reg[i].mask);
	}
}

u8 pmic_check_ldo_type(u8 ldo)
{
	switch (ldo) {
	case PMIC_LDO1:
	case PMIC_LDO2:
	case PMIC_LDO6:
	case PMIC_LDO7:
	case PMIC_LDO8:
	case PMIC_LDO15:
		return PMIC_LDO_NTYPE;
	default:
		return PMIC_LDO_PTYPE;
	}
}

static void pmic_ldo_write_table(struct s5m9767_ldo_control *pmic_ldo_reg)
{
	u8 ldo_index;
	u8 ldo_reg_index;

	for (ldo_index = 0; ; ldo_index++) {
		if (pmic_ldo_reg[ldo_index].def_state == PMIC_REG_END &&
			pmic_ldo_reg[ldo_index].mdef_state == PMIC_REG_END)
			break;

		if (ldo_index < 2)
			ldo_reg_index = PMIC_LDO_REG_BASE + ldo_index;
		else
			ldo_reg_index = PMIC_LDO_REG_BASE + ldo_index + 3;

		if (pmic_ldo_reg[ldo_index].def_state != pmic_ldo_reg[ldo_index].mdef_state) {
			if (pmic_ldo_reg[ldo_index].voltage_update != PMIC_VOL_NOCHANGE) {
				/* voltage & state change */
				printf("[s5m8767] update LDO%d vol[%d], onoff[%d]\n",
						ldo_index+1, pmic_ldo_reg[ldo_index].voltage_update,
						pmic_ldo_reg[ldo_index].mdef_state);
				if (pmic_check_ldo_type(ldo_index) == PMIC_LDO_NTYPE) {
					pmic_reg_update(ldo_reg_index,
						S5M8767_LDO_ONOFF(pmic_ldo_reg[ldo_index].mdef_state) |
						S5M8767_NTYPE_VOLTAGE(pmic_ldo_reg[ldo_index].voltage_update),
						PMIC_MASK_ALL);
				} else {
					pmic_reg_update(ldo_reg_index,
						S5M8767_LDO_ONOFF(pmic_ldo_reg[ldo_index].mdef_state) |
						S5M8767_PTYPE_VOLTAGE(pmic_ldo_reg[ldo_index].voltage_update),
						PMIC_MASK_ALL);
				}
			} else {
				/* state change */
				printf("[s5m8767] update LDO%d onoff[%d]\n",
						ldo_index+1, pmic_ldo_reg[ldo_index].mdef_state);
				pmic_reg_update(ldo_reg_index,
						S5M8767_LDO_ONOFF(pmic_ldo_reg[ldo_index].mdef_state),
						PMIC_ONOFF_MASK);
			}
		} else {
			if (pmic_ldo_reg[ldo_index].voltage_update != PMIC_VOL_NOCHANGE) {
				/* voltage change */
				printf("[s5m8767] update LDO%d vol[%d]\n",
						ldo_index+1, pmic_ldo_reg[ldo_index].voltage_update);
				if (pmic_check_ldo_type(ldo_index) == PMIC_LDO_NTYPE) {
					pmic_reg_update(ldo_reg_index,
						S5M8767_NTYPE_VOLTAGE(pmic_ldo_reg[ldo_index].voltage_update),
						PMIC_VOL_MASK);
				} else {
					pmic_reg_update(ldo_reg_index,
						S5M8767_PTYPE_VOLTAGE(pmic_ldo_reg[ldo_index].voltage_update),
						PMIC_VOL_MASK);
				}
			}
		}
	}
}

void I2C_S5M8767_VolSetting(PMIC_RegNum eRegNum, u8 ucVolLevel, u8 ucEnable)
{
	u8 reg_addr, reg_bitpos, reg_bitmask, vol_level;
	u8 read_data;

	if (S5M8767_probe())
		return;

	reg_bitpos = 0;
	reg_bitmask = 0xFF;
	if (eRegNum == 0)
		reg_addr = 0x33;
	else if (eRegNum == 1)
		reg_addr = 0x35;
	else if (eRegNum == 2)
		reg_addr = 0x3E;
	else if (eRegNum == 3)
		reg_addr = 0x47;
	else if (eRegNum == 4)
		reg_addr = 0x48;
	else
		hang();

	vol_level = ucVolLevel&reg_bitmask;

	PMIC_WRITE(reg_addr, vol_level);
}

static unsigned int pmic_bus = 5;

static int S5M8767_probe(void)
{
	unsigned char addr = S5M8767_I2C_ADDRESS>>1;

	i2c_set_bus_num(S5M8767_I2C_CH);

	if (i2c_probe(addr)) {
		puts("Can't found PMIC_S5M8767\n");
		return 1;
	}
	return 0;
}


void pmic_init()
{
	float vdd_arm, vdd_int, vdd_g3d;
	float vdd_mif;
	u8 read_data;

	if (S5M8767_probe())
		return;
	vdd_arm = CONFIG_PM_VDD_ARM;
	vdd_int = CONFIG_PM_VDD_INT;
	vdd_g3d = CONFIG_PM_VDD_G3D;
	vdd_mif = CONFIG_PM_VDD_MIF;

	I2C_S5M8767_VolSetting(PMIC_BUCK1, CALC_S5M8767_VOLT1(vdd_mif * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK2, CALC_S5M8767_VOLT2(vdd_arm * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK3, CALC_S5M8767_VOLT2(vdd_int * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK4, CALC_S5M8767_VOLT2(vdd_g3d * 1000), 1);

	/*
	====================================
	Read pmic id register : 0x00
	====================================
	pmic evt2 : 0x67
	pmic evt3 : 0x01		(2012.1.13 / s2plus rev0.2 board)
	====================================
	*/
	PMIC_READ(S5M8767_REG_ID, &S5M8767_regs[0]);

	/*
	====================================
	Read pmic id register : 0x5A
	====================================
	BUCK9_EN : [7:6] : buck9 enable control
		00 : Always off (default)
		01 : On/off by BUCK9EN (high : on, low: off)
		1x : Always on
	RSVD : [5] : reserved
	DSCH_B9 : [4] : Active Discharge of SW9
		0 : No active discharge
		1 : Active discharge
	MODE_B9 : [3:2] : Mode control
		10 : Auto mode (default)
		11 : forced PWM
	RSVD : [1:0] : reserved
	====================================
	*/
	PMIC_WRITE(S5M8767_REG_BUCK9CTRL1, 0x58);

	if (S5M8767_regs[0] == 0x67) {
		printf("[s5m8767] set additional register for performance rev evt2\n");
		PMIC_WRITE(0x9D, 0x08);
		PMIC_WRITE(0xA7, 0x08);

		PMIC_WRITE(0x9C, 0x55);
		PMIC_WRITE(0xA6, 0x55);
	}

	/* read interrupt / status registers */
	PMIC_READ(S5M8767_INTSRC, &S5M8767_regs[1]);
	PMIC_READ(S5M8767_REG_STATUS1, &S5M8767_regs[2]);
	PMIC_READ(S5M8767_REG_STATUS2, &S5M8767_regs[3]);
	PMIC_READ(S5M8767_REG_STATUS3, &S5M8767_regs[4]);
	PMIC_READ(S5M8767_REG_INT1, &S5M8767_irq[0]);
	PMIC_READ(S5M8767_REG_INT2, &S5M8767_irq[1]);
	PMIC_READ(S5M8767_REG_INT3, &S5M8767_irq[2]);

	/* Initialize PMIC regs */
	pmic_write_table(s5m8767_init);

	/* update ldo voltage / state */
	pmic_ldo_write_table(s5m8767_default_ldo_control);
}

void pmic_onoff(char *name, int onoff)
{
}

static int pmic_detbat()
{
	return 1;
}

void pmic_get_status(void)
{
	printf("====================\n");
	printf("s5m8767 pmic register\n");
	printf("====================\n");
	printf("ID = 0x%02x\n", S5M8767_regs[0]);
	printf("IRQSRC = 0x%02x\n", S5M8767_regs[1]);
	printf("STATUS1 = 0x%02x\n", S5M8767_regs[2]);
	printf("STATUS2 = 0x%02x\n", S5M8767_regs[3]);
	printf("STATUS3 = 0x%02x\n", S5M8767_regs[4]);
	printf("IRQ1 = 0x%02x\n", S5M8767_irq[0]);
	printf("IRQ2 = 0x%02x\n", S5M8767_irq[1]);
	printf("IRQ3 = 0x%02x\n", S5M8767_irq[2]);
}

void pmic_lowpower()
{
	/* TODO: .. */
}

void pmic_highpower()
{
	/* TODO: .. */
}

int pmic_read(unsigned char reg, unsigned char *pch)
{
	if (S5M8767_probe())
		return -1;
	PMIC_READ(reg, pch);
	return 0;
}

int pmic_write(unsigned char reg, unsigned char pch)
{
	if (S5M8767_probe())
		return -1;
	PMIC_WRITE(reg, pch);
	return 0;
}

int pmic_ldo_enable(u8 ldo)
{
	if (ldo <= 1)
		pmic_reg_update(S5M8767_REG_LDO1CTRL+(ldo), 0xC0, 0xC0);
	else if (ldo > 1)
		pmic_reg_update(S5M8767_REG_LDO3CTRL+(ldo)-2, 0xC0, 0xC0);

	return 0;
}

int pmic_ldo_disable(u8 ldo)
{
	if (ldo <= 1)
		pmic_reg_update(S5M8767_REG_LDO1CTRL+(ldo), 0xC0, 0xC0);
	else if (ldo > 1)
		pmic_reg_update(S5M8767_REG_LDO3CTRL+(ldo)-2, 0x00, 0xC0);

	return 0;
}

void battery_pole_disconnect_check(void)
{
	/* dummy function! */
}

int pmic_7sec_reset()
{
	u8 tmp = S5M8767_irq[1];
	return !!(tmp & (1 << 2));
}

int pmic_check_rtc_alarm(void)
{
	u8 tmp = S5M8767_irq[2];
	return !!(tmp & (1 << 1));
}

int pmic_check_powerkey_irq(void)
{
	u8 tmp;
	if (S5M8767_probe())
		return -1;
	/* S5M8767_STATUS1_REG */
	PMIC_READ(S5M8767_REG_INT1, &tmp);
	return !!(tmp & S5M8767_PWRONSRC_PWRON);
}

int pmic_check_longkey_reset(void)
{
	u8 tmp;
	if (S5M8767_probe())
		return -1;
	/* S5M8767_STATUS1_REG */
	PMIC_READ(S5M8767_REG_STATUS1, &tmp);
	return !!(tmp & S5M8767_STATUS1_PWRON);
}
int pmic_check_jig_on(void)
{
	u8 tmp;

	if (S5M8767_probe())
		return -1;
	/* S5M8767_STATUS1_REG */
	PMIC_READ(S5M8767_REG_STATUS1, &tmp);
	return !!(tmp & S5M8767_STATUS1_JIGON);
}

int pmic_check_dcinok(void)
{
	u8 tmp;

	if (S5M8767_probe())
		return -1;
	/* S5M8767_STATUS1_REG */
	PMIC_READ(S5M8767_REG_STATUS2, &tmp);

	/* JIGON Active low (0:low, 1:high) */
	return !(tmp & S5M8767_STATUS2_ACINOK);
}

int pmic_set_mrstb(int en, int timeout)
{
	u8 tmp;

	if (S5M8767_probe())
		return -1;
	tmp = (en << 3) | (timeout & 0x7);
	PMIC_WRITE(S5M8767_REG_CTRL2, tmp);
	return 0;
}

int pmic_check_charger_intr(void)
{
	u8 tmp = S5M8767_irq[1];
	return !!(tmp & S5M8767_INT2_CHGINS);
}

int pmic_check_wtsr_intr(void)
{
	u8 tmp = S5M8767_irq[2];

	return !!(tmp & S5M8767_INT3_WTSR);
}

int pmic_check_smpl_intr(void)
{
	u8 tmp = S5M8767_irq[2];
	return !!(tmp & S5M8767_INT3_SMPL_INT);
}
