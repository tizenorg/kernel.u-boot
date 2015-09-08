
#include <common.h>
#include <asm/types.h>
#include <command.h>
#include <i2c.h>
#include <max17040.h>

static uchar chip_addr = MAX17040_I2C_ADDRESS;
static u32 i2c_bus_num = 0;

int (* check_quick_start_fuel_gauge)(unsigned int vcell, unsigned int soc, fuel_gauge_linear_data data[10]);
//==============================================
//fuel gauge i2c read / write function
//==============================================
static unsigned int fuelgauge_read_i2c(u8 addr)
{
	u8 data[2];
	
	i2c_read(chip_addr, addr, 1, data, 2);

	return ( (data[0] << 8 | data[1] ) );

}

static void fuelgauge_write_i2c(u8 addr, unsigned int w_data)
{
	u8 data[2];

	data[0] = w_data >> 8;
	data[1] = w_data * 0xFF;

	i2c_write(chip_addr, addr, 1, data, 2);
}

//==============================================
// fuel gauge vcell / soc function 
//=============================================
static unsigned int max17040_get_vcell(void)
{
	unsigned int tmpADC = 0;
	unsigned int vcell = 0;

	tmpADC = fuelgauge_read_i2c(MAX17040_REG_VCELL);
	vcell = (tmpADC >> 4) * 125 / 100;

	return vcell;
}

static unsigned int max17040_get_rcomp(void)
{
	unsigned int tmpRCOMP = 0;

	tmpRCOMP = fuelgauge_read_i2c(MAX17040_REG_RCOMP);

	return tmpRCOMP;
}

static unsigned int max17040_get_soc(void)
{
	unsigned int tmpSOC = 0;
	unsigned int soc = 0;

	tmpSOC = fuelgauge_read_i2c(MAX17040_REG_SOC);

	soc = tmpSOC >> 8;

	return soc;
}

static void max17040_set_rcomp(unsigned int rcomp)
{
	fuelgauge_write_i2c(MAX17040_REG_RCOMP, rcomp);
}

//SOC calculation = (vcell - y_interception) / slope
int quick_start_fuel_gauge(unsigned int cell, unsigned int soc, fuel_gauge_linear_data max17040_quick_table[10])
{
	unsigned int vcell = cell * 100 * 1000;
	int run_quick_start = 1;
	int i, cur_idx, soc_cal;
	int limit_min, limit_max;

	for (i = 0; i < 9; i++) {
		if (vcell < max17040_quick_table[i].min_vcell &&
			vcell >= max17040_quick_table[i+1].min_vcell) {
			cur_idx = i+1;
			break;
		}
	}

	if (cur_idx > 0 && cur_idx < 9) {
	//	printf("quick start index [%d]\r\n",cur_idx);
		soc_cal = (int) ((float)(vcell - max17040_quick_table[cur_idx].y_interception) / (float)max17040_quick_table[cur_idx].slope);
	
		limit_min = soc_cal - 15;
		limit_max = soc_cal + 15;
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

void quick_start(void)
{
	fuelgauge_write_i2c(MAX17040_REG_MODE, MAX17040_QUICK_START_VALUE);
	//printf("start fuel gauge quick start nodelay!!\r\n");
	//udelay(3 * 1000 * 100);
}

//=============================================
// fuel gauge init function 
//=============================================
// busnum | rcomp | quick_condition | table
//=============================================
unsigned int init_fuel_gauge(u32 i2c_bus, int quick_start_table_list, fuel_gauge_linear_data fuel_check_data[10])
{
	unsigned int soc, vcell, rcomp = MAX17040_DEFAULT_RCOMP_VALUE;
	unsigned int run_quick_start = 1;
	unsigned int check_quick_start = 1;	

	//set i2c bus
	i2c_set_bus_num(i2c_bus);

	//check it's valid device
	if (i2c_probe(chip_addr)) {
		printf("can't found max17040 fuel gauge !!!\r\n");
		return 0;
	}

	//udelay(15 * 1000 * 10);

	vcell = max17040_get_vcell();
	soc = max17040_get_soc();

	switch(quick_start_table_list)
	{
	case S1_QUICK_START:
		check_quick_start_fuel_gauge = quick_start_fuel_gauge;
		rcomp = S1_RCOMP_VALUE;
		break;
	case SLP45_QUICK_START:
		check_quick_start_fuel_gauge = quick_start_fuel_gauge;
		rcomp = SLP45_RCOMP_VALUE;
		break;
	default :
		check_quick_start = 0;
		break;
	}

	//check quick start condition
	if (check_quick_start) { 
		run_quick_start = check_quick_start_fuel_gauge(vcell, soc, fuel_check_data);
	}

	//set rcomp if it's not 0
	max17040_set_rcomp(rcomp);

	//run quick start if it's not skip condition
	if (run_quick_start) {
		quick_start();
	}

	printf("%s: vcell = %dmV, soc = %d, rcomp = %x\n", "battery", vcell, soc, rcomp);
	
	return soc;
}
